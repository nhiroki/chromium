/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "modules/websockets/WebSocketPerMessageDeflate.h"
#include "wtf/Vector.h"
#include "wtf/text/StringHash.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <iterator>

using namespace WebCore;

namespace {

TEST(WebSocketPerMessageDeflateTest, TestDeflateHelloTakeOver)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, true, false, false, "Hello", 5);
    WebSocketFrame f2(opcode, true, false, false, "Hello", 5);

    ASSERT_TRUE(c.deflate(f1));
    EXPECT_EQ(7u, f1.payloadLength);
    EXPECT_EQ(0, memcmp("\xf2\x48\xcd\xc9\xc9\x07\x00", f1.payload, f1.payloadLength));
    EXPECT_TRUE(f1.compress);
    EXPECT_TRUE(f1.final);

    c.resetDeflateBuffer();
    ASSERT_TRUE(c.deflate(f2));
    EXPECT_EQ(5u, f2.payloadLength);
    EXPECT_EQ(0, memcmp("\xf2\x00\x11\x00\x00", f2.payload, f2.payloadLength));
    EXPECT_TRUE(f2.compress);
    EXPECT_TRUE(f2.final);
}

TEST(WebSocketPerMessageTest, TestDeflateHelloNoTakeOver)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::DoNotTakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, true, false, false, "Hello", 5);
    WebSocketFrame f2(opcode, true, false, false, "Hello", 5);

    ASSERT_TRUE(c.deflate(f1));
    EXPECT_EQ(7u, f1.payloadLength);
    EXPECT_EQ(0, memcmp("\xf2\x48\xcd\xc9\xc9\x07\x00", f1.payload, f1.payloadLength));
    EXPECT_TRUE(f1.compress);
    EXPECT_TRUE(f1.final);

    c.resetDeflateBuffer();
    ASSERT_TRUE(c.deflate(f2));
    EXPECT_EQ(7u, f2.payloadLength);
    EXPECT_EQ(0, memcmp("\xf2\x48\xcd\xc9\xc9\x07\x00", f2.payload, f2.payloadLength));
    EXPECT_TRUE(f2.compress);
    EXPECT_TRUE(f2.final);
}

TEST(WebSocketPerMessageDeflateTest, TestDeflateInflateMultipleFrame)
{
    WebSocketPerMessageDeflate c;
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeText;
    c.enable(8, WebSocketDeflater::DoNotTakeOverContext);
    size_t length = 1024;
    std::string payload;
    std::string expected;
    std::string actual;
    std::string inflated;
    // Generate string by a linear congruential generator.
    uint64_t r = 0;
    for (size_t i = 0; i < length; ++i) {
        payload += 'a' + (r % 25);
        r = (r * 12345 + 1103515245) % (static_cast<uint64_t>(1) << 31);
    }

    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame frame(opcode, true, false, false, &payload[0], payload.size());
    ASSERT_TRUE(c.deflate(frame));
    ASSERT_TRUE(frame.final);
    ASSERT_TRUE(frame.compress);
    expected = std::string(frame.payload, frame.payloadLength);
    for (size_t i = 0; i < length; ++i) {
        c.resetDeflateBuffer();
        // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
        WebSocketFrame frame(opcode, i == length - 1, false, false, &payload[i], 1);
        ASSERT_TRUE(c.deflate(frame));
        ASSERT_EQ(i == length - 1, frame.final);
        ASSERT_EQ(!i, frame.compress);
        actual += std::string(frame.payload, frame.payloadLength);
    }
    EXPECT_EQ(expected, actual);

    for (size_t i = 0; i < actual.size(); ++i) {
        c.resetInflateBuffer();
        // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
        WebSocketFrame frame(opcode, i == length - 1, !i, false, &actual[i], 1);
        ASSERT_TRUE(c.inflate(frame));
        ASSERT_EQ(i == length - 1, frame.final);
        ASSERT_FALSE(frame.compress);
        inflated += std::string(frame.payload, frame.payloadLength);
    }
    EXPECT_EQ(payload, inflated);
}

TEST(WebSocketPerMessageDeflateTest, TestDeflateBinary)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeBinary;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, true, false, false, "Hello", 5);

    ASSERT_TRUE(c.deflate(f1));
    EXPECT_EQ(7u, f1.payloadLength);
    EXPECT_EQ(0, memcmp("\xf2\x48\xcd\xc9\xc9\x07\x00", f1.payload, f1.payloadLength));
    EXPECT_EQ(opcode, f1.opCode);
    EXPECT_TRUE(f1.compress);
    EXPECT_TRUE(f1.final);
}

