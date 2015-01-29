// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer = importer || {};

/**
 * A persistent data store for Cloud Import history information.
 *
 * @interface
 */
importer.ImportHistory = function() {};

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise.<boolean>} Resolves with true if the FileEntry
 *     was previously copied to the device.
 */
importer.ImportHistory.prototype.wasCopied;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise.<boolean>} Resolves with true if the FileEntry
 *     was previously imported to the specified destination.
 */
importer.ImportHistory.prototype.wasImported;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @param {string} destinationUrl
 */
importer.ImportHistory.prototype.markCopied;

/**
 * List urls of all files that are marked as copied, but not marked as synced.
 * @param {!importer.Destination} destination
 * @return {!Promise.<!Array.<string>>}
 */
importer.ImportHistory.prototype.listUnimportedUrls;

/**
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise.<?>} Resolves when the operation is completed.
 */
importer.ImportHistory.prototype.markImported;

/**
 * @param {string} destinationUrl
 * @return {!Promise.<?>} Resolves when the operation is completed.
 */
importer.ImportHistory.prototype.markImportedByUrl;

/**
 * Adds an observer, which will be notified when cloud import history changes.
 *
 * @param {!importer.ImportHistory.Observer} observer
 */
importer.ImportHistory.prototype.addObserver;

/**
 * Remove a previously registered observer.
 *
 * @param {!importer.ImportHistory.Observer} observer
 */
importer.ImportHistory.prototype.removeObserver;

/** @enum {string} */
importer.ImportHistory.State = {
  'COPIED': 'copied',
  'IMPORTED': 'imported'
};

/**
 * @typedef {{
 *   state: !importer.ImportHistory.State,
 *   entry: !FileEntry
 * }}
 */
importer.ImportHistory.ChangedEvent;

/** @typedef {function(!importer.ImportHistory.ChangedEvent)} */
importer.ImportHistory.Observer;

/**
 * An dummy {@code ImportHistory} implementation. This class can conveniently
 * be used when cloud import is disabled.
 * @param {boolean} answer The value to answer all {@code wasImported}
 *     queries with.
 *
 * @constructor
 * @implements {importer.HistoryLoader}
 * @implements {importer.ImportHistory}
 */
importer.DummyImportHistory = function(answer) {
  /** @private {boolean} */
  this.answer_ = answer;
};

/** @override */
importer.DummyImportHistory.prototype.getHistory = function() {
  return Promise.resolve(this);
};

/** @override */
importer.DummyImportHistory.prototype.wasCopied =
    function(entry, destination) {
  return Promise.resolve(this.answer_);
};

/** @override */
importer.DummyImportHistory.prototype.wasImported =
    function(entry, destination) {
  return Promise.resolve(this.answer_);
};

/** @override */
importer.DummyImportHistory.prototype.markCopied =
    function(entry, destination, destinationUrl) {
  return Promise.resolve();
};

/** @override */
importer.DummyImportHistory.prototype.listUnimportedUrls =
    function(destination) {
  return Promise.resolve([]);
};

/** @override */
importer.DummyImportHistory.prototype.markImported =
    function(entry, destination) {
  return Promise.resolve();
};

/** @override */
importer.DummyImportHistory.prototype.markImportedByUrl =
    function(destinationUrl) {
  return Promise.resolve();
};

/** @override */
importer.DummyImportHistory.prototype.addHistoryLoadedListener =
    function(listener) {};

/** @override */
importer.DummyImportHistory.prototype.addObserver = function(observer) {};

/** @override */
importer.DummyImportHistory.prototype.removeObserver = function(observer) {};

/**
 * @private @enum {number}
 */
importer.RecordType_ = {
  COPY: 0,
  IMPORT: 1
};

/**
 * @typedef {{
 *   sourceUrl: string,
 *   destinationUrl: string
 * }}
 */
importer.Urls;

/**
 * An {@code ImportHistory} implementation that reads from and
 * writes to a storage object.
 *
 * @constructor
 * @implements {importer.ImportHistory}
 * @struct
 *
 * @param {function(!FileEntry): !Promise.<string>} hashGenerator
 * @param {!importer.RecordStorage} storage
 */
