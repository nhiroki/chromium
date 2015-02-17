// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * FileGrid constructor.
 *
 * Represents grid for the Grid View in the File Manager.
 * @constructor
 * @extends {cr.ui.Grid}
 */

function FileGrid() {
  throw new Error('Use FileGrid.decorate');
}

/**
 * Thumbnail quality.
 * @enum {number}
 */
FileGrid.ThumbnailQuality = {
  LOW: 0,
  HIGH: 1
};

/**
 * Inherits from cr.ui.Grid.
 */
FileGrid.prototype.__proto__ = cr.ui.Grid.prototype;

/**
 * Decorates an HTML element to be a FileGrid.
 * @param {!Element} self The grid to decorate.
 * @param {MetadataCache} metadataCache Metadata cache for thumbnails.
 * @param {!FileSystemMetadata} fileSystemMetadata File system metadata.
 * @param {VolumeManagerWrapper} volumeManager Volume manager instance.
 * @param {!importer.HistoryLoader} historyLoader
 */
FileGrid.decorate = function(
    self, metadataCache, fileSystemMetadata, volumeManager, historyLoader) {
  cr.ui.Grid.decorate(self);
  self.__proto__ = FileGrid.prototype;
  self.metadataCache_ = metadataCache;
  self.fileSystemMetadata_ = fileSystemMetadata;
  self.volumeManager_ = volumeManager;
  self.historyLoader_ = historyLoader;

  /** @private {ListThumbnailLoader} */
  self.listThumbnailLoader_ = null;

  /** @private {number} */
  self.beginIndex_ = 0;

  /** @private {number} */
  self.endIndex_ = 0;

  /** @private {function(!Event)} */
  self.onThumbnailLoadedBound_ = self.onThumbnailLoaded_.bind(self);

  self.scrollBar_ = new ScrollBar();
  self.scrollBar_.initialize(self.parentElement, self);

  /**
   * Map of URL and ListItem generated at the previous update time.
   * This is used for updating existing item synchronously.
   * @type {Object.<string, !FileGrid.Item>}
   * @private
   * @const
   */
  self.previousItems_ = {};

  self.itemConstructor = function(entry) {
    var item = self.ownerDocument.createElement('li');
    FileGrid.Item.decorate(
        item,
        entry,
        /** @type {FileGrid} */ (self),
        self.previousItems_[entry.toURL()]);
    self.previousItems_[entry.toURL()] = /** @type {!FileGrid.Item} */ (item);
    return item;
  };

  self.relayoutRateLimiter_ =
      new AsyncUtil.RateLimiter(self.relayoutImmediately_.bind(self));
};

/**
 * Sets list thumbnail loader.
 * @param {ListThumbnailLoader} listThumbnailLoader A list thumbnail loader.
 * @private
 */
FileGrid.prototype.setListThumbnailLoader = function(listThumbnailLoader) {
  if (this.listThumbnailLoader_) {
    this.listThumbnailLoader_.removeEventListener(
        'thumbnailLoaded', this.onThumbnailLoadedBound_);
  }

  this.listThumbnailLoader_ = listThumbnailLoader;

  if (this.listThumbnailLoader_) {
    this.listThumbnailLoader_.addEventListener(
        'thumbnailLoaded', this.onThumbnailLoadedBound_);
    this.listThumbnailLoader_.setHighPriorityRange(
        this.beginIndex_, this.endIndex_);
  }
};

/**
 * Handles thumbnail loaded event.
 * @param {!Event} event An event.
 * @private
 */
FileGrid.prototype.onThumbnailLoaded_ = function(event) {
  var listItem = this.getListItemByIndex(event.index);
  if (listItem) {
    var thumbnail = listItem.querySelector('.img-container > .thumbnail');
    if (thumbnail) {
      FileGrid.setThumbnailImage_(
          assertInstanceof(thumbnail, HTMLDivElement), event.dataUrl);
    }
  }
};

/**
 * @override
 */
