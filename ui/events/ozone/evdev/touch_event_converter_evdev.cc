// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_event_converter_evdev.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#include <cmath>
#include <limits>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/scoped_vector.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/devices/device_util_linux.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_switches.h"

namespace {

struct TouchCalibration {
  int bezel_left;
  int bezel_right;
  int bezel_top;
  int bezel_bottom;
};

void GetTouchCalibration(TouchCalibration* cal) {
  std::vector<std::string> parts;
  if (Tokenize(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                   switches::kTouchCalibration),
               ",", &parts) >= 4) {
    if (!base::StringToInt(parts[0], &cal->bezel_left))
      DLOG(ERROR) << "Incorrect left border calibration value passed.";
    if (!base::StringToInt(parts[1], &cal->bezel_right))
      DLOG(ERROR) << "Incorrect right border calibration value passed.";
    if (!base::StringToInt(parts[2], &cal->bezel_top))
      DLOG(ERROR) << "Incorrect top border calibration value passed.";
    if (!base::StringToInt(parts[3], &cal->bezel_bottom))
      DLOG(ERROR) << "Incorrect bottom border calibration value passed.";
  }
}

}  // namespace

namespace ui {

TouchEventConverterEvdev::InProgressEvents::InProgressEvents()
    : x_(0),
      y_(0),
      id_(-1),
      finger_(-1),
      type_(ET_UNKNOWN),
      radius_x_(0),
      radius_y_(0),
      pressure_(0) {
}

TouchEventConverterEvdev::TouchEventConverterEvdev(
    int fd,
    base::FilePath path,
    int id,
    InputDeviceType type,
    const TouchEventDispatchCallback& touch_callback)
    : EventConverterEvdev(fd, path, id, type),
      touch_callback_(touch_callback),
      syn_dropped_(false),
      is_type_a_(false),
      current_slot_(0) {
}

TouchEventConverterEvdev::~TouchEventConverterEvdev() {
  Stop();
  close(fd_);
}

void TouchEventConverterEvdev::Initialize(const EventDeviceInfo& info) {
  pressure_min_ = info.GetAbsMinimum(ABS_MT_PRESSURE);
  pressure_max_ = info.GetAbsMaximum(ABS_MT_PRESSURE);
  x_min_tuxels_ = info.GetAbsMinimum(ABS_MT_POSITION_X);
  x_num_tuxels_ = info.GetAbsMaximum(ABS_MT_POSITION_X) - x_min_tuxels_ + 1;
  y_min_tuxels_ = info.GetAbsMinimum(ABS_MT_POSITION_Y);
  y_num_tuxels_ = info.GetAbsMaximum(ABS_MT_POSITION_Y) - y_min_tuxels_ + 1;

  // Apply --touch-calibration.
  if (type() == INPUT_DEVICE_INTERNAL) {
    TouchCalibration cal = {};
    GetTouchCalibration(&cal);
    x_min_tuxels_ += cal.bezel_left;
    x_num_tuxels_ -= cal.bezel_left + cal.bezel_right;
    y_min_tuxels_ += cal.bezel_top;
    y_num_tuxels_ -= cal.bezel_top + cal.bezel_bottom;

    VLOG(1) << "applying touch calibration: "
            << base::StringPrintf("[%d, %d, %d, %d]", cal.bezel_left,
                                  cal.bezel_right, cal.bezel_top,
                                  cal.bezel_bottom);
  }

  native_size_ = gfx::Size(x_num_tuxels_, y_num_tuxels_);

  for (int i = 0;
       i < std::min<int>(info.GetAbsMaximum(ABS_MT_SLOT) + 1, MAX_FINGERS);
       ++i) {
    events_[i].finger_ = info.GetSlotValue(ABS_MT_TRACKING_ID, i);
    events_[i].type_ =
        events_[i].finger_ < 0 ? ET_TOUCH_RELEASED : ET_TOUCH_PRESSED;
    events_[i].x_ = info.GetSlotValue(ABS_MT_POSITION_X, i);
    events_[i].y_ = info.GetSlotValue(ABS_MT_POSITION_Y, i);
    events_[i].radius_x_ = info.GetSlotValue(ABS_MT_TOUCH_MAJOR, i);
    events_[i].radius_y_ = info.GetSlotValue(ABS_MT_TOUCH_MINOR, i);
    events_[i].pressure_ = info.GetSlotValue(ABS_MT_PRESSURE, i);
  }
}

bool TouchEventConverterEvdev::Reinitialize() {
  EventDeviceInfo info;
  if (info.Initialize(fd_)) {
    Initialize(info);
    return true;
  }
  return false;
}

bool TouchEventConverterEvdev::HasTouchscreen() const {
  return true;
}

gfx::Size TouchEventConverterEvdev::GetTouchscreenSize() const {
  return native_size_;
}

void TouchEventConverterEvdev::OnFileCanReadWithoutBlocking(int fd) {
  input_event inputs[MAX_FINGERS * 6 + 1];
  ssize_t read_size = read(fd, inputs, sizeof(inputs));
  if (read_size < 0) {
    if (errno == EINTR || errno == EAGAIN)
      return;
    if (errno != ENODEV)
      PLOG(ERROR) << "error reading device " << path_.value();
    Stop();
    return;
  }

  if (ignore_events_)
    return;

  for (unsigned i = 0; i < read_size / sizeof(*inputs); i++) {
    ProcessInputEvent(inputs[i]);
  }
}

void TouchEventConverterEvdev::ProcessInputEvent(const input_event& input) {
  if (input.type == EV_SYN) {
    ProcessSyn(input);
  } else if(syn_dropped_) {
    // Do nothing. This branch indicates we have lost sync with the driver.
  } else if (input.type == EV_ABS) {
    if (current_slot_ >= MAX_FINGERS) {
      LOG(ERROR) << "too many touch events: " << current_slot_;
      return;
    }
    ProcessAbs(input);
  } else if (input.type == EV_KEY) {
    switch (input.code) {
      case BTN_TOUCH:
        break;
      default:
        NOTIMPLEMENTED() << "invalid code for EV_KEY: " << input.code;
    }
  } else {
    NOTIMPLEMENTED() << "invalid type: " << input.type;
  }
}

void TouchEventConverterEvdev::ProcessAbs(const input_event& input) {
  switch (input.code) {
    case ABS_MT_TOUCH_MAJOR:
      altered_slots_.set(current_slot_);
      // TODO(spang): If we have all of major, minor, and orientation,
      // we can scale the ellipse correctly. However on the Pixel we get
      // neither minor nor orientation, so this is all we can do.
      events_[current_slot_].radius_x_ = input.value / 2.0f;
      break;
    case ABS_MT_TOUCH_MINOR:
      altered_slots_.set(current_slot_);
      events_[current_slot_].radius_y_ = input.value / 2.0f;
      break;
    case ABS_MT_POSITION_X:
      altered_slots_.set(current_slot_);
      events_[current_slot_].x_ = input.value;
      break;
    case ABS_MT_POSITION_Y:
      altered_slots_.set(current_slot_);
      events_[current_slot_].y_ = input.value;
      break;
    case ABS_MT_TRACKING_ID:
      altered_slots_.set(current_slot_);
      if (input.value < 0) {
        events_[current_slot_].type_ = ET_TOUCH_RELEASED;
      } else {
        events_[current_slot_].finger_ = input.value;
        events_[current_slot_].type_ = ET_TOUCH_PRESSED;
      }
      break;
    case ABS_MT_PRESSURE:
      altered_slots_.set(current_slot_);
      events_[current_slot_].pressure_ = input.value - pressure_min_;
      events_[current_slot_].pressure_ /= pressure_max_ - pressure_min_;
      break;
    case ABS_MT_SLOT:
      if (input.value >= MAX_FINGERS) {
        LOG(ERROR) << "multi-touch slot " << input.value
                   << " exceeds MAX_FINGERS";
        break;
      }
      current_slot_ = input.value;
      altered_slots_.set(current_slot_);
      break;
    default:
      DVLOG(5) << "unhandled code for EV_ABS: " << input.code;
  }
}

void TouchEventConverterEvdev::ProcessSyn(const input_event& input) {
  switch (input.code) {
    case SYN_REPORT:
      if (syn_dropped_) {
        // Have to re-initialize.
        if (Reinitialize()) {
          syn_dropped_ = false;
          altered_slots_.reset();
        } else {
          LOG(ERROR) << "failed to re-initialize device info";
        }
      } else {
        ReportEvents(base::TimeDelta::FromMicroseconds(
            input.time.tv_sec * 1000000 + input.time.tv_usec));
      }
      if (is_type_a_)
        current_slot_ = 0;
      break;
    case SYN_MT_REPORT:
      // For type A devices, we just get a stream of all current contacts,
      // in some arbitrary order.
      events_[current_slot_++].type_ = ET_TOUCH_PRESSED;
      is_type_a_ = true;
      break;
    case SYN_DROPPED:
      // Some buffer has overrun. We ignore all events up to and
      // including the next SYN_REPORT.
      syn_dropped_ = true;
      break;
    default:
      NOTIMPLEMENTED() << "invalid code for EV_SYN: " << input.code;
  }
}

void TouchEventConverterEvdev::ReportEvent(int touch_id,
                                           const InProgressEvents& event,
                                           const base::TimeDelta& timestamp) {
  touch_callback_.Run(TouchEventParams(
      id_, touch_id, event.type_, gfx::PointF(event.x_, event.y_),
      gfx::Vector2dF(event.radius_x_, event.radius_y_), event.pressure_,
      timestamp));
}

void TouchEventConverterEvdev::ReportEvents(base::TimeDelta delta) {
  for (int i = 0; i < MAX_FINGERS; i++) {
    if (altered_slots_[i]) {
      ReportEvent(i, events_[i], delta);

      // Subsequent events for this finger will be touch-move until it
      // is released.
      events_[i].type_ = ET_TOUCH_MOVED;
    }
  }
  altered_slots_.reset();
}

}  // namespace ui
