// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.widget.Toast;

import org.chromium.base.CommandLine;
import org.chromium.content.browser.ActivityContentVideoViewClient;
import org.chromium.content.browser.ContentVideoViewClient;
import org.chromium.content.browser.ContentViewClient;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.ui.base.WindowAndroid;

/**
 * Activity for managing the Cast shell.
 */
public class CastShellActivity extends Activity {
    private static final String TAG = "CastShellActivity";

    private static final String ACTIVE_SHELL_URL_KEY = "activeUrl";
    private static final int DEFAULT_HEIGHT_PIXELS = 720;
    public static final String ACTION_EXTRA_RESOLUTION_HEIGHT =
        "org.chromium.chromecast.shell.intent.extra.RESOLUTION_HEIGHT";

    private CastWindowManager mCastWindowManager;
    private AudioManager mAudioManager;
    private BroadcastReceiver mBroadcastReceiver;

    // Native window instance.
    // TODO(byungchul, gunsch): CastShellActivity, CastWindowAndroid, and native CastWindowAndroid
    // have a one-to-one relationship. Consider instantiating CastWindow here and CastWindow having
    // this native shell instance.
    private long mNativeCastWindow;

    /**
     * Returns whether or not CastShellActivity should launch the browser startup sequence.
     * Intended to be overridden.
     */
    protected boolean shouldLaunchBrowser() {
        return true;
    }

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        exitIfUrlMissing();

        if (shouldLaunchBrowser()) {
            if (!CastBrowserHelper.initializeBrowser(getApplicationContext())) {
                Toast.makeText(this,
                        R.string.browser_process_initialization_failed,
                        Toast.LENGTH_SHORT).show();
                finish();
            }
        }

        // Whenever our app is visible, volume controls should modify the music stream.
        // For more information read:
        // http://developer.android.com/training/managing-audio/volume-playback.html
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        // Set flags to both exit sleep mode when this activity starts and
        // avoid entering sleep mode while playing media. We cannot distinguish
        // between video and audio so this applies to both.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);

        setContentView(R.layout.cast_shell_activity);
        mCastWindowManager = (CastWindowManager) findViewById(R.id.shell_container);
        mCastWindowManager.setDelegate(new CastWindowManager.Delegate() {
                @Override
                public void onCreated() {
                }

                @Override
                public void onClosed() {
                    mNativeCastWindow = 0;
                    mCastWindowManager.setDelegate(null);
                    finish();
                }
            });
        setResolution();
        mCastWindowManager.setWindow(new WindowAndroid(this));

        registerBroadcastReceiver();

        String url = getIntent().getDataString();
        Log.d(TAG, "onCreate startupUrl: " + url);
        mNativeCastWindow = mCastWindowManager.launchCastWindow(url);