FileGrid.prototype.mergeItems = function(beginIndex, endIndex) {
  cr.ui.Grid.prototype.mergeItems.call(this, beginIndex, endIndex);

  // Keep these values to set range when a new list thumbnail loader is set.
  this.beginIndex_ = beginIndex;
  this.endIndex_ = endIndex;
  if (this.listThumbnailLoader_ !== null)
    this.listThumbnailLoader_.setHighPriorityRange(beginIndex, endIndex);

  // Update item cache.
  for (var url in this.previousItems_) {
    if (this.getIndexOfListItem(this.previousItems_[url]) === -1)
      delete this.previousItems_[url];
  }
};

/**
 * Updates items to reflect metadata changes.
 * @param {string} type Type of metadata changed.
 * @param {Array.<Entry>} entries Entries whose metadata changed.
 */
FileGrid.prototype.updateListItemsMetadata = function(type, entries) {
  var urls = util.entriesToURLs(entries);
  var boxes = this.querySelectorAll('.img-container');
  for (var i = 0; i < boxes.length; i++) {
    var box = boxes[i];
    var listItem = this.getListItemAncestor(box);
    var entry = listItem && this.dataModel.item(listItem.listIndex);
    if (!entry || urls.indexOf(entry.toURL()) === -1)
      continue;

    FileGrid.decorateThumbnailBox_(box,
                                   entry,
                                   this.fileSystemMetadata_,
                                   this.volumeManager_,
                                   this.historyLoader_,
                                   this.listThumbnailLoader_);
  }
};

/**
 * Redraws the UI. Skips multiple consecutive calls.
 */
FileGrid.prototype.relayout = function() {
  this.relayoutRateLimiter_.run();
};

/**
 * Redraws the UI immediately.
 * @private
 */
FileGrid.prototype.relayoutImmediately_ = function() {
  this.startBatchUpdates();
  this.columns = 0;
  this.redraw();
  this.endBatchUpdates();
  cr.dispatchSimpleEvent(this, 'relayout');
};

/**
 * Decorates thumbnail.
 * @param {cr.ui.ListItem} li List item.
 * @param {!Entry} entry Entry to render a thumbnail for.
 * @param {MetadataCache} metadataCache To retrieve thumbnail.
 * @param {!FileSystemMetadata} fileSystemMetadata To retrieve metadata.
 * @param {VolumeManagerWrapper} volumeManager Volume manager instance.
 * @param {!importer.HistoryLoader} historyLoader
 * @param {FileGrid.Item} previousItem Existing grid item. Usually it is the
 *     item used for the same entry before calling redraw() method. If it is
 *     non-null, the item show the thumbnail immediately until the new thumbanil
 *     is loaded.
 * @private
 */
FileGrid.decorateThumbnail_ = function(
    li,
    entry,
    metadataCache,
    fileSystemMetadata,
    volumeManager,
    historyLoader,
    listThumbnailLoader,
    previousItem) {
  li.className = 'thumbnail-item';
  if (entry)
    filelist.decorateListItem(li, entry, fileSystemMetadata);

  var frame = li.ownerDocument.createElement('div');
  frame.className = 'thumbnail-frame';
  li.appendChild(frame);

  // TODO(yawano) Most of following codes seems to be unnecessary anymore.
  // Investigate it, and make this simple.
  var previousBox =
      previousItem ? previousItem.querySelector('.img-container') : null;
  var box;
  var shouldLoadThumbnail;
  if (previousItem) {
    box = previousBox;
    var previousImage = box.querySelector('img');
    if (previousImage) {
      previousImage.classList.add('cached');
      shouldLoadThumbnail = !!entry;
    } else {
      shouldLoadThumbnail = false;
    }
  } else {
    box = li.ownerDocument.createElement('div');
    shouldLoadThumbnail = !!entry;
  }
  if (shouldLoadThumbnail) {
    FileGrid.decorateThumbnailBox_(
        box,
        entry,
        fileSystemMetadata,
        volumeManager,
        historyLoader,
        listThumbnailLoader);
  }
  frame.appendChild(box);

  var checkmark = li.ownerDocument.createElement('div');
  checkmark.className = 'checkmark';
  checkmark.addEventListener(
      'mouseup', util.repeatMouseEventWithCtrlKey.bind(null, li));
  checkmark.addEventListener(
      'mousedown', util.repeatMouseEventWithCtrlKey.bind(null, li));
  frame.appendChild(checkmark);

  var bottom = li.ownerDocument.createElement('div');
  bottom.className = 'thumbnail-bottom';
  var badge = li.ownerDocument.createElement('div');
  badge.className = 'badge';
  bottom.appendChild(badge);
  bottom.appendChild(filelist.renderFileTypeIcon(li.ownerDocument, entry));
  bottom.appendChild(filelist.renderFileNameLabel(li.ownerDocument, entry));
  frame.appendChild(bottom);
};

