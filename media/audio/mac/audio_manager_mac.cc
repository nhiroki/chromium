// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <CoreAudio/AudioHardware.h>

#include <string>

#include "base/command_line.h"
#include "base/mac/mac_logging.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/sys_string_conversions.h"
#include "media/audio/audio_util.h"
#include "media/audio/mac/audio_input_mac.h"
#include "media/audio/mac/audio_low_latency_input_mac.h"
#include "media/audio/mac/audio_low_latency_output_mac.h"
#include "media/audio/mac/audio_manager_mac.h"
#include "media/audio/mac/audio_output_mac.h"
#include "media/audio/mac/audio_synchronized_mac.h"
#include "media/audio/mac/audio_unified_mac.h"
#include "media/base/limits.h"
#include "media/base/media_switches.h"

namespace media {

// Maximum number of output streams that can be open simultaneously.
static const int kMaxOutputStreams = 50;

// Maximum buffer size that CoreAudio can support, used by low latency path.
static const int kMaxLowLatencyBufferSize = 2047;

static bool HasAudioHardware(AudioObjectPropertySelector selector) {
  AudioDeviceID output_device_id = kAudioObjectUnknown;
  const AudioObjectPropertyAddress property_address = {
    selector,
    kAudioObjectPropertyScopeGlobal,            // mScope
    kAudioObjectPropertyElementMaster           // mElement
  };
  size_t output_device_id_size = sizeof(output_device_id);
  OSStatus err = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                            &property_address,
                                            0,     // inQualifierDataSize
                                            NULL,  // inQualifierData
                                            &output_device_id_size,
                                            &output_device_id);
  return err == kAudioHardwareNoError &&
      output_device_id != kAudioObjectUnknown;
}

// Returns true if the default input device is the same as
// the default output device.
static bool HasUnifiedDefaultIO() {
  AudioDeviceID input_id, output_id;

  AudioObjectPropertyAddress pa;
  pa.mSelector = kAudioHardwarePropertyDefaultInputDevice;
  pa.mScope = kAudioObjectPropertyScopeGlobal;
  pa.mElement = kAudioObjectPropertyElementMaster;
  UInt32 size = sizeof(input_id);

  // Get the default input.
  OSStatus result = AudioObjectGetPropertyData(
      kAudioObjectSystemObject,
      &pa,
      0,
      0,
      &size,
      &input_id);

  if (result != noErr)
    return false;

  // Get the default output.
  pa.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  result = AudioObjectGetPropertyData(
      kAudioObjectSystemObject,
      &pa,
      0,
      0,
      &size,
      &output_id);

  if (result != noErr)
    return false;

  return input_id == output_id;
}

static void GetAudioDeviceInfo(bool is_input,
                               media::AudioDeviceNames* device_names) {
  DCHECK(device_names);
  device_names->clear();

  // Query the number of total devices.
  AudioObjectPropertyAddress property_address = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };
  UInt32 size = 0;
  OSStatus result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                                   &property_address,
                                                   0,
                                                   NULL,
                                                   &size);
  if (result || !size)
    return;

  int device_count = size / sizeof(AudioDeviceID);

  // Get the array of device ids for all the devices, which includes both
  // input devices and output devices.
  scoped_ptr_malloc<AudioDeviceID>
      devices(reinterpret_cast<AudioDeviceID*>(malloc(size)));
  AudioDeviceID* device_ids = devices.get();
  result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                      &property_address,
                                      0,
                                      NULL,
                                      &size,
                                      device_ids);
  if (result)
    return;

  // Iterate over all available devices to gather information.
  for (int i = 0; i < device_count; ++i) {
    int channels = 0;
    // Get the number of input or output channels of the device.
    property_address.mScope = is_input ?
        kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
    property_address.mSelector = kAudioDevicePropertyStreamConfiguration;
    result = AudioObjectGetPropertyDataSize(device_ids[i],
                                            &property_address,
                                            0,
                                            NULL,
                                            &size);
    if (result)
      continue;

    scoped_ptr_malloc<AudioBufferList>
        buffer(reinterpret_cast<AudioBufferList*>(malloc(size)));
    AudioBufferList* buffer_list = buffer.get();
    result = AudioObjectGetPropertyData(device_ids[i],
                                        &property_address,
                                        0,
                                        NULL,
                                        &size,
                                        buffer_list);
    if (result)
      continue;

    for (uint32 j = 0; j < buffer_list->mNumberBuffers; ++j)
      channels += buffer_list->mBuffers[j].mNumberChannels;

    // Exclude those devices without the type of channel we are interested in.
    if (!channels)
      continue;

    // Get device UID.
    CFStringRef uid = NULL;
    size = sizeof(uid);
    property_address.mSelector = kAudioDevicePropertyDeviceUID;
    property_address.mScope = kAudioObjectPropertyScopeGlobal;
    result = AudioObjectGetPropertyData(device_ids[i],
                                        &property_address,
                                        0,
                                        NULL,
                                        &size,
                                        &uid);
    if (result)
      continue;

    // Get device name.
    CFStringRef name = NULL;
    property_address.mSelector = kAudioObjectPropertyName;
    property_address.mScope = kAudioObjectPropertyScopeGlobal;
    result = AudioObjectGetPropertyData(device_ids[i],
                                        &property_address,
                                        0,
                                        NULL,
                                        &size,
                                        &name);
    if (result) {
      if (uid)
        CFRelease(uid);
      continue;
    }

    // Store the device name and UID.
    media::AudioDeviceName device_name;
    device_name.device_name = base::SysCFStringRefToUTF8(name);
    device_name.unique_id = base::SysCFStringRefToUTF8(uid);
    device_names->push_back(device_name);

    // We are responsible for releasing the returned CFObject.  See the
    // comment in the AudioHardware.h for constant
    // kAudioDevicePropertyDeviceUID.
    if (uid)
      CFRelease(uid);
    if (name)
      CFRelease(name);
  }
}

