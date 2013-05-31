/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DeviceMotionDispatcher_h
#define DeviceMotionDispatcher_h

#include "DeviceMotionData.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"
#include <public/WebDeviceMotionListener.h>

namespace WebKit {
class WebDeviceMotionData;
}

namespace WebCore {

class DeviceMotionController;
class DeviceMotionData;

// This class listens to device motion data and dispatches it to all
// listening controllers.
class DeviceMotionDispatcher : public WebKit::WebDeviceMotionListener {
public:
    static DeviceMotionDispatcher& instance();

    void addController(DeviceMotionController*);
    void removeController(DeviceMotionController*);

    // Note that the returned object is owned by this class.
    // FIXME: make the return value const, see crbug.com/233174.
    DeviceMotionData* latestDeviceMotionData();

    // This method is called every time new device motion data is available.
    virtual void didChangeDeviceMotion(const WebKit::WebDeviceMotionData&) OVERRIDE;

private:
    DeviceMotionDispatcher();
    ~DeviceMotionDispatcher();

    void startListening();
    void stopListening();

    void purgeControllers();

    RefPtr<DeviceMotionData> m_lastDeviceMotionData;
    Vector<DeviceMotionController*> m_controllers;
};

} // namespace WebCore

#endif // DeviceMotionDispatcher_h
