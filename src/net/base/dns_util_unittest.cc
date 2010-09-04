// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/dns_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

class DNSUtilTest : public testing::Test {
};

// IncludeNUL converts a char* to a std::string and includes the terminating
// NUL in the result.
static std::string IncludeNUL(const char* in) {
  return std::string(in, strlen(in) + 1);
}

TEST_F(DNSUtilTest, DNSDomainFromDot) {
  std::string out;

  EXPECT_TRUE(DNSDomainFromDot("", &out));
  EXPECT_EQ(out, IncludeNUL(""));
  EXPECT_TRUE(DNSDomainFromDot("com", &out));
  EXPECT_EQ(out, IncludeNUL("\003com"));
  EXPECT_TRUE(DNSDomainFromDot("google.com", &out));
  EXPECT_EQ(out, IncludeNUL("\x006google\003com"));
  EXPECT_TRUE(DNSDomainFromDot("www.google.com", &out));
  EXPECT_EQ(out, IncludeNUL("\003www\006google\003com"));

  // Label is 63 chars: still valid
  EXPECT_TRUE(DNSDomainFromDot("123456789a123456789a123456789a123456789a123456789a123456789a123", &out));
  EXPECT_EQ(out, IncludeNUL("\077123456789a123456789a123456789a123456789a123456789a123456789a123"));

  // Label is too long: invalid
  EXPECT_FALSE(DNSDomainFromDot("123456789a123456789a123456789a123456789a123456789a123456789a1234", &out));

  // 253 characters in the name: still valid
  EXPECT_TRUE(DNSDomainFromDot("123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123", &out));
  EXPECT_EQ(out, IncludeNUL("\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\011123456789\003123"));

  // 254 characters in the name: invalid
  EXPECT_FALSE(DNSDomainFromDot("123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.1234", &out));

  // Zero length labels should be dropped.
  EXPECT_TRUE(DNSDomainFromDot("www.google.com.", &out));
  EXPECT_EQ(out, IncludeNUL("\003www\006google\003com"));

  EXPECT_TRUE(DNSDomainFromDot(".google.com", &out));
  EXPECT_EQ(out, IncludeNUL("\006google\003com"));

  EXPECT_TRUE(DNSDomainFromDot("www..google.com", &out));
  EXPECT_EQ(out, IncludeNUL("\003www\006google\003com"));
}

TEST_F(DNSUtilTest, STD3ASCII) {
  EXPECT_TRUE(IsSTD3ASCIIValidCharacter('a'));
  EXPECT_TRUE(IsSTD3ASCIIValidCharacter('b'));
  EXPECT_TRUE(IsSTD3ASCIIValidCharacter('c'));
  EXPECT_TRUE(IsSTD3ASCIIValidCharacter('1'));
  EXPECT_TRUE(IsSTD3ASCIIValidCharacter('2'));
  EXPECT_TRUE(IsSTD3ASCIIValidCharacter('3'));

  EXPECT_FALSE(IsSTD3ASCIIValidCharacter('.'));
  EXPECT_FALSE(IsSTD3ASCIIValidCharacter('/'));
  EXPECT_FALSE(IsSTD3ASCIIValidCharacter('\x00'));
}

}  // namespace net