importer.PersistentImportHistory = function(hashGenerator, storage) {
  /** @private {function(!FileEntry): !Promise.<string>} */
  this.createKey_ = hashGenerator;

  /** @private {!importer.RecordStorage} */
  this.storage_ = storage;

  /**
   * An in-memory representation of local copy history.
   * The first value is the "key" (as generated internally
   * from a file entry).
   * @private {!Object.<string, !Object.<!importer.Destination, importer.Urls>>}
   */
  this.copiedEntries_ = {};

  /**
   * An in-memory index from destination URL to key.
   *
   * @private {!Object.<string, string>}
   */
  this.copyKeyIndex_ = {};

  /**
   * An in-memory representation of import history.
   * The first value is the "key" (as generated internally
   * from a file entry).
   * @private {!Object.<string, !Array.<importer.Destination>>}
   */
  this.importedEntries_ = {};

  /** @private {!Array.<!importer.ImportHistory.Observer>} */
  this.observers_ = [];

  /** @private {!Promise.<!importer.PersistentImportHistory>} */
  this.whenReady_ = this.refresh_();
};

/**
 * Loads history from storage and merges in any previously existing entries
 * that are not present in the newly loaded data. Should be called
 * when the storage is externally changed.
 *
 * @return {!Promise.<!importer.PersistentImportHistory>} Resolves when history
 *     has been refreshed.
 * @private
 */
importer.PersistentImportHistory.prototype.refresh_ = function() {
  var oldCopiedEntries = this.copiedEntries_;
  // NOTE: importDetailsIndex is built in relation to the copiedEntries_
  // data, so we don't need to preserve the information in that field.
  var oldImportedEntries = this.importedEntries_;

  this.copiedEntries_ = {};
  this.importedEntries_ = {};

  return this.storage_.readAll()
      .then(this.updateHistoryRecords_.bind(this))
      .then(this.mergeCopiedEntries_.bind(this, oldCopiedEntries))
      .then(this.mergeImportedEntries_.bind(this, oldImportedEntries))
      .then(
          /**
           * @return {!importer.PersistentImportHistory}
           * @this {importer.PersistentImportHistory}
           */
          function() {
            return this;
          }.bind(this))
      .catch(importer.getLogger().catcher('import-history-refresh'));
};

/**
 * Adds all copied entries into existing entries.
 *
 * @param {!Object.<string, !Object.<!importer.Destination, importer.Urls>>}
 *     entries
 * @return {!Promise.<?>} Resolves once all updates are completed.
 * @private
 */
importer.PersistentImportHistory.prototype.mergeCopiedEntries_ =
    function(entries) {
  var promises = [];
  for (var key in entries) {
    for (var destination in entries[key]) {
      // This method is only called when data is reloaded from disk.
      // In such a situation we defend against loss of in-memory data
      // by copying it out of the way, reloading data from disk, then
      // merging that data back into the freshly loaded data. For that
      // reason we will write newly created entries back to disk.

      var urls = entries[key][destination];
      if (this.updateInMemoryCopyRecord_(
          key,
          destination,
          urls.sourceUrl,
          urls.destinationUrl)) {
        promises.push(this.storage_.write([
            importer.RecordType_.COPY,
            key,
            destination,
            urls.sourceUrl,
            urls.destinationUrl]));
      }
    }
  }
  return Promise.all(promises);
};

/**
 * Adds all imported entries into existing entries.
 *
 * @param {!Object.<string, !Array.<!importer.Destination>>} entries
 * @return {!Promise.<?>} Resolves once all updates are completed.
 * @private
 */
importer.PersistentImportHistory.prototype.mergeImportedEntries_ =
    function(entries) {
  var promises = [];
  for (var key in entries) {
    entries[key].forEach(
        /**
         * @param {!importer.Destination} destination
         * @this {importer.PersistentImportHistory}
         */
        function(destination) {
          if (this.updateInMemoryImportRecord_(key, destination)) {
            promises.push(this.storage_.write(
                [importer.RecordType_.IMPORT, key, destination]));
          }
        }.bind(this));
  }
  return Promise.all(promises);
};

/**
 * Reloads history from disk. Should be called when the file
 * is changed by an external source.
 *
 * @return {!Promise.<!importer.PersistentImportHistory>} Resolves when
 *     history has been refreshed.
 */
importer.PersistentImportHistory.prototype.refresh = function() {
  this.whenReady_ = this.refresh_();
  return this.whenReady_;
};

/**
 * @return {!Promise.<!importer.ImportHistory>}
 */
importer.PersistentImportHistory.prototype.whenReady = function() {
  return /** @type {!Promise.<!importer.ImportHistory>} */ (this.whenReady_);
};