static AudioDeviceID GetAudioDeviceIdByUId(bool is_input,
                                           const std::string& device_id) {
  AudioObjectPropertyAddress property_address = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };
  AudioDeviceID audio_device_id = kAudioObjectUnknown;
  UInt32 device_size = sizeof(audio_device_id);
  OSStatus result = -1;

  if (device_id == AudioManagerBase::kDefaultDeviceId) {
    // Default Device.
    property_address.mSelector = is_input ?
        kAudioHardwarePropertyDefaultInputDevice :
        kAudioHardwarePropertyDefaultOutputDevice;

    result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                        &property_address,
                                        0,
                                        0,
                                        &device_size,
                                        &audio_device_id);
  } else {
    // Non-default device.
    base::mac::ScopedCFTypeRef<CFStringRef>
        uid(base::SysUTF8ToCFStringRef(device_id));
    AudioValueTranslation value;
    value.mInputData = &uid;
    value.mInputDataSize = sizeof(CFStringRef);
    value.mOutputData = &audio_device_id;
    value.mOutputDataSize = device_size;
    UInt32 translation_size = sizeof(AudioValueTranslation);

    property_address.mSelector = kAudioHardwarePropertyDeviceForUID;
    result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                        &property_address,
                                        0,
                                        0,
                                        &translation_size,
                                        &value);
  }

  if (result) {
    OSSTATUS_DLOG(WARNING, result) << "Unable to query device " << device_id
                                   << " for AudioDeviceID";
  }

  return audio_device_id;
}

AudioManagerMac::AudioManagerMac() {
  SetMaxOutputStreamsAllowed(kMaxOutputStreams);
}

AudioManagerMac::~AudioManagerMac() {
  Shutdown();
}

bool AudioManagerMac::HasAudioOutputDevices() {
  return HasAudioHardware(kAudioHardwarePropertyDefaultOutputDevice);
}

bool AudioManagerMac::HasAudioInputDevices() {
  return HasAudioHardware(kAudioHardwarePropertyDefaultInputDevice);
}

void AudioManagerMac::GetAudioInputDeviceNames(
    media::AudioDeviceNames* device_names) {
  // This is needed because AudioObjectGetPropertyDataSize has memory leak
  // when there is no soundcard in the machine.
  if (!HasAudioInputDevices())
    return;

  GetAudioDeviceInfo(true, device_names);
  if (!device_names->empty()) {
    // Prepend the default device to the list since we always want it to be
    // on the top of the list for all platforms. There is no duplicate
    // counting here since the default device has been abstracted out before.
    media::AudioDeviceName name;
    name.device_name = AudioManagerBase::kDefaultDeviceName;
    name.unique_id =  AudioManagerBase::kDefaultDeviceId;
    device_names->push_front(name);
  }
}

AudioOutputStream* AudioManagerMac::MakeLinearOutputStream(
    const AudioParameters& params) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  return new PCMQueueOutAudioOutputStream(this, params);
}

AudioOutputStream* AudioManagerMac::MakeLowLatencyOutputStream(
    const AudioParameters& params) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());

  // TODO(crogers): remove once we properly handle input device selection.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableWebAudioInput)) {
    if (HasUnifiedDefaultIO())
      return new AudioHardwareUnifiedStream(this, params);

    // kAudioDeviceUnknown translates to "use default" here.
    return new AudioSynchronizedStream(this,
                                       params,
                                       kAudioDeviceUnknown,
                                       kAudioDeviceUnknown);
  }

  return new AUAudioOutputStream(this, params);
}

AudioInputStream* AudioManagerMac::MakeLinearInputStream(
    const AudioParameters& params, const std::string& device_id) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  return new PCMQueueInAudioInputStream(this, params);
}

AudioInputStream* AudioManagerMac::MakeLowLatencyInputStream(
    const AudioParameters& params, const std::string& device_id) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  // Gets the AudioDeviceID that refers to the AudioOutputDevice with the device
  // unique id. This AudioDeviceID is used to set the device for Audio Unit.
  AudioDeviceID audio_device_id = GetAudioDeviceIdByUId(true, device_id);
  AudioInputStream* stream = NULL;
  if (audio_device_id != kAudioObjectUnknown)
    stream = new AUAudioInputStream(this, params, audio_device_id);

  return stream;
}

AudioManager* CreateAudioManager() {
  return new AudioManagerMac();
}

AudioParameters AudioManagerMac::GetPreferredLowLatencyOutputStreamParameters(
    const AudioParameters& params) {
  // Applications should use their own preferred buffer size when no resampler
  // is needed, and Apple CoreAudio can accept any buffer size up to 2047.
  int native_sample_rate = GetAudioHardwareSampleRate();
  int buffer_size = GetAudioHardwareBufferSize();
  if (native_sample_rate == params.sample_rate() &&
      params.frames_per_buffer() <= kMaxLowLatencyBufferSize) {
    buffer_size = params.frames_per_buffer();
  }

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                         params.channel_layout(),
                         native_sample_rate,
                         16,
                         buffer_size);
}

}  // namespace media
