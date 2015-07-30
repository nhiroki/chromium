// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import android.view.View;

import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.layouts.components.CompositorButton;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutHelper;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutHelperManager;
import org.chromium.chrome.browser.compositor.overlays.strip.StripLayoutTab;
import org.chromium.chrome.test.ChromeTabbedActivityTestBase;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestTouchUtils;

/**
 * A utility class that contains methods generic to all TabStrip test classes.
 */
public class TabStripUtils {

    /**
     * @param activity The main activity that contains the TabStrips.
     * @param incognito Whether or not the TabStrip should be from the incognito or normal model.
     * @return The TabStrip for the specified model.
     */
    public static StripLayoutHelper getStripLayoutHelper(ChromeTabbedActivity activity,
            boolean incognito) {
        StripLayoutHelperManager manager = getStripLayoutHelperManager(activity);
        if (manager != null) {
            return manager.getStripLayoutHelper(incognito);
        }
        return null;
    }

    /**
     * @param activity The main activity that contains the TabStrips.
     * @return The TabStrip for the specified model.
     */
    public static StripLayoutHelper getActiveStripLayoutHelper(ChromeTabbedActivity activity) {
        StripLayoutHelperManager manager = getStripLayoutHelperManager(activity);
        if (manager != null) {
            return manager.getActiveStripLayoutHelper();
        }
        return null;
    }

    /**
     * @param activity The main activity that contains the TabStrips.
     * @return The TabStrip for the specified model.
     */
    public static StripLayoutHelperManager getStripLayoutHelperManager(
            ChromeTabbedActivity activity) {
        StripLayoutHelperManager manager =
                activity.getLayoutManager().getStripLayoutHelperManager();
        return manager;
    }

    /**
     * Finds a TabView from a TabStrip based on a Tab id.
     * @param activity The main activity that contains the TabStrips.
     * @param incognito Whether or not to use the incognito strip or the nromal strip.
     * @param id The Tab id to look for.
     * @return The TabView that represents the Tab identified by the id.  Null if not found.
     */
    public static StripLayoutTab findStripLayoutTab(ChromeTabbedActivity activity,
            boolean incognito, int id) {
        StripLayoutHelper strip = getStripLayoutHelper(activity, incognito);
        return strip.findTabById(id);
    }

    /**
     * Click a compositor tab strip tab;
     * @param tab The tab to click.
     * @param base The ChromeTabbedActivityTestBase where we're calling this from.
     */
    public static void clickTab(StripLayoutTab tab, ChromeTabbedActivityTestBase base) {
        View view = base.getActivity().getTabsView();
        final float x = tab.getDrawX() + tab.getWidth() / 2;
        final float y = tab.getDrawY() + tab.getHeight() / 2;
        TestTouchUtils.singleClickView(base.getInstrumentation(), view, (int) x, (int) y);
    }

    /**
     * Click a compositor button;
     * @param button The button to click.
     * @param base The ChromeTabbedActivityTestBase where we're calling this from.
     */
    public static void clickCompositorButton(CompositorButton button,
            ChromeTabbedActivityTestBase base) {
        final StripLayoutHelperManager manager = getStripLayoutHelperManager(base.getActivity());
        final float x = button.getX() + button.getWidth() / 2;
        final float y = button.getY() + button.getHeight() / 2;
        base.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                manager.click(0, x, y, false, 0);
            }
        });
    }

    /**
     * Long press a compositor button.
     * @param button The button to long press.
     * @param base The ChromeTabbedActivityTestBase where we're calling this from.
     */
    public static void longPressCompositorButton(CompositorButton button,
            ChromeTabbedActivityTestBase base) {
        final StripLayoutHelperManager manager = getStripLayoutHelperManager(base.getActivity());
        final float x = button.getX() + button.getWidth() / 2;
        final float y = button.getY() + button.getHeight() / 2;
        base.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                manager.onLongPress(0, x, y);
            }
        });
    }

    /**
     * @param tabStrip The tab strip to wait for.
     */
    public static void settleDownCompositor(final StripLayoutHelper tabStrip) throws
            InterruptedException {
        CriteriaHelper.pollForUIThreadCriteria(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return !tabStrip.isAnimating();
            }
        });
    }
}