/**
 * Adds a history entry to the in-memory history model.
 * @param {!Array.<!Array.<*>>} records
 * @private
 */
importer.PersistentImportHistory.prototype.updateHistoryRecords_ =
    function(records) {
  records.forEach(this.updateInMemoryRecord_.bind(this));
};

/**
 * @param {!Array.<*>} record
 * @this {importer.PersistentImportHistory}
 */
importer.PersistentImportHistory.prototype.updateInMemoryRecord_ =
    function(record) {
  switch (record[0]) {
    case importer.RecordType_.COPY:
      this.updateInMemoryCopyRecord_(
          /** @type {string} */ (
              record[1]),  // key
          /** @type {!importer.Destination} */ (
              record[2]),
          /** @type {string } */ (
              record[3]),  // sourceUrl
          /** @type {string } */ (
              record[4])); // destinationUrl
      return;
    case importer.RecordType_.IMPORT:
      this.updateInMemoryImportRecord_(
          /** @type {string } */ (
              record[1]),  // key
          /** @type {!importer.Destination} */ (
              record[2]));
      return;
    default:
      assertNotReached('Ignoring record with unrecognized type: ' + record[0]);
  }
};

/**
 * Adds an import record to the in-memory history model.
 *
 * @param {string} key
 * @param {!importer.Destination} destination
 *
 * @return {boolean} True if a record was created.
 * @private
 */
importer.PersistentImportHistory.prototype.updateInMemoryImportRecord_ =
    function(key, destination) {
  if (!this.importedEntries_.hasOwnProperty(key)) {
    this.importedEntries_[key] = [destination];
    return true;
  } else if (this.importedEntries_[key].indexOf(destination) === -1) {
    this.importedEntries_[key].push(destination);
    return true;
  }
  return false;
};

/**
 * Adds a copy record to the in-memory history model.
 *
 * @param {string} key
 * @param {!importer.Destination} destination
 * @param {string} sourceUrl
 * @param {string} destinationUrl
 *
 * @return {boolean} True if a record was created.
 * @private
 */
importer.PersistentImportHistory.prototype.updateInMemoryCopyRecord_ =
    function(key, destination, sourceUrl, destinationUrl) {
  this.copyKeyIndex_[destinationUrl] = key;
  if (!this.copiedEntries_.hasOwnProperty(key)) {
    this.copiedEntries_[key] = {};
  }
  if (!this.copiedEntries_[key].hasOwnProperty(destination)) {
    this.copiedEntries_[key][destination] = {
      sourceUrl: sourceUrl,
      destinationUrl: destinationUrl
    };
    return true;
  }
  return false;
};

/** @override */
importer.PersistentImportHistory.prototype.wasCopied =
    function(entry, destination) {
  return this.whenReady_
      .then(this.createKey_.bind(this, entry))
      .then(
          /**
           * @param {string} key
           * @return {boolean}
           * @this {importer.PersistentImportHistory}
           */
          function(key) {
            return key in this.copiedEntries_ &&
                destination in this.copiedEntries_[key];
          }.bind(this));
};

/** @override */
importer.PersistentImportHistory.prototype.wasImported =
    function(entry, destination) {
  return this.whenReady_
      .then(this.createKey_.bind(this, entry))
      .then(
          /**
           * @param {string} key
           * @return {boolean}
           * @this {importer.PersistentImportHistory}
           */
          function(key) {
            return this.getDestinations_(key).indexOf(destination) >= 0;
          }.bind(this));
};

/** @override */
importer.PersistentImportHistory.prototype.markCopied =
    function(entry, destination, destinationUrl) {
  return this.whenReady_
      .then(this.createKey_.bind(this, entry))
      .then(
          /**
           * @param {string} key
           * @return {!Promise.<?>}
           * @this {importer.ImportHistory}
           */
          function(key) {
            return this.storeRecord_([
                importer.RecordType_.COPY,
                key,
                destination,
                entry.toURL(),
                destinationUrl]);
          }.bind(this))
      .then(this.notifyObservers_.bind(
          this,
          importer.ImportHistory.State.COPIED,
          entry,
          destination,
          destinationUrl));
};

