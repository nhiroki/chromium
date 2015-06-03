// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_video_frame_provider.h"

namespace cc {

FakeVideoFrameProvider::FakeVideoFrameProvider()
    : frame_(NULL), client_(NULL) {}

FakeVideoFrameProvider::~FakeVideoFrameProvider() {
  if (client_)
    client_->StopUsingProvider();
}

bool FakeVideoFrameProvider::UpdateCurrentFrame(base::TimeTicks deadline_min,
                                                base::TimeTicks deadline_max) {
  return false;
}

void FakeVideoFrameProvider::SetVideoFrameProviderClient(Client* client) {
  client_ = client;
}

bool FakeVideoFrameProvider::HasCurrentFrame() {
  return frame_;
}

scoped_refptr<media::VideoFrame> FakeVideoFrameProvider::GetCurrentFrame() {
  return frame_;
}

}  // namespace cc
