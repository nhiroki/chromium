// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_
#define NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/ct_verify_result.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/openssl_ssl_util.h"
#include "net/ssl/ssl_client_cert_type.h"
#include "net/ssl/ssl_config_service.h"

// Avoid including misc OpenSSL headers, i.e.:
// <openssl/bio.h>
typedef struct bio_st BIO;
// <openssl/evp.h>
typedef struct evp_pkey_st EVP_PKEY;
// <openssl/ssl.h>
typedef struct ssl_st SSL;
// <openssl/x509.h>
typedef struct x509_st X509;
// <openssl/ossl_type.h>
typedef struct x509_store_ctx_st X509_STORE_CTX;

namespace net {

class CertVerifier;
class CTVerifier;
class SingleRequestCertVerifier;
class SSLCertRequestInfo;
class SSLInfo;

// An SSL client socket implemented with OpenSSL.
class SSLClientSocketOpenSSL : public SSLClientSocket {
 public:
  // Takes ownership of the transport_socket, which may already be connected.
  // The given hostname will be compared with the name(s) in the server's
  // certificate during the SSL handshake.  ssl_config specifies the SSL
  // settings.
  SSLClientSocketOpenSSL(scoped_ptr<ClientSocketHandle> transport_socket,
                         const HostPortPair& host_and_port,
                         const SSLConfig& ssl_config,
                         const SSLClientSocketContext& context);
  ~SSLClientSocketOpenSSL() override;

  const HostPortPair& host_and_port() const { return host_and_port_; }
  const std::string& ssl_session_cache_shard() const {
    return ssl_session_cache_shard_;
  }

  // SSLClientSocket implementation.
  void GetSSLCertRequestInfo(SSLCertRequestInfo* cert_request_info) override;
  NextProtoStatus GetNextProto(std::string* proto) override;
  ChannelIDService* GetChannelIDService() const override;

  // SSLSocket implementation.
  int ExportKeyingMaterial(const base::StringPiece& label,
                           bool has_context,
                           const base::StringPiece& context,
                           unsigned char* out,
                           unsigned int outlen) override;
  int GetTLSUniqueChannelBinding(std::string* out) override;

  // StreamSocket implementation.
  int Connect(const CompletionCallback& callback) override;
  void Disconnect() override;
  bool IsConnected() const override;
  bool IsConnectedAndIdle() const override;
  int GetPeerAddress(IPEndPoint* address) const override;
  int GetLocalAddress(IPEndPoint* address) const override;
  const BoundNetLog& NetLog() const override;
  void SetSubresourceSpeculation() override;
  void SetOmniboxSpeculation() override;
  bool WasEverUsed() const override;
  bool UsingTCPFastOpen() const override;
  bool GetSSLInfo(SSLInfo* ssl_info) override;

  // Socket implementation.
  int Read(IOBuffer* buf,
           int buf_len,
           const CompletionCallback& callback) override;
  int Write(IOBuffer* buf,
            int buf_len,
            const CompletionCallback& callback) override;
  int SetReceiveBufferSize(int32 size) override;
  int SetSendBufferSize(int32 size) override;

 protected:
  // SSLClientSocket implementation.
  scoped_refptr<X509Certificate> GetUnverifiedServerCertificateChain()
      const override;

 private:
  class PeerCertificateChain;
  class SSLContext;
  friend class SSLClientSocket;
  friend class SSLContext;

  int Init();
  void DoReadCallback(int result);
  void DoWriteCallback(int result);

  bool DoTransportIO();
  int DoHandshake();
  int DoChannelIDLookup();
  int DoChannelIDLookupComplete(int result);
  int DoVerifyCert(int result);
  int DoVerifyCertComplete(int result);
  void DoConnectCallback(int result);
  void UpdateServerCert();
  void VerifyCT();

  void OnHandshakeIOComplete(int result);
  void OnSendComplete(int result);
  void OnRecvComplete(int result);

  int DoHandshakeLoop(int last_io_result);
  int DoReadLoop();
  int DoWriteLoop();
  int DoPayloadRead();
  int DoPayloadWrite();

  int BufferSend();
  int BufferRecv();
  void BufferSendComplete(int result);
  void BufferRecvComplete(int result);
  void TransportWriteComplete(int result);
  int TransportReadComplete(int result);

  // Callback from the SSL layer that indicates the remote server is requesting
  // a certificate for this client.
  int ClientCertRequestCallback(SSL* ssl);

  // CertVerifyCallback is called to verify the server's certificates. We do
  // verification after the handshake so this function only enforces that the
  // certificates don't change during renegotiation.
  int CertVerifyCallback(X509_STORE_CTX *store_ctx);

  // Callback from the SSL layer to check which NPN protocol we are supporting
  int SelectNextProtoCallback(unsigned char** out, unsigned char* outlen,
                              const unsigned char* in, unsigned int inlen);

  // Called during an operation on |transport_bio_|'s peer. Checks saved
  // transport error state and, if appropriate, returns an error through
  // OpenSSL's error system.
  long MaybeReplayTransportError(BIO *bio,
                                 int cmd,
                                 const char *argp, int argi, long argl,
                                 long retvalue);

  // Callback from the SSL layer when an operation is performed on
  // |transport_bio_|'s peer.
  static long BIOCallback(BIO *bio,
                          int cmd,
                          const char *argp, int argi, long argl,
                          long retvalue);

  // Called after the initial handshake completes and after the server
  // certificate has been verified. The order of handshake completion and
  // certificate verification depends on whether the connection was false
  // started. After both have happened (thus calling this twice), the session is
  // safe to cache and will be cached.
  void MaybeCacheSession();