TEST(WebSocketPerMessageDeflateTest, TestDeflateEmptyFrame)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, false, false, false, "Hello", 5);
    WebSocketFrame f2(opcode, true, false, false, "", 0);

    ASSERT_TRUE(c.deflate(f1));
    EXPECT_EQ(0u, f1.payloadLength);
    EXPECT_FALSE(f1.final);
    EXPECT_TRUE(f1.compress);

    c.resetDeflateBuffer();
    ASSERT_TRUE(c.deflate(f2));
    EXPECT_EQ(7u, f2.payloadLength);
    EXPECT_EQ(0, memcmp("\xf2\x48\xcd\xc9\xc9\x07\x00", f2.payload, f2.payloadLength));
    EXPECT_TRUE(f2.final);
    EXPECT_FALSE(f2.compress);
}

TEST(WebSocketPerMessageDeflateTest, TestControlMessage)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeClose;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, true, false, false, "Hello", 5);

    ASSERT_TRUE(c.deflate(f1));
    EXPECT_TRUE(f1.final);
    EXPECT_FALSE(f1.compress);
    EXPECT_EQ(std::string("Hello"), std::string(f1.payload, f1.payloadLength));
}

TEST(WebSocketPerMessageDeflateTest, TestDeflateControlMessageBetweenTextFrames)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode close = WebSocketFrame::OpCodeClose;
    WebSocketFrame::OpCode text = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(text, false, false, false, "Hello", 5);
    WebSocketFrame f2(close, true, false, false, "close", 5);
    WebSocketFrame f3(text, true, false, false, "", 0);

    std::vector<char> compressed;
    ASSERT_TRUE(c.deflate(f1));
    EXPECT_FALSE(f1.final);
    EXPECT_TRUE(f1.compress);
    std::copy(&f1.payload[0], &f1.payload[f1.payloadLength], std::inserter(compressed, compressed.end()));

    c.resetDeflateBuffer();
    ASSERT_TRUE(c.deflate(f2));
    EXPECT_TRUE(f2.final);
    EXPECT_FALSE(f2.compress);
    EXPECT_EQ(std::string("close"), std::string(f2.payload, f2.payloadLength));

    c.resetDeflateBuffer();
    ASSERT_TRUE(c.deflate(f3));
    EXPECT_TRUE(f3.final);
    EXPECT_FALSE(f3.compress);
    std::copy(&f3.payload[0], &f3.payload[f3.payloadLength], std::inserter(compressed, compressed.end()));

    EXPECT_EQ(7u, compressed.size());
    EXPECT_EQ(0, memcmp("\xf2\x48\xcd\xc9\xc9\x07\x00", &compressed[0], compressed.size()));
}

TEST(WebSocketPerMessageDeflateTest, TestInflate)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    std::string expected = "HelloHi!Hello";
    std::string actual;
    WebSocketFrame f1(opcode, true, true, false, "\xf2\x48\xcd\xc9\xc9\x07\x00", 7);
    WebSocketFrame f2(opcode, true, false, false, "Hi!", 3);
    WebSocketFrame f3(opcode, true, true, false, "\xf2\x00\x11\x00\x00", 5);

    ASSERT_TRUE(c.inflate(f1));
    EXPECT_EQ(5u, f1.payloadLength);
    EXPECT_EQ(std::string("Hello"), std::string(f1.payload, f1.payloadLength));
    EXPECT_FALSE(f1.compress);
    EXPECT_TRUE(f1.final);

    c.resetInflateBuffer();
    ASSERT_TRUE(c.inflate(f2));
    EXPECT_EQ(3u, f2.payloadLength);
    EXPECT_EQ(std::string("Hi!"), std::string(f2.payload, f2.payloadLength));
    EXPECT_FALSE(f2.compress);
    EXPECT_TRUE(f2.final);

    c.resetInflateBuffer();
    ASSERT_TRUE(c.inflate(f3));
    EXPECT_EQ(5u, f3.payloadLength);
    EXPECT_EQ(std::string("Hello"), std::string(f3.payload, f3.payloadLength));
    EXPECT_FALSE(f3.compress);
    EXPECT_TRUE(f3.final);
}

