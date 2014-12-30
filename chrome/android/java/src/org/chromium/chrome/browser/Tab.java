// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;

import org.chromium.base.CalledByNative;
import org.chromium.base.ObserverList;
import org.chromium.base.TraceEvent;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.TabState.WebContentsState;
import org.chromium.chrome.browser.banners.AppBannerManager;
import org.chromium.chrome.browser.contextmenu.ChromeContextMenuItemDelegate;
import org.chromium.chrome.browser.contextmenu.ChromeContextMenuPopulator;
import org.chromium.chrome.browser.contextmenu.ContextMenuParams;
import org.chromium.chrome.browser.contextmenu.ContextMenuPopulator;
import org.chromium.chrome.browser.contextmenu.ContextMenuPopulatorWrapper;
import org.chromium.chrome.browser.contextmenu.EmptyChromeContextMenuItemDelegate;
import org.chromium.chrome.browser.dom_distiller.DomDistillerFeedbackReporter;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.infobar.InfoBarContainer;
import org.chromium.chrome.browser.printing.TabPrinter;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.SadTabViewFactory;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelBase;
import org.chromium.chrome.browser.toolbar.ToolbarModel;
import org.chromium.chrome.browser.ui.toolbar.ToolbarModelSecurityLevel;
import org.chromium.content.browser.ContentView;
import org.chromium.content.browser.ContentViewClient;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.WebContentsObserver;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.common.Referrer;
import org.chromium.content_public.common.TopControlsState;
import org.chromium.printing.PrintManagerDelegateImpl;
import org.chromium.printing.PrintingController;
import org.chromium.printing.PrintingControllerImpl;
import org.chromium.ui.base.Clipboard;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.gfx.DeviceDisplayInfo;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * The basic Java representation of a tab.  Contains and manages a {@link ContentView}.
 *
 * Tab provides common functionality for ChromeShell Tab as well as Chrome on Android's
 * tab. It is intended to be extended either on Java or both Java and C++, with ownership managed
 * by this base class.
 *
 * Extending just Java:
 *  - Just extend the class normally.  Do not override initializeNative().
 * Extending Java and C++:
 *  - Because of the inner-workings of JNI, the subclass is responsible for constructing the native
 *    subclass, which in turn constructs TabAndroid (the native counterpart to Tab), which in
 *    turn sets the native pointer for Tab.  For destruction, subclasses in Java must clear
 *    their own native pointer reference, but Tab#destroy() will handle deleting the native
 *    object.
 *
 * Notes on {@link Tab#getId()}:
 *
 *    Tabs are all generated using a static {@link AtomicInteger} which means they are unique across
 *  all {@link Activity}s running in the same {@link android.app.Application} process.  Calling
 *  {@link Tab#incrementIdCounterTo(int)} will ensure new {@link Tab}s get ids greater than or equal
 *  to the parameter passed to that method.  This should be used when doing things like loading
 *  persisted {@link Tab}s from disk on process start to ensure all new {@link Tab}s don't have id
 *  collision.
 *    Some {@link Activity}s will not call this because they do not persist state, which means those
 *  ids can potentially conflict with the ones restored from persisted state depending on which
 *  {@link Activity} runs first on process start.  If {@link Tab}s are ever shared across
 *  {@link Activity}s or mixed with {@link Tab}s from other {@link Activity}s conflicts can occur
 *  unless special care is taken to make sure {@link Tab#incrementIdCounterTo(int)} is called with
 *  the correct value across all affected {@link Activity}s.
 */
public class Tab {
    public static final int INVALID_TAB_ID = -1;

    /** Used for logging. */
    private static final String TAG = "Tab";

    /** Used for automatically generating tab ids. */
    private static final AtomicInteger sIdCounter = new AtomicInteger();

    private long mNativeTabAndroid;

    /** Unique id of this tab (within its container). */
    private final int mId;

    /** Whether or not this tab is an incognito tab. */
    private final boolean mIncognito;

    /**
     * An Application {@link Context}.  Unlike {@link #mContext}, this is the only one that is
     * publicly exposed to help prevent leaking the {@link Activity}.
     */
    private final Context mApplicationContext;

    /**
     * The {@link Context} used to create {@link View}s and other Android components.  Unlike
     * {@link #mApplicationContext}, this is not publicly exposed to help prevent leaking the
     * {@link Activity}.
     */
    private final Context mContext;

    /** Gives {@link Tab} a way to interact with the Android window. */
    private final WindowAndroid mWindowAndroid;

    /** The current native page (e.g. chrome-native://newtab), or {@code null} if there is none. */
    private NativePage mNativePage;

    /** InfoBar container to show InfoBars for this tab. */
    private InfoBarContainer mInfoBarContainer;

    /** Manages app banners shown for this tab. */
    private AppBannerManager mAppBannerManager;

    /** The sync id of the Tab if session sync is enabled. */
    private int mSyncId;

    /** {@link ContentViewCore} showing the current page, or {@code null} if the tab is frozen. */
    private ContentViewCore mContentViewCore;

    /** A list of Tab observers.  These are used to broadcast Tab events to listeners. */
    private final ObserverList<TabObserver> mObservers = new ObserverList<TabObserver>();

    // Content layer Observers and Delegates
    private ContentViewClient mContentViewClient;
    private WebContentsObserver mWebContentsObserver;
    private VoiceSearchTabHelper mVoiceSearchTabHelper;
    private TabChromeWebContentsDelegateAndroid mWebContentsDelegate;
    private DomDistillerFeedbackReporter mDomDistillerFeedbackReporter;

    /**
     * If this tab was opened from another tab, store the id of the tab that
     * caused it to be opened so that we can activate it when this tab gets
     * closed.
     */
    private int mParentId = INVALID_TAB_ID;

    /**
     * Whether the tab should be grouped with its parent tab.
     */
    private boolean mGroupedWithParent = true;

    private boolean mIsClosing = false;

    private Bitmap mFavicon;

    private String mFaviconUrl;

    /**
     * The number of pixel of 16DP.
     */
    private int mNumPixel16DP;

    /** Whether or not the TabState has changed. */
    private boolean mIsTabStateDirty = true;

    /**
     * Saves how this tab was launched (from a link, external app, etc) so that
     * we can determine the different circumstances in which it should be
     * closed. For example, a tab opened from an external app should be closed
     * when the back stack is empty and the user uses the back hardware key. A
     * standard tab however should be kept open and the entire activity should
     * be moved to the background.
     */
    private final TabLaunchType mLaunchType;

    /**
     * Navigation state of the WebContents as returned by nativeGetContentsStateAsByteBuffer(),
     * stored to be inflated on demand using unfreezeContents(). If this is not null, there is no
     * WebContents around. Upon tab switch WebContents will be unfrozen and the variable will be set
     * to null.
     */
    private WebContentsState mFrozenContentsState;

    /**
     * URL load to be performed lazily when the Tab is next shown.
     */
    private LoadUrlParams mPendingLoadParams;

    /**
     * URL of the page currently loading. Used as a fall-back in case tab restore fails.
     */
    private String mUrl;

    /**
     * The external application that this Tab is associated with (null if not associated with any
     * app). Allows reusing of tabs opened from the same application.
     */
    private String mAppAssociatedWith;

    /**
     * Keeps track of whether the Tab should be kept in the TabModel after the user hits "back".
     * Used by Document mode to keep track of whether we want to remove the tab when user hits back.
     */
    private boolean mShouldPreserve;

    private FullscreenManager mFullscreenManager;
    private float mPreviousFullscreenTopControlsOffsetY = Float.NaN;
    private float mPreviousFullscreenContentOffsetY = Float.NaN;
    private float mPreviousFullscreenOverdrawBottomHeight = Float.NaN;
    private int mFullscreenHungRendererToken = FullscreenManager.INVALID_TOKEN;

    /**
     * Reference to the current sadTabView if one is defined.
     */
    private View mSadTabView;

    /**
     * A default {@link ChromeContextMenuItemDelegate} that supports some of the context menu
     * functionality.
     */
    protected class TabChromeContextMenuItemDelegate
            extends EmptyChromeContextMenuItemDelegate {
        private final Clipboard mClipboard;

        /**
         * Builds a {@link TabChromeContextMenuItemDelegate} instance.
         */
        public TabChromeContextMenuItemDelegate() {
            mClipboard = new Clipboard(getApplicationContext());
        }

        @Override
        public boolean isIncognito() {
            return mIncognito;
        }

        @Override
        public void onSaveToClipboard(String text, boolean isUrl) {
            mClipboard.setText(text, text);
        }

        @Override
        public void onSaveImageToClipboard(String url) {
            mClipboard.setHTMLText("<img src=\"" + url + "\">", url, url);
        }

        @Override
        public String getPageUrl() {
            return getUrl();
        }
    }

    /**
     * A basic {@link ChromeWebContentsDelegateAndroid} that forwards some calls to the registered
     * {@link TabObserver}s.  Meant to be overridden by subclasses.
     */
    public class TabChromeWebContentsDelegateAndroid
            extends ChromeWebContentsDelegateAndroid {
        @Override
        public void onLoadProgressChanged(int progress) {
            for (TabObserver observer : mObservers) {
                observer.onLoadProgressChanged(Tab.this, progress);
            }
        }

        @Override
        public void onLoadStarted() {
            for (TabObserver observer : mObservers) observer.onLoadStarted(Tab.this);
        }

        @Override
        public void onLoadStopped() {
            for (TabObserver observer : mObservers) observer.onLoadStopped(Tab.this);
        }

        @Override
        public void onUpdateUrl(String url) {
            for (TabObserver observer : mObservers) observer.onUpdateUrl(Tab.this, url);
        }

        @Override
        public void showRepostFormWarningDialog() {
            RepostFormWarningDialog warningDialog = new RepostFormWarningDialog(
                    new Runnable() {
                        @Override
                        public void run() {
                            getWebContents().getNavigationController().cancelPendingReload();
                        }
                    }, new Runnable() {
                        @Override
                        public void run() {
                            getWebContents().getNavigationController().continuePendingReload();
                        }
                    });
            Activity activity = (Activity) mContext;
            warningDialog.show(activity.getFragmentManager(), null);
        }

        @Override
        public void toggleFullscreenModeForTab(boolean enableFullscreen) {
            if (mFullscreenManager != null) {
                mFullscreenManager.setPersistentFullscreenMode(enableFullscreen);
            }

            for (TabObserver observer : mObservers) {
                observer.onToggleFullscreenMode(Tab.this, enableFullscreen);
            }
        }

        @Override
        public void navigationStateChanged(int flags) {
            if ((flags & INVALIDATE_TYPE_TITLE) != 0) {
                for (TabObserver observer : mObservers) observer.onTitleUpdated(Tab.this);
            }
            if ((flags & INVALIDATE_TYPE_URL) != 0) {
                for (TabObserver observer : mObservers) observer.onUrlUpdated(Tab.this);
            }
        }

        @Override
        public void visibleSSLStateChanged() {
            for (TabObserver observer : mObservers) observer.onSSLStateUpdated(Tab.this);
        }

        @Override
        public void webContentsCreated(long sourceWebContents, long openerRenderFrameId,
                String frameName, String targetUrl, long newWebContents) {
            for (TabObserver observer : mObservers) {
                observer.webContentsCreated(Tab.this, sourceWebContents, openerRenderFrameId,
                        frameName, targetUrl, newWebContents);
            }
        }

        @Override
        public void rendererUnresponsive() {
            super.rendererUnresponsive();
            if (mFullscreenManager == null) return;
            mFullscreenHungRendererToken =
                    mFullscreenManager.showControlsPersistentAndClearOldToken(
                            mFullscreenHungRendererToken);
        }

        @Override
        public void rendererResponsive() {
            super.rendererResponsive();
            if (mFullscreenManager == null) return;
            mFullscreenManager.hideControlsPersistent(mFullscreenHungRendererToken);
            mFullscreenHungRendererToken = FullscreenManager.INVALID_TOKEN;
        }

        @Override
        public boolean isFullscreenForTabOrPending() {
            return mFullscreenManager == null
                    ? false : mFullscreenManager.getPersistentFullscreenMode();
        }
    }

    /**
     * ContentViewClient that provides basic tab functionality and is meant to be extended
     * by child classes.
     */
    protected class TabContentViewClient extends ContentViewClient {
        @Override
        public void onContextualActionBarShown() {
            for (TabObserver observer : mObservers) {
                observer.onContextualActionBarVisibilityChanged(Tab.this, true);
            }
        }

        @Override
        public void onContextualActionBarHidden() {
            for (TabObserver observer : mObservers) {
                observer.onContextualActionBarVisibilityChanged(Tab.this, false);
            }
        }

        @Override
        public void onImeEvent() {
            // Some text was set in the page. Don't reuse it if a tab is
            // open from the same external application, we might lose some
            // user data.
            mAppAssociatedWith = null;
        }
    }

    private class TabContextMenuPopulator extends ContextMenuPopulatorWrapper {
        public TabContextMenuPopulator(ContextMenuPopulator populator) {
            super(populator);
        }

        @Override
        public void buildContextMenu(ContextMenu menu, Context context, ContextMenuParams params) {
            super.buildContextMenu(menu, context, params);
            for (TabObserver observer : mObservers) observer.onContextMenuShown(Tab.this, menu);
        }
    }

    private class TabWebContentsObserver extends WebContentsObserver {
        public TabWebContentsObserver(WebContents webContents) {
            super(webContents);
        }

        @Override
        public void navigationEntryCommitted() {
            if (getNativePage() != null) {
                pushNativePageStateToNavigationEntry();
            }
        }

        @Override
        public void didFailLoad(boolean isProvisionalLoad, boolean isMainFrame, int errorCode,
                String description, String failingUrl) {
            for (TabObserver observer : mObservers) {
                observer.onDidFailLoad(Tab.this, isProvisionalLoad, isMainFrame, errorCode,
                        description, failingUrl);
            }
        }

        @Override
        public void didStartProvisionalLoadForFrame(long frameId, long parentFrameId,
                boolean isMainFrame, String validatedUrl, boolean isErrorPage,
                boolean isIframeSrcdoc) {
            for (TabObserver observer : mObservers) {
                observer.onDidStartProvisionalLoadForFrame(Tab.this, frameId, parentFrameId,
                        isMainFrame, validatedUrl, isErrorPage, isIframeSrcdoc);
            }
        }

        @Override
        public void didCommitProvisionalLoadForFrame(long frameId, boolean isMainFrame, String url,
                int transitionType) {
            for (TabObserver observer : mObservers) {
                observer.onDidCommitProvisionalLoadForFrame(
                        Tab.this, frameId, isMainFrame, url, transitionType);
            }
        }

        @Override
        public void didNavigateMainFrame(String url, String baseUrl,
                boolean isNavigationToDifferentPage, boolean isFragmentNavigation, int statusCode) {
            for (TabObserver observer : mObservers) {
                observer.onDidNavigateMainFrame(
                        Tab.this, url, baseUrl, isNavigationToDifferentPage,
                        isFragmentNavigation, statusCode);

            }
        }

        @Override
        public void didChangeThemeColor(int color) {
            for (TabObserver observer : mObservers) {
                observer.onDidChangeThemeColor(color);
            }
        }

        @Override
        public void didAttachInterstitialPage() {
            for (TabObserver observer : mObservers) {
                observer.onDidAttachInterstitialPage(Tab.this);
            }
        }

        @Override
        public void didDetachInterstitialPage() {
            for (TabObserver observer : mObservers) {
                observer.onDidDetachInterstitialPage(Tab.this);
            }
        }
    }

    /**
     * Creates an instance of a {@link Tab} with no id.
     * @param incognito Whether or not this tab is incognito.
     * @param context   An instance of a {@link Context}.
     * @param window    An instance of a {@link WindowAndroid}.
     */
    @VisibleForTesting
    public Tab(boolean incognito, Context context, WindowAndroid window) {
        this(INVALID_TAB_ID, incognito, context, window);
    }

    /**
     * Creates an instance of a {@link Tab}.
     * @param id        The id this tab should be identified with.
     * @param incognito Whether or not this tab is incognito.
     * @param context   An instance of a {@link Context}.
     * @param window    An instance of a {@link WindowAndroid}.
     */
    public Tab(int id, boolean incognito, Context context, WindowAndroid window) {
        this(id, INVALID_TAB_ID, incognito, context, window, null);
    }

    /**
     * Creates an instance of a {@link Tab}.
     * @param id        The id this tab should be identified with.
     * @param parentId  The id id of the tab that caused this tab to be opened.
     * @param incognito Whether or not this tab is incognito.
     * @param context   An instance of a {@link Context}.
     * @param window    An instance of a {@link WindowAndroid}.
     */
    public Tab(int id, int parentId, boolean incognito, Context context, WindowAndroid window,
            TabLaunchType type) {
        this(id, parentId, incognito, context, window, type, null);
    }

    /**
     * Creates an instance of a {@link Tab}.
     * @param id          The id this tab should be identified with.
     * @param parentId    The id id of the tab that caused this tab to be opened.
     * @param incognito   Whether or not this tab is incognito.
     * @param context     An instance of a {@link Context}.
     * @param window      An instance of a {@link WindowAndroid}.
     * @param frozenState State containing information about this Tab, if it was persisted.
     */
    public Tab(int id, int parentId, boolean incognito, Context context, WindowAndroid window,
            TabLaunchType type, TabState frozenState) {
        // We need a valid Activity Context to build the ContentView with.
        assert context == null || context instanceof Activity;

        mId = generateValidId(id);
        mParentId = parentId;
        mIncognito = incognito;
        // TODO(dtrainor): Only store application context here.
        mContext = context;
        mApplicationContext = context != null ? context.getApplicationContext() : null;
        mWindowAndroid = window;
        mLaunchType = type;
        if (mContext != null) {
            mNumPixel16DP = (int) (DeviceDisplayInfo.create(mContext).getDIPScale() * 16);
        }
        if (mNumPixel16DP == 0) mNumPixel16DP = 16;

        // Restore data from the TabState, if it existed.
        if (frozenState == null) {
            assert type != TabLaunchType.FROM_RESTORE;
        } else {
            assert type == TabLaunchType.FROM_RESTORE;
            restoreFieldsFromState(frozenState);
        }
    }

    /**
     * Restores member fields from the given TabState.
     * @param state TabState containing information about this Tab.
     */
    protected void restoreFieldsFromState(TabState state) {
        assert state != null;
        mAppAssociatedWith = state.openerAppId;
        mFrozenContentsState = state.contentsState;
        mSyncId = (int) state.syncId;
        mShouldPreserve = state.shouldPreserve;
        mUrl = state.getVirtualUrlFromState();
    }

    /**
     * Adds a {@link TabObserver} to be notified on {@link Tab} changes.
     * @param observer The {@link TabObserver} to add.
     */
    public void addObserver(TabObserver observer) {
        mObservers.addObserver(observer);
    }

    /**
     * Removes a {@link TabObserver}.
     * @param observer The {@link TabObserver} to remove.
     */
    public void removeObserver(TabObserver observer) {
        mObservers.removeObserver(observer);
    }

    /**
     * @return Whether or not this tab has a previous navigation entry.
     */
    public boolean canGoBack() {
        return getWebContents() != null && getWebContents().getNavigationController().canGoBack();
    }

    /**
     * @return Whether or not this tab has a navigation entry after the current one.
     */
    public boolean canGoForward() {
        return getWebContents() != null && getWebContents().getNavigationController()
                .canGoForward();
    }

    /**
     * Goes to the navigation entry before the current one.
     */
    public void goBack() {
        if (getWebContents() != null) getWebContents().getNavigationController().goBack();
    }

    /**
     * Goes to the navigation entry after the current one.
     */
    public void goForward() {
        if (getWebContents() != null) getWebContents().getNavigationController().goForward();
    }

    /**
     * Loads the current navigation if there is a pending lazy load (after tab restore).
     */
    public void loadIfNecessary() {
        if (getWebContents() != null) getWebContents().getNavigationController().loadIfNecessary();
    }

    /**
     * Requests the current navigation to be loaded upon the next call to loadIfNecessary().
     */
    protected void requestRestoreLoad() {
        if (getWebContents() != null) {
            getWebContents().getNavigationController().requestRestoreLoad();
        }
    }

    /**
     * Causes this tab to navigate to the specified URL.
     * @param params parameters describing the url load. Note that it is important to set correct
     *               page transition as it is used for ranking URLs in the history so the omnibox
     *               can report suggestions correctly.
     * @return FULL_PRERENDERED_PAGE_LOAD or PARTIAL_PRERENDERED_PAGE_LOAD if the page has been
     *         prerendered. DEFAULT_PAGE_LOAD if it had not.
     */
    public int loadUrl(LoadUrlParams params) {
        try {
            TraceEvent.begin("Tab.loadUrl");
            removeSadTabIfPresent();

            // Clear the app association if the user navigated to a different page from the omnibox.
            if ((params.getTransitionType() & PageTransition.FROM_ADDRESS_BAR)
                    == PageTransition.FROM_ADDRESS_BAR) {
                mAppAssociatedWith = null;
            }

            // We load the URL from the tab rather than directly from the ContentView so the tab has
            // a chance of using a prerenderer page is any.
            int loadType = nativeLoadUrl(
                    mNativeTabAndroid,
                    params.getUrl(),
                    params.getVerbatimHeaders(),
                    params.getPostData(),
                    params.getTransitionType(),
                    params.getReferrer() != null ? params.getReferrer().getUrl() : null,
                    // Policy will be ignored for null referrer url, 0 is just a placeholder.
                    // TODO(ppi): Should we pass Referrer jobject and add JNI methods to read it
                    //            from the native?
                    params.getReferrer() != null ? params.getReferrer().getPolicy() : 0,
                    params.getIsRendererInitiated());

            for (TabObserver observer : mObservers) {
                observer.onLoadUrl(this, params.getUrl(), loadType);
            }
            return loadType;
        } finally {
            TraceEvent.end("Tab.loadUrl");
        }
    }

    /**
     * @return Whether or not the {@link Tab} is currently showing an interstitial page, such as
     *         a bad HTTPS page.
     */
    public boolean isShowingInterstitialPage() {
        return getWebContents() != null && getWebContents().isShowingInterstitialPage();
    }

    /**
     * @return Whether or not the tab has something valid to render.
     */
    public boolean isReady() {
        return mNativePage != null || (getWebContents() != null && getWebContents().isReady());
    }

    /**
     * @return The {@link View} displaying the current page in the tab. This might be a
     *         native view or a placeholder view for content rendered by the compositor.
     *         This can be {@code null}, if the tab is frozen or being initialized or destroyed.
     */
    public View getView() {
        return mNativePage != null ? mNativePage.getView() :
                (mContentViewCore != null ? mContentViewCore.getContainerView() : null);
    }

    /**
     * @return The width of the content of this tab.  Can be 0 if there is no content.
     */
    public int getWidth() {
        View view = getView();
        return view != null ? view.getWidth() : 0;
    }

    /**
     * @return The height of the content of this tab.  Can be 0 if there is no content.
     */
    public int getHeight() {
        View view = getView();
        return view != null ? view.getHeight() : 0;
    }

    /**
     * @return The application {@link Context} associated with this tab.
     */
    protected Context getApplicationContext() {
        return mApplicationContext;
    }

    /**
     * @return The infobar container.
     */
    public final InfoBarContainer getInfoBarContainer() {
        return mInfoBarContainer;
    }

    /** @return An opaque "state" object that can be persisted to storage. */
    public TabState getState() {
        if (!isInitialized()) return null;
        TabState tabState = new TabState();
        tabState.contentsState = getWebContentsState();
        tabState.openerAppId = getAppAssociatedWith();
        tabState.parentId = getParentId();
        tabState.shouldPreserve = shouldPreserve();
        tabState.syncId = getSyncId();
        return tabState;
    }

    /** @return WebContentsState representing the state of the WebContents (navigations, etc.) */
    public WebContentsState getFrozenContentsState() {
        return mFrozenContentsState;
    }

    /** Returns an object representing the state of the Tab's WebContents. */
    protected TabState.WebContentsState getWebContentsState() {
        if (mFrozenContentsState != null) return mFrozenContentsState;

        // Native call returns null when buffer allocation needed to serialize the state failed.
        ByteBuffer buffer = getWebContentsStateAsByteBuffer();
        if (buffer == null) return null;

        TabState.WebContentsState state = new TabState.WebContentsStateNative(buffer);
        state.setVersion(TabState.CONTENTS_STATE_CURRENT_VERSION);
        return state;
    }

    /** Returns an ByteBuffer representing the state of the Tab's WebContents. */
    protected ByteBuffer getWebContentsStateAsByteBuffer() {
        if (mPendingLoadParams == null) {
            return TabState.getContentsStateAsByteBuffer(this);
        } else {
            Referrer referrer = mPendingLoadParams.getReferrer();
            return TabState.createSingleNavigationStateAsByteBuffer(
                    mPendingLoadParams.getUrl(),
                    referrer != null ? referrer.getUrl() : null,
                    // Policy will be ignored for null referrer url, 0 is just a placeholder.
                    referrer != null ? referrer.getPolicy() : 0,
                    isIncognito());
        }
    }

    /**
     * Prints the current page.
     *
     * @return Whether the printing process is started successfully.
     **/
    public boolean print() {
        assert mNativeTabAndroid != 0;
        return nativePrint(mNativeTabAndroid);
    }

    @CalledByNative
    public void setPendingPrint() {
        PrintingController printingController = PrintingControllerImpl.getInstance();
        if (printingController == null) return;

        printingController.setPendingPrint(new TabPrinter(this),
                new PrintManagerDelegateImpl(mContext));
    }

    /**
     * Reloads the current page content.
     */
    public void reload() {
        // TODO(dtrainor): Should we try to rebuild the ContentView if it's frozen?
        if (getWebContents() != null) getWebContents().getNavigationController().reload(true);
    }

    /**
     * Reloads the current page content.
     * This version ignores the cache and reloads from the network.
     */
    public void reloadIgnoringCache() {
        if (getWebContents() != null) {
            getWebContents().getNavigationController().reloadIgnoringCache(true);
        }
    }

    /** Stop the current navigation. */
    public void stopLoading() {
        if (getWebContents() != null) getWebContents().stop();
    }

    /**
     * @return The background color of the tab.
     */
    public int getBackgroundColor() {
        if (mNativePage != null) return mNativePage.getBackgroundColor();
        if (getWebContents() != null) return getWebContents().getBackgroundColor();
        return Color.WHITE;
    }

    /**
     * @return The web contents associated with this tab.
     */
    public WebContents getWebContents() {
        return mContentViewCore != null ? mContentViewCore.getWebContents() : null;
    }

    /**
     * @return The profile associated with this tab.
     */
    public Profile getProfile() {
        if (mNativeTabAndroid == 0) return null;
        return nativeGetProfileAndroid(mNativeTabAndroid);
    }

    /**
     * For more information about the uniqueness of {@link #getId()} see comments on {@link Tab}.
     * @see Tab
     * @return The id representing this tab.
     */
    @CalledByNative
    public int getId() {
        return mId;
    }

    /**
     * @return Whether or not this tab is incognito.
     */
    public boolean isIncognito() {
        return mIncognito;
    }

    /**
     * @return The {@link ContentViewCore} associated with the current page, or {@code null} if
     *         there is no current page or the current page is displayed using a native view.
     */
    public ContentViewCore getContentViewCore() {
        return mNativePage == null ? mContentViewCore : null;
    }

    /**
     * @return The {@link NativePage} associated with the current page, or {@code null} if there is
     *         no current page or the current page is displayed using something besides
     *         {@link NativePage}.
     */
    public NativePage getNativePage() {
        return mNativePage;
    }

    /**
     * @return Whether or not the {@link Tab} represents a {@link NativePage}.
     */
    public boolean isNativePage() {
        return mNativePage != null;
    }

    /**
     * Set whether or not the {@link ContentViewCore} should be using a desktop user agent for the
     * currently loaded page.
     * @param useDesktop     If {@code true}, use a desktop user agent.  Otherwise use a mobile one.
     * @param reloadOnChange Reload the page if the user agent has changed.
     */
    public void setUseDesktopUserAgent(boolean useDesktop, boolean reloadOnChange) {
        if (getWebContents() != null) {
            getWebContents().getNavigationController()
                    .setUseDesktopUserAgent(useDesktop, reloadOnChange);
        }
    }

    /**
     * @return Whether or not the {@link ContentViewCore} is using a desktop user agent.
     */
    public boolean getUseDesktopUserAgent() {
        return getWebContents() != null && getWebContents().getNavigationController()
                .getUseDesktopUserAgent();
    }

    /**
     * @return The current {ToolbarModelSecurityLevel} for the tab.
     */
    // TODO(tedchoc): Remove this and transition all clients to use ToolbarModel directly.
    public int getSecurityLevel() {
        return ToolbarModel.getSecurityLevelForWebContents(getWebContents());
    }

    /**
     * @return The sync id of the tab if session sync is enabled, {@code 0} otherwise.
     */
    @CalledByNative
    protected int getSyncId() {
        return mSyncId;
    }

    /**
     * @param syncId The sync id of the tab if session sync is enabled.
     */
    @CalledByNative
    protected void setSyncId(int syncId) {
        mSyncId = syncId;
    }

    /**
     * @return An {@link ObserverList.RewindableIterator} instance that points to all of
     *         the current {@link TabObserver}s on this class.  Note that calling
     *         {@link java.util.Iterator#remove()} will throw an
     *         {@link UnsupportedOperationException}.
     */
    protected ObserverList.RewindableIterator<TabObserver> getTabObservers() {
        return mObservers.rewindableIterator();
    }

    /**
     * @return The {@link ContentViewClient} currently bound to any {@link ContentViewCore}
     *         associated with the current page.  There can still be a {@link ContentViewClient}
     *         even when there is no {@link ContentViewCore}.
     */
    protected ContentViewClient getContentViewClient() {
        return mContentViewClient;
    }

    /**
     * @param client The {@link ContentViewClient} to be bound to any current or new
     *               {@link ContentViewCore}s associated with this {@link Tab}.
     */
    protected void setContentViewClient(ContentViewClient client) {
        if (mContentViewClient == client) return;

        ContentViewClient oldClient = mContentViewClient;
        mContentViewClient = client;

        if (mContentViewCore == null) return;

        if (mContentViewClient != null) {
            mContentViewCore.setContentViewClient(mContentViewClient);
        } else if (oldClient != null) {
            // We can't set a null client, but we should clear references to the last one.
            mContentViewCore.setContentViewClient(new ContentViewClient());
        }
    }

    /**
     * Triggers the showing logic for the view backing this tab.
     */
    protected void show() {
        if (mContentViewCore != null) mContentViewCore.onShow();
    }

    /**
     * Triggers the hiding logic for the view backing the tab.
     */
    protected void hide() {
        if (mContentViewCore != null) mContentViewCore.onHide();

        // Clean up any fullscreen state that might impact other tabs.
        if (mFullscreenManager != null) {
            mFullscreenManager.setPersistentFullscreenMode(false);
            mFullscreenManager.hideControlsPersistent(mFullscreenHungRendererToken);
            mFullscreenHungRendererToken = FullscreenManager.INVALID_TOKEN;
            mPreviousFullscreenOverdrawBottomHeight = Float.NaN;
        }
    }

    /**
     * Shows the given {@code nativePage} if it's not already showing.
     * @param nativePage The {@link NativePage} to show.
     */
    protected void showNativePage(NativePage nativePage) {
        if (mNativePage == nativePage) return;
        NativePage previousNativePage = mNativePage;
        mNativePage = nativePage;
        pushNativePageStateToNavigationEntry();
        for (TabObserver observer : mObservers) observer.onContentChanged(this);
        destroyNativePageInternal(previousNativePage);
    }

    /**
     * Replaces the current NativePage with a empty stand-in for a NativePage. This can be used
     * to reduce memory pressure.
     */
    public void freezeNativePage() {
        if (mNativePage == null || mNativePage instanceof FrozenNativePage) return;
        assert mNativePage.getView().getParent() == null : "Cannot freeze visible native page";
        mNativePage = FrozenNativePage.freeze(mNativePage);
    }

    /**
     * Hides the current {@link NativePage}, if any, and shows the {@link ContentViewCore}'s view.
     */
    protected void showRenderedPage() {
        if (mNativePage == null) return;
        NativePage previousNativePage = mNativePage;
        mNativePage = null;
        for (TabObserver observer : mObservers) observer.onContentChanged(this);
        destroyNativePageInternal(previousNativePage);
    }

    /**
     * Initializes this {@link Tab}.
     */
    public void initialize() {
        initializeNative();
    }

    /**
     * Builds the native counterpart to this class.  Meant to be overridden by subclasses to build
     * subclass native counterparts instead.  Subclasses should not call this via super and instead
     * rely on the native class to create the JNI association.
     */
    protected void initializeNative() {
        if (mNativeTabAndroid == 0) nativeInit();
        assert mNativeTabAndroid != 0;
    }

    /**
     * A helper method to initialize a {@link ContentViewCore} without any
     * native WebContents pointer.
     */
    protected final void initContentViewCore() {
        initContentViewCore(ContentViewUtil.createNativeWebContents(mIncognito));
    }

    /**
     * Creates and initializes the {@link ContentViewCore}.
     *
     * @param nativeWebContents The native web contents pointer.
     */
    protected void initContentViewCore(long nativeWebContents) {
        ContentViewCore cvc = new ContentViewCore(mContext);
        ContentView cv = ContentView.newInstance(mContext, cvc);
        cv.setContentDescription(mContext.getResources().getString(
                R.string.accessibility_content_view));
        cvc.initialize(cv, cv, nativeWebContents, getWindowAndroid());
        setContentViewCore(cvc);
    }

    /**
     * Completes the {@link ContentViewCore} specific initialization around a native WebContents
     * pointer. {@link #getNativePage()} will still return the {@link NativePage} if there is one.
     * All initialization that needs to reoccur after a web contents swap should be added here.
     * <p />
     * NOTE: If you attempt to pass a native WebContents that does not have the same incognito
     * state as this tab this call will fail.
     *
     * @param cvc The content view core that needs to be set as active view for the tab.
     */
    protected void setContentViewCore(ContentViewCore cvc) {
        NativePage previousNativePage = mNativePage;
        mNativePage = null;
        destroyNativePageInternal(previousNativePage);

        mContentViewCore = cvc;

        mWebContentsDelegate = createWebContentsDelegate();
        mWebContentsObserver = new TabWebContentsObserver(mContentViewCore.getWebContents());
        mVoiceSearchTabHelper = new VoiceSearchTabHelper(mContentViewCore.getWebContents());

        if (mContentViewClient != null) mContentViewCore.setContentViewClient(mContentViewClient);

        assert mNativeTabAndroid != 0;
        nativeInitWebContents(
                mNativeTabAndroid, mIncognito, mContentViewCore, mWebContentsDelegate,
                new TabContextMenuPopulator(createContextMenuPopulator()));

        // In the case where restoring a Tab or showing a prerendered one we already have a
        // valid infobar container, no need to recreate one.
        if (mInfoBarContainer == null) {
            // The InfoBarContainer needs to be created after the ContentView has been natively
            // initialized.
            WebContents webContents = mContentViewCore.getWebContents();
            mInfoBarContainer = new InfoBarContainer(
                    (Activity) mContext, getId(), mContentViewCore.getContainerView(), webContents);
        } else {
            mInfoBarContainer.onParentViewChanged(getId(), mContentViewCore.getContainerView());
        }

        if (AppBannerManager.isEnabled() && mAppBannerManager == null) {
            mAppBannerManager = new AppBannerManager(this);
        }

        if (DomDistillerFeedbackReporter.isEnabled() && mDomDistillerFeedbackReporter == null) {
            mDomDistillerFeedbackReporter = new DomDistillerFeedbackReporter(this);
        }

        for (TabObserver observer : mObservers) observer.onContentChanged(this);

        // For browser tabs, we want to set accessibility focus to the page
        // when it loads. This is not the default behavior for embedded
        // web views.
        mContentViewCore.setShouldSetAccessibilityFocusOnPageLoad(true);
    }

    /**
     * Constructs and shows a sad tab (Aw, Snap!).
     */
    protected void showSadTab() {
        if (getContentViewCore() != null) {
            OnClickListener suggestionAction = new OnClickListener() {
                @Override
                public void onClick(View view) {
                    loadUrl(new LoadUrlParams(UrlConstants.CRASH_REASON_URL));
                }
            };
            OnClickListener reloadButtonAction = new OnClickListener() {
                @Override
                public void onClick(View view) {
                    reload();
                }
            };

            // Make sure we are not adding the "Aw, snap" view over an existing one.
            assert mSadTabView == null;
            mSadTabView = SadTabViewFactory.createSadTabView(
                    getApplicationContext(), suggestionAction, reloadButtonAction);

            // Show the sad tab inside ContentView.
            getContentViewCore().getContainerView().addView(
                    mSadTabView, new FrameLayout.LayoutParams(
                            LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
        }
        FullscreenManager fullscreenManager = getFullscreenManager();
        if (fullscreenManager != null) {
            fullscreenManager.setPositionsForTabToNonFullscreen();
        }
    }

    /**
     * Removes the sad tab view if present.
     */
    protected void removeSadTabIfPresent() {
        if (isShowingSadTab()) getContentViewCore().getContainerView().removeView(mSadTabView);
        mSadTabView = null;
    }

    /**
     * @return Whether or not the sad tab is showing.
     */
    public boolean isShowingSadTab() {
        return mSadTabView != null && getContentViewCore() != null
                && mSadTabView.getParent() == getContentViewCore().getContainerView();
    }

    /**
     * Cleans up all internal state, destroying any {@link NativePage} or {@link ContentViewCore}
     * currently associated with this {@link Tab}.  This also destroys the native counterpart
     * to this class, which means that all subclasses should erase their native pointers after
     * this method is called.  Once this call is made this {@link Tab} should no longer be used.
     */
    public void destroy() {
        for (TabObserver observer : mObservers) observer.onDestroyed(this);
        mObservers.clear();

        NativePage currentNativePage = mNativePage;
        mNativePage = null;
        destroyNativePageInternal(currentNativePage);
        destroyContentViewCore(true);

        // Destroys the native tab after destroying the ContentView but before destroying the
        // InfoBarContainer. The native tab should be destroyed before the infobar container as
        // destroying the native tab cleanups up any remaining infobars. The infobar container
        // expects all infobars to be cleaned up before its own destruction.
        assert mNativeTabAndroid != 0;
        nativeDestroy(mNativeTabAndroid);
        assert mNativeTabAndroid == 0;

        if (mInfoBarContainer != null) {
            mInfoBarContainer.destroy();
            mInfoBarContainer = null;
        }

        mPreviousFullscreenTopControlsOffsetY = Float.NaN;
        mPreviousFullscreenContentOffsetY = Float.NaN;
    }

    /**
     * @return Whether or not this Tab has a live native component.
     */
    public boolean isInitialized() {
        return mNativeTabAndroid != 0;
    }

    /**
     * @return The URL associated with the tab.
     */
    @CalledByNative
    public String getUrl() {
        String url = getWebContents() != null ? getWebContents().getUrl() : "";

        // If we have a ContentView, or a NativePage, or the url is not empty, we have a WebContents
        // so cache the WebContent's url. If not use the cached version.
        if (getContentViewCore() != null || getNativePage() != null || !TextUtils.isEmpty(url)) {
            mUrl = url;
        }

        return mUrl != null ? mUrl : "";
    }

    /**
     * @return The tab title.
     */
    @CalledByNative
    public String getTitle() {
        if (mNativePage != null) return mNativePage.getTitle();
        if (getWebContents() != null) return getWebContents().getTitle();
        return "";
    }

    /**
     * @return The bitmap of the favicon scaled to 16x16dp. null if no favicon
     *         is specified or it requires the default favicon.
     */
    public Bitmap getFavicon() {
        // If we have no content or a native page, return null.
        if (getContentViewCore() == null) return null;

        if (mFavicon != null) return mFavicon;

        // Cache the result so we don't keep querying it.
        mFavicon = nativeGetFavicon(mNativeTabAndroid);
        // Invalidate the favicon URL so that if we do get a favicon for this page we don't drop it.
        mFaviconUrl = null;
        return mFavicon;
    }

    /**
     * Loads the tab if it's not loaded (e.g. because it was killed in background).
     * This will trigger a regular load for tabs with pending lazy first load (tabs opened in
     * background on low-memory devices).
     * @return true iff the Tab handled the request.
     */
    @CalledByNative
    public boolean loadIfNeeded() {
        if (mContext == null) {
            Log.e(TAG, "Tab couldn't be loaded because Context was null.");
            return false;
        }

        if (mPendingLoadParams != null) {
            assert isFrozen();
            initContentViewCore(ContentViewUtil.createNativeWebContents(isIncognito(), isHidden()));
            loadUrl(mPendingLoadParams);
            mPendingLoadParams = null;
            return true;
        }

        return false;
    }

    /**
     * Restores the WebContents from its saved state.
     * @return Whether or not the restoration was successful.
     */
    public boolean unfreezeContents() {
        try {
            TraceEvent.begin("Tab.unfreezeContents");
            assert mFrozenContentsState != null;
            assert getContentViewCore() == null;

            boolean forceNavigate = false;
            long webContents = mFrozenContentsState.restoreContentsFromByteBuffer(isHidden());
            if (webContents == 0) {
                // State restore failed, just create a new empty web contents as that is the best
                // that can be done at this point. TODO(jcivelli) http://b/5910521 - we should show
                // an error page instead of a blank page in that case (and the last loaded URL).
                webContents = ContentViewUtil.createNativeWebContents(isIncognito(), isHidden());
                forceNavigate = true;
            }

            mFrozenContentsState = null;
            initContentViewCore(webContents);

            if (forceNavigate) {
                String url = TextUtils.isEmpty(mUrl) ? UrlConstants.NTP_URL : mUrl;
                loadUrl(new LoadUrlParams(url, PageTransition.GENERATED));
            }
            return !forceNavigate;
        } finally {
            TraceEvent.end("Tab.unfreezeContents");
        }
    }

    /**
     * TODO(dfalcantara): Make this function accurate with hide() and show() calls.
     * @return Whether or not the tab is hidden.
     */
    public boolean isHidden() {
        return false;
    }

    /**
     * @return Whether or not the tab is in the closing process.
     */
    public boolean isClosing() {
        return mIsClosing;
    }

    /**
     * @param closing Whether or not the tab is in the closing process.
     */
    public void setClosing(boolean closing) {
        mIsClosing = closing;
    }

    /**
     * @return The id of the tab that caused this tab to be opened.
     */
    public int getParentId() {
        return mParentId;
    }

    /**
     * @return Whether the tab should be grouped with its parent tab (true by default).
     */
    public boolean isGroupedWithParent() {
        return mGroupedWithParent;
    }

    /**
     * Sets whether the tab should be grouped with its parent tab.
     *
     * @param groupedWithParent The new value.
     * @see #isGroupedWithParent
     */
    public void setGroupedWithParent(boolean groupedWithParent) {
        mGroupedWithParent = groupedWithParent;
    }

    private void destroyNativePageInternal(NativePage nativePage) {
        if (nativePage == null) return;
        assert nativePage != mNativePage : "Attempting to destroy active page.";

        nativePage.destroy();
    }

    /**
     * Called when the background color for the content changes.
     * @param color The current for the background.
     */
    protected void onBackgroundColorChanged(int color) {
        for (TabObserver observer : mObservers) observer.onBackgroundColorChanged(this, color);
    }

    /**
     * Destroys the current {@link ContentViewCore}.
     * @param deleteNativeWebContents Whether or not to delete the native WebContents pointer.
     */
    protected final void destroyContentViewCore(boolean deleteNativeWebContents) {
        if (mContentViewCore == null) return;

        destroyContentViewCoreInternal(mContentViewCore);

        if (mInfoBarContainer != null && mInfoBarContainer.getParent() != null) {
            mInfoBarContainer.removeFromParentView();
        }
        mContentViewCore.destroy();

        mContentViewCore = null;
        mWebContentsDelegate = null;
        mWebContentsObserver = null;
        mVoiceSearchTabHelper = null;

        assert mNativeTabAndroid != 0;
        nativeDestroyWebContents(mNativeTabAndroid, deleteNativeWebContents);
    }

    /**
     * Gives subclasses the chance to clean up some state associated with this
     * {@link ContentViewCore}. This is because {@link #getContentViewCore()}
     * can return {@code null} if a {@link NativePage} is showing.
     *
     * @param cvc The {@link ContentViewCore} that should have associated state
     *            cleaned up.
     */
    protected void destroyContentViewCoreInternal(ContentViewCore cvc) {
    }

    /**
     * A helper method to allow subclasses to build their own delegate.
     * @return An instance of a {@link TabChromeWebContentsDelegateAndroid}.
     */
    protected TabChromeWebContentsDelegateAndroid createWebContentsDelegate() {
        return new TabChromeWebContentsDelegateAndroid();
    }

    /**
     * A helper method to allow subclasses to handle the Instant support
     * disabled event.
     */
    @CalledByNative
    private void onWebContentsInstantSupportDisabled() {
        for (TabObserver observer : mObservers) observer.onWebContentsInstantSupportDisabled();
    }

    /**
     * A helper method to allow subclasses to build their own menu populator.
     * @return An instance of a {@link ContextMenuPopulator}.
     */
    protected ContextMenuPopulator createContextMenuPopulator() {
        return new ChromeContextMenuPopulator(new TabChromeContextMenuItemDelegate());
    }

    /**
     * @return The {@link WindowAndroid} associated with this {@link Tab}.
     */
    public WindowAndroid getWindowAndroid() {
        return mWindowAndroid;
    }

    /**
     * @return The current {@link TabChromeWebContentsDelegateAndroid} instance.
     */
    protected TabChromeWebContentsDelegateAndroid getChromeWebContentsDelegateAndroid() {
        return mWebContentsDelegate;
    }

    @CalledByNative
    protected void onFaviconAvailable(Bitmap icon) {
        boolean needUpdate = false;
        String url = getUrl();
        boolean pageUrlChanged = !url.equals(mFaviconUrl);

        // This method will be called multiple times if the page has more than one favicon.
        // we are trying to use the 16x16 DP icon here, Bitmap.createScaledBitmap will return
        // the origin bitmap if it is already 16x16 DP.
        if (pageUrlChanged || (icon.getWidth() == mNumPixel16DP
                && icon.getHeight() == mNumPixel16DP)) {
            mFavicon = Bitmap.createScaledBitmap(icon, mNumPixel16DP, mNumPixel16DP, true);
            needUpdate = true;
        }

        if (pageUrlChanged) {
            mFaviconUrl = url;
            needUpdate = true;
        }

        if (!needUpdate) return;

        for (TabObserver observer : mObservers) observer.onFaviconUpdated(this);
    }
    /**
     * Called when the navigation entry containing the historyitem changed,
     * for example because of a scroll offset or form field change.
     */
    @CalledByNative
    protected void onNavEntryChanged() {
    }

    /**
     * @return The native pointer representing the native side of this {@link Tab} object.
     */
    @CalledByNative
    protected long getNativePtr() {
        return mNativeTabAndroid;
    }

    /** This is currently called when committing a pre-rendered page. */
    @CalledByNative
    private void swapWebContents(
            long newWebContents, boolean didStartLoad, boolean didFinishLoad) {
        ContentViewCore cvc = new ContentViewCore(mContext);
        ContentView cv = ContentView.newInstance(mContext, cvc);
        cv.setContentDescription(mContext.getResources().getString(
                R.string.accessibility_content_view));
        cvc.initialize(cv, cv, newWebContents, getWindowAndroid());
        swapContentViewCore(cvc, false, didStartLoad, didFinishLoad);
    }

    /**
     * Called to swap out the current view with the one passed in.
     *
     * @param newContentViewCore The content view that should be swapped into the tab.
     * @param deleteOldNativeWebContents Whether to delete the native web
     *         contents of old view.
     * @param didStartLoad Whether
     *         WebContentsObserver::DidStartProvisionalLoadForFrame() has
     *         already been called.
     * @param didFinishLoad Whether WebContentsObserver::DidFinishLoad() has
     *         already been called.
     */
    protected void swapContentViewCore(ContentViewCore newContentViewCore,
            boolean deleteOldNativeWebContents, boolean didStartLoad, boolean didFinishLoad) {
        int originalWidth = 0;
        int originalHeight = 0;
        if (mContentViewCore != null) {
            originalWidth = mContentViewCore.getViewportWidthPix();
            originalHeight = mContentViewCore.getViewportHeightPix();
            mContentViewCore.onHide();
        }
        destroyContentViewCore(deleteOldNativeWebContents);
        NativePage previousNativePage = mNativePage;
        mNativePage = null;
        setContentViewCore(newContentViewCore);
        // Size of the new ContentViewCore is zero at this point. If we don't call onSizeChanged(),
        // next onShow() call would send a resize message with the current ContentViewCore size
        // (zero) to the renderer process, although the new size will be set soon.
        // However, this size fluttering may confuse Blink and rendered result can be broken
        // (see http://crbug.com/340987).
        mContentViewCore.onSizeChanged(originalWidth, originalHeight, 0, 0);
        mContentViewCore.onShow();
        mContentViewCore.attachImeAdapter();
        destroyNativePageInternal(previousNativePage);
        for (TabObserver observer : mObservers) {
            observer.onWebContentsSwapped(this, didStartLoad, didFinishLoad);
        }
    }

    @CalledByNative
    private void clearNativePtr() {
        assert mNativeTabAndroid != 0;
        mNativeTabAndroid = 0;
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeTabAndroid == 0;
        mNativeTabAndroid = nativePtr;
    }

    @CalledByNative
    private long getNativeInfoBarContainer() {
        return getInfoBarContainer().getNative();
    }

    /**
     * @return Whether the TabState representing this Tab has been updated.
     */
    public boolean isTabStateDirty() {
        return mIsTabStateDirty;
    }

    /**
     * Set whether the TabState representing this Tab has been updated.
     * @param isDirty Whether the Tab's state has changed.
     */
    public void setIsTabStateDirty(boolean isDirty) {
        mIsTabStateDirty = isDirty;
    }

    /**
     * @return Whether the Tab should be preserved in Android's Recents list when users hit "back".
     */
    public boolean shouldPreserve() {
        return mShouldPreserve;
    }

    /**
     * Sets whether the Tab should be preserved in Android's Recents list when users hit "back".
     * @param preserve Whether the tab should be preserved.
     */
    public void setShouldPreserve(boolean preserve) {
        mShouldPreserve = preserve;
    }

    /**
     * @return Parameters that should be used for a lazily loaded Tab.  May be null.
     */
    protected LoadUrlParams getPendingLoadParams() {
        return mPendingLoadParams;
    }

    /**
     * @param params Parameters that should be used for a lazily loaded Tab.
     */
    protected void setPendingLoadParams(LoadUrlParams params) {
        mPendingLoadParams = params;
        mUrl = params.getUrl();
    }

    /**
     * @see #setAppAssociatedWith(String) for more information.
     * TODO(aurimas): investigate reducing the visibility of this method after TabModel refactoring.
     *
     * @return The id of the application associated with that tab (null if not
     *         associated with an app).
     */
    public String getAppAssociatedWith() {
        return mAppAssociatedWith;
    }

    /**
     * Associates this tab with the external app with the specified id. Once a Tab is associated
     * with an app, it is reused when a new page is opened from that app (unless the user typed in
     * the location bar or in the page, in which case the tab is dissociated from any app)
     * TODO(aurimas): investigate reducing the visibility of this method after TabModel refactoring.
     *
     * @param appId The ID of application associated with the tab.
     */
    public void setAppAssociatedWith(String appId) {
        mAppAssociatedWith = appId;
    }

    /**
     * Validates {@code id} and increments the internal counter to make sure future ids don't
     * collide.
     * @param id The current id.  Maybe {@link #INVALID_TAB_ID}.
     * @return   A new id if {@code id} was {@link #INVALID_TAB_ID}, or {@code id}.
     */
    public static int generateValidId(int id) {
        if (id == INVALID_TAB_ID) id = generateNextId();
        incrementIdCounterTo(id + 1);

        return id;
    }

    /**
     * Restores a tab either frozen or from state.
     * TODO(aurimas): investigate reducing the visibility of this method after TabModel refactoring.
     */
    public void createHistoricalTab() {
        if (!isFrozen()) {
            nativeCreateHistoricalTab(mNativeTabAndroid);
        } else if (mFrozenContentsState != null) {
            mFrozenContentsState.createHistoricalTab();
        }
    }

    /**
     * @return The reason the Tab was launched.
     */
    public TabLaunchType getLaunchType() {
        return mLaunchType;
    }

    /**
     * @return true iff the tab doesn't hold a live page. This happens before initialize() and when
     * the tab holds frozen WebContents state that is yet to be inflated.
     */
    @VisibleForTesting
    public boolean isFrozen() {
        return getNativePage() == null && getContentViewCore() == null;
    }

    /**
     * @return An instance of a {@link FullscreenManager}.
     */
    protected FullscreenManager getFullscreenManager() {
        return mFullscreenManager;
    }

    /**
     * Clears hung renderer state.
     */
    protected void clearHungRendererState() {
        if (mFullscreenManager == null) return;

        mFullscreenManager.hideControlsPersistent(mFullscreenHungRendererToken);
        mFullscreenHungRendererToken = FullscreenManager.INVALID_TOKEN;
        updateFullscreenEnabledState();
    }

    /**
     * Called when offset values related with fullscreen functionality has been changed by the
     * compositor.
     * @param topControlsOffsetY The Y offset of the top controls in physical pixels.
     * @param contentOffsetY The Y offset of the content in physical pixels.
     * @param overdrawBottomHeight The overdraw height.
     * @param isNonFullscreenPage Whether a current page is non-fullscreen page or not.
     */
    protected void onOffsetsChanged(float topControlsOffsetY, float contentOffsetY,
            float overdrawBottomHeight, boolean isNonFullscreenPage) {
        mPreviousFullscreenTopControlsOffsetY = topControlsOffsetY;
        mPreviousFullscreenContentOffsetY = contentOffsetY;
        mPreviousFullscreenOverdrawBottomHeight = overdrawBottomHeight;

        if (mFullscreenManager == null) return;
        if (isNonFullscreenPage || isNativePage()) {
            mFullscreenManager.setPositionsForTabToNonFullscreen();
        } else {
            mFullscreenManager.setPositionsForTab(topControlsOffsetY, contentOffsetY);
        }
        TabModelBase.setActualTabSwitchLatencyMetricRequired();
    }

    /**
     * Push state about whether or not the top controls can show or hide to the renderer.
     */
    public void updateFullscreenEnabledState() {
        if (isFrozen() || mFullscreenManager == null) return;

        updateTopControlsState(getTopControlsStateConstraints(), TopControlsState.BOTH, true);

        if (getContentViewCore() != null) {
            getContentViewCore().updateMultiTouchZoomSupport(
                    !mFullscreenManager.getPersistentFullscreenMode());
        }
    }

    /**
     * Updates the top controls state for this tab.  As these values are set at the renderer
     * level, there is potential for this impacting other tabs that might share the same
     * process.
     *
     * @param constraints The constraints that determine whether the controls can be shown
     *                    or hidden at all.
     * @param current The desired current state for the controls.  Pass
     *                {@link TopControlsState#BOTH} to preserve the current position.
     * @param animate Whether the controls should animate to the specified ending condition or
     *                should jump immediately.
     */
    protected void updateTopControlsState(int constraints, int current, boolean animate) {
        if (mNativeTabAndroid == 0) return;
        nativeUpdateTopControlsState(mNativeTabAndroid, constraints, current, animate);
    }

    /**
     * Updates the top controls state for this tab.  As these values are set at the renderer
     * level, there is potential for this impacting other tabs that might share the same
     * process.
     *
     * @param current The desired current state for the controls.  Pass
     *                {@link TopControlsState#BOTH} to preserve the current position.
     * @param animate Whether the controls should animate to the specified ending condition or
     *                should jump immediately.
     */
    public void updateTopControlsState(int current, boolean animate) {
        int constraints = getTopControlsStateConstraints();
        // Do nothing if current and constraints conflict to avoid error in
        // renderer.
        if ((constraints == TopControlsState.HIDDEN && current == TopControlsState.SHOWN)
                || (constraints == TopControlsState.SHOWN && current == TopControlsState.HIDDEN)) {
            return;
        }
        updateTopControlsState(getTopControlsStateConstraints(), current, animate);
    }

    /**
     * @return Whether hiding top controls is enabled or not.
     */
    protected boolean isHidingTopControlsEnabled() {
        String url = getUrl();
        boolean enableHidingTopControls = url != null && !url.startsWith(UrlConstants.CHROME_SCHEME)
                && !url.startsWith(UrlConstants.CHROME_NATIVE_SCHEME);

        int securityState = getSecurityLevel();
        enableHidingTopControls &= (securityState != ToolbarModelSecurityLevel.SECURITY_ERROR
                && securityState != ToolbarModelSecurityLevel.SECURITY_WARNING);

        enableHidingTopControls &=
                !AccessibilityUtil.isAccessibilityEnabled(getApplicationContext());
        return enableHidingTopControls;
    }

    /**
     * @return The current visibility constraints for the display of top controls.
     *         {@link TopControlsState} defines the valid return options.
     */
    protected int getTopControlsStateConstraints() {
        if (mFullscreenManager == null) return TopControlsState.SHOWN;

        boolean enableHidingTopControls = isHidingTopControlsEnabled();
        boolean enableShowingTopControls = !mFullscreenManager.getPersistentFullscreenMode();

        int constraints = TopControlsState.BOTH;
        if (!enableShowingTopControls) {
            constraints = TopControlsState.HIDDEN;
        } else if (!enableHidingTopControls) {
            constraints = TopControlsState.SHOWN;
        }
        return constraints;
    }

    /**
     * @param manager The fullscreen manager that should be notified of changes to this tab (if
     *                set to null, no more updates will come from this tab).
     */
    public void setFullscreenManager(FullscreenManager manager) {
        mFullscreenManager = manager;
        if (mFullscreenManager != null) {
            if (Float.isNaN(mPreviousFullscreenTopControlsOffsetY)
                    || Float.isNaN(mPreviousFullscreenContentOffsetY)) {
                mFullscreenManager.setPositionsForTabToNonFullscreen();
            } else {
                mFullscreenManager.setPositionsForTab(
                        mPreviousFullscreenTopControlsOffsetY, mPreviousFullscreenContentOffsetY);
            }
            mFullscreenManager.showControlsTransient();
            updateFullscreenEnabledState();
        }
    }

    /**
     * @return The most recent frame's overdraw bottom height in pixels.
     */
    public float getFullscreenOverdrawBottomHeightPix() {
        return mPreviousFullscreenOverdrawBottomHeight;
    }

    /**
     * @return An unused id.
     */
    private static int generateNextId() {
        return sIdCounter.getAndIncrement();
    }

    private void pushNativePageStateToNavigationEntry() {
        assert mNativeTabAndroid != 0 && getNativePage() != null;
        nativeSetActiveNavigationEntryTitleForUrl(mNativeTabAndroid, getNativePage().getUrl(),
                getNativePage().getTitle());
    }

    /**
     * Ensures the counter is at least as high as the specified value.  The counter should always
     * point to an unused ID (which will be handed out next time a request comes in).  Exposed so
     * that anything externally loading tabs and ids can set enforce new tabs start at the correct
     * id.
     * TODO(aurimas): Investigate reducing the visiblity of this method.
     * @param id The minimum id we should hand out to the next new tab.
     */
    public static void incrementIdCounterTo(int id) {
        int diff = id - sIdCounter.get();
        if (diff <= 0) return;
        // It's possible idCounter has been incremented between the get above and the add below
        // but that's OK, because in the worst case we'll overly increment idCounter.
        sIdCounter.addAndGet(diff);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeTabAndroid);
    private native void nativeInitWebContents(long nativeTabAndroid, boolean incognito,
            ContentViewCore contentViewCore, ChromeWebContentsDelegateAndroid delegate,
            ContextMenuPopulator contextMenuPopulator);
    private native void nativeDestroyWebContents(long nativeTabAndroid, boolean deleteNative);
    private native Profile nativeGetProfileAndroid(long nativeTabAndroid);
    private native int nativeLoadUrl(long nativeTabAndroid, String url, String extraHeaders,
            byte[] postData, int transition, String referrerUrl, int referrerPolicy,
            boolean isRendererInitiated);
    private native void nativeSetActiveNavigationEntryTitleForUrl(long nativeTabAndroid, String url,
            String title);
    private native boolean nativePrint(long nativeTabAndroid);
    private native Bitmap nativeGetFavicon(long nativeTabAndroid);
    private native void nativeCreateHistoricalTab(long nativeTabAndroid);
    private native void nativeUpdateTopControlsState(
            long nativeTabAndroid, int constraints, int current, boolean animate);
}