  // Callback from the SSL layer when the internal state machine progresses. It
  // is used to listen for when the handshake completes entirely; |Connect| may
  // return early if false starting.
  void InfoCallback(int type, int val);

  // Adds the SignedCertificateTimestamps from ct_verify_result_ to |ssl_info|.
  // SCTs are held in three separate vectors in ct_verify_result, each
  // vetor representing a particular verification state, this method associates
  // each of the SCTs with the corresponding SCTVerifyStatus as it adds it to
  // the |ssl_info|.signed_certificate_timestamps list.
  void AddSCTInfoToSSLInfo(SSLInfo* ssl_info) const;

  // Returns a unique key string for the SSL session cache for
  // this socket.
  std::string GetSessionCacheKey() const;

  bool transport_send_busy_;
  bool transport_recv_busy_;

  // Buffers which are shared by BoringSSL and SSLClientSocketOpenSSL.
  // GrowableIOBuffer is used to keep ownership and setting offset.
  scoped_refptr<GrowableIOBuffer> send_buffer_;
  scoped_refptr<GrowableIOBuffer> recv_buffer_;

  CompletionCallback user_connect_callback_;
  CompletionCallback user_read_callback_;
  CompletionCallback user_write_callback_;

  // Used by Read function.
  scoped_refptr<IOBuffer> user_read_buf_;
  int user_read_buf_len_;

  // Used by Write function.
  scoped_refptr<IOBuffer> user_write_buf_;
  int user_write_buf_len_;

  // Used by DoPayloadRead() when attempting to fill the caller's buffer with
  // as much data as possible without blocking.
  // If DoPayloadRead() encounters an error after having read some data, stores
  // the result to return on the *next* call to DoPayloadRead().  A value > 0
  // indicates there is no pending result, otherwise 0 indicates EOF and < 0
  // indicates an error.
  int pending_read_error_;

  // If there is a pending read result, the OpenSSL result code (output of
  // SSL_get_error) associated with it.
  int pending_read_ssl_error_;

  // If there is a pending read result, the OpenSSLErrorInfo associated with it.
  OpenSSLErrorInfo pending_read_error_info_;

  // Used by TransportReadComplete() to signify an error reading from the
  // transport socket. A value of OK indicates the socket is still
  // readable. EOFs are mapped to ERR_CONNECTION_CLOSED.
  int transport_read_error_;

  // Used by TransportWriteComplete() and TransportReadComplete() to signify an
  // error writing to the transport socket. A value of OK indicates no error.
  int transport_write_error_;

  // Set when Connect finishes.
  scoped_ptr<PeerCertificateChain> server_cert_chain_;
  scoped_refptr<X509Certificate> server_cert_;
  CertVerifyResult server_cert_verify_result_;
  bool completed_connect_;

  // Set when Read() or Write() successfully reads or writes data to or from the
  // network.
  bool was_ever_used_;

  // Stores client authentication information between ClientAuthHandler and
  // GetSSLCertRequestInfo calls.
  bool client_auth_cert_needed_;
  // List of DER-encoded X.509 DistinguishedName of certificate authorities
  // allowed by the server.
  std::vector<std::string> cert_authorities_;
  // List of SSLClientCertType values for client certificates allowed by the
  // server.
  std::vector<SSLClientCertType> cert_key_types_;

  CertVerifier* const cert_verifier_;
  scoped_ptr<SingleRequestCertVerifier> verifier_;
  base::TimeTicks start_cert_verification_time_;

  // Certificate Transparency: Verifier and result holder.
  ct::CTVerifyResult ct_verify_result_;
  CTVerifier* cert_transparency_verifier_;

  // The service for retrieving Channel ID keys.  May be NULL.
  ChannelIDService* channel_id_service_;

  // OpenSSL stuff
  SSL* ssl_;
  BIO* transport_bio_;

  scoped_ptr<ClientSocketHandle> transport_;
  const HostPortPair host_and_port_;
  SSLConfig ssl_config_;
  // ssl_session_cache_shard_ is an opaque string that partitions the SSL
  // session cache. i.e. sessions created with one value will not attempt to
  // resume on the socket with a different value.
  const std::string ssl_session_cache_shard_;

  enum State {
    STATE_NONE,
    STATE_HANDSHAKE,
    STATE_CHANNEL_ID_LOOKUP,
    STATE_CHANNEL_ID_LOOKUP_COMPLETE,
    STATE_VERIFY_CERT,
    STATE_VERIFY_CERT_COMPLETE,
  };
  State next_handshake_state_;
  NextProtoStatus npn_status_;
  std::string npn_proto_;
  // Written by the |channel_id_service_|.
  std::string channel_id_private_key_;
  std::string channel_id_cert_;
  // True if channel ID extension was negotiated.
  bool channel_id_xtn_negotiated_;
  // True if the initial handshake has completed.
  bool handshake_completed_;
  // True if the initial handshake's certificate has been verified.
  bool certificate_verified_;
  // The request handle for |channel_id_service_|.
  ChannelIDService::RequestHandle channel_id_request_handle_;

  TransportSecurityState* transport_security_state_;

  CertPolicyEnforcer* const policy_enforcer_;

  // pinning_failure_log contains a message produced by
  // TransportSecurityState::CheckPublicKeyPins in the event of a
  // pinning failure. It is a (somewhat) human-readable string.
  std::string pinning_failure_log_;

  BoundNetLog net_log_;
  base::WeakPtrFactory<SSLClientSocketOpenSSL> weak_factory_;
};

}  // namespace net

#endif  // NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_
