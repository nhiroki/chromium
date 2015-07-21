// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Overrided metadata worker's path.
 * @type {string}
 */
ContentMetadataProvider.WORKER_SCRIPT = '/js/metadata_worker.js';

/**
 * Gallery for viewing and editing image files.
 *
 * @param {!VolumeManagerWrapper} volumeManager
 * @constructor
 * @struct
 */
function Gallery(volumeManager) {
  /**
   * @type {{appWindow: chrome.app.window.AppWindow, readonlyDirName: string,
   *     displayStringFunction: function(), loadTimeData: Object}}
   * @private
   */
  this.context_ = {
    appWindow: chrome.app.window.current(),
    readonlyDirName: '',
    displayStringFunction: function() { return ''; },
    loadTimeData: {},
  };
  this.container_ = queryRequiredElement(document, '.gallery');
  this.document_ = document;
  this.volumeManager_ = volumeManager;
  /**
   * @private {!MetadataModel}
   * @const
   */
  this.metadataModel_ = MetadataModel.create(volumeManager);
  /**
   * @private {!ThumbnailModel}
   * @const
   */
  this.thumbnailModel_ = new ThumbnailModel(this.metadataModel_);
  this.selectedEntry_ = null;
  this.onExternallyUnmountedBound_ = this.onExternallyUnmounted_.bind(this);
  this.initialized_ = false;

  this.dataModel_ = new GalleryDataModel(this.metadataModel_);
  var downloadVolumeInfo = this.volumeManager_.getCurrentProfileVolumeInfo(
      VolumeManagerCommon.VolumeType.DOWNLOADS);
  downloadVolumeInfo.resolveDisplayRoot().then(function(entry) {
    this.dataModel_.fallbackSaveDirectory = entry;
  }.bind(this)).catch(function(error) {
    console.error(
        'Failed to obtain the fallback directory: ' + (error.stack || error));
  });
  this.selectionModel_ = new cr.ui.ListSelectionModel();

  /**
   * @type {(SlideMode|ThumbnailMode)}
   * @private
   */
  this.currentMode_ = null;

  /**
   * @type {boolean}
   * @private
   */
  this.changingMode_ = false;

  // -----------------------------------------------------------------
  // Initializes the UI.

  // Initialize the dialog label.
  cr.ui.dialogs.BaseDialog.OK_LABEL = str('GALLERY_OK_LABEL');
  cr.ui.dialogs.BaseDialog.CANCEL_LABEL = str('GALLERY_CANCEL_LABEL');

  var content = queryRequiredElement(document, '#content');
  content.addEventListener('click', this.onContentClick_.bind(this));

  this.topToolbar_ = queryRequiredElement(document, '#top-toolbar');
  this.bottomToolbar_ = queryRequiredElement(document, '#bottom-toolbar');

  this.filenameSpacer_ = queryRequiredElement(this.topToolbar_,
      '.filename-spacer');
  this.filenameEdit_ = util.createChild(this.filenameSpacer_,
                                        'namebox', 'input');

  this.filenameEdit_.setAttribute('type', 'text');
  this.filenameEdit_.addEventListener('blur',
      this.onFilenameEditBlur_.bind(this));

  this.filenameEdit_.addEventListener('focus',
      this.onFilenameFocus_.bind(this));

  this.filenameEdit_.addEventListener('keydown',
      this.onFilenameEditKeydown_.bind(this));

  var buttonSpacer = queryRequiredElement(this.topToolbar_, '.button-spacer');

  this.prompt_ = new ImageEditor.Prompt(this.container_, strf);

  this.errorBanner_ = new ErrorBanner(this.container_);

  var slideModeButton = queryRequiredElement(
      this.topToolbar_, '.button.slide-mode');
  slideModeButton.addEventListener(
      'click', this.onSlideModeButtonClicked_.bind(this));

  var thumbnailModeButton = queryRequiredElement(
      this.topToolbar_, '.button.thumbnail-mode');
  thumbnailModeButton.addEventListener(
      'click', this.onThumbnailModeButtonClicked_.bind(this));

  this.thumbnailMode_ = new ThumbnailMode(
      assertInstanceof(document.querySelector('.thumbnail-view'), HTMLElement),
      this.dataModel_,
      this.selectionModel_,
      this.changeCurrentMode_.bind(this, this.slideMode_));
  this.thumbnailMode_.hide();

  this.slideMode_ = new SlideMode(this.container_,
                                  content,
                                  this.topToolbar_,
                                  this.bottomToolbar_,
                                  this.prompt_,
                                  this.errorBanner_,
                                  this.dataModel_,
                                  this.selectionModel_,
                                  this.metadataModel_,
                                  this.thumbnailModel_,
                                  this.context_,
                                  this.volumeManager_,
                                  this.toggleMode_.bind(this),
                                  str);

  this.slideMode_.addEventListener('image-displayed', function() {
    cr.dispatchSimpleEvent(this, 'image-displayed');
  }.bind(this));

  this.deleteButton_ = queryRequiredElement(this.topToolbar_, '.button.delete');
  this.deleteButton_.addEventListener('click', this.delete_.bind(this));

  this.shareButton_ = queryRequiredElement(this.topToolbar_, '.button.share');
  this.shareButton_.addEventListener(
      'click', this.onShareButtonClick_.bind(this));

  this.dataModel_.addEventListener('splice', this.onSplice_.bind(this));
  this.dataModel_.addEventListener('content', this.onContentChange_.bind(this));

  this.selectionModel_.addEventListener('change', this.onSelection_.bind(this));
  this.slideMode_.addEventListener('useraction', this.onUserAction_.bind(this));

  this.shareDialog_ = new ShareDialog(this.container_);

  // -----------------------------------------------------------------
  // Initialize listeners.

  this.keyDownBound_ = this.onKeyDown_.bind(this);
  this.document_.body.addEventListener('keydown', this.keyDownBound_);

  this.inactivityWatcher_ = new MouseInactivityWatcher(
      this.container_, Gallery.FADE_TIMEOUT, this.hasActiveTool.bind(this));

  // TODO(hirono): Add observer to handle thumbnail update.
  this.volumeManager_.addEventListener(
      'externally-unmounted', this.onExternallyUnmountedBound_);
  // The 'pagehide' event is called when the app window is closed.
  window.addEventListener('pagehide', this.onPageHide_.bind(this));
}

