// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;


import org.chromium.base.ThreadUtils;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content_public.browser.JavaScriptCallback;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * This class is used to provide callback hooks for tests and related classes.
 */
public class TestCallbackHelperContainer {
    private final TestContentViewClient mTestContentViewClient;
    private TestWebContentsObserver mTestWebContentsObserver;

    public TestCallbackHelperContainer(final ContentViewCore contentViewCore) {
        mTestContentViewClient = new TestContentViewClient();
        contentViewCore.setContentViewClient(mTestContentViewClient);
        // TODO(yfriedman): Change callers to be executed on the UI thread. Unfortunately this is
        // super convenient as the caller is nearly always on the test thread which is fine to block
        // and it's cumbersome to keep bouncing to the UI thread.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestWebContentsObserver = new TestWebContentsObserver(contentViewCore);
            }
        });
    }

    protected TestCallbackHelperContainer(
            TestContentViewClient viewClient, TestWebContentsObserver contentsObserver) {
        mTestContentViewClient = viewClient;
        mTestWebContentsObserver = contentsObserver;
    }

    /**
     * CallbackHelper for OnPageFinished.
     */
    public static class OnPageFinishedHelper extends CallbackHelper {
        private String mUrl;
        public void notifyCalled(String url) {
            mUrl = url;
            notifyCalled();
        }
        public String getUrl() {
            assert getCallCount() > 0;
            return mUrl;
        }
    }

    /**
     * CallbackHelper for OnPageStarted.
     */
    public static class OnPageStartedHelper extends CallbackHelper {
        private String mUrl;
        public void notifyCalled(String url) {
            mUrl = url;
            notifyCalled();
        }
        public String getUrl() {
            assert getCallCount() > 0;
            return mUrl;
        }
    }

    /**
     * CallbackHelper for OnReceivedError.
     */
    public static class OnReceivedErrorHelper extends CallbackHelper {
        private int mErrorCode;
        private String mDescription;
        private String mFailingUrl;
        public void notifyCalled(int errorCode, String description, String failingUrl) {
            mErrorCode = errorCode;
            mDescription = description;
            mFailingUrl = failingUrl;
            notifyCalled();
        }
        public int getErrorCode() {
            assert getCallCount() > 0;
            return mErrorCode;
        }
        public String getDescription() {
            assert getCallCount() > 0;
            return mDescription;
        }
        public String getFailingUrl() {
            assert getCallCount() > 0;
            return mFailingUrl;
        }
    }

    /**
     * CallbackHelper for OnEvaluateJavaScriptResult.
     * This class wraps the evaluation of JavaScript code allowing test code to
     * synchronously evaluate JavaScript and then test the result.
     */
    public static class OnEvaluateJavaScriptResultHelper extends CallbackHelper {
        private String mJsonResult;

        /**
         * Starts evaluation of a given JavaScript code on a given contentViewCore.
         * @param contentViewCore A ContentViewCore instance to be used.
         * @param code A JavaScript code to be evaluated.
         */
        public void evaluateJavaScript(ContentViewCore contentViewCore, String code) {
            JavaScriptCallback callback =
                new JavaScriptCallback() {
                    @Override
                    public void handleJavaScriptResult(String jsonResult) {
                        notifyCalled(jsonResult);
                    }
                };
            contentViewCore.evaluateJavaScript(code, callback);
            mJsonResult = null;
        }

        /**
         * Returns true if the evaluation started by evaluateJavaScript() has completed.
         */
        public boolean hasValue() {
            return mJsonResult != null;
        }

        /**
         * Returns the JSON result of a previously completed JavaScript evaluation and
         * resets the helper to accept new evaluations.
         * @return String JSON result of a previously completed JavaScript evaluation.
         */
        public String getJsonResultAndClear() {
            assert hasValue();
            String result = mJsonResult;
            mJsonResult = null;
            return result;
        }


        /**
         * Returns a criteria that checks that the evaluation has finished.
         */
        public Criteria getHasValueCriteria() {
            return new Criteria() {
                @Override
                public boolean isSatisfied() {
                    return hasValue();
                }
            };
        }

        /**
         * Waits till the JavaScript evaluation finishes and returns true if a value was returned,
         * false if it timed-out.
         */
        public boolean waitUntilHasValue(long timeout, TimeUnit timeoutUnits)
                throws InterruptedException, TimeoutException {
            waitUntilCriteria(getHasValueCriteria(), timeout, timeoutUnits);
            return hasValue();
        }

        public boolean waitUntilHasValue() throws InterruptedException, TimeoutException {
            waitUntilCriteria(getHasValueCriteria());
            return hasValue();
        }

        public void notifyCalled(String jsonResult) {
            assert !hasValue();
            mJsonResult = jsonResult;
            notifyCalled();
        }
    }

    /**
     * CallbackHelper for OnStartContentIntent.
     */
    public static class OnStartContentIntentHelper extends CallbackHelper {
        private String mIntentUrl;
        public void notifyCalled(String intentUrl) {
            mIntentUrl = intentUrl;
            notifyCalled();
        }
        public String getIntentUrl() {
            assert getCallCount() > 0;
            return mIntentUrl;
        }
    }

    public OnPageStartedHelper getOnPageStartedHelper() {
        return mTestWebContentsObserver.getOnPageStartedHelper();
    }

    public OnPageFinishedHelper getOnPageFinishedHelper() {
        return mTestWebContentsObserver.getOnPageFinishedHelper();
    }

    public OnReceivedErrorHelper getOnReceivedErrorHelper() {
        return mTestWebContentsObserver.getOnReceivedErrorHelper();
    }

    public OnStartContentIntentHelper getOnStartContentIntentHelper() {
        return mTestContentViewClient.getOnStartContentIntentHelper();
    }
}
