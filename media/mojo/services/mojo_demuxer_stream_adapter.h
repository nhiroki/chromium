// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_DEMUXER_STREAM_ADAPTER_H_
#define MEDIA_MOJO_SERVICES_MOJO_DEMUXER_STREAM_ADAPTER_H_

#include <queue>

#include "base/memory/weak_ptr.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/demuxer_stream.h"
#include "media/base/video_decoder_config.h"
#include "media/mojo/interfaces/demuxer_stream.mojom.h"

namespace media {

// This class acts as a MojoRendererService-side stub for a real DemuxerStream
// that is part of a Pipeline in a remote application. Roughly speaking, it
// takes a interfaces::DemuxerStreamPtr and exposes it as a DemuxerStream for
// use by
// media components.
class MojoDemuxerStreamAdapter : public DemuxerStream {
 public:
  // |demuxer_stream| is connected to the interfaces::DemuxerStream that |this|
  // will
  //     become the client of.
  // |stream_ready_cb| will be invoked when |demuxer_stream| has fully
  //     initialized and |this| is ready for use.
  // NOTE: Illegal to call any methods until |stream_ready_cb| is invoked.
  MojoDemuxerStreamAdapter(interfaces::DemuxerStreamPtr demuxer_stream,
                           const base::Closure& stream_ready_cb);
  ~MojoDemuxerStreamAdapter() override;

  // DemuxerStream implementation.
  void Read(const ReadCB& read_cb) override;
  AudioDecoderConfig audio_decoder_config() override;
  VideoDecoderConfig video_decoder_config() override;
  Type type() const override;
  void EnableBitstreamConverter() override;
  bool SupportsConfigChanges() override;
  VideoRotation video_rotation() override;

 private:
  void OnStreamReady(interfaces::DemuxerStream::Type type,
                     mojo::ScopedDataPipeConsumerHandle pipe,
                     interfaces::AudioDecoderConfigPtr audio_config,
                     interfaces::VideoDecoderConfigPtr video_config);

  // The callback from |demuxer_stream_| that a read operation has completed.
  // |read_cb| is a callback from the client who invoked Read() on |this|.
  void OnBufferReady(interfaces::DemuxerStream::Status status,
                     interfaces::DecoderBufferPtr buffer,
                     interfaces::AudioDecoderConfigPtr audio_config,
                     interfaces::VideoDecoderConfigPtr video_config);

  void UpdateConfig(interfaces::AudioDecoderConfigPtr audio_config,
                    interfaces::VideoDecoderConfigPtr video_config);

  // See constructor for descriptions.
  interfaces::DemuxerStreamPtr demuxer_stream_;
  base::Closure stream_ready_cb_;

  // The last ReadCB received through a call to Read().
  // Used to store the results of OnBufferReady() in the event it is called
  // with DemuxerStream::Status::kConfigChanged and we don't have an up to
  // date AudioDecoderConfig yet. In that case we can't forward the results
  // on to the caller of Read() until OnAudioDecoderConfigChanged is observed.
  DemuxerStream::ReadCB read_cb_;

  // The current config.
  AudioDecoderConfig audio_config_;
  VideoDecoderConfig video_config_;

  DemuxerStream::Type type_;

  // DataPipe for deserializing the data section of DecoderBuffers from.
  mojo::ScopedDataPipeConsumerHandle stream_pipe_;

  base::WeakPtrFactory<MojoDemuxerStreamAdapter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MojoDemuxerStreamAdapter);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_DEMUXER_STREAM_ADAPTER_H_
