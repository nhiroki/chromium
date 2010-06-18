/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_SOCKETS)

#include "WebSocketChannel.h"

#include "CookieJar.h"
#include "Document.h"
#include "Logging.h"
#include "PlatformString.h"
#include "ScriptExecutionContext.h"
#include "SocketStreamError.h"
#include "SocketStreamHandle.h"
#include "StringHash.h"
#include "WebSocketChannelClient.h"
#include "WebSocketHandshake.h"

#include <wtf/text/CString.h>
#include <wtf/Deque.h>
#include <wtf/FastMalloc.h>
#include <wtf/HashMap.h>

namespace WebCore {

WebSocketChannel::WebSocketChannel(ScriptExecutionContext* context, WebSocketChannelClient* client, const KURL& url, const String& protocol)
    : m_context(context)
    , m_client(client)
    , m_handshake(url, protocol, context)
    , m_buffer(0)
    , m_bufferSize(0)
    , m_resumeTimer(this, &WebSocketChannel::resumeTimerFired)
    , m_suspended(false)
    , m_closed(false)
    , m_unhandledBufferedAmount(0)
{
}

WebSocketChannel::~WebSocketChannel()
{
    fastFree(m_buffer);
}

void WebSocketChannel::connect()
{
    LOG(Network, "WebSocketChannel %p connect", this);
    ASSERT(!m_handle);
    ASSERT(!m_suspended);
    m_handshake.reset();
    ref();
    m_handle = SocketStreamHandle::create(m_handshake.url(), this);
}

bool WebSocketChannel::send(const String& msg)
{
    LOG(Network, "WebSocketChannel %p send %s", this, msg.utf8().data());
    ASSERT(m_handle);
    ASSERT(!m_suspended);
    Vector<char> buf;
    buf.append('\0');  // frame type
    buf.append(msg.utf8().data(), msg.utf8().length());
    buf.append('\xff');  // frame end
    return m_handle->send(buf.data(), buf.size());
}

unsigned long WebSocketChannel::bufferedAmount() const
{
    LOG(Network, "WebSocketChannel %p bufferedAmount", this);
    ASSERT(m_handle);
    ASSERT(!m_suspended);
    return m_handle->bufferedAmount();
}

void WebSocketChannel::close()
{
    LOG(Network, "WebSocketChannel %p close", this);
    ASSERT(!m_suspended);
    if (m_handle)
        m_handle->close();  // will call didClose()
}

void WebSocketChannel::disconnect()
{
    LOG(Network, "WebSocketChannel %p disconnect", this);
    m_handshake.clearScriptExecutionContext();
    m_client = 0;
    m_context = 0;
    if (m_handle)
        m_handle->close();
}

void WebSocketChannel::suspend()
{
    m_suspended = true;
}

void WebSocketChannel::resume()
{
    m_suspended = false;
    if ((m_buffer || m_closed) && m_client && !m_resumeTimer.isActive())
        m_resumeTimer.startOneShot(0);
}

void WebSocketChannel::didOpen(SocketStreamHandle* handle)
{
    LOG(Network, "WebSocketChannel %p didOpen", this);
    ASSERT(handle == m_handle);
    if (!m_context)
        return;
    const CString& handshakeMessage = m_handshake.clientHandshakeMessage();
    if (!handle->send(handshakeMessage.data(), handshakeMessage.length())) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error sending handshake message.", 0, m_handshake.clientOrigin());
        handle->close();
    }
}

void WebSocketChannel::didClose(SocketStreamHandle* handle)
{
    LOG(Network, "WebSocketChannel %p didClose", this);
    ASSERT_UNUSED(handle, handle == m_handle || !m_handle);
    m_closed = true;
    if (m_handle) {
        m_unhandledBufferedAmount = m_handle->bufferedAmount();
        if (m_suspended)
            return;
        WebSocketChannelClient* client = m_client;
        m_client = 0;
        m_context = 0;
        m_handle = 0;
        if (client)
            client->didClose(m_unhandledBufferedAmount);
    }
    deref();
}

void WebSocketChannel::didReceiveData(SocketStreamHandle* handle, const char* data, int len)
{
    LOG(Network, "WebSocketChannel %p didReceiveData %d", this, len);
    RefPtr<WebSocketChannel> protect(this); // The client can close the channel, potentially removing the last reference.
    ASSERT(handle == m_handle);
    if (!m_context) {
        return;
    }
    if (!m_client) {
        handle->close();
        return;
    }
    if (!appendToBuffer(data, len)) {
        handle->close();
        return;
    }
    while (!m_suspended && m_client && m_buffer)
        if (!processBuffer())
            break;
}