/** @override */
importer.PersistentImportHistory.prototype.listUnimportedUrls =
    function(destination) {
  return this.whenReady_.then(
      function() {
        // TODO(smckay): Merge copy and sync records for simpler
        // unimported file discovery.
        var unimported = [];
        for (var key in this.copiedEntries_) {
          var imported = this.importedEntries_[key];
          for (var destination in this.copiedEntries_[key]) {
            if (!imported || imported.indexOf(destination) === -1) {
              unimported.push(
                  this.copiedEntries_[key][destination].destinationUrl);
            }
          }
        }
        return unimported;
      }.bind(this));
};

/** @override */
importer.PersistentImportHistory.prototype.markImported =
    function(entry, destination) {
  return this.whenReady_
      .then(this.createKey_.bind(this, entry))
      .then(
          /**
           * @param {string} key
           * @return {!Promise.<?>}
           * @this {importer.ImportHistory}
           */
          function(key) {
            return this.storeRecord_([
                importer.RecordType_.IMPORT,
                key,
                destination]);
          }.bind(this))
      .then(this.notifyObservers_.bind(
          this,
          importer.ImportHistory.State.IMPORTED,
          entry,
          destination));
};

/** @override */
importer.PersistentImportHistory.prototype.markImportedByUrl =
    function(destinationUrl) {
  var key = this.copyKeyIndex_[destinationUrl];
  if (!!key) {
    var copyData = this.copiedEntries_[key];

    // we could build an index of this as well, but it seems
    // unnecessary given the fact that there will almost always
    // be just one destination for a file (assumption).
    for (var destination in copyData) {
      if (copyData[destination].destinationUrl === destinationUrl) {
        return this.storeRecord_([
            importer.RecordType_.IMPORT,
            key,
            destination]).then(
              function() {
                // Here we try to create an Entry for the source URL.
                // This will allow observers to update the UI if the
                // source entry is in view.
                util.urlToEntry(copyData[destination].sourceUrl).then(
                    function(entry) {
                      if (entry.isFile) {
                        this.notifyObservers_(
                            importer.ImportHistory.State.IMPORTED,
                            /** @type {!FileEntry} */ (entry),
                            destination);
                      }
                    }.bind(this));
              }.bind(this)
            );
      }
    }
  }

  return Promise.reject(
      'Unabled to match destination URL to import record > ' + destinationUrl);
};

/** @override */
importer.PersistentImportHistory.prototype.addObserver =
    function(observer) {
  this.observers_.push(observer);
};

/** @override */
importer.PersistentImportHistory.prototype.removeObserver =
    function(observer) {
  var index = this.observers_.indexOf(observer);
  if (index > -1) {
    this.observers_.splice(index, 1);
  } else {
    console.warn('Ignoring request to remove observer that is not registered.');
  }
};

/**
 * @param {!importer.ImportHistory.State} state
 * @param {!FileEntry} entry
 * @param {!importer.Destination} destination
 * @param {string=} opt_destinationUrl
 * @private
 */
importer.PersistentImportHistory.prototype.notifyObservers_ =
    function(state, entry, destination, opt_destinationUrl) {
  this.observers_.forEach(
      /**
       * @param {!importer.ImportHistory.Observer} observer
       * @this {importer.PersistentImportHistory}
       */
      function(observer) {
        observer({
          state: state,
          entry: entry,
          destination: destination,
          destinationUrl: opt_destinationUrl
        });
      }.bind(this));
};

/**
 * @param {!Array.<*>} record
 *
 * @return {!Promise.<?>} Resolves once the write has been completed.
 * @private
 */
importer.PersistentImportHistory.prototype.storeRecord_ = function(record) {
  this.updateInMemoryRecord_(record);
  return this.storage_.write(record);
};

/**
 * @param {string} key
 * @return {!Array.<string>} The list of previously noted
 *     destinations, or an empty array, if none.
 * @private
 */
importer.PersistentImportHistory.prototype.getDestinations_ = function(key) {
  return key in this.importedEntries_ ? this.importedEntries_[key] : [];
};

/**
 * Provider of lazy loaded importer.ImportHistory. This is the main
 * access point for a fully prepared {@code importer.ImportHistory} object.
 *
 * @interface
 */
importer.HistoryLoader = function() {};

/**
 * Instantiates an {@code importer.ImportHistory} object and manages any
 * necessary ongoing maintenance of the object with respect to
 * its external dependencies.
 *
 * @see importer.SynchronizedHistoryLoader for an example.
 *
 * @return {!Promise.<!importer.ImportHistory>} Resolves when history instance
 *     is ready.
 */
