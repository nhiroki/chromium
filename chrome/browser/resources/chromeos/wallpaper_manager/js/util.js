// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var WallpaperUtil = {
  strings: null,  // Object that contains all the flags
  syncFs: null,   // syncFileSystem handler
  webkitFs: null  // webkitFileSystem handler
};

/**
 * Deletes |wallpaperFileName| and its associated thumbnail from local FS.
 * @param {string} wallpaperFilename Name of the file that will be deleted
 */
WallpaperUtil.deleteWallpaperFromLocalFS = function(wallpaperFilename) {
  WallpaperUtil.requestLocalFS(function(fs) {
    var originalPath = Constants.WallpaperDirNameEnum.ORIGINAL + '/' +
                       wallpaperFilename;
    var thumbnailPath = Constants.WallpaperDirNameEnum.THUMBNAIL + '/' +
                        wallpaperFilename;
    fs.root.getFile(originalPath,
                    {create: false},
                    function(fe) {
                      fe.remove(function() {}, null);
                    },
                    // NotFoundError is expected. After we receive a delete
                    // event from either original wallpaper or wallpaper
                    // thumbnail, we delete both of them in local FS to achieve
                    // a faster synchronization. So each file is expected to be
                    // deleted twice and the second attempt is a noop.
                    function(e) {
                      if (e.name != 'NotFoundError')
                        WallpaperUtil.onFileSystemError(e);
                    });
    fs.root.getFile(thumbnailPath,
                    {create: false},
                    function(fe) {
                      fe.remove(function() {}, null);
                    },
                    function(e) {
                      if (e.name != 'NotFoundError')
                        WallpaperUtil.onFileSystemError(e);
                    });
  });
};

/**
 * Loads a wallpaper from sync file system and saves it and its thumbnail to
 *     local file system.
 * @param {string} wallpaperFileEntry File name of wallpaper image.
 */
WallpaperUtil.storeWallpaperFromSyncFSToLocalFS = function(wallpaperFileEntry) {
  var filenName = wallpaperFileEntry.name;
  var storeDir = Constants.WallpaperDirNameEnum.ORIGINAL;
  if (filenName.indexOf(Constants.CustomWallpaperThumbnailSuffix) != -1)
    storeDir = Constants.WallpaperDirNameEnum.THUMBNAIL;
  filenName = filenName.replace(Constants.CustomWallpaperThumbnailSuffix, '');
  wallpaperFileEntry.file(function(file) {
    var reader = new FileReader();
    reader.onloadend = function() {
      WallpaperUtil.storeWallpaperToLocalFS(filenName, reader.result, storeDir);
    };
    reader.readAsArrayBuffer(file);
  }, WallpaperUtil.onFileSystemError);
};

/**
 * Deletes |wallpaperFileName| and its associated thumbnail from syncFileSystem.
 * @param {string} wallpaperFilename Name of the file that will be deleted.
 */
WallpaperUtil.deleteWallpaperFromSyncFS = function(wallpaperFilename) {
  var thumbnailFilename = wallpaperFilename +
                          Constants.CustomWallpaperThumbnailSuffix;
  var success = function(fs) {
    fs.root.getFile(wallpaperFilename,
                    {create: false},
                    function(fe) {
                      fe.remove(function() {}, null);
                    },
                    WallpaperUtil.onFileSystemError);
    fs.root.getFile(thumbnailFilename,
                    {create: false},
                    function(fe) {
                      fe.remove(function() {}, null);
                    },
                    WallpaperUtil.onFileSystemError);
  };
  WallpaperUtil.requestSyncFS(success);
};

/**
 * Executes callback if sync theme is enabled.
 * @param {function} callback The callback will be executed if sync themes is
 *     enabled.
 */
WallpaperUtil.enabledSyncThemesCallback = function(callback) {
  chrome.wallpaperPrivate.getSyncSetting(function(setting) {
    if (setting.syncThemes)
      callback();
  });
};

/**
 * Request a syncFileSystem handle and run callback on it.
 * @param {function} callback The callback to execute after syncFileSystem
 *     handler is available.
 */
WallpaperUtil.requestSyncFS = function(callback) {
  WallpaperUtil.enabledSyncThemesCallback(function() {
    if (WallpaperUtil.syncFs) {
      callback(WallpaperUtil.syncFs);
    } else {
      chrome.syncFileSystem.requestFileSystem(function(fs) {
        WallpaperUtil.syncFs = fs;
        callback(WallpaperUtil.syncFs);
      });
    }
  });
};

