// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_basic_stream.h"

#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_stream_parser.h"

namespace net {

HttpBasicStream::HttpBasicStream(ClientSocketHandle* connection)
    : read_buf_(new GrowableIOBuffer()),
      connection_(connection) {
}

int HttpBasicStream::InitializeStream(const HttpRequestInfo* request_info,
                                      const BoundNetLog& net_log,
                                      CompletionCallback* callback) {
  parser_.reset(new HttpStreamParser(connection_, request_info,
                                     read_buf_, net_log));
  connection_ = NULL;
  return OK;
}


int HttpBasicStream::SendRequest(const std::string& headers,
                                 UploadDataStream* request_body,
                                 HttpResponseInfo* response,
                                 CompletionCallback* callback) {
  DCHECK(parser_.get());
  return parser_->SendRequest(headers, request_body, response, callback);
}

HttpBasicStream::~HttpBasicStream() {}

uint64 HttpBasicStream::GetUploadProgress() const {
  return parser_->GetUploadProgress();
}

int HttpBasicStream::ReadResponseHeaders(CompletionCallback* callback) {
  return parser_->ReadResponseHeaders(callback);
}

const HttpResponseInfo* HttpBasicStream::GetResponseInfo() const {
  return parser_->GetResponseInfo();
}

int HttpBasicStream::ReadResponseBody(IOBuffer* buf, int buf_len,
                                      CompletionCallback* callback) {
  return parser_->ReadResponseBody(buf, buf_len, callback);
}

void HttpBasicStream::Close(bool not_reusable) {
  parser_->Close(not_reusable);
}

bool HttpBasicStream::IsResponseBodyComplete() const {
  return parser_->IsResponseBodyComplete();
}

bool HttpBasicStream::CanFindEndOfResponse() const {
  return parser_->CanFindEndOfResponse();
}

bool HttpBasicStream::IsMoreDataBuffered() const {
  return parser_->IsMoreDataBuffered();
}

bool HttpBasicStream::IsConnectionReused() const {
  return parser_->IsConnectionReused();
}

void HttpBasicStream::SetConnectionReused() {
  parser_->SetConnectionReused();
}

void HttpBasicStream::GetSSLInfo(SSLInfo* ssl_info) {
  parser_->GetSSLInfo(ssl_info);
}

void HttpBasicStream::GetSSLCertRequestInfo(
    SSLCertRequestInfo* cert_request_info) {
  parser_->GetSSLCertRequestInfo(cert_request_info);
}

}  // namespace net