importer.HistoryLoader.prototype.getHistory;

/**
 * Adds a listener to be notified when history is fully loaded for the first
 * time. If history is already loaded...will be called immediately.
 *
 * @param {function(!importer.ImportHistory)} listener
 */
importer.HistoryLoader.prototype.addHistoryLoadedListener;

/**
 * Class responsible for lazy loading of {@code importer.ImportHistory},
 * and reloading when the underlying data is updated (via sync).
 *
 * @constructor
 * @implements {importer.HistoryLoader}
 * @struct
 *
 * @param {!importer.SyncFileEntryProvider} fileProvider
 */
importer.SynchronizedHistoryLoader = function(fileProvider) {

  /** @private {!importer.SyncFileEntryProvider} */
  this.fileProvider_ = fileProvider;

  /** @private {boolean} */
  this.needsInitialization_ = true;

  /** @private {!importer.Resolver} */
  this.historyResolver_ = new importer.Resolver();
};

/** @override */
importer.SynchronizedHistoryLoader.prototype.getHistory = function() {
  if (this.needsInitialization_) {
    this.needsInitialization_ = false;
    this.fileProvider_.addSyncListener(
        this.onSyncedDataChanged_.bind(this));

    this.fileProvider_.getSyncFileEntry()
        .then(
            /**
             * @param {!FileEntry} fileEntry
             * @return {!Promise.<!importer.ImportHistory>}
             * @this {importer.SynchronizedHistoryLoader}
             */
            function(fileEntry) {
              var storage = new importer.FileEntryRecordStorage(fileEntry);
              var history = new importer.PersistentImportHistory(
                  importer.createMetadataHashcode,
                  storage);
              new importer.DriveSyncWatcher(history);
              history.refresh().then(
                  /** @this {importer.SynchronizedHistoryLoader} */
                  function() {
                    this.historyResolver_.resolve(history);
                  }.bind(this));
            }.bind(this))
        .catch(importer.getLogger().catcher('history-load-chain'));
  }

  return this.historyResolver_.promise;
};

/** @override */
importer.SynchronizedHistoryLoader.prototype.addHistoryLoadedListener =
    function(listener) {
  this.historyResolver_.promise.then(listener);
};

/**
 * Handles file sync events, by simply reloading history. The presumption
 * is that 99% of the time these events will basically be happening when
 * there is no active import process.
 *
 * @private
 */
importer.SynchronizedHistoryLoader.prototype.onSyncedDataChanged_ =
    function() {
  this.historyResolver_.promise.then(
      /** @param {!importer.ImportHistory} history */
      function(history) {
        history.refresh();
      });
};

/**
 * An simple record storage mechanism.
 *
 * @interface
 */
importer.RecordStorage = function() {};

/**
 * Adds a new record.
 *
 * @param {!Array.<*>} record
 * @return {!Promise.<?>} Resolves when record is added.
 */
importer.RecordStorage.prototype.write;

/**
 * Reads all records.
 *
 * @return {!Promise.<!Array.<!Array.<*>>>}
 */
importer.RecordStorage.prototype.readAll;

/**
 * A {@code RecordStore} that persists data in a {@code FileEntry}.
 *
 * @param {!FileEntry} fileEntry
 *
 * @constructor
 * @implements {importer.RecordStorage}
 * @struct
 */
importer.FileEntryRecordStorage = function(fileEntry) {
  /** @private {!importer.PromisingFileEntry} */
  this.fileEntry_ = new importer.PromisingFileEntry(fileEntry);

  /**
   * Serializes all writes and reads.
   * @private {!Promise.<?>}
   * */
  this.latestOperation_ = Promise.resolve(null);
};

/** @override */
importer.FileEntryRecordStorage.prototype.write = function(record) {
  return this.latestOperation_ = this.latestOperation_
      .then(
          /**
           * @param {?} ignore
           * @this {importer.FileEntryRecordStorage}
           */
          function(ignore) {
            return this.fileEntry_.createWriter();
          }.bind(this))
      .then(this.writeRecord_.bind(this, record))
      .catch(importer.getLogger().catcher('record-writing'));
};

/**
 * Appends a new record to the end of the file.
 *
 * @param {!Object} record
 * @param {!FileWriter} writer
 * @return {!Promise.<?>} Resolves when write is complete.
 * @private
 */