/**
 * Gallery extends cr.EventTarget.
 */
Gallery.prototype.__proto__ = cr.EventTarget.prototype;

/**
 * Tools fade-out timeout in milliseconds.
 * @const
 * @type {number}
 */
Gallery.FADE_TIMEOUT = 2000;

/**
 * First time tools fade-out timeout in milliseconds.
 * @const
 * @type {number}
 */
Gallery.FIRST_FADE_TIMEOUT = 1000;

/**
 * Time until mosaic is initialized in the background. Used to make gallery
 * in the slide mode load faster. In milliseconds.
 * @const
 * @type {number}
 */
Gallery.MOSAIC_BACKGROUND_INIT_DELAY = 1000;

/**
 * Types of metadata Gallery uses (to query the metadata cache).
 * @const
 * @type {!Array<string>}
 */
Gallery.PREFETCH_PROPERTY_NAMES =
    ['imageWidth', 'imageHeight', 'imageRotation', 'size', 'present'];

/**
 * Closes gallery when a volume containing the selected item is unmounted.
 * @param {!Event} event The unmount event.
 * @private
 */
Gallery.prototype.onExternallyUnmounted_ = function(event) {
  if (!this.selectedEntry_)
    return;

  if (this.volumeManager_.getVolumeInfo(this.selectedEntry_) ===
      event.volumeInfo) {
    window.close();
  }
};

/**
 * Unloads the Gallery.
 * @private
 */
Gallery.prototype.onPageHide_ = function() {
  this.volumeManager_.removeEventListener(
      'externally-unmounted', this.onExternallyUnmountedBound_);
  this.volumeManager_.dispose();
};

/**
 * Loads the content.
 *
 * @param {!Array<!Entry>} selectedEntries Array of selected entries.
 */