/**
 * Decorates the box containing a centered thumbnail image.
 *
 * @param {Element} box Box to decorate.
 * @param {Entry} entry Entry which thumbnail is generating for.
 * @param {!FileSystemMetadata} fileSystemMetadata To retrieve metadata.
 * @param {VolumeManagerWrapper} volumeManager Volume manager instance.
 * @param {!importer.HistoryLoader} historyLoader
 * @param {function(HTMLImageElement)=} opt_imageLoadCallback Callback called
 *     when the image has been loaded before inserting it into the DOM.
 * @private
 */
FileGrid.decorateThumbnailBox_ = function(
    box, entry, fileSystemMetadata, volumeManager, historyLoader,
    listThumbnailLoader, opt_imageLoadCallback) {
  box.className = 'img-container';

  if (importer.isEligibleEntry(volumeManager, entry)) {
    historyLoader.getHistory().then(
        FileGrid.applyHistoryBadges_.bind(
            null,
            /** @type {!FileEntry} */ (entry),
            box));
  }

  if (entry.isDirectory) {
    box.setAttribute('generic-thumbnail', 'folder');

    var shared = !!fileSystemMetadata.getCache([entry], ['shared'])[0].shared;
    box.classList.toggle('shared', shared);

    if (opt_imageLoadCallback)
      setTimeout(opt_imageLoadCallback, 0, null /* callback parameter */);
    return;
  }

  // Create a box for thumbnail here to perform transition correctly.
  var thumbnail = box.ownerDocument.createElement('div');
  thumbnail.classList.add('thumbnail');
  box.appendChild(thumbnail);

  // Set thumbnail if it's already in cache.
  if (listThumbnailLoader &&
      listThumbnailLoader.getThumbnailFromCache(entry)) {
    FileGrid.setThumbnailImage_(
        assertInstanceof(thumbnail, HTMLDivElement),
        listThumbnailLoader.getThumbnailFromCache(entry).dataUrl);
  } else {
    var mediaType = FileType.getMediaType(entry);
    box.setAttribute('generic-thumbnail', mediaType);
  }
};

/**
 * Sets thumbnail image to the box.
 * @param {!HTMLDivElement} thumbnail A div element to be filled with thumbnail.
 * @param {string} dataUrl Data url of thumbnail.
 * @private
 */
FileGrid.setThumbnailImage_ = function(thumbnail, dataUrl) {
  // TODO(yawano): Add transition animation for thumbnail update.
  thumbnail.style.backgroundImage = 'url(' + dataUrl + ')';
  thumbnail.classList.add('loaded');
};

/**
 * Applies cloud import history badges as appropriate for the Entry.
 *
 * @param {!FileEntry} entry
 * @param {Element} box Box to decorate.
 * @param {!importer.ImportHistory} history
 *
 * @private
 */
FileGrid.applyHistoryBadges_ = function(entry, box, history) {
  history.wasImported(entry, importer.Destination.GOOGLE_DRIVE)
      .then(
          function(imported) {
            if (imported) {
              // TODO(smckay): update badges when history changes
              // "box" is currently the sibling of the elemement
              // we want to style. So rather than employing
              // a possibly-fragile sibling selector we just
              // plop the imported class on the parent of both.
              box.parentElement.classList.add('imported');
            } else {
              history.wasCopied(entry, importer.Destination.GOOGLE_DRIVE)
                  .then(
                      function(copied) {
                        if (copied) {
                          // TODO(smckay): update badges when history changes
                          // "box" is currently the sibling of the elemement
                          // we want to style. So rather than employing
                          // a possibly-fragile sibling selector we just
                          // plop the imported class on the parent of both.
                          box.parentElement.classList.add('copied');
                        }
                      });
            }
          });
};

/**
 * Item for the Grid View.
 * @constructor
 * @extends {cr.ui.ListItem}
 */