/**
 * Request a Local Fs handle and run callback on it.
 * @param {function} callback The callback to execute after Local handler is
 *     available.
 */
WallpaperUtil.requestLocalFS = function(callback) {
  if (WallpaperUtil.webkitFs) {
    callback(WallpaperUtil.webkitFs);
  } else {
    window.webkitRequestFileSystem(window.PERSISTENT, 1024 * 1024 * 100,
                                   function(fs) {
                                     WallpaperUtil.webkitFs = fs;
                                     callback(fs);
                                   });
  }
};

/**
 * Print error to console.error.
 * @param {Event} e The error will be printed to console.error.
 */
// TODO(ranj): Handle different errors differently.
WallpaperUtil.onFileSystemError = function(e) {
  console.error(e);
};

/**
 * Write jpeg/png file data into file entry.
 * @param {FileEntry} fileEntry The file entry that going to be writen.
 * @param {ArrayBuffer} wallpaperData Data for image file.
 * @param {function=} writeCallback The callback that will be executed after
 *     writing data.
 */
WallpaperUtil.writeFile = function(fileEntry, wallpaperData, writeCallback) {
  fileEntry.createWriter(function(fileWriter) {
    var blob = new Blob([new Int8Array(wallpaperData)]);
    fileWriter.write(blob);
    if (writeCallback)
      writeCallback();
  }, WallpaperUtil.onFileSystemError);
};

/**
 * Write jpeg/png file data into syncFileSystem.
 * @param {string} wallpaperFilename The filename that going to be writen.
 * @param {ArrayBuffer} wallpaperData Data for image file.
 */
WallpaperUtil.storeWallpaperToSyncFS = function(wallpaperFilename,
                                                wallpaperData) {
  var callback = function(fs) {
    fs.root.getFile(wallpaperFilename,
                    {create: false},
                    function() {},  // already exists
                    function(e) {  // not exists, create
                      fs.root.getFile(wallpaperFilename, {create: true},
                                      function(fe) {
                                        WallpaperUtil.writeFile(
                                            fe, wallpaperData);
                                      },
                                      WallpaperUtil.onFileSystemError);
                    });
  };
  WallpaperUtil.requestSyncFS(callback);
};

/**
 * Stores jpeg/png wallpaper into |localDir| in local file system.
 * @param {string} wallpaperFilename File name of wallpaper image.
 * @param {ArrayBuffer} wallpaperData The wallpaper data.
 * @param {string} saveDir The path to store wallpaper in local file system.
 */
WallpaperUtil.storeWallpaperToLocalFS = function(wallpaperFilename,
    wallpaperData, saveDir) {
  if (!wallpaperData) {
    console.error('wallpaperData is null');
    return;
  }
  var getDirSuccess = function(dirEntry) {
    dirEntry.getFile(wallpaperFilename,
                    {create: false},
                    function() {},  // already exists
                    function(e) {   // not exists, create
                    dirEntry.getFile(wallpaperFilename, {create: true},
                                     function(fe) {
                                       WallpaperUtil.writeFile(fe,
                                                               wallpaperData);
                                     },
                                     WallpaperUtil.onFileSystemError);
                    });
  };
  WallpaperUtil.requestLocalFS(function(fs) {
    fs.root.getDirectory(saveDir, {create: true}, getDirSuccess,
                         WallpaperUtil.onFileSystemError);
  });
};

/**
 * Sets wallpaper from synced file system.
 * @param {string} wallpaperFilename File name used to set wallpaper.
 * @param {string} wallpaperLayout Layout used to set wallpaper.
 * @param {function=} onSuccess Callback if set successfully.
 */
WallpaperUtil.setCustomWallpaperFromSyncFS = function(
    wallpaperFilename, wallpaperLayout, onSuccess) {
  var setWallpaperFromSyncCallback = function(fs) {
    if (!wallpaperFilename) {
      console.error('wallpaperFilename is not provided.');
      return;
    }
    if (!wallpaperLayout)
      wallpaperLayout = 'CENTER_CROPPED';
    fs.root.getFile(wallpaperFilename, {create: false}, function(fileEntry) {
      fileEntry.file(function(file) {
        var reader = new FileReader();
        reader.onloadend = function() {
          chrome.wallpaperPrivate.setCustomWallpaper(
              reader.result,
              wallpaperLayout,
              true,
              wallpaperFilename,
              function(thumbnailData) {
                // TODO(ranj): Ignore 'canceledWallpaper' error.
                if (chrome.runtime.lastError) {
                  console.error(chrome.runtime.lastError.message);
                  return;
                }
                WallpaperUtil.storeWallpaperToLocalFS(wallpaperFilename,
                    reader.result, Constants.WallpaperDirNameEnum.ORIGINAL);
                WallpaperUtil.storeWallpaperToLocalFS(wallpaperFilename,
                    reader.result, Constants.WallpaperDirNameEnum.THUMBNAIL);
                if (onSuccess)
                  onSuccess();
              });
        };
        reader.readAsArrayBuffer(file);
      }, WallpaperUtil.onFileSystemError);
    }, function(e) {}  // fail to read file, expected due to download delay
    );
  };
  WallpaperUtil.requestSyncFS(setWallpaperFromSyncCallback);
};

