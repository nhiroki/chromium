// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.ui.VSyncMonitor;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

/**
 * The window base class that has the minimum functionality.
 */
@JNINamespace("ui")
public class WindowAndroid {
    private static final String TAG = "WindowAndroid";

    // Native pointer to the c++ WindowAndroid object.
    private long mNativeWindowAndroid = 0;
    private final VSyncMonitor mVSyncMonitor;

    // A string used as a key to store intent errors in a bundle
    static final String WINDOW_CALLBACK_ERRORS = "window_callback_errors";

    // Error code returned when an Intent fails to start an Activity.
    public static final int START_INTENT_FAILURE = -1;

    protected Context mApplicationContext;
    protected SparseArray<IntentCallback> mOutstandingIntents;

    // Ideally, this would be a SparseArray<String>, but there's no easy way to store a
    // SparseArray<String> in a bundle during saveInstanceState(). So we use a HashMap and suppress
    // the Android lint warning "UseSparseArrays".
    protected HashMap<Integer, String> mIntentErrors;

    // We track all animations over content and provide a drawing placeholder for them.
    private HashSet<Animator> mAnimationsOverContent = new HashSet<Animator>();
    private View mAnimationPlaceholderView;

    private ViewGroup mKeyboardAccessoryView;

    /**
     * An interface to notify listeners of changes in the soft keyboard's visibility.
     */
    public interface KeyboardVisibilityListener {
        public void keyboardVisibilityChanged(boolean isShowing);
    }
    private LinkedList<KeyboardVisibilityListener> mKeyboardVisibilityListeners =
            new LinkedList<KeyboardVisibilityListener>();

    private final VSyncMonitor.Listener mVSyncListener = new VSyncMonitor.Listener() {
        @Override
        public void onVSync(VSyncMonitor monitor, long vsyncTimeMicros) {
            if (mNativeWindowAndroid != 0) {
                nativeOnVSync(mNativeWindowAndroid,
                              vsyncTimeMicros,
                              mVSyncMonitor.getVSyncPeriodInMicroseconds());
            }
        }
    };

    /**
     * @return true if onVSync handler is executing.
     * @see org.chromium.ui.VSyncMonitor#isInsideVSync().
     */
    public boolean isInsideVSync() {
        return mVSyncMonitor.isInsideVSync();
    }

    /**
     * @param context The application context.
     */
    @SuppressLint("UseSparseArrays")
    public WindowAndroid(Context context) {
        assert context == context.getApplicationContext();
        mApplicationContext = context;
        mOutstandingIntents = new SparseArray<IntentCallback>();
        mIntentErrors = new HashMap<Integer, String>();
        mVSyncMonitor = new VSyncMonitor(context, mVSyncListener);
    }

    /**
     * Shows an intent and returns the results to the callback object.
     * @param intent   The PendingIntent that needs to be shown.
     * @param callback The object that will receive the results for the intent.
     * @param errorId  The ID of error string to be show if activity is paused before intent
     *                 results, or null if no message is required.
     * @return Whether the intent was shown.
     */
    public boolean showIntent(PendingIntent intent, IntentCallback callback, Integer errorId) {
        return showCancelableIntent(intent, callback, errorId) >= 0;
    }

    /**
     * Shows an intent and returns the results to the callback object.
     * @param intent   The intent that needs to be shown.
     * @param callback The object that will receive the results for the intent.
     * @param errorId  The ID of error string to be show if activity is paused before intent
     *                 results, or null if no message is required.
     * @return Whether the intent was shown.
     */
    public boolean showIntent(Intent intent, IntentCallback callback, Integer errorId) {
        return showCancelableIntent(intent, callback, errorId) >= 0;
    }

    /**
     * Shows an intent that could be canceled and returns the results to the callback object.
     * @param  intent   The PendingIntent that needs to be shown.
     * @param  callback The object that will receive the results for the intent.
     * @param  errorId  The ID of error string to be show if activity is paused before intent
     *                  results, or null if no message is required.
     * @return A non-negative request code that could be used for finishActivity, or
     *         START_INTENT_FAILURE if failed.
     */
    public int showCancelableIntent(
            PendingIntent intent, IntentCallback callback, Integer errorId) {
        Log.d(TAG, "Can't show intent as context is not an Activity: " + intent);
        return START_INTENT_FAILURE;
    }

