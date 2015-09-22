// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.accounts.Account;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.preference.Preference;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunSigninProcessor;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileDownloader;
import org.chromium.chrome.browser.signin.AccountManagementFragment;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInAllowedObserver;
import org.chromium.sync.signin.AccountManagerHelper;
import org.chromium.sync.signin.ChromeSigninController;

import java.util.List;

/**
 * A preference that displays "Sign in to Chrome" when the user is not sign in, and displays
 * the user's name, email, and profile image when the user is signed in.
 */
public class SignInPreference extends Preference implements SignInAllowedObserver,
        ProfileDownloader.Observer {

    private boolean mViewEnabled;

    /**
     * Constructor for inflating from XML.
     */
    public SignInPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setShouldDisableView(false);
        update();
    }

    /**
     * Starts listening for updates to the sign-in state.
     */
    public void registerForUpdates() {
        SigninManager manager = SigninManager.get(getContext());
        manager.addSignInAllowedObserver(this);
        ProfileDownloader.addObserver(this);
        FirstRunSigninProcessor.updateSigninManagerFirstRunCheckDone(getContext());
    }

    /**
     * Stops listening for updates to the sign-in state. Every call to registerForUpdates() must
     * be matched with a call to this method.
     */
    public void unregisterForUpdates() {
        SigninManager manager = SigninManager.get(getContext());
        manager.removeSignInAllowedObserver(this);
        ProfileDownloader.removeObserver(this);
    }

    /**
     * Updates the title, summary, and image based on the current sign-in state.
     */
    private void update() {
        String title;
        String summary;
        String fragment;

        Account account = ChromeSigninController.get(getContext()).getSignedInUser();
        if (account == null) {
            title = getContext().getString(R.string.sign_in_to_chrome);
            summary = getContext().getString(R.string.sign_in_to_chrome_summary);
            fragment = null;
        } else {
            List<String> accounts = AccountManagerHelper.get(getContext()).getGoogleAccountNames();
            if (accounts.size() == 1) {
                summary = accounts.get(0);
            } else {
                summary = getContext().getString(
                        R.string.number_of_signed_in_accounts, accounts.size());
            }
            fragment = AccountManagementFragment.class.getName();
            title = AccountManagementFragment.getCachedUserName(account.name);
            if (title == null) {
                final Profile profile = Profile.getLastUsedProfile();
                String cachedName = ProfileDownloader.getCachedFullName(profile);
                Bitmap cachedBitmap = ProfileDownloader.getCachedAvatar(profile);
                if (TextUtils.isEmpty(cachedName) || cachedBitmap == null) {
                    AccountManagementFragment.startFetchingAccountInformation(
                            getContext(), profile, account.name);
                }
                title = TextUtils.isEmpty(cachedName) ? account.name : cachedName;
            }
        }

        setTitle(title);
        setSummary(summary);
        setFragment(fragment);

        ChromeSigninController signinController = ChromeSigninController.get(getContext());
        boolean enabled = signinController.isSignedIn()
                || SigninManager.get(getContext()).isSignInAllowed();
        if (mViewEnabled != enabled) {
            mViewEnabled = enabled;
            notifyChanged();
        }
        if (!enabled) setFragment(null);

        if (SigninManager.get(getContext()).isSigninDisabledByPolicy()) {
            setIcon(ManagedPreferencesUtils.getManagedByEnterpriseIconId());
        } else {
            Resources resources = getContext().getResources();
            Bitmap bitmap = AccountManagementFragment.getUserPicture(
                    signinController.getSignedInAccountName(), resources);
            setIcon(new BitmapDrawable(resources, bitmap));
        }
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        view.setEnabled(mViewEnabled);
        view.findViewById(android.R.id.title).setEnabled(mViewEnabled);
        view.findViewById(android.R.id.summary).setEnabled(mViewEnabled);
    }

    // SignInAllowedObserver

    @Override
    public void onSignInAllowedChanged() {
        update();
    }

    // ProfileDownloader.Observer

    @Override
    public void onProfileDownloaded(String accountId, String fullName, String givenName,
            Bitmap bitmap) {
        AccountManagementFragment.updateUserNamePictureCache(accountId, fullName, bitmap);
        update();
    }
}