Gallery.prototype.load = function(selectedEntries) {
  GalleryUtil.createEntrySet(selectedEntries).then(function(allEntries) {
    this.loadInternal_(allEntries, selectedEntries);
  }.bind(this));
};

/**
 * Loads the content.
 *
 * @param {!Array<!FileEntry>} entries Array of entries.
 * @param {!Array<!FileEntry>} selectedEntries Array of selected entries.
 * @private
 */
Gallery.prototype.loadInternal_ = function(entries, selectedEntries) {
  // Add the entries to data model.
  var items = [];
  for (var i = 0; i < entries.length; i++) {
    var locationInfo = this.volumeManager_.getLocationInfo(entries[i]);
    if (!locationInfo)  // Skip the item, since gone.
      return;
    items.push(new Gallery.Item(
        entries[i],
        locationInfo,
        null,
        null,
        true));
  }
  this.dataModel_.splice(0, this.dataModel_.length);
  this.updateThumbnails_();  // Remove the caches.

  GalleryDataModel.prototype.splice.apply(
      this.dataModel_, [0, 0].concat(items));

  // Apply the selection.
  var selectedSet = {};
  for (var i = 0; i < selectedEntries.length; i++) {
    selectedSet[selectedEntries[i].toURL()] = true;
  }
  for (var i = 0; i < items.length; i++) {
    if (!selectedSet[items[i].getEntry().toURL()])
      continue;
    this.selectionModel_.setIndexSelected(i, true);
  }
  this.onSelection_();

  // Obtains max chank size.
  var maxChunkSize = 20;
  var volumeInfo = this.volumeManager_.getVolumeInfo(entries[0]);
  if (volumeInfo) {
    if (GalleryUtil.isOnMTPVolume(entries[0], this.volumeManager_))
      maxChunkSize = 1;

    if (volumeInfo.isReadOnly ||
        GalleryUtil.isOnMTPVolume(entries[0], this.volumeManager_)) {
      this.context_.readonlyDirName = volumeInfo.label;
    }
  }

  // If items are empty, stop initialization.
  if (items.length === 0) {
    this.dataModel_.splice(0, this.dataModel_.length);
    return;
  }

  // Load entries.
  // Use the self variable capture-by-closure because it is faster than bind.
  var self = this;
  var thumbnailModel = new ThumbnailModel(this.metadataModel_);
  var loadChunk = function(firstChunk) {
    // Extract chunk.
    var chunk = items.splice(0, maxChunkSize);
    if (!chunk.length)
      return;
    var entries = chunk.map(function(chunkItem) {
      return chunkItem.getEntry();
    });
    var metadataPromise = self.metadataModel_.get(
        entries, Gallery.PREFETCH_PROPERTY_NAMES);
    var thumbnailPromise = thumbnailModel.get(entries);
    return Promise.all([metadataPromise, thumbnailPromise]).then(
        function(metadataLists) {
      // Add items to the model.
      chunk.forEach(function(chunkItem, index) {
        chunkItem.setMetadataItem(metadataLists[0][index]);
        chunkItem.setThumbnailMetadataItem(metadataLists[1][index]);

        var event = new Event('content');
        event.item = chunkItem;
        event.oldEntry = chunkItem.getEntry();
        event.thumbnailChanged = true;
        self.dataModel_.dispatchEvent(event);
      });

      // Init modes after the first chunk is loaded.
      if (firstChunk && !self.initialized_) {
        // Determine the initial mode.
        var shouldShowThumbnail = selectedEntries.length > 1 ||
            (self.context_.pageState &&
             self.context_.pageState.gallery === 'thumbnail');
        self.setCurrentMode_(
            shouldShowThumbnail ? self.thumbnailMode_ : self.slideMode_);

        // Do the initialization for each mode.
        if (shouldShowThumbnail) {
          self.inactivityWatcher_.check();  // Show the toolbar.
          self.thumbnailMode_.show();
          cr.dispatchSimpleEvent(self, 'loaded');
        } else {
          self.slideMode_.enter(
              null,
              function() {
                // Flash the toolbar briefly to show it is there.
                self.inactivityWatcher_.kick(Gallery.FIRST_FADE_TIMEOUT);
              },
              function() {
                cr.dispatchSimpleEvent(self, 'loaded');
              });
        }
        self.initialized_ = true;
      }

      // Continue to load chunks.
      return loadChunk(/* firstChunk */ false);
    });
  };
  loadChunk(/* firstChunk */ true).catch(function(error) {
    console.error(error.stack || error);
  });
};

