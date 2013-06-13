// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill;

import android.content.Context;
import android.graphics.Paint;
import android.graphics.Rect;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnLayoutChangeListener;
import android.widget.AdapterView;
import android.widget.ListPopupWindow;
import android.widget.TextView;

import java.util.ArrayList;

import org.chromium.chrome.R;
import org.chromium.ui.ViewAndroidDelegate;

/**
 * The Autofill suggestion popup that lists relevant suggestions.
 */
public class AutofillPopup extends ListPopupWindow implements AdapterView.OnItemClickListener {

    /**
     * Constants defining types of Autofill suggestion entries.
     * Has to be kept in sync with enum in WebAutofillClient.h
     *
     * Not supported: MenuItemIDWarningMessage, MenuItemIDSeparator, MenuItemIDClearForm, and
     * MenuItemIDAutofillOptions.
     */
    private static final int ITEM_ID_AUTOCOMPLETE_ENTRY = 0;
    private static final int ITEM_ID_PASSWORD_ENTRY = -2;
    private static final int ITEM_ID_DATA_LIST_ENTRY = -6;

    private static final int TEXT_PADDING_DP = 40;

    private final AutofillPopupDelegate mAutofillCallback;
    private final Context mContext;
    private final ViewAndroidDelegate mViewAndroidDelegate;
    private View mAnchorView;
    private float mAnchorWidth;
    private float mAnchorHeight;
    private float mAnchorX;
    private float mAnchorY;
    private Paint mNameViewPaint;
    private Paint mLabelViewPaint;
    private OnLayoutChangeListener mLayoutChangeListener;

    /**
     * An interface to handle the touch interaction with an AutofillPopup object.
     */
    public interface AutofillPopupDelegate {
        /**
         * Requests the controller to hide AutofillPopup.
         */
        public void requestHide();

        /**
         * Handles the selection of an Autofill suggestion from an AutofillPopup.
         * @param listIndex The index of the selected Autofill suggestion.
         */
        public void suggestionSelected(int listIndex);
    }

    /**
     * Creates an AutofillWindow with specified parameters.
     * @param context Application context.
     * @param viewAndroidDelegate View delegate used to add and remove views.
     * @param autofillCallback A object that handles the calls to the native AutofillPopupView.
     */
    public AutofillPopup(Context context, ViewAndroidDelegate viewAndroidDelegate,
            AutofillPopupDelegate autofillCallback) {
        super(context);
        mContext = context;
        mViewAndroidDelegate = viewAndroidDelegate ;
        mAutofillCallback = autofillCallback;

        setOnItemClickListener(this);

        mAnchorView = mViewAndroidDelegate.acquireAnchorView();
        mViewAndroidDelegate.setAnchorViewPosition(mAnchorView, mAnchorX, mAnchorY, mAnchorWidth,
                mAnchorHeight);

        mLayoutChangeListener = new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                if (v == mAnchorView) AutofillPopup.this.show();
            }
        };

        mAnchorView.addOnLayoutChangeListener(mLayoutChangeListener);
        setAnchorView(mAnchorView);
    }

    /**
     * Sets the location and the size of the anchor view that the AutofillPopup will use to attach
     * itself.
     * @param x X coordinate of the top left corner of the anchor view.
     * @param y Y coordinate of the top left corner of the anchor view.
     * @param width The width of the anchor view.
     * @param height The height of the anchor view.
     */
    public void setAnchorRect(float x, float y, float width, float height) {
        mAnchorWidth = width;
        mAnchorHeight = height;
        mAnchorX = x;
        mAnchorY = y;
        if (mAnchorView != null) {
            mViewAndroidDelegate.setAnchorViewPosition(mAnchorView, mAnchorX, mAnchorY,
                    mAnchorWidth, mAnchorHeight);
        }
    }

    /**
     * Sets the Autofill suggestions to display in the popup and shows the popup.
     * @param suggestions Autofill suggestion data.
     */
    public void show(AutofillSuggestion[] suggestions) {
        // Remove the AutofillSuggestions with IDs that are not supported by Android
        ArrayList<AutofillSuggestion> cleanedData = new ArrayList<AutofillSuggestion>();
        for (int i = 0; i < suggestions.length; i++) {
            int itemId = suggestions[i].mUniqueId;
            if (itemId > 0 || itemId == ITEM_ID_AUTOCOMPLETE_ENTRY ||
                    itemId == ITEM_ID_PASSWORD_ENTRY || itemId == ITEM_ID_DATA_LIST_ENTRY) {
                cleanedData.add(suggestions[i]);
            }
        }
        setAdapter(new AutofillListAdapter(mContext, cleanedData));
        // Once the mAnchorRect is resized and placed correctly, it will show the Autofill popup.
        mAnchorWidth = Math.max(getDesiredWidth(suggestions), mAnchorWidth);
        mViewAndroidDelegate.setAnchorViewPosition(mAnchorView, mAnchorX, mAnchorY, mAnchorWidth,
                mAnchorHeight);
    }

    /**
     * Overrides the default dismiss behavior to request the controller to dismiss the view.
     */
    @Override
    public void dismiss() {
        mAutofillCallback.requestHide();
    }

    /**
     * Hides the popup and removes the anchor view from the ContainerView.
     */
    public void hide() {
        super.dismiss();
        mAnchorView.removeOnLayoutChangeListener(mLayoutChangeListener);
        mViewAndroidDelegate.releaseAnchorView(mAnchorView);
    }

    /**
     * Get desired popup window width by calculating the maximum text length from Autofill data.
     * @param data Autofill suggestion data.
     * @return The popup window width in DIP.
     */
    private float getDesiredWidth(AutofillSuggestion[] data) {
        if (mNameViewPaint == null || mLabelViewPaint == null) {
            LayoutInflater inflater =
                    (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View layout = inflater.inflate(R.layout.autofill_text, null);
            TextView nameView = (TextView) layout.findViewById(R.id.autofill_name);
            mNameViewPaint = nameView.getPaint();
            TextView labelView = (TextView) layout.findViewById(R.id.autofill_label);
            mLabelViewPaint = labelView.getPaint();
        }

        float maxTextWidth = 0;
        Rect bounds = new Rect();
        for (int i = 0; i < data.length; ++i) {
            bounds.setEmpty();
            String name = data[i].mName;
            float width = 0;
            if (name.length() > 0) {
                mNameViewPaint.getTextBounds(name, 0, name.length(), bounds);
            }
            width += bounds.width();

            bounds.setEmpty();
            String label = data[i].mLabel;
            if (label.length() > 0) {
                mLabelViewPaint.getTextBounds(label, 0, label.length(), bounds);
            }
            width += bounds.width();
            maxTextWidth = Math.max(width, maxTextWidth);
        }
        // Scale it down to make it unscaled by screen density.
        maxTextWidth = maxTextWidth / mContext.getResources().getDisplayMetrics().density;
        // Adding padding.
        return maxTextWidth + TEXT_PADDING_DP;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        mAutofillCallback.suggestionSelected(position);
    }

}