TEST(WebSocketPerMessageDeflateTest, TestInflateEmptyFrame)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, false, true, false, "", 0);
    WebSocketFrame f2(opcode, true, false, false, "\xf2\x48\xcd\xc9\xc9\x07\x00", 7);

    ASSERT_TRUE(c.inflate(f1));
    EXPECT_EQ(0u, f1.payloadLength);
    EXPECT_FALSE(f1.compress);
    EXPECT_FALSE(f1.final);

    c.resetInflateBuffer();
    ASSERT_TRUE(c.inflate(f2));
    EXPECT_EQ(5u, f2.payloadLength);
    EXPECT_EQ(std::string("Hello"), std::string(f2.payload, f2.payloadLength));
    EXPECT_FALSE(f2.compress);
    EXPECT_TRUE(f2.final);
}

TEST(WebSocketPerMessageDeflateTest, TestInflateControlMessageBetweenTextFrames)
{
    WebSocketPerMessageDeflate c;
    c.enable(8, WebSocketDeflater::TakeOverContext);
    WebSocketFrame::OpCode close = WebSocketFrame::OpCodeClose;
    WebSocketFrame::OpCode text = WebSocketFrame::OpCodeText;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(text, false, true, false, "\xf2\x48", 2);
    WebSocketFrame f2(close, true, false, false, "close", 5);
    WebSocketFrame f3(text, true, false, false, "\xcd\xc9\xc9\x07\x00", 5);

    std::vector<char> decompressed;
    ASSERT_TRUE(c.inflate(f1));
    EXPECT_FALSE(f1.final);
    EXPECT_FALSE(f1.compress);
    std::copy(&f1.payload[0], &f1.payload[f1.payloadLength], std::inserter(decompressed, decompressed.end()));

    c.resetInflateBuffer();
    ASSERT_TRUE(c.inflate(f2));
    EXPECT_TRUE(f2.final);
    EXPECT_FALSE(f2.compress);
    EXPECT_EQ(std::string("close"), std::string(f2.payload, f2.payloadLength));

    c.resetInflateBuffer();
    ASSERT_TRUE(c.inflate(f3));
    std::copy(&f3.payload[0], &f3.payload[f3.payloadLength], std::inserter(decompressed, decompressed.end()));
    EXPECT_TRUE(f3.final);
    EXPECT_FALSE(f3.compress);

    EXPECT_EQ(std::string("Hello"), std::string(&decompressed[0], decompressed.size()));
}

TEST(WebSocketPerMessageDeflateTest, TestNotEnabled)
{
    WebSocketPerMessageDeflate c;
    WebSocketFrame::OpCode opcode = WebSocketFrame::OpCodeClose;
    // WebSocketFrame(opcode, final, compress, masked, payload, payloadLength)
    WebSocketFrame f1(opcode, true, true, false, "Hello", 5);
    WebSocketFrame f2(opcode, true, true, false, "\xf2\x48\xcd\xc9\xc9\x07\x00", 7);

    // deflate and inflate return true and do nothing if it is not enabled.
    ASSERT_TRUE(c.deflate(f1));
    ASSERT_TRUE(f1.compress);
    ASSERT_TRUE(c.inflate(f2));
    ASSERT_TRUE(f2.compress);
}

bool processResponse(const HashMap<String, String>& serverParameters)
{
    return WebSocketPerMessageDeflate().createExtensionProcessor()->processResponse(serverParameters);
}

TEST(WebSocketPerMessageDeflateTest, TestValidNegotiationResponse)
{
    {
        HashMap<String, String> params;
        EXPECT_TRUE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "15");
        EXPECT_TRUE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "8");
        EXPECT_TRUE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "15");
        params.add("c2s_no_context_takeover", String());
        EXPECT_TRUE(processResponse(params));
    }
}

TEST(WebSocketPerMessageDeflateTest, TestInvalidNegotiationResponse)
{
    {
        HashMap<String, String> params;
        params.add("method", "deflate");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("foo", "");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("foo", "bar");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "16");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "7");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "+15");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "0x9");
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("c2s_max_window_bits", "08");
        EXPECT_FALSE(processResponse(params));
    }
    // Although the spec allows s2c_no_context_takeover or s2c_max_window_bits to be passed for a client,
    // we don't allow it because it is not in accordance with a negotiation request by this implementation.
    {
        HashMap<String, String> params;
        params.add("s2c_no_context_takeover", String());
        EXPECT_FALSE(processResponse(params));
    }
    {
        HashMap<String, String> params;
        params.add("s2c_max_window_bits", "15");
        EXPECT_FALSE(processResponse(params));
    }
}

TEST(WebSocketPerMessageDeflateTest, TestNegotiationRequest)
{
    String actual = WebSocketPerMessageDeflate().createExtensionProcessor()->handshakeString();
    EXPECT_EQ(String("permessage-deflate; c2s_max_window_bits"), actual);
}
} // namespace