/**
 * @return {boolean} True if some tool is currently active.
 */
Gallery.prototype.hasActiveTool = function() {
  return (this.currentMode_ && this.currentMode_.hasActiveTool()) ||
      this.isRenaming_();
};

/**
* External user action event handler.
* @private
*/
Gallery.prototype.onUserAction_ = function() {
  // Show the toolbar and hide it after the default timeout.
  this.inactivityWatcher_.kick();
};

/**
 * Sets the current mode, update the UI.
 * @param {!(SlideMode|ThumbnailMode)} mode Current mode.
 * @private
 *
 * TODO(yawano): Since this method is confusing with changeCurrentMode_. Rename
 *     or remove this method.
 */
Gallery.prototype.setCurrentMode_ = function(mode) {
  if (mode !== this.slideMode_ && mode !== this.thumbnailMode_)
    console.error('Invalid Gallery mode');

  this.currentMode_ = mode;
  this.container_.setAttribute('mode', this.currentMode_.getName());
  this.updateSelectionAndState_();
};

/**
 * Handles click event of SlideModeButton.
 * @param {!Event} event An event.
 * @private
 */
Gallery.prototype.onSlideModeButtonClicked_ = function(event) {
  // If it's in editing, leave edit mode.
  if (this.slideMode_.isEditing())
    this.slideMode_.toggleEditor();

  this.changeCurrentMode_(this.slideMode_, event);
};

/**
 * Handles click event of ThumbnailModeButton.
 * @param {!Event} event An event.
 * @private
 */
Gallery.prototype.onThumbnailModeButtonClicked_ = function(event) {
  this.changeCurrentMode_(this.thumbnailMode_, event);
};

/**
 * Change current mode.
 * @param {!(SlideMode|ThumbnailMode)} mode Target mode.
 * @param {Event=} opt_event Event that caused this call.
 * @private
 */
Gallery.prototype.changeCurrentMode_ = function(mode, opt_event) {
  return new Promise(function(fulfill, reject) {
    // Do not re-enter while changing the mode.
    if (this.currentMode_ === mode || this.changingMode_) {
      fulfill();
      return;
    }

    if (opt_event)
      this.onUserAction_();

    this.changingMode_ = true;

    var onModeChanged = function() {
      this.changingMode_ = false;
      fulfill();
    }.bind(this);

    var thumbnailIndex = Math.max(0, this.selectionModel_.selectedIndex);
    var thumbnailRect = ImageRect.createFromBounds(
        this.thumbnailMode_.getThumbnailRect(thumbnailIndex));

    if (mode === this.thumbnailMode_) {
      this.setCurrentMode_(this.thumbnailMode_);
      this.slideMode_.leave(
          thumbnailRect,
          function() {
            // Show thumbnail mode and perform animation.
            this.thumbnailMode_.show();
            var fromRect = this.slideMode_.getSelectedImageRect();
            if (fromRect) {
              this.thumbnailMode_.performEnterAnimation(
                  thumbnailIndex, fromRect);
            }

            onModeChanged();
          }.bind(this));
      this.bottomToolbar_.hidden = true;
    } else {
      // TODO(yawano): Make animation smooth. With this implementation,
      //     animation starts after the image is fully loaded.
      this.setCurrentMode_(this.slideMode_);
      this.slideMode_.enter(
          thumbnailRect,
          function() {
            // Animate to zoomed position.
            this.thumbnailMode_.hide();
          }.bind(this),
          onModeChanged);
      this.bottomToolbar_.hidden = false;
    }
  }.bind(this));
};

/**
 * Mode toggle event handler.
 * @param {function()=} opt_callback Callback.
 * @param {Event=} opt_event Event that caused this call.
 * @private
 */
