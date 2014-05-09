// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_CAPTURE_VIDEO_CAPTURE_DEVICE_FACTORY_H_
#define MEDIA_VIDEO_CAPTURE_VIDEO_CAPTURE_DEVICE_FACTORY_H_

#include "base/threading/thread_checker.h"
#include "media/video/capture/video_capture_device.h"

namespace media {

// VideoCaptureDeviceFactory is the base class for creation of video capture
// devices in the different platforms. VCDFs are created by MediaStreamManager
// on IO thread and plugged into VideoCaptureManager, who owns and operates them
// in Device Thread (a.k.a. Audio Thread).
class MEDIA_EXPORT VideoCaptureDeviceFactory {
 public:
  VideoCaptureDeviceFactory();
  virtual ~VideoCaptureDeviceFactory() {}

  // Creates a VideoCaptureDevice object. Returns NULL if something goes wrong.
  virtual scoped_ptr<VideoCaptureDevice> Create(
      const VideoCaptureDevice::Name& device_name);

  // Gets the names of all video capture devices connected to this computer.
  virtual void GetDeviceNames(VideoCaptureDevice::Names* device_names);

  // Gets the supported formats of a particular device attached to the system.
  // This method should be called before allocating or starting a device. In
  // case format enumeration is not supported, or there was a problem, the
  // formats array will be empty.
  virtual void GetDeviceSupportedFormats(
      const VideoCaptureDevice::Name& device,
      VideoCaptureFormats* supported_formats);

 protected:
  base::ThreadChecker thread_checker_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoCaptureDeviceFactory);
};

}  // namespace media

#endif  // MEDIA_VIDEO_CAPTURE_VIDEO_CAPTURE_DEVICE_FACTORY_H_
