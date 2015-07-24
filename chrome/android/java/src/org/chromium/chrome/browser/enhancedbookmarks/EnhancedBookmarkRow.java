// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.enhancedbookmarks;

import android.content.Context;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Checkable;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ListPopupWindow;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BookmarksBridge.BookmarkItem;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;

import java.util.List;

/**
 * Common logic for bookmark and folder rows.
 */
abstract class EnhancedBookmarkRow extends FrameLayout implements EnhancedBookmarkUIObserver,
        Checkable, OnClickListener, OnLongClickListener {

    protected ImageView mIconImageView;
    protected TextView mTitleView;
    private TintedImageButton mMoreIcon;
    private EnhancedBookmarkItemHighlightView mHighlightView;

    protected EnhancedBookmarkDelegate mDelegate;
    protected BookmarkId mBookmarkId;
    private ListPopupWindow mPopupMenu;
    private boolean mIsAttachedToWindow = false;

    /**
     * Constructor for inflating from XML.
     */
    public EnhancedBookmarkRow(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Updates this row for the given {@link BookmarkId}.
     * @return The {@link BookmarkItem} corresponding the given {@link BookmarkId}.
     */
    BookmarkItem setBookmarkId(BookmarkId bookmarkId) {
        mBookmarkId = bookmarkId;
        BookmarkItem bookmarkItem = mDelegate.getModel().getBookmarkById(bookmarkId);
        mMoreIcon.setVisibility(bookmarkItem.isEditable() ? VISIBLE : GONE);
        setChecked(mDelegate.isBookmarkSelected(bookmarkId));
        return bookmarkItem;
    }

    /**
     * Same as {@link OnClickListener#onClick(View)} on this.
     * Subclasses should override this instead of setting their own OnClickListener because this
     * class handles onClick events in selection mode, and won't forward events to subclasses in
     * that case.
     */
    protected abstract void onClick();

    private void initialize() {
        mDelegate.addUIObserver(this);
        updateSelectionState();
    }

    private void cleanup() {
        if (mPopupMenu != null) mPopupMenu.dismiss();
        if (mDelegate != null) mDelegate.removeUIObserver(this);
    }

    private void updateSelectionState() {
        mMoreIcon.setClickable(!mDelegate.isSelectionEnabled());
    }

    /**
     * Show drop-down menu after user click on more-info icon
     * @param view The anchor view for the menu
     */
    private void showMenu(View view) {
        if (mPopupMenu == null) {
            mPopupMenu = new ListPopupWindow(getContext(), null, 0,
                    R.style.EnhancedBookmarkMenuStyle);
            mPopupMenu.setAdapter(new ArrayAdapter<String>(
                    getContext(), R.layout.eb_popup_item, new String[] {
                            getContext().getString(R.string.enhanced_bookmark_item_select),
                            getContext().getString(R.string.enhanced_bookmark_item_edit),
                            getContext().getString(R.string.enhanced_bookmark_item_move),
                            getContext().getString(R.string.enhanced_bookmark_item_delete)}) {
                private static final int MOVE_POSITION = 1;

                @Override
                public boolean areAllItemsEnabled() {
                    return false;
                }

                @Override
                public boolean isEnabled(int position) {
                    // Partner bookmarks can't move, so disable that.
                    return mBookmarkId.getType() != BookmarkType.PARTNER
                            || position != MOVE_POSITION;
                }

                @Override
                public View getView(int position, View convertView, ViewGroup parent) {
                    View view = super.getView(position, convertView, parent);
                    view.setEnabled(isEnabled(position));
                    return view;
                }
            });
            mPopupMenu.setAnchorView(view);
            mPopupMenu.setWidth(getResources().getDimensionPixelSize(
                            R.dimen.enhanced_bookmark_item_popup_width));
            mPopupMenu.setVerticalOffset(-view.getHeight());
            mPopupMenu.setModal(true);
            mPopupMenu.setOnItemClickListener(new OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position,
                        long id) {
                    if (position == 0) {
                        setChecked(mDelegate.toggleSelectionForBookmark(mBookmarkId));
                    } else if (position == 1) {
                        BookmarkItem item = mDelegate.getModel().getBookmarkById(mBookmarkId);
                        if (item.isFolder()) {
                            EnhancedBookmarkAddEditFolderActivity.startEditFolderActivity(
                                    getContext(), item.getId());
                        } else {
                            EnhancedBookmarkUtils.startEditActivity(getContext(), item.getId());
                        }
                    } else if (position == 2) {
                        EnhancedBookmarkFolderSelectActivity.startFolderSelectActivity(getContext(),
                                mBookmarkId);
                    } else if (position == 3) {
                        mDelegate.getModel().deleteBookmarks(mBookmarkId);
                    }
                    mPopupMenu.dismiss();
                }
            });
        }
        mPopupMenu.show();
        mPopupMenu.getListView().setDivider(null);
    }

    // FrameLayout implementations.

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mIconImageView = (ImageView) findViewById(R.id.bookmark_image);
        mTitleView = (TextView) findViewById(R.id.title);
        mMoreIcon = (TintedImageButton) findViewById(R.id.more);
        mHighlightView = (EnhancedBookmarkItemHighlightView) findViewById(R.id.highlight);

        mMoreIcon.setColorFilterMode(PorterDuff.Mode.MULTIPLY);
        mMoreIcon.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                showMenu(view);
            }
        });

        setOnClickListener(this);
        setOnLongClickListener(this);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mIsAttachedToWindow = true;
        if (mDelegate != null) initialize();
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mIsAttachedToWindow = false;
        cleanup();
    }

    // OnClickListener implementation.

    @Override
    public final void onClick(View view) {
        assert view == this;

        if (mDelegate.isSelectionEnabled()) {
            onLongClick(view);
        } else {
            onClick();
        }
    }

    // OnLongClickListener implementation.

    @Override
    public boolean onLongClick(View view) {
        assert view == this;

        setChecked(mDelegate.toggleSelectionForBookmark(mBookmarkId));
        return true;
    }

    // Checkable implementations.

    @Override
    public boolean isChecked() {
        return mHighlightView.isChecked();
    }

    @Override
    public void toggle() {
        setChecked(!isChecked());
    }

    @Override
    public void setChecked(boolean checked) {
        mHighlightView.setChecked(checked);
    }

    // EnhancedBookmarkUIObserver implementations.

    @Override
    public void onEnhancedBookmarkDelegateInitialized(EnhancedBookmarkDelegate delegate) {
        assert mDelegate == null;
        mDelegate = delegate;
        if (mIsAttachedToWindow) initialize();
    }

    @Override
    public void onDestroy() {
        cleanup();
    }

    @Override
    public void onAllBookmarksStateSet() {
    }

    @Override
    public void onFolderStateSet(BookmarkId folder) {
    }

    @Override
    public void onSelectionStateChange(List<BookmarkId> selectedBookmarks) {
        updateSelectionState();
    }
}