Gallery.prototype.toggleMode_ = function(opt_callback, opt_event) {
  var targetMode = this.currentMode_ === this.slideMode_ ?
      this.thumbnailMode_ : this.slideMode_;

  this.changeCurrentMode_(targetMode, opt_event).then(function() {
    if (opt_callback)
      opt_callback();
  });
};

/**
 * Deletes the selected items.
 * @private
 */
Gallery.prototype.delete_ = function() {
  this.onUserAction_();

  // Clone the sorted selected indexes array.
  var indexesToRemove = this.selectionModel_.selectedIndexes.slice();
  if (!indexesToRemove.length)
    return;

  /* TODO(dgozman): Implement Undo delete, Remove the confirmation dialog. */

  var itemsToRemove = this.getSelectedItems();
  var plural = itemsToRemove.length > 1;
  var param = plural ? itemsToRemove.length : itemsToRemove[0].getFileName();

  function deleteNext() {
    if (!itemsToRemove.length)
      return;  // All deleted.

    var entry = itemsToRemove.pop().getEntry();
    entry.remove(deleteNext, function() {
      console.error('Error deleting: ' + entry.name);
      deleteNext();
    });
  }

  // Prevent the Gallery from handling Esc and Enter.
  this.document_.body.removeEventListener('keydown', this.keyDownBound_);
  var restoreListener = function() {
    this.document_.body.addEventListener('keydown', this.keyDownBound_);
  }.bind(this);


  var confirm = new cr.ui.dialogs.ConfirmDialog(this.container_);
  confirm.setOkLabel(str('DELETE_BUTTON_LABEL'));
  confirm.show(strf(plural ?
      'GALLERY_CONFIRM_DELETE_SOME' : 'GALLERY_CONFIRM_DELETE_ONE', param),
      function() {
        restoreListener();
        this.selectionModel_.unselectAll();
        this.selectionModel_.leadIndex = -1;
        // Remove items from the data model, starting from the highest index.
        while (indexesToRemove.length)
          this.dataModel_.splice(indexesToRemove.pop(), 1);
        // Delete actual files.
        deleteNext();
      }.bind(this),
      function() {
        // Restore the listener after a timeout so that ESC is processed.
        setTimeout(restoreListener, 0);
      },
      null);
};

/**
 * @return {!Array<Gallery.Item>} Current selection.
 */
Gallery.prototype.getSelectedItems = function() {
  return this.selectionModel_.selectedIndexes.map(
      this.dataModel_.item.bind(this.dataModel_));
};

/**
 * @return {!Array<Entry>} Array of currently selected entries.
 */
Gallery.prototype.getSelectedEntries = function() {
  return this.selectionModel_.selectedIndexes.map(function(index) {
    return this.dataModel_.item(index).getEntry();
  }.bind(this));
};

/**
 * @return {?Gallery.Item} Current single selection.
 */
Gallery.prototype.getSingleSelectedItem = function() {
  var items = this.getSelectedItems();
  if (items.length > 1) {
    console.error('Unexpected multiple selection');
    return null;
  }
  return items[0];
};

/**
  * Selection change event handler.
  * @private
  */
Gallery.prototype.onSelection_ = function() {
  this.updateSelectionAndState_();
};

/**
  * Data model splice event handler.
  * @private
  */
Gallery.prototype.onSplice_ = function() {
  this.selectionModel_.adjustLength(this.dataModel_.length);
  this.selectionModel_.selectedIndexes =
      this.selectionModel_.selectedIndexes.filter(function(index) {
    return 0 <= index && index < this.dataModel_.length;
  }.bind(this));
};

/**
 * Content change event handler.
 * @param {!Event} event Event.
 * @private
*/
Gallery.prototype.onContentChange_ = function(event) {
  this.updateSelectionAndState_();
};

/**
 * Keydown handler.
 *
 * @param {!Event} event Event.
 * @private
 */