void WebSocketChannel::didFail(SocketStreamHandle* handle, const SocketStreamError&)
{
    LOG(Network, "WebSocketChannel %p didFail", this);
    ASSERT(handle == m_handle || !m_handle);
    handle->close();
}

void WebSocketChannel::didReceiveAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&)
{
}

void WebSocketChannel::didCancelAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&)
{
}

bool WebSocketChannel::appendToBuffer(const char* data, int len)
{
    char* newBuffer = 0;
    if (tryFastMalloc(m_bufferSize + len).getValue(newBuffer)) {
        if (m_buffer)
            memcpy(newBuffer, m_buffer, m_bufferSize);
        memcpy(newBuffer + m_bufferSize, data, len);
        fastFree(m_buffer);
        m_buffer = newBuffer;
        m_bufferSize += len;
        return true;
    }
    m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, String::format("WebSocket frame (at %d bytes) is too long.", m_bufferSize + len), 0, m_handshake.clientOrigin());
    return false;
}

void WebSocketChannel::skipBuffer(int len)
{
    ASSERT(len <= m_bufferSize);
    m_bufferSize -= len;
    if (!m_bufferSize) {
        fastFree(m_buffer);
        m_buffer = 0;
        return;
    }
    memmove(m_buffer, m_buffer + len, m_bufferSize);
}

bool WebSocketChannel::processBuffer()
{
    ASSERT(!m_suspended);
    ASSERT(m_client);
    ASSERT(m_buffer);

    if (m_handshake.mode() == WebSocketHandshake::Incomplete) {
        int headerLength = m_handshake.readServerHandshake(m_buffer, m_bufferSize);
        if (headerLength <= 0)
            return false;
        if (m_handshake.mode() == WebSocketHandshake::Connected) {
            if (!m_handshake.serverSetCookie().isEmpty()) {
                if (m_context->isDocument()) {
                    Document* document = static_cast<Document*>(m_context);
                    if (cookiesEnabled(document)) {
                        ExceptionCode ec; // Exception (for sandboxed documents) ignored.
                        document->setCookie(m_handshake.serverSetCookie(), ec);
                    }
                }
            }
            // FIXME: handle set-cookie2.
            LOG(Network, "WebSocketChannel %p connected", this);
            skipBuffer(headerLength);
            m_client->didConnect();
            LOG(Network, "remaining in read buf %ul", m_bufferSize);
            return m_buffer;
        }
        LOG(Network, "WebSocketChannel %p connection failed", this);
        skipBuffer(headerLength);
        if (!m_closed)
            m_handle->close();
        return false;
    }
    if (m_handshake.mode() != WebSocketHandshake::Connected)
        return false;

    const char* nextFrame = m_buffer;
    const char* p = m_buffer;
    const char* end = p + m_bufferSize;

    unsigned char frameByte = static_cast<unsigned char>(*p++);
    if ((frameByte & 0x80) == 0x80) {
        int length = 0;
        while (p < end) {
            if (length > std::numeric_limits<int>::max() / 128) {
                LOG(Network, "frame length overflow %d", length);
                skipBuffer(p + length - m_buffer);
                m_client->didReceiveMessageError();
                if (!m_client)
                    return false;
                if (!m_closed)
                    m_handle->close();
                return false;
            }
            char msgByte = *p;
            length = length * 128 + (msgByte & 0x7f);
            ++p;
            if (!(msgByte & 0x80))
                break;
        }
        if (p + length < end) {
            p += length;
            nextFrame = p;
            skipBuffer(nextFrame - m_buffer);
            m_client->didReceiveMessageError();
            return m_buffer;
        }
        return false;
    }

    const char* msgStart = p;
    while (p < end && *p != '\xff')
        ++p;
    if (p < end && *p == '\xff') {
        int msgLength = p - msgStart;
        ++p;
        nextFrame = p;
        if (frameByte == 0x00) {
            String msg = String::fromUTF8(msgStart, msgLength);
            skipBuffer(nextFrame - m_buffer);
            m_client->didReceiveMessage(msg);
        } else {
            skipBuffer(nextFrame - m_buffer);
            m_client->didReceiveMessageError();
        }
        return m_buffer;
    }
    return false;
}

void WebSocketChannel::resumeTimerFired(Timer<WebSocketChannel>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_resumeTimer);

    RefPtr<WebSocketChannel> protect(this); // The client can close the channel, potentially removing the last reference.
    while (!m_suspended && m_client && m_buffer)
        if (!processBuffer())
            break;
    if (!m_suspended && m_client && m_closed && m_handle)
        didClose(m_handle.get());
}

}  // namespace WebCore

#endif  // ENABLE(WEB_SOCKETS)