FileGrid.Item = function() {
  throw new Error();
};

/**
 * Inherits from cr.ui.ListItem.
 */
FileGrid.Item.prototype.__proto__ = cr.ui.ListItem.prototype;

Object.defineProperty(FileGrid.Item.prototype, 'label', {
  /**
   * @this {FileGrid.Item}
   * @return {string} Label of the item.
   */
  get: function() {
    return this.querySelector('filename-label').textContent;
  }
});

/**
 * @param {Element} li List item element.
 * @param {!Entry} entry File entry.
 * @param {FileGrid} grid Owner.
 * @param {FileGrid.Item} previousItem Existing grid item. Usually it is the
 *     item used for the same entry before calling redraw() method. If it is
 *     non-null, the item show the thumbnail immediately until the new thumbanil
 *     is loaded.
 */
FileGrid.Item.decorate = function(li, entry, grid, previousItem) {
  li.__proto__ = FileGrid.Item.prototype;
  li = /** @type {!FileGrid.Item} */ (li);
  // TODO(mtomasz): Pass the metadata cache and the volume manager directly
  // instead of accessing private members of grid.
  FileGrid.decorateThumbnail_(
      li,
      entry,
      grid.metadataCache_,
      grid.fileSystemMetadata_,
      grid.volumeManager_,
      grid.historyLoader_,
      grid.listThumbnailLoader_,
      previousItem);

  // Override the default role 'listitem' to 'option' to match the parent's
  // role (listbox).
  li.setAttribute('role', 'option');
};

/**
 * Obtains if the drag selection should be start or not by referring the mouse
 * event.
 * @param {MouseEvent} event Drag start event.
 * @return {boolean} True if the mouse is hit to the background of the list.
 */
FileGrid.prototype.shouldStartDragSelection = function(event) {
  var pos = DragSelector.getScrolledPosition(this, event);
  return this.getHitElements(pos.x, pos.y).length === 0;
};

/**
 * Obtains the column/row index that the coordinate points.
 * @param {number} coordinate Vertical/horizontal coordinate value that points
 *     column/row.
 * @param {number} step Length from a column/row to the next one.
 * @param {number} threshold Threshold that determines whether 1 offset is added
 *     to the return value or not. This is used in order to handle the margin of
 *     column/row.
 * @return {number} Index of hit column/row.
 * @private
 */
FileGrid.prototype.getHitIndex_ = function(coordinate, step, threshold) {
  var index = ~~(coordinate / step);
  return (coordinate % step >= threshold) ? index + 1 : index;
};

/**
 * Obtains the index list of elements that are hit by the point or the
 * rectangle.
 *
 * We should match its argument interface with FileList.getHitElements.
 *
 * @param {number} x X coordinate value.
 * @param {number} y Y coordinate value.
 * @param {number=} opt_width Width of the coordinate.
 * @param {number=} opt_height Height of the coordinate.
 * @return {Array.<number>} Index list of hit elements.
 */
FileGrid.prototype.getHitElements = function(x, y, opt_width, opt_height) {
  var currentSelection = [];
  var right = x + (opt_width || 0);
  var bottom = y + (opt_height || 0);
  var itemMetrics = this.measureItem();
  var horizontalStartIndex = this.getHitIndex_(
      x, itemMetrics.width, itemMetrics.width - itemMetrics.marginRight);
  var horizontalEndIndex = Math.min(this.columns, this.getHitIndex_(
      right, itemMetrics.width, itemMetrics.marginLeft));
  var verticalStartIndex = this.getHitIndex_(
      y, itemMetrics.height, itemMetrics.height - itemMetrics.marginBottom);
  var verticalEndIndex = this.getHitIndex_(
      bottom, itemMetrics.height, itemMetrics.marginTop);
  for (var verticalIndex = verticalStartIndex;
       verticalIndex < verticalEndIndex;
       verticalIndex++) {
    var indexBase = this.getFirstItemInRow(verticalIndex);
    for (var horizontalIndex = horizontalStartIndex;
         horizontalIndex < horizontalEndIndex;
         horizontalIndex++) {
      var index = indexBase + horizontalIndex;
      if (0 <= index && index < this.dataModel.length)
        currentSelection.push(index);
    }
  }
  return currentSelection;
};