/**
 * Saves value to local storage that associates with key.
 * @param {string} key The key that associates with value.
 * @param {string} value The value to save to local storage.
 * @param {boolen} sync True if the value is saved to sync storage.
 * @param {function=} opt_callback The callback on success, or on failure.
 */
WallpaperUtil.saveToStorage = function(key, value, sync, opt_callback) {
  var items = {};
  items[key] = value;
  if (sync)
    WallpaperUtil.enabledSyncThemesCallback(function() {
      Constants.WallpaperSyncStorage.set(items, opt_callback);
    });
  else
    Constants.WallpaperLocalStorage.set(items, opt_callback);
};

/**
 * Saves user's wallpaper infomation to local and sync storage. Note that local
 * value should be saved first.
 * @param {string} url The url address of wallpaper. For custom wallpaper, it is
 *     the file name.
 * @param {string} layout The wallpaper layout.
 * @param {string} source The wallpaper source.
 */
WallpaperUtil.saveWallpaperInfo = function(url, layout, source) {
  var wallpaperInfo = {
      url: url,
      layout: layout,
      source: source
  };
  WallpaperUtil.saveToStorage(Constants.AccessLocalWallpaperInfoKey,
                              wallpaperInfo, false, function() {
    WallpaperUtil.saveToStorage(Constants.AccessSyncWallpaperInfoKey,
                                wallpaperInfo, true);
  });
};

/**
 * Downloads resources from url. Calls onSuccess and opt_onFailure accordingly.
 * @param {string} url The url address where we should fetch resources.
 * @param {string} type The response type of XMLHttprequest.
 * @param {function} onSuccess The success callback. It must be called with
 *     current XMLHttprequest object.
 * @param {function} onFailure The failure callback.
 * @param {XMLHttpRequest=} opt_xhr The XMLHttpRequest object.
 */
WallpaperUtil.fetchURL = function(url, type, onSuccess, onFailure, opt_xhr) {
  var xhr;
  if (opt_xhr)
    xhr = opt_xhr;
  else
    xhr = new XMLHttpRequest();

  try {
    // Do not use loadend here to handle both success and failure case. It gets
    // complicated with abortion. Unexpected error message may show up. See
    // http://crbug.com/242581.
    xhr.addEventListener('load', function(e) {
      if (this.status == 200) {
        onSuccess(this);
      } else {
        onFailure(this.status);
      }
    });
    xhr.addEventListener('error', onFailure);
    xhr.open('GET', url, true);
    xhr.responseType = type;
    xhr.send(null);
  } catch (e) {
    onFailure();
  }
};

/**
 * Sets wallpaper to online wallpaper specified by url and layout
 * @param {string} url The url address where we should fetch resources.
 * @param {string} layout The layout of online wallpaper.
 * @param {function} onSuccess The success callback.
 * @param {function} onFailure The failure callback.
 */
WallpaperUtil.setOnlineWallpaper = function(url, layout, onSuccess, onFailure) {
  var self = this;
  chrome.wallpaperPrivate.setWallpaperIfExists(url, layout, function(exists) {
    if (exists) {
      onSuccess();
      return;
    }

    self.fetchURL(url, 'arraybuffer', function(xhr) {
      if (xhr.response != null) {
        chrome.wallpaperPrivate.setWallpaper(xhr.response, layout, url,
                                             onSuccess);
        self.saveWallpaperInfo(url, layout,
                               Constants.WallpaperSourceEnum.Online);
      } else {
        onFailure();
      }
    }, onFailure);
  });
};

/**
 * Runs chrome.test.sendMessage in test environment. Does nothing if running
 * in production environment.
 *
 * @param {string} message Test message to send.
 */
WallpaperUtil.testSendMessage = function(message) {
  var test = chrome.test || window.top.chrome.test;
  if (test)
    test.sendMessage(message);
};