Gallery.prototype.onKeyDown_ = function(event) {
  var keyString = util.getKeyModifiers(event) + event.keyIdentifier;

  // Handle debug shortcut keys.
  switch (keyString) {
    case 'Ctrl-Shift-U+0049': // Ctrl+Shift+I
      chrome.fileManagerPrivate.openInspector('normal');
      break;
    case 'Ctrl-Shift-U+004A': // Ctrl+Shift+J
      chrome.fileManagerPrivate.openInspector('console');
      break;
    case 'Ctrl-Shift-U+0043': // Ctrl+Shift+C
      chrome.fileManagerPrivate.openInspector('element');
      break;
    case 'Ctrl-Shift-U+0042': // Ctrl+Shift+B
      chrome.fileManagerPrivate.openInspector('background');
      break;
  }

  // Do not capture keys when share dialog is shown.
  if (this.shareDialog_.isShowing())
    return;

  // Handle mode specific shortcut keys.
  if (this.currentMode_.onKeyDown(event))
    return;

  // Handle application wide shortcut keys.
  switch (keyString) {
    case 'U+0008': // Backspace.
      // The default handler would call history.back and close the Gallery.
      event.preventDefault();
      break;

    case 'U+004D':  // 'm' switches between Slide and Mosaic mode.
      this.toggleMode_(undefined, event);
      break;

    case 'U+0056':  // 'v'
    case 'MediaPlayPause':
      this.slideMode_.startSlideshow(SlideMode.SLIDESHOW_INTERVAL_FIRST, event);
      break;

    case 'U+007F':  // Delete
    case 'Shift-U+0033':  // Shift+'3' (Delete key might be missing).
    case 'U+0044':  // 'd'
      this.delete_();
      break;

    case 'U+001B':  // Escape
      window.close();
      break;
  }
};

// Name box and rename support.

/**
 * Updates the UI related to the selected item and the persistent state.
 *
 * @private
 */
Gallery.prototype.updateSelectionAndState_ = function() {
  var numSelectedItems = this.selectionModel_.selectedIndexes.length;
  var selectedEntryURL = null;

  // If it's selecting something, update the variable values.
  if (numSelectedItems) {
    // Delete button is available when all images are NOT readOnly.
    this.deleteButton_.disabled = !this.selectionModel_.selectedIndexes
        .every(function(i) {
          return !this.dataModel_.item(i).getLocationInfo().isReadOnly;
        }, this);

    // Obtains selected item.
    var selectedItem =
        this.dataModel_.item(this.selectionModel_.selectedIndex);
    this.selectedEntry_ = selectedItem.getEntry();
    selectedEntryURL = this.selectedEntry_.toURL();

    // Update cache.
    selectedItem.touch();
    this.dataModel_.evictCache();

    // Update the title and the display name.
    if (numSelectedItems === 1) {
      document.title = this.selectedEntry_.name;
      this.filenameEdit_.disabled = selectedItem.getLocationInfo().isReadOnly;
      this.filenameEdit_.value =
          ImageUtil.getDisplayNameFromName(this.selectedEntry_.name);
      this.shareButton_.hidden = !selectedItem.getLocationInfo().isDriveBased;
    } else {
      if (this.context_.curDirEntry) {
        // If the Gallery was opened on search results the search query will not
        // be recorded in the app state and the relaunch will just open the
        // gallery in the curDirEntry directory.
        document.title = this.context_.curDirEntry.name;
      } else {
        document.title = '';
      }
      this.filenameEdit_.disabled = true;
      this.filenameEdit_.value =
          strf('GALLERY_ITEMS_SELECTED', numSelectedItems);
      this.shareButton_.hidden = true;
    }
  } else {
    document.title = '';
    this.filenameEdit_.disabled = true;
    this.deleteButton_.disabled = true;
    this.filenameEdit_.value = '';
    this.shareButton_.hidden = true;
  }

  util.updateAppState(
      null,  // Keep the current directory.
      selectedEntryURL,  // Update the selection.
      {
        gallery: (this.currentMode_ === this.thumbnailMode_ ?
                  'thumbnail' : 'slide')
      });
};

/**
 * Click event handler on filename edit box
 * @private
 */
