// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Tests the focus behavior of the search box.
 */
testcase.searchBoxFocus = function() {
  var appId;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DRIVE, this.next);
    },
    // Check that the file list has the focus on launch.
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, ['#file-list:focus']).then(this.next);
    },
    // Press the Ctrl-F key.
    function(element) {
      remoteCall.callRemoteTestUtil('fakeKeyDown',
                                    appId,
                                    ['body', 'U+0046', true],
                                    this.next);
    },
    // Check that the search box has the focus.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(appId, ['#search-box input:focus']).
          then(this.next); },
    // Press the Esc key.
    function(element) {
      remoteCall.callRemoteTestUtil('fakeKeyDown',
                                    appId,
                                    ['#search-box input', 'U+001B', false],
                                    this.next);
    },
    // Check that the file list has the focus.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'file-list').then(this.next);
    },
    // Check for errors.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests the tab focus behavior of Files.app when no file is selected.
 */
testcase.tabindexFocus = function() {
  var appId;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DRIVE, this.next);
    },
    // Check that the file list has the focus on launch.
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, ['#file-list:focus']).then(this.next);
    },
    function(element) {
      remoteCall.waitForElement(appId, ['#drive-welcome-link']).then(this.next);
    },
    function(element) {
      remoteCall.callRemoteTestUtil('getActiveElement', appId, [], this.next);
    // Press the Tab key.
    }, function(element) {
      chrome.test.assertEq('list', element.attributes['class']);
      remoteCall.checkNextTabFocus(appId, 'search-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'view-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'gear-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'directory-tree').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'drive-welcome-link').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'file-list').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests the tab focus behavior of Files.app when no file is selected in
 * Downloads directory.
 */
testcase.tabindexFocusDownloads = function() {
  var appId;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    // Check that the file list has the focus on launch.
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, ['#file-list:focus']).then(this.next);
    },
    // Press the Tab key.
    function(element) {
      remoteCall.callRemoteTestUtil('getActiveElement', appId, [], this.next);
    }, function(element) {
      chrome.test.assertEq('list', element.attributes['class']);
      remoteCall.checkNextTabFocus(appId, 'search-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'view-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'gear-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'directory-tree').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'file-list').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests the tab focus behavior of Files.app when a directory is selected.
 */
testcase.tabindexFocusDirectorySelected = function() {
  var appId;
  StepsRunner.run([
    // Set up File Manager.
    function() {
      setupAndWaitUntilReady(null, RootPath.DRIVE, this.next);
    },
    // Check that the file list has the focus on launch.
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, ['#file-list:focus']).then(this.next);
    },
    function(element) {
      remoteCall.waitForElement(appId, ['#drive-welcome-link']).then(this.next);
    },
    function(element) {
      remoteCall.callRemoteTestUtil('getActiveElement', appId, [], this.next);
    }, function(element) {
      chrome.test.assertEq('list', element.attributes['class']);
      // Select the directory named 'photos'.
      remoteCall.callRemoteTestUtil(
          'selectFile', appId, ['photos']).then(this.next);
    // Press the Tab key.
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'share-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'delete-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'search-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'view-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'gear-button').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'directory-tree').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'drive-welcome-link').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      remoteCall.checkNextTabFocus(appId, 'file-list').then(this.next);
    }, function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Tests the tab focus in the dialog and closes the dialog.
 *
 * @param {Object} dialogParams Dialog parameters to be passed to
 *     chrome.fileSystem.chooseEntry.
 * @param {string} volumeName Volume name passed to the selectVolume remote
 *     function.
 * @param {Array.<TestEntryInfo>} expectedSet Expected set of the entries.
 * @param {boolean} selectFile True to select 'hello.txt' before the tab order
 *     check, false do not select any file before the check.
 * @param {string} initialElement Selector of the element which shows ready.
 * @param {Array.<string>} expectedTabOrder Array with the IDs of the element
 *     with the corresponding order of expected tab-indexes.
 */
function tabindexFocus(dialogParams, volumeName, expectedSet, selectFile,
                       initialElement, expectedTabOrder) {
  var localEntriesPromise = addEntries(['local'], BASIC_LOCAL_ENTRY_SET);
  var driveEntriesPromise = addEntries(['drive'], BASIC_DRIVE_ENTRY_SET);
  var setupPromise = Promise.all([localEntriesPromise, driveEntriesPromise]);

  var selectAndCheckAndClose = function(appId) {
    var promise = Promise.resolve();

    if (selectFile) {
      promise = promise.then(function() {
        return remoteCall.callRemoteTestUtil(
            'selectFile', appId, ['hello.txt']);
      });
    }

    promise = promise.then(function() {
      return remoteCall.callRemoteTestUtil('getActiveElement', appId, []);
    });

    // Waits for the initial element.
    promise = promise.then(function() {
      return remoteCall.waitForElement(appId, [initialElement]);
    });

    // Checks tabfocus.
    expectedTabOrder.forEach(function(className) {
      promise = promise.then(function() {
        return remoteCall.checkNextTabFocus(appId, className);
      }).then(function(result) {
        chrome.test.assertTrue(result);
      });
    });

    promise = promise.then(function() {
      // Closes the window by pressing Enter.
      return remoteCall.callRemoteTestUtil(
          'fakeKeyDown', appId, ['#file-list', 'Enter', false]);
    });

    return promise;
  };

  return setupPromise.then(function() {
    return openAndWaitForClosingDialog(
        dialogParams, volumeName, expectedSet, selectAndCheckAndClose);
  });
}

/**
 * Tests the tab focus behavior of Open Dialog (Downloads).
 */
testcase.tabindexOpenDialogDownloads = function() {
  testPromise(tabindexFocus(
      {type: 'openFile'}, 'downloads', BASIC_LOCAL_ENTRY_SET, true,
      '#ok-button:not([disabled])',
      ['ok-button', 'cancel-button', 'search-button', 'view-button',
       'gear-button', 'directory-tree', 'file-list']));
};

/**
 * Tests the tab focus behavior of Open Dialog (Drive).
 */
testcase.tabindexOpenDialogDrive = function() {
  testPromise(tabindexFocus(
      {type: 'openFile'}, 'drive', BASIC_DRIVE_ENTRY_SET, true,
      '#ok-button:not([disabled])',
      ['ok-button', 'cancel-button', 'search-button', 'view-button',
       'gear-button', 'directory-tree', 'file-list']));
};

/**
 * Tests the tab focus behavior of Save File Dialog (Downloads).
 */
testcase.tabindexSaveFileDialogDownloads = function() {
  testPromise(tabindexFocus(
      {
        type: 'saveFile',
        suggestedName: 'hoge.txt'  // Prevent showing a override prompt
      },
      'downloads', BASIC_LOCAL_ENTRY_SET, false,
      '#ok-button:not([disabled])',
      ['ok-button', 'cancel-button', 'search-button', 'view-button',
       'gear-button', 'directory-tree', 'file-list', 'new-folder-button',
       'filename-input-textbox']));
};

/**
 * Tests the tab focus behavior of Save File Dialog (Drive).
 */
testcase.tabindexSaveFileDialogDrive = function() {
  testPromise(tabindexFocus(
      {
        type: 'saveFile',
        suggestedName: 'hoge.txt'  // Prevent showing a override prompt
      },
      'drive', BASIC_DRIVE_ENTRY_SET, false,
      '#ok-button:not([disabled])',
      ['ok-button', 'cancel-button', 'search-button', 'view-button',
       'gear-button', 'directory-tree', 'file-list', 'new-folder-button',
       'filename-input-textbox']));
};
