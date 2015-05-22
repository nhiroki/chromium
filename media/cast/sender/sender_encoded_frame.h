// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_SENDER_SENDER_ENCODED_FRAME_H_
#define MEDIA_CAST_SENDER_SENDER_ENCODED_FRAME_H_

#include "media/cast/net/cast_transport_config.h"

namespace media {
namespace cast {

// Extends EncodedFrame with additional fields used within the sender-side of
// the library.
struct SenderEncodedFrame : public EncodedFrame {
  SenderEncodedFrame();
  ~SenderEncodedFrame() final;

  // The amount of real-world time it took to encode the frame, divided by the
  // maximum amount of time allowed.  Example: For the software VP8 encoder,
  // this would be the elapsed encode time (according to the base::TimeTicks
  // clock) divided by the VideoFrame's duration.
  //
  // Meaningful values are non-negative, with 0.0 [impossibly] representing 0%
  // utilization, 1.0 representing 100% utilization, and values greater than 1.0
  // indicating the encode time took longer than the media duration of the
  // frame.  Negative values indicate the field was not computed.
  double deadline_utilization;

  // The amount of "lossiness" needed to encode the frame within the targeted
  // bandwidth.  More-complex frame content and/or lower target encode bitrates
  // will cause this value to rise.
  //
  // Meaningful values are non-negative, with 0.0 indicating the frame is very
  // simple and/or the target encode bitrate is very large, 1.0 indicating the
  // frame contains very complex content and/or the target encode bitrate is
  // very small, and values greater than 1.0 indicating the encoder cannot
  // encode the frame within the target bitrate (even at its lowest quality
  // setting).  Negative values indicate the field was not computed.
  double lossy_utilization;
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_SENDER_SENDER_ENCODED_FRAME_H_