importer.FileEntryRecordStorage.prototype.writeRecord_ =
    function(record, writer) {
  var blob = new Blob(
      [JSON.stringify(record) + ',\n'],
      {type: 'text/plain; charset=UTF-8'});
  return new Promise(
      /**
       * @param {function()} resolve
       * @param {function()} reject
       * @this {importer.FileEntryRecordStorage}
       */
      function(resolve, reject) {
        writer.onwriteend = resolve;
        writer.onerror = reject;

        writer.seek(writer.length);
        writer.write(blob);
      }.bind(this));
};

/** @override */
importer.FileEntryRecordStorage.prototype.readAll = function() {
  return /** @type {!Promise.<!Array.<!Array.<*>>>} */ (this.latestOperation_ =
      this.latestOperation_
          .then(
              /**
               * @param {?} ignore
               * @this {importer.FileEntryRecordStorage}
               */
              function(ignore) {
                return this.fileEntry_.file();
              }.bind(this))
          .then(
              this.readFileAsText_.bind(this),
              /**
               * @return {string}
               * @this {importer.FileEntryRecordStorage}
               */
              function() {
                console.error('Unable to read from history file.');
                return '';
              }.bind(this))
          .then(
              /**
               * @param {string} fileContents
               * @this {importer.FileEntryRecordStorage}
               */
              function(fileContents) {
                return this.parse_(fileContents);
              }.bind(this))
          .catch(importer.getLogger().catcher('record-reading')));
};

/**
 * Reads the entire entry as a single string value.
 *
 * @param {!File} file
 * @return {!Promise.<string>}
 * @private
 */
importer.FileEntryRecordStorage.prototype.readFileAsText_ = function(file) {
  return new Promise(
      /**
       * @param {function()} resolve
       * @param {function()} reject
       * @this {importer.FileEntryRecordStorage}
       */
      function(resolve, reject) {
        var reader = new FileReader();

        reader.onloadend = function() {
          if (reader.error) {
            console.error(reader.error);
            reject();
          } else {
            resolve(reader.result);
          }
        }.bind(this);

        reader.onerror = function(error) {
            console.error(error);
          reject(error);
        }.bind(this);

        reader.readAsText(file);
      }.bind(this));
};

/**
 * Parses the text.
 *
 * @param {string} text
 * @return {!Array.<!Array.<*>>}
 * @private
 */
importer.FileEntryRecordStorage.prototype.parse_ = function(text) {
  if (text.length === 0) {
    return [];
  } else {
    // Dress up the contents of the file like an array,
    // so the JSON object can parse it using JSON.parse.
    // That means we need to both:
    //   1) Strip the trailing ',\n' from the last record
    //   2) Surround the whole string in brackets.
    // NOTE: JSON.parse is WAY faster than parsing this
    // ourselves in javascript.
    var json = '[' + text.substring(0, text.length - 2) + ']';
    return /** @type {!Array.<!Array.<*>>} */ (JSON.parse(json));
  }
};

/**
 * This class makes the "drive" badges appear by way of marking entries as
 * imported in history when a previously imported file is fully synced to drive.
 *
 * @constructor
 * @struct
 *
 * @param {!importer.ImportHistory} history
 */
importer.DriveSyncWatcher = function(history) {
  /** @private {!importer.ImportHistory} */
  this.history_ = history;

  this.history_.addObserver(
      this.onHistoryChanged_.bind(this));

  this.history_.whenReady_
      .then(
          function() {
            this.history_.listUnimportedUrls(importer.Destination.GOOGLE_DRIVE)
                .then(this.updateSyncStatus_.bind(
                    this,
                    importer.Destination.GOOGLE_DRIVE));
          }.bind(this));

  // Listener is only registered once the history object is initialized.
  // No need to register synchonously since we don't want to be
  // woken up to respond to events.
  chrome.fileManagerPrivate.onFileTransfersUpdated.addListener(
      this.onFileTransfersUpdated_.bind(this));
  // TODO(smckay): Listen also for errors on onDriveSyncError.
};

/** @const {number} */
importer.DriveSyncWatcher.UPDATE_DELAY_MS = 3500;

/**
 * @param {!importer.Destination} destination
 * @param {!Array.<string>} unimportedUrls
 * @private
 */
