// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.externalnav;

import android.content.Intent;

import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.util.IntentUtils;

import java.security.SecureRandom;
import java.util.Arrays;

/**
 * This class hold a token for the most recent launched external intent that has
 * user gesture. If the external intent resolves to chrome itself, the user
 * gesture will be applied to the new url request. Since there could be at most
 * one intent chooser at a time, this class only stores the most recent intent
 * that has user gesture. A previously launched intent with user gesture will
 * have its token invalidated if a new one comes.
 */
public class IntentWithGesturesHandler {
    /**
     * Extra to record whether the intent is launched by user gesture.
     */
    public static final String EXTRA_USER_GESTURE_TOKEN =
            "org.chromium.chrome.browser.user_gesture_token";

    private static final Object INSTANCE_LOCK = new Object();
    private static IntentWithGesturesHandler sIntentWithGesturesHandler;
    private SecureRandom mSecureRandom;
    private byte[] mIntentToken;
    private String mUri;

    /**
     * Get the singleton instance of this object.
     */
    public static IntentWithGesturesHandler getInstance() {
        synchronized (INSTANCE_LOCK) {
            if (sIntentWithGesturesHandler == null) {
                sIntentWithGesturesHandler = new IntentWithGesturesHandler();
            }
        }
        return sIntentWithGesturesHandler;
    }

    private IntentWithGesturesHandler() {
        mSecureRandom = new SecureRandom();
    }

    /**
     * Generate a new token for the intent that has user gesture. This will
     * invalidate the token on the previously launched intent with user gesture.
     *
     * @param intent Intent with user gesture.
     */
    public void onNewIntentWithGesture(Intent intent) {
        mIntentToken = new byte[32];
        mSecureRandom.nextBytes(mIntentToken);
        intent.putExtra(EXTRA_USER_GESTURE_TOKEN, mIntentToken);
        mUri = IntentHandler.getUrlFromIntent(intent);
    }

    /**
     * Get the user gesture from the intent and clear the stored token.
     *
     * @param intent Intent that is used to launch chrome.
     * @return true if the intent has user gesture, or false otherwise.
     */
    public boolean getUserGestureAndClear(Intent intent) {
        if (mIntentToken == null || mUri == null) return false;
        byte[] bytes = IntentUtils.safeGetByteArrayExtra(intent, EXTRA_USER_GESTURE_TOKEN);
        boolean ret = (bytes != null) && Arrays.equals(bytes, mIntentToken)
                && mUri.equals(IntentHandler.getUrlFromIntent(intent));
        clear();
        return ret;
    }

    /**
     * Clear the gesture token.
     */
    public void clear() {
        mIntentToken = null;
        mUri = null;
    }
}