Gallery.prototype.onFilenameFocus_ = function() {
  ImageUtil.setAttribute(this.filenameSpacer_, 'renaming', true);
  this.filenameEdit_.originalValue = this.filenameEdit_.value;
  setTimeout(this.filenameEdit_.select.bind(this.filenameEdit_), 0);
  this.onUserAction_();
};

/**
 * Blur event handler on filename edit box.
 *
 * @param {!Event} event Blur event.
 * @private
 */
Gallery.prototype.onFilenameEditBlur_ = function(event) {
  var item = this.getSingleSelectedItem();
  if (item) {
    var oldEntry = item.getEntry();

    item.rename(this.filenameEdit_.value).then(function() {
      var event = new Event('content');
      event.item = item;
      event.oldEntry = oldEntry;
      event.thumbnailChanged = false;
      this.dataModel_.dispatchEvent(event);
    }.bind(this), function(error) {
      if (error === 'NOT_CHANGED')
        return Promise.resolve();
      this.filenameEdit_.value =
          ImageUtil.getDisplayNameFromName(item.getEntry().name);
      this.filenameEdit_.focus();
      if (typeof error === 'string')
        this.prompt_.showStringAt('center', error, 5000);
      else
        return Promise.reject(error);
    }.bind(this)).catch(function(error) {
      console.error(error.stack || error);
    });
  }

  ImageUtil.setAttribute(this.filenameSpacer_, 'renaming', false);
  this.onUserAction_();
};

/**
 * Keydown event handler on filename edit box
 * @param {!Event} event A keyboard event.
 * @private
 */
Gallery.prototype.onFilenameEditKeydown_ = function(event) {
  event = assertInstanceof(event, KeyboardEvent);
  switch (event.keyCode) {
    case 27:  // Escape
      this.filenameEdit_.value = this.filenameEdit_.originalValue;
      this.filenameEdit_.blur();
      break;

    case 13:  // Enter
      this.filenameEdit_.blur();
      break;
  }
  event.stopPropagation();
};

/**
 * @return {boolean} True if file renaming is currently in progress.
 * @private
 */
Gallery.prototype.isRenaming_ = function() {
  return this.filenameSpacer_.hasAttribute('renaming');
};

/**
 * Content area click handler.
 * @private
 */
Gallery.prototype.onContentClick_ = function() {
  this.filenameEdit_.blur();
};

/**
 * Share button handler.
 * @private
 */
Gallery.prototype.onShareButtonClick_ = function() {
  var item = this.getSingleSelectedItem();
  if (!item)
    return;
  this.shareDialog_.showEntry(item.getEntry(), function() {});
};

/**
 * Updates thumbnails.
 * @private
 */
Gallery.prototype.updateThumbnails_ = function() {
  if (this.currentMode_ === this.slideMode_)
    this.slideMode_.updateThumbnails();
};

/**
 * Singleton gallery.
 * @type {Gallery}
 */
var gallery = null;

/**
 * (Re-)loads entries.
 */
function reload() {
  initializePromise.then(function() {
    util.URLsToEntries(window.appState.urls, function(entries) {
      gallery.load(entries);
    });
  });
}

/**
 * Promise to initialize the load time data.
 * @type {!Promise}
 */
var loadTimeDataPromise = new Promise(function(fulfill, reject) {
  chrome.fileManagerPrivate.getStrings(function(strings) {
    window.loadTimeData.data = strings;
    i18nTemplate.process(document, loadTimeData);
    fulfill(true);
  });
});

/**
 * Promise to initialize volume manager.
 * @type {!Promise}
 */
var volumeManagerPromise = new Promise(function(fulfill, reject) {
  var volumeManager = new VolumeManagerWrapper(
      VolumeManagerWrapper.NonNativeVolumeStatus.ENABLED);
  volumeManager.ensureInitialized(fulfill.bind(null, volumeManager));
});

/**
 * Promise to initialize both the volume manager and the load time data.
 * @type {!Promise}
 */
var initializePromise =
    Promise.all([loadTimeDataPromise, volumeManagerPromise]).
    then(function(args) {
      var volumeManager = args[1];
      gallery = new Gallery(volumeManager);
    });

// Loads entries.
initializePromise.then(reload);