    /**
     * Shows an intent that could be canceled and returns the results to the callback object.
     * @param  intent   The intent that needs to be showed.
     * @param  callback The object that will receive the results for the intent.
     * @param  errorId  The ID of error string to be show if activity is paused before intent
     *                  results, or null if no message is required.
     * @return A non-negative request code that could be used for finishActivity, or
     *         START_INTENT_FAILURE if failed.
     */
    public int showCancelableIntent(Intent intent, IntentCallback callback, Integer errorId) {
        Log.d(TAG, "Can't show intent as context is not an Activity: " + intent);
        return START_INTENT_FAILURE;
    }

    /**
     * Force finish another activity that you had previously started with showCancelableIntent.
     * @param requestCode The request code returned from showCancelableIntent.
     */
    public void cancelIntent(int requestCode) {
        Log.d(TAG, "Can't cancel intent as context is not an Activity: " + requestCode);
    }

    /**
     * Removes a callback from the list of pending intents, so that nothing happens if/when the
     * result for that intent is received.
     * @param callback The object that should have received the results
     * @return True if the callback was removed, false if it was not found.
    */
    public boolean removeIntentCallback(IntentCallback callback) {
        int requestCode = mOutstandingIntents.indexOfValue(callback);
        if (requestCode < 0) return false;
        mOutstandingIntents.remove(requestCode);
        mIntentErrors.remove(requestCode);
        return true;
    }

