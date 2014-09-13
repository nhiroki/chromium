// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_MP2T_ES_PARSER_MPEG1AUDIO_H_
#define MEDIA_FORMATS_MP2T_ES_PARSER_MPEG1AUDIO_H_

#include <list>
#include <utility>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/media_export.h"
#include "media/base/media_log.h"
#include "media/formats/mp2t/es_parser.h"

namespace media {
class AudioTimestampHelper;
class BitReader;
class OffsetByteQueue;
class StreamParserBuffer;
}

namespace media {
namespace mp2t {

class MEDIA_EXPORT EsParserMpeg1Audio : public EsParser {
 public:
  typedef base::Callback<void(const AudioDecoderConfig&)> NewAudioConfigCB;

  EsParserMpeg1Audio(const NewAudioConfigCB& new_audio_config_cb,
                     const EmitBufferCB& emit_buffer_cb,
                     const LogCB& log_cb);
  virtual ~EsParserMpeg1Audio();

  // EsParser implementation.
  virtual void Flush() OVERRIDE;

 private:
  // Used to link a PTS with a byte position in the ES stream.
  typedef std::pair<int64, base::TimeDelta> EsPts;
  typedef std::list<EsPts> EsPtsList;

  struct Mpeg1AudioFrame;

  // EsParser implementation.
  virtual bool ParseFromEsQueue() OVERRIDE;
  virtual void ResetInternal() OVERRIDE;

  // Synchronize the stream on a Mpeg1 audio syncword (consuming bytes from
  // |es_queue_| if needed).
  // Returns true when a full Mpeg1 audio frame has been found: in that case
  // |mpeg1audio_frame| structure is filled up accordingly.
  // Returns false otherwise (no Mpeg1 audio syncword found or partial Mpeg1
  // audio frame).
  bool LookForMpeg1AudioFrame(Mpeg1AudioFrame* mpeg1audio_frame);

  // Signal any audio configuration change (if any).
  // Return false if the current audio config is not
  // a supported Mpeg1 audio config.
  bool UpdateAudioConfiguration(const uint8* mpeg1audio_header);

  void SkipMpeg1AudioFrame(const Mpeg1AudioFrame& mpeg1audio_frame);

  LogCB log_cb_;

  // Callbacks:
  // - to signal a new audio configuration,
  // - to send ES buffers.
  NewAudioConfigCB new_audio_config_cb_;
  EmitBufferCB emit_buffer_cb_;

  // Interpolated PTS for frames that don't have one.
  scoped_ptr<AudioTimestampHelper> audio_timestamp_helper_;

  // Last audio config.
  AudioDecoderConfig last_audio_decoder_config_;

  DISALLOW_COPY_AND_ASSIGN(EsParserMpeg1Audio);
};

}  // namespace mp2t
}  // namespace media

#endif  // MEDIA_FORMATS_MP2T_ES_PARSER_MPEG1AUDIO_H_
