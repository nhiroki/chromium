// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/test_tools/quic_dispatcher_peer.h"

#include "net/tools/quic/quic_dispatcher.h"
#include "net/tools/quic/quic_packet_writer_wrapper.h"

namespace net {
namespace tools {
namespace test {

// static
void QuicDispatcherPeer::SetTimeWaitListManager(
    QuicDispatcher* dispatcher,
    QuicTimeWaitListManager* time_wait_list_manager) {
  dispatcher->time_wait_list_manager_.reset(time_wait_list_manager);
}

// static
void QuicDispatcherPeer::UseWriter(QuicDispatcher* dispatcher,
                                   QuicPacketWriterWrapper* writer) {
  writer->set_writer(dispatcher->writer_.release());
  dispatcher->writer_.reset(writer);
}

// static
QuicPacketWriter* QuicDispatcherPeer::GetWriter(QuicDispatcher* dispatcher) {
  return dispatcher->writer_.get();
}

// static
void QuicDispatcherPeer::SetPacketWriterFactory(
    QuicDispatcher* dispatcher,
    QuicDispatcher::PacketWriterFactory* packet_writer_factory) {
  dispatcher->packet_writer_factory_.reset(packet_writer_factory);
}

// static
QuicEpollConnectionHelper* QuicDispatcherPeer::GetHelper(
    QuicDispatcher* dispatcher) {
  return dispatcher->helper_.get();
}

// static
QuicConnection* QuicDispatcherPeer::CreateQuicConnection(
    QuicDispatcher* dispatcher,
    QuicConnectionId connection_id,
    const IPEndPoint& server,
    const IPEndPoint& client) {
  return dispatcher->CreateQuicConnection(connection_id,
                                          server,
                                          client);
}

// static
QuicDispatcher::WriteBlockedList* QuicDispatcherPeer::GetWriteBlockedList(
    QuicDispatcher* dispatcher) {
  return &dispatcher->write_blocked_list_;
}

}  // namespace test
}  // namespace tools
}  // namespace net