        getActiveContentViewCore().setContentViewClient(new ContentViewClient() {
            @Override
            public ContentVideoViewClient getContentVideoViewClient() {
                return new ActivityContentVideoViewClient(CastShellActivity.this);
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        unregisterBroadcastReceiver();

        if (mNativeCastWindow != 0) {
            mCastWindowManager.stopCastWindow(mNativeCastWindow);
            mNativeCastWindow = 0;
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        // Only handle direct intents (e.g. "fling") if this activity is also managing
        // the browser process.
        if (!shouldLaunchBrowser()) return;

        String url = intent.getDataString();
        Log.d(TAG, "onNewIntent: " + url);

        // Reset broadcast intent uri and receiver.
        setIntent(intent);
        exitIfUrlMissing();
        getActiveCastWindow().loadUrl(url);
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Inform ContentView that this activity is being shown.
        ContentViewCore view = getActiveContentViewCore();
        if (view != null) view.onShow();

        // Request audio focus so any other audio playback doesn't continue in the background.
        if (mAudioManager.requestAudioFocus(
                null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
                != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            Log.e(TAG, "Failed to obtain audio focus");
        }
    }

    @Override
    protected void onPause() {
        // As soon as the cast app is no longer in the foreground, we ought to immediately tear
        // everything down. Apps should not continue running and playing sound in the background.
        super.onPause();

        // Release the audio focus. Note that releasing audio focus does not stop audio playback,
        // it just notifies the framework that this activity has stopped playing audio.
        if (mAudioManager.abandonAudioFocus(null) != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            Log.e(TAG, "Failed to abandon audio focus");
        }

        ContentViewCore view = getActiveContentViewCore();
        if (view != null) view.onHide();

        finishGracefully();
    }

    protected void finishGracefully() {
        if (mNativeCastWindow != 0) {
            mCastWindowManager.stopCastWindow(mNativeCastWindow);
            mNativeCastWindow = 0;
        }
    }

    private void registerBroadcastReceiver() {
        if (mBroadcastReceiver == null) {
            mBroadcastReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Log.d(TAG, "Received intent: action=" + intent.getAction());
                    if (CastWindowAndroid.ACTION_ENABLE_DEV_TOOLS.equals(intent.getAction())) {
                        mCastWindowManager.nativeEnableDevTools(true);
                    } else if (CastWindowAndroid.ACTION_DISABLE_DEV_TOOLS.equals(
                            intent.getAction())) {
                        mCastWindowManager.nativeEnableDevTools(false);
                    }
                }
            };
        }

        IntentFilter devtoolsBroadcastIntentFilter = new IntentFilter();
        devtoolsBroadcastIntentFilter.addAction(CastWindowAndroid.ACTION_ENABLE_DEV_TOOLS);
        devtoolsBroadcastIntentFilter.addAction(CastWindowAndroid.ACTION_DISABLE_DEV_TOOLS);
        LocalBroadcastManager.getInstance(this)
                .registerReceiver(mBroadcastReceiver, devtoolsBroadcastIntentFilter);
    }

    private void unregisterBroadcastReceiver() {
        LocalBroadcastManager broadcastManager = LocalBroadcastManager.getInstance(this);
        broadcastManager.unregisterReceiver(mBroadcastReceiver);
    }

    private void setResolution() {
        int requestedHeight = getIntent().getIntExtra(
                ACTION_EXTRA_RESOLUTION_HEIGHT, DEFAULT_HEIGHT_PIXELS);
        int displayHeight = getResources().getDisplayMetrics().heightPixels;
        // Clamp within [DEFAULT_HEIGHT_PIXELS, displayHeight]
        int desiredHeight =
                Math.min(displayHeight, Math.max(DEFAULT_HEIGHT_PIXELS, requestedHeight));
        double deviceScaleFactor = ((double)displayHeight) / desiredHeight;
        Log.d(TAG, "Using scale factor " + deviceScaleFactor + " to set height " + desiredHeight);
        CommandLine.getInstance().appendSwitchWithValue("force-device-scale-factor",
                String.valueOf(deviceScaleFactor));
    }

    private void exitIfUrlMissing() {
        Intent intent = getIntent();
        if (intent != null && intent.getData() != null && !intent.getData().equals(Uri.EMPTY)) {
            return;
        }
        // Log an exception so that the exit cause is obvious when reading the logs.
        Log.e(TAG, "Activity will not start",
            new IllegalArgumentException("Intent did not contain a valid url"));
        System.exit(-1);
    }

    /**
     * @return The currently visible {@link CastWindowAndroid} or null if one is not showing.
     */
    public CastWindowAndroid getActiveCastWindow() {
        return mCastWindowManager.getActiveCastWindow();
    }

    /**
     * @return The {@link ContentViewCore} owned by the currently visible {@link CastWindowAndroid},
     *         or null if one is not showing.
     */
    public ContentViewCore getActiveContentViewCore() {
        CastWindowAndroid shell = getActiveCastWindow();
        return shell != null ? shell.getContentViewCore() : null;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode != KeyEvent.KEYCODE_BACK) {
            return super.onKeyUp(keyCode, event);
        }

        // Just finish this activity to go back to the previous activity or launcher.
        finishGracefully();
        return true;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode = event.getKeyCode();
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            return super.dispatchKeyEvent(event);
        }
        return false;
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event) {
        return false;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent ev) {
        return false;
    }
}
