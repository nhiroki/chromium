// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/host_port_allocator.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/string_number_conversions.h"
#include "googleurl/src/gurl.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/host/network_settings.h"
#include "third_party/libjingle/source/talk/base/basicpacketsocketfactory.h"

namespace remoting {

namespace {

class HostPortAllocatorSession
    : public cricket::HttpPortAllocatorSessionBase {
 public:
  HostPortAllocatorSession(
      cricket::HttpPortAllocatorBase* allocator,
      const std::string& channel_name,
      int component,
      const std::vector<talk_base::SocketAddress>& stun_hosts,
      const std::vector<std::string>& relay_hosts,
      const std::string& relay,
      const scoped_refptr<net::URLRequestContextGetter>& url_context);
  virtual ~HostPortAllocatorSession();

  // cricket::HttpPortAllocatorBase overrides.
  virtual void ConfigReady(cricket::PortConfiguration* config) OVERRIDE;
  virtual void SendSessionRequest(const std::string& host, int port) OVERRIDE;

 private:
  void OnSessionRequestDone(UrlFetcher* url_fetcher,
                            const net::URLRequestStatus& status,
                            int response_code,
                            const std::string& response);

  scoped_refptr<net::URLRequestContextGetter> url_context_;
  std::set<UrlFetcher*> url_fetchers_;

  DISALLOW_COPY_AND_ASSIGN(HostPortAllocatorSession);
};

HostPortAllocatorSession::HostPortAllocatorSession(
    cricket::HttpPortAllocatorBase* allocator,
    const std::string& channel_name,
    int component,
    const std::vector<talk_base::SocketAddress>& stun_hosts,
    const std::vector<std::string>& relay_hosts,
    const std::string& relay,
    const scoped_refptr<net::URLRequestContextGetter>& url_context)
    : HttpPortAllocatorSessionBase(
        allocator, channel_name, component, stun_hosts, relay_hosts, relay, ""),
      url_context_(url_context) {
}

HostPortAllocatorSession::~HostPortAllocatorSession() {
  STLDeleteElements(&url_fetchers_);
}

void HostPortAllocatorSession::ConfigReady(cricket::PortConfiguration* config) {
  // Filter out non-UDP relay ports, so that we don't try using TCP.
  for (cricket::PortConfiguration::RelayList::iterator relay =
           config->relays.begin(); relay != config->relays.end(); ++relay) {
    cricket::PortConfiguration::PortList filtered_ports;
    for (cricket::PortConfiguration::PortList::iterator port =
             relay->ports.begin(); port != relay->ports.end(); ++port) {
      if (port->proto == cricket::PROTO_UDP) {
        filtered_ports.push_back(*port);
      }
    }
    relay->ports = filtered_ports;
  }
  cricket::BasicPortAllocatorSession::ConfigReady(config);
}

void HostPortAllocatorSession::SendSessionRequest(const std::string& host,
                                                  int port) {
  GURL url("https://" + host + ":" + base::IntToString(port) +
           GetSessionRequestUrl() + "&sn=1");
  scoped_ptr<UrlFetcher> url_fetcher(new UrlFetcher(url, UrlFetcher::GET));
  url_fetcher->SetRequestContext(url_context_);
  url_fetcher->SetHeader("X-Talk-Google-Relay-Auth", relay_token());
  url_fetcher->SetHeader("X-Google-Relay-Auth", relay_token());
  url_fetcher->SetHeader("X-Stream-Type", channel_name());
  url_fetcher->Start(base::Bind(&HostPortAllocatorSession::OnSessionRequestDone,
                                base::Unretained(this), url_fetcher.get()));
  url_fetchers_.insert(url_fetcher.release());
}

void HostPortAllocatorSession::OnSessionRequestDone(
    UrlFetcher* url_fetcher,
    const net::URLRequestStatus& status,
    int response_code,
    const std::string& response) {
  url_fetchers_.erase(url_fetcher);
  delete url_fetcher;

  if (response_code != net::HTTP_OK) {
    LOG(WARNING) << "Received error when allocating relay session: "
                 << response_code;
    TryCreateRelaySession();
    return;
  }

  ReceiveSessionResponse(response);
}

}  // namespace

// static
scoped_ptr<HostPortAllocator> HostPortAllocator::Create(
    const scoped_refptr<net::URLRequestContextGetter>& url_context,
    const NetworkSettings& network_settings) {
  scoped_ptr<talk_base::NetworkManager> network_manager(
      new talk_base::BasicNetworkManager());
  scoped_ptr<talk_base::PacketSocketFactory> socket_factory(
      new talk_base::BasicPacketSocketFactory());
  scoped_ptr<HostPortAllocator> result(
      new HostPortAllocator(url_context, network_manager.Pass(),
                            socket_factory.Pass()));

  // We always use PseudoTcp to provide a reliable channel. It
  // provides poor performance when combined with TCP-based transport,
  // so we have to disable TCP ports.
  int flags = cricket::PORTALLOCATOR_DISABLE_TCP;
  if (network_settings.nat_traversal_mode !=
      NetworkSettings::NAT_TRAVERSAL_ENABLED) {
    flags |= cricket::PORTALLOCATOR_DISABLE_STUN |
        cricket::PORTALLOCATOR_DISABLE_RELAY;
  }
  result->set_flags(flags);
  result->SetPortRange(network_settings.min_port,
                       network_settings.max_port);

  return result.Pass();
}

HostPortAllocator::HostPortAllocator(
    const scoped_refptr<net::URLRequestContextGetter>& url_context,
    scoped_ptr<talk_base::NetworkManager> network_manager,
    scoped_ptr<talk_base::PacketSocketFactory> socket_factory)
    : HttpPortAllocatorBase(network_manager.get(), socket_factory.get(), ""),
      url_context_(url_context),
      network_manager_(network_manager.Pass()),
      socket_factory_(socket_factory.Pass()) {
}

HostPortAllocator::~HostPortAllocator() {
}

cricket::PortAllocatorSession* HostPortAllocator::CreateSession(
    const std::string& channel_name,
    int component) {
  return new HostPortAllocatorSession(
      this, channel_name, component, stun_hosts(),
      relay_hosts(), relay_token(), url_context_);
}

}  // namespace remoting