importer.DriveSyncWatcher.prototype.updateSyncStatus_ =
    function(destination, unimportedUrls) {
  // TODO(smckay): Chunk processing of urls...to ensure we're not
  // blocking interactive tasks. For now, we just defer the update
  // for a few seconds.
  setTimeout(
      function() {
        unimportedUrls.forEach(
            function(url) {
              this.checkSyncStatus_(destination, url);
            }.bind(this));
      }.bind(this),
      importer.DriveSyncWatcher.UPDATE_DELAY_MS);
};

/**
 * @param {!FileTransferStatus} status
 * @private
 */
importer.DriveSyncWatcher.prototype.onFileTransfersUpdated_ =
    function(status) {
  if (status.transferState === 'completed') {
    this.history_.markImportedByUrl(status.fileUrl);
  }
};

/**
 * @param {!importer.ImportHistory.ChangedEvent} event
 * @private
 */
importer.DriveSyncWatcher.prototype.onHistoryChanged_ =
    function(event) {
  if (event.state === importer.ImportHistory.State.COPIED) {
    // Check sync status incase the file synced *before* it was able
    // to mark be marked as copied.
    this.checkSyncStatus_(
        event.destination,
        event.destinationUrl,
        event.entry);
  }
};

/**
 * @param {!importer.Destination} destination
 * @param {string} url
 * @param {!FileEntry=} opt_entry Pass this if you have an entry
 *     on hand, else, we'll jump through some extra hoops to
 *     make do without it.
 * @private
 */
importer.DriveSyncWatcher.prototype.checkSyncStatus_ =
    function(destination, url, opt_entry) {
  console.assert(
      destination === importer.Destination.GOOGLE_DRIVE,
      'Unsupported destination: ' + destination);

  this.getSyncStatus_(url)
      .then(
          /**
           * @param {boolean} synced True if file is synced
           * @this {importer.DriveSyncWatcher}
           */
          function(synced) {
            if (synced) {
              if (opt_entry) {
                this.history_.markImported(opt_entry, destination);
              } else {
                this.history_.markImportedByUrl(url);
              }
            }
          }.bind(this));
};

/**
 * @param {string} url
 * @return {!Promise.<boolean>} Resolves with true if the
 *     file has been synced to the named destination.
 * @private
 */
importer.DriveSyncWatcher.prototype.getSyncStatus_ =
    function(url) {
  return new Promise(
      /** @this {importer.DriveSyncWatcher} */
      function(resolve, reject) {
        // TODO(smckay): User Metadata Cache...once it is available
        // in the background.
        chrome.fileManagerPrivate.getEntryProperties(
            [url],
            /**
             * @param {!Array.<Object>} propertiesList
             * @this {importer.DriveSyncWatcher}
             */
            function(propertiesList) {
              console.assert(
                  propertiesList.length === 1,
                  'Got an unexpected number of results.');
              if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
              } else {
                var data = propertiesList[0];
                resolve(!data['isDirty']);
              }
            }.bind(this));
      }.bind(this));
};

/**
 * History loader that provides an ImportHistorty appropriate
 * to user settings (if import history is enabled/disabled).
 *
 * TODO(smckay): Use SynchronizedHistoryLoader directly
 *     once cloud-import feature is enabled by default.
 *
 * @constructor
 * @implements {importer.HistoryLoader}
 * @struct
 */
importer.RuntimeHistoryLoader = function() {
  /** @private {boolean} */
  this.needsInitialization_ = true;

  /** @private {!importer.Resolver.<!importer.ImportHistory>} */
  this.historyResolver_ = new importer.Resolver();
};

/** @override */
importer.RuntimeHistoryLoader.prototype.getHistory = function() {
  if (this.needsInitialization_) {
    this.needsInitialization_ = false;
    importer.importEnabled()
        .then(
            /**
             * @param {boolean} enabled
             * @return {!Promise.<!importer.ImportHistory>}
             * @this {importer.RuntimeHistoryLoader}
             */
            function(enabled) {
              importer.getHistoryFilename().then(
                  function(filename) {
                    var loader = enabled ?
                        new importer.SynchronizedHistoryLoader(
                            new importer.ChromeSyncFileEntryProvider(
                                filename)) :
                        new importer.DummyImportHistory(false);

                    this.historyResolver_.resolve(loader.getHistory());
                  }.bind(this));
            }.bind(this));
  }

  return this.historyResolver_.promise;
};

/** @override */
importer.RuntimeHistoryLoader.prototype.addHistoryLoadedListener =
    function(listener) {
  this.historyResolver_.promise.then(listener);
};