    /**
     * Displays an error message with a provided error message string.
     * @param error The error message string to be displayed.
     */
    public void showError(String error) {
        if (error != null) {
            Toast.makeText(mApplicationContext, error, Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Displays an error message from the given resource id.
     * @param resId The error message string's resource id.
     */
    public void showError(int resId) {
        showError(mApplicationContext.getString(resId));
    }

    /**
     * Displays an error message for a nonexistent callback.
     * @param error The error message string to be displayed.
     */
    protected void showCallbackNonExistentError(String error) {
        showError(error);
    }

    /**
     * Broadcasts the given intent to all interested BroadcastReceivers.
     */
    public void sendBroadcast(Intent intent) {
        mApplicationContext.sendBroadcast(intent);
    }

    /**
     * @return A reference to owning Activity.  The returned WeakReference will never be null, but
     *         the contained Activity can be null (either if it has been garbage collected or if
     *         this is in the context of a WebView that was not created using an Activity).
     */
    public WeakReference<Activity> getActivity() {
        return new WeakReference<Activity>(null);
    }

    /**
     * @return The application context for this activity.
     */
    public Context getApplicationContext() {
        return mApplicationContext;
    }

    /**
     * Saves the error messages that should be shown if any pending intents would return
     * after the application has been put onPause.
     * @param bundle The bundle to save the information in onPause
     */
    public void saveInstanceState(Bundle bundle) {
        bundle.putSerializable(WINDOW_CALLBACK_ERRORS, mIntentErrors);
    }

    /**
     * Restores the error messages that should be shown if any pending intents would return
     * after the application has been put onPause.
     * @param bundle The bundle to restore the information from onResume
     */
    public void restoreInstanceState(Bundle bundle) {
        if (bundle == null) return;

        Object errors = bundle.getSerializable(WINDOW_CALLBACK_ERRORS);
        if (errors instanceof HashMap) {
            @SuppressWarnings("unchecked")
            HashMap<Integer, String> intentErrors = (HashMap<Integer, String>) errors;
            mIntentErrors = intentErrors;
        }
    }

    /**
     * Responds to the intent result if the intent was created by the native window.
     * @param requestCode Request code of the requested intent.
     * @param resultCode Result code of the requested intent.
     * @param data The data returned by the intent.
     * @return Boolean value of whether the intent was started by the native window.
     */
    public boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        return false;
    }

    @CalledByNative
    private void requestVSyncUpdate() {
        mVSyncMonitor.requestUpdate();
    }

    /**
     * An interface that intent callback objects have to implement.
     */
    public interface IntentCallback {
        /**
         * Handles the data returned by the requested intent.
         * @param window A window reference.
         * @param resultCode Result code of the requested intent.
         * @param contentResolver An instance of ContentResolver class for accessing returned data.
         * @param data The data returned by the intent.
         */
        void onIntentCompleted(WindowAndroid window, int resultCode,
                ContentResolver contentResolver, Intent data);
    }

    /**
     * Tests that an activity is available to handle the passed in intent.
     * @param  intent The intent to check.
     * @return True if an activity is available to process this intent when started, meaning that
     *         Context.startActivity will not throw ActivityNotFoundException.
     */
    public boolean canResolveActivity(Intent intent) {
        return mApplicationContext.getPackageManager().resolveActivity(intent, 0) != null;
    }

    /**
     * Destroys the c++ WindowAndroid object if one has been created.
     */
    public void destroy() {
        if (mNativeWindowAndroid != 0) {
            nativeDestroy(mNativeWindowAndroid);
            mNativeWindowAndroid = 0;
        }
    }

    /**
     * Returns a pointer to the c++ AndroidWindow object and calls the initializer if
     * the object has not been previously initialized.
     * @return A pointer to the c++ AndroidWindow.
     */
    public long getNativePointer() {
        if (mNativeWindowAndroid == 0) {
            mNativeWindowAndroid = nativeInit();
        }
        return mNativeWindowAndroid;
    }

    /**
     * Set the animation placeholder view, which we set to 'draw' during animations, such that
     * animations aren't clipped by the SurfaceView 'hole'. This can be the SurfaceView itself
     * or a view directly on top of it. This could be extended to many views if we ever need it.
     */
    public void setAnimationPlaceholderView(View view) {
        mAnimationPlaceholderView = view;
    }

    /**
     * Sets the keyboard accessory view.
     * @param view This view sits at the bottom of the content area and pushes the content up rather
     *             than overlaying it. Currently used as a container for Autofill suggestions.
     */
    public void setKeyboardAccessoryView(ViewGroup view) {
        mKeyboardAccessoryView = view;
    }

    /**
     * {@see setKeyboardAccessoryView(ViewGroup)}.
     */
    public ViewGroup getKeyboardAccessoryView() {
        return mKeyboardAccessoryView;
    }

    /**
     * Adds a listener that is updated of keyboard visibility changes. This works as a best guess.
     * {@see UiUtils.isKeyboardShowing}
     */
    public void addKeyboardVisibilityListener(KeyboardVisibilityListener listener) {
        mKeyboardVisibilityListeners.add(listener);
    }

    /**
     * {@see addKeyboardVisibilityListener()}.
     */
    public void removeKeyboardVisibilityListener(KeyboardVisibilityListener listener) {
        mKeyboardVisibilityListeners.remove(listener);
    }

    /**
     * To be called when the keyboard visibility state has changed. Informs listeners of the state
     * change.
     */
    public void keyboardVisibilityChanged(boolean isShowing) {
        // Clone the list in case a listener tries to remove itself during the callback.
        LinkedList<KeyboardVisibilityListener> listeners =
                new LinkedList<KeyboardVisibilityListener>(mKeyboardVisibilityListeners);
        for (KeyboardVisibilityListener listener : listeners) {
            listener.keyboardVisibilityChanged(isShowing);
        }
    }

    /**
     * Start a post-layout animation on top of web content.
     *
     * By default, Android optimizes what it shows on top of SurfaceViews (saves power).
     * Effectively, layouts determine what gets drawn and post-layout animations outside
     * of this area may be 'clipped'. Using this method to start such animations will
     * ensure that nothing is clipped during the animation, and restore the optimal
     * state when the animation ends.
     */
    public void startAnimationOverContent(Animator animation) {
        // We may not need an animation placeholder (eg. Webview doesn't use SurfaceView)
        if (mAnimationPlaceholderView == null) return;
        if (animation.isStarted()) throw new IllegalArgumentException("Already started.");
        boolean added = mAnimationsOverContent.add(animation);
        if (!added) throw new IllegalArgumentException("Already Added.");

        // We start the animation in this method to help guarantee that we never get stuck in this
        // state or leak objects into the set. Starting the animation here should guarantee that we
        // get an onAnimationEnd callback, and remove this animation.
        animation.start();

        // When the first animation starts, make the placeholder 'draw' itself.
        if (mAnimationPlaceholderView.willNotDraw()) {
            mAnimationPlaceholderView.setWillNotDraw(false);
        }

        // When the last animation ends, remove the placeholder view,
        // returning to the default optimized state.
        animation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                animation.removeListener(this);
                mAnimationsOverContent.remove(animation);
                if (mAnimationsOverContent.isEmpty()) {
                    mAnimationPlaceholderView.setWillNotDraw(true);
                }
            }
        });
    }

    private native long nativeInit();
    private native void nativeOnVSync(long nativeWindowAndroid,
                                      long vsyncTimeMicros,
                                      long vsyncPeriodMicros);
    private native void nativeDestroy(long nativeWindowAndroid);

}
