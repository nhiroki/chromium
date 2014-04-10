/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "modules/webaudio/AudioScheduledSourceNode.h"

#include "bindings/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/events/Event.h"
#include "platform/audio/AudioUtilities.h"
#include "modules/webaudio/AudioContext.h"
#include <algorithm>
#include "wtf/MathExtras.h"

using namespace std;

namespace WebCore {

const double AudioScheduledSourceNode::UnknownTime = -1;

AudioScheduledSourceNode::AudioScheduledSourceNode(AudioContext* context, float sampleRate)
    : AudioSourceNode(context, sampleRate)
    , m_playbackState(UNSCHEDULED_STATE)
    , m_startTime(0)
    , m_endTime(UnknownTime)
    , m_hasEndedListener(false)
{
}

void AudioScheduledSourceNode::updateSchedulingInfo(size_t quantumFrameSize,
                                                    AudioBus* outputBus,
                                                    size_t& quantumFrameOffset,
                                                    size_t& nonSilentFramesToProcess)
{
    ASSERT(outputBus);
    if (!outputBus)
        return;

    ASSERT(quantumFrameSize == AudioNode::ProcessingSizeInFrames);
    if (quantumFrameSize != AudioNode::ProcessingSizeInFrames)
        return;

    double sampleRate = this->sampleRate();

    // quantumStartFrame     : Start frame of the current time quantum.
    // quantumEndFrame       : End frame of the current time quantum.
    // startFrame            : Start frame for this source.
    // endFrame              : End frame for this source.
    size_t quantumStartFrame = context()->currentSampleFrame();
    size_t quantumEndFrame = quantumStartFrame + quantumFrameSize;
    size_t startFrame = AudioUtilities::timeToSampleFrame(m_startTime, sampleRate);
    size_t endFrame = m_endTime == UnknownTime ? 0 : AudioUtilities::timeToSampleFrame(m_endTime, sampleRate);

    // If we know the end time and it's already passed, then don't bother doing any more rendering this cycle.
    if (m_endTime != UnknownTime && endFrame <= quantumStartFrame)
        finish();

    if (m_playbackState == UNSCHEDULED_STATE || m_playbackState == FINISHED_STATE || startFrame >= quantumEndFrame) {
        // Output silence.
        outputBus->zero();
        nonSilentFramesToProcess = 0;
        return;
    }

    // Check if it's time to start playing.
    if (m_playbackState == SCHEDULED_STATE) {
        // Increment the active source count only if we're transitioning from SCHEDULED_STATE to PLAYING_STATE.
        m_playbackState = PLAYING_STATE;
    }

    quantumFrameOffset = startFrame > quantumStartFrame ? startFrame - quantumStartFrame : 0;
    quantumFrameOffset = min(quantumFrameOffset, quantumFrameSize); // clamp to valid range
    nonSilentFramesToProcess = quantumFrameSize - quantumFrameOffset;

    if (!nonSilentFramesToProcess) {
        // Output silence.
        outputBus->zero();
        return;
    }

    // Handle silence before we start playing.
    // Zero any initial frames representing silence leading up to a rendering start time in the middle of the quantum.
    if (quantumFrameOffset) {
        for (unsigned i = 0; i < outputBus->numberOfChannels(); ++i)
            memset(outputBus->channel(i)->mutableData(), 0, sizeof(float) * quantumFrameOffset);
    }

    // Handle silence after we're done playing.
    // If the end time is somewhere in the middle of this time quantum, then zero out the
    // frames from the end time to the very end of the quantum.
    if (m_endTime != UnknownTime && endFrame >= quantumStartFrame && endFrame < quantumEndFrame) {
        size_t zeroStartFrame = endFrame - quantumStartFrame;
        size_t framesToZero = quantumFrameSize - zeroStartFrame;

        bool isSafe = zeroStartFrame < quantumFrameSize && framesToZero <= quantumFrameSize && zeroStartFrame + framesToZero <= quantumFrameSize;
        ASSERT(isSafe);

        if (isSafe) {
            if (framesToZero > nonSilentFramesToProcess)
                nonSilentFramesToProcess = 0;
            else
                nonSilentFramesToProcess -= framesToZero;

            for (unsigned i = 0; i < outputBus->numberOfChannels(); ++i)
                memset(outputBus->channel(i)->mutableData() + zeroStartFrame, 0, sizeof(float) * framesToZero);
        }

        finish();
    }

    return;
}

void AudioScheduledSourceNode::start(double when, ExceptionState& exceptionState)
{
    ASSERT(isMainThread());

    if (m_playbackState != UNSCHEDULED_STATE) {
        exceptionState.throwDOMException(
            InvalidStateError,
            "cannot call start more than once.");
        return;
    }

    m_startTime = when;
    m_playbackState = SCHEDULED_STATE;
}

void AudioScheduledSourceNode::stop(double when, ExceptionState& exceptionState)
{
    ASSERT(isMainThread());

    if (m_playbackState == UNSCHEDULED_STATE) {
        exceptionState.throwDOMException(
            InvalidStateError,
            "cannot call stop without calling start first.");
    } else {
        // stop() can be called more than once, with the last call to stop taking effect, unless the
        // source has already stopped due to earlier calls to stop. No exceptions are thrown in any
        // case.
        when = max(0.0, when);
        m_endTime = when;
    }
}

void AudioScheduledSourceNode::setOnended(PassRefPtr<EventListener> listener)
{
    m_hasEndedListener = listener;
    setAttributeEventListener(EventTypeNames::ended, listener);
}

void AudioScheduledSourceNode::finish()
{
    if (m_playbackState != FINISHED_STATE) {
        // Let the context dereference this AudioNode.
        context()->notifyNodeFinishedProcessing(this);
        m_playbackState = FINISHED_STATE;
    }

    if (m_hasEndedListener) {
        // |task| will keep the AudioScheduledSourceNode alive until the listener has been handled.
        OwnPtr<NotifyEndedTask> task = adoptPtr(new NotifyEndedTask(this));
        callOnMainThread(&AudioScheduledSourceNode::notifyEndedDispatch, task.leakPtr());
    }
}

void AudioScheduledSourceNode::notifyEndedDispatch(void* userData)
{
    OwnPtr<NotifyEndedTask> task = adoptPtr(static_cast<NotifyEndedTask*>(userData));

    task->notifyEnded();
}

AudioScheduledSourceNode::NotifyEndedTask::NotifyEndedTask(PassRefPtr<AudioScheduledSourceNode> sourceNode)
    : m_scheduledNode(sourceNode)
{
}

void AudioScheduledSourceNode::NotifyEndedTask::notifyEnded()
{
    RefPtrWillBeRawPtr<Event> event = Event::create(EventTypeNames::ended);
    event->setTarget(m_scheduledNode);
    m_scheduledNode->dispatchEvent(event.get());
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
