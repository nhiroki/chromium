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

#include "WebUserGestureToken.h"

#include <gtest/gtest.h>
#include "WebScopedUserGesture.h"
#include "WebUserGestureIndicator.h"
#include "platform/UserGestureIndicator.h"

using namespace blink;
using namespace WebCore;

namespace {

class GestureHandlerTest : public WebUserGestureHandler {
public:
    GestureHandlerTest()
        : m_reached(false) { }

    void onGesture()
    {
        m_reached = true;
    }

    bool m_reached;
};

TEST(WebUserGestureTokenTest, Basic)
{
    WebUserGestureToken token;
    EXPECT_FALSE(token.hasGestures());

    {
        WebScopedUserGesture indicator(token);
        EXPECT_FALSE(WebUserGestureIndicator::isProcessingUserGesture());
    }

    {
        UserGestureIndicator indicator(DefinitelyProcessingNewUserGesture);
        EXPECT_TRUE(WebUserGestureIndicator::isProcessingUserGesture());
        token = WebUserGestureIndicator::currentUserGestureToken();
    }

    EXPECT_TRUE(token.hasGestures());
    EXPECT_FALSE(WebUserGestureIndicator::isProcessingUserGesture());

    {
        WebScopedUserGesture indicator(token);
        EXPECT_TRUE(WebUserGestureIndicator::isProcessingUserGesture());
        WebUserGestureIndicator::consumeUserGesture();
        EXPECT_FALSE(WebUserGestureIndicator::isProcessingUserGesture());
    }

    EXPECT_FALSE(token.hasGestures());

    {
        WebScopedUserGesture indicator(token);
        EXPECT_FALSE(WebUserGestureIndicator::isProcessingUserGesture());
    }

    {
        GestureHandlerTest handler;
        WebUserGestureIndicator::setHandler(&handler);
        UserGestureIndicator indicator(DefinitelyProcessingNewUserGesture);
        EXPECT_TRUE(handler.m_reached);
        WebUserGestureIndicator::setHandler(0);
    }
}

}
