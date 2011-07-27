// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// require: cr/ui/print_preview_cloud.js

var localStrings = new LocalStrings();

// If useCloudPrint is true we attempt to connect to cloud print
// and populate the list of printers with cloud print printers.
var useCloudPrint = false;

// Store the last selected printer index.
var lastSelectedPrinterIndex = 0;

// Used to disable some printing options when the preview is not modifiable.
var previewModifiable = false;

// Destination list special value constants.
const ADD_CLOUD_PRINTER = 'addCloudPrinter';
const MANAGE_CLOUD_PRINTERS = 'manageCloudPrinters';
const MANAGE_LOCAL_PRINTERS = 'manageLocalPrinters';
const MORE_PRINTERS = 'morePrinters';
const SIGN_IN = 'signIn';
const PRINT_TO_PDF = 'Print to PDF';

// State of the print preview settings.
var printSettings = new PrintSettings();

// The name of the default or last used printer.
var defaultOrLastUsedPrinterName = '';

// True when a pending print preview request exists.
var hasPendingPreviewRequest = false;

// The ID of the last preview request.
var lastPreviewRequestID = -1;

// True when a pending print file request exists.
var hasPendingPrintDocumentRequest = false;

// True when preview tab is hidden.
var isTabHidden = false;

// Object holding all the pages related settings.
var pageSettings;

// Object holding all the copies related settings.
var copiesSettings;

// Object holding all the layout related settings.
var layoutSettings;

// True if the user has click 'Advanced...' in order to open the system print
// dialog.
var showingSystemDialog = false;

// The range of options in the printer dropdown controlled by cloud print.
var firstCloudPrintOptionPos = 0;
var lastCloudPrintOptionPos = firstCloudPrintOptionPos;


// TODO(abodenha@chromium.org) A lot of cloud print specific logic has
// made its way into this file.  Refactor to create a cleaner boundary
// between print preview and GCP code.  Reference bug 88098 when fixing.

// A dictionary of cloud printers that have been added to the printer
// dropdown.
var addedCloudPrinters = {};

// The maximum number of cloud printers to allow in the dropdown.
const maxCloudPrinters = 10;


/**
 * Window onload handler, sets up the page and starts print preview by getting
 * the printer list.
 */
function onLoad() {
  cr.enablePlatformSpecificCSSRules();

  if (!checkCompatiblePluginExists()) {
    disableInputElementsInSidebar();
    displayErrorMessageWithButton(localStrings.getString('noPlugin'),
                                  localStrings.getString('launchNativeDialog'),
                                  launchNativePrintDialog);
    $('mainview').parentElement.removeChild($('dummy-viewer'));
    return;
  }

  $('system-dialog-link').addEventListener('click', onSystemDialogLinkClicked);
  $('mainview').parentElement.removeChild($('dummy-viewer'));

  $('printer-list').disabled = true;

  printHeader = print_preview.PrintHeader.getInstance();
  pageSettings = print_preview.PageSettings.getInstance();
  copiesSettings = print_preview.CopiesSettings.getInstance();
  layoutSettings = print_preview.LayoutSettings.getInstance();
  printHeader.addEventListeners();
  pageSettings.addEventListeners();
  copiesSettings.addEventListeners();
  layoutSettings.addEventListeners();

  showLoadingAnimation();
  chrome.send('getDefaultPrinter');
}

/**
 * Adds event listeners to the settings controls.
 */
function addEventListeners() {
  // Controls that require preview rendering.
  $('printer-list').onchange = updateControlsWithSelectedPrinterCapabilities;

  // Controls that do not require preview rendering.
  $('color').onclick = function() { setColor(true); };
  $('bw').onclick = function() { setColor(false); };
}

/**
 * Removes event listeners from the settings controls.
 */
function removeEventListeners() {
  if (pageSettings)
    clearTimeout(pageSettings.timerId_);

  // Controls that require preview rendering
  $('printer-list').onchange = null;

  // Controls that don't require preview rendering.
  $('color').onclick = null;
  $('bw').onclick = null;
}

/**
 * Disables the input elements in the sidebar.
 */
function disableInputElementsInSidebar() {
  var els = $('sidebar').querySelectorAll('input, button, select');
  for (var i = 0; i < els.length; i++)
    els[i].disabled = true;
}

/**
 * Disables the controls in the sidebar, shows the throbber and instructs the
 * backend to open the native print dialog.
 */
function onSystemDialogLinkClicked() {
  showingSystemDialog = true;
  disableInputElementsInSidebar();
  $('system-dialog-throbber').classList.remove('hidden');
  chrome.send('showSystemDialog');
}

/**
 * Similar to onSystemDialogLinkClicked(), but specific to the
 * 'Launch native print dialog' UI.
 */
function launchNativePrintDialog() {
  showingSystemDialog = true;
  $('error-button').disabled = true;
  $('native-print-dialog-throbber').classList.remove('hidden');
  chrome.send('showSystemDialog');
}

/**
 * Disables the controls which need the initiator tab to generate preview
 * data. This function is called when the initiator tab is closed.
 * @param {string} initiatorTabURL The URL of the initiator tab.
 */
function onInitiatorTabClosed(initiatorTabURL) {
  disableInputElementsInSidebar();
  displayErrorMessageWithButton(
      localStrings.getString('initiatorTabClosed'),
      localStrings.getString('reopenPage'),
      function() { window.location = initiatorTabURL; });
}

/**
 * Gets the selected printer capabilities and updates the controls accordingly.
 */
function updateControlsWithSelectedPrinterCapabilities() {
  var printerList = $('printer-list');
  var selectedIndex = printerList.selectedIndex;
  if (selectedIndex < 0)
    return;
  var skip_refresh = false;
  var selectedValue = printerList.options[selectedIndex].value;
  if (cloudprint.isCloudPrint(printerList.options[selectedIndex])) {
    updateWithCloudPrinterCapabilities();
    skip_refresh = true;
  } else if (selectedValue == SIGN_IN ||
             selectedValue == MANAGE_CLOUD_PRINTERS ||
             selectedValue == MANAGE_LOCAL_PRINTERS ||
             selectedValue == ADD_CLOUD_PRINTER) {
    printerList.selectedIndex = lastSelectedPrinterIndex;
    chrome.send(selectedValue);
    skip_refresh = true;
  } else if (selectedValue == MORE_PRINTERS) {
    onSystemDialogLinkClicked();
    skip_refresh = true;
  } else if (selectedValue == PRINT_TO_PDF) {
    updateWithPrinterCapabilities({
        'disableColorOption': true,
        'setColorAsDefault': true,
        'setDuplexAsDefault': false,
        'disableCopiesOption': true});
  } else {
    // This message will call back to 'updateWithPrinterCapabilities'
    // function.
    chrome.send('getPrinterCapabilities', [selectedValue]);
  }
  if (!skip_refresh) {
    lastSelectedPrinterIndex = selectedIndex;

    // Regenerate the preview data based on selected printer settings.
    setDefaultValuesAndRegeneratePreview();
  }
}

/**
 * Updates the printer capabilities for the currently selected
 * cloud print printer.
 */
function updateWithCloudPrinterCapabilities() {
  var printerList = $('printer-list');
  var selectedIndex = printerList.selectedIndex;
  cloudprint.updatePrinterCaps(printerList.options[selectedIndex],
                               doUpdateCloudPrinterCapabilities);
}

/**
 * Helper function to do the actual work of updating cloud printer
 * capabilities.
 * @param {Object} printer The printer object to set capabilities for.
 */
function doUpdateCloudPrinterCapabilities(printer) {
  var settings = {'disableColorOption': !cloudprint.supportsColor(printer),
                  'setColorAsDefault': cloudprint.colorIsDefault(printer),
                  'disableCopiesOption': true,
                  'disableLandscapeOption': true};
  updateWithPrinterCapabilities(settings);
  var printerList = $('printer-list');
  var selectedIndex = printerList.selectedIndex;
  lastSelectedPrinterIndex = selectedIndex;

  // Regenerate the preview data based on selected printer settings.
  setDefaultValuesAndRegeneratePreview();
}

/**
 * Updates the controls with printer capabilities information.
 * @param {Object} settingInfo printer setting information.
 */
function updateWithPrinterCapabilities(settingInfo) {
  var customEvent = new cr.Event("printerCapabilitiesUpdated");
  customEvent.printerCapabilities = settingInfo;
  document.dispatchEvent(customEvent);

  var disableColorOption = settingInfo.disableColorOption;
  var setColorAsDefault = settingInfo.setColorAsDefault;
  var color = $('color');
  var bw = $('bw');
  var colorOptions = $('color-options');

  disableColorOption ? fadeOutElement(colorOptions) :
      fadeInElement(colorOptions);
  colorOptions.setAttribute('aria-hidden', disableColorOption);

  if (color.checked != setColorAsDefault) {
    color.checked = setColorAsDefault;
    bw.checked = !setColorAsDefault;
    setColor(color.checked);
  }
}

/**
 * Turn on the integration of Cloud Print.
 * @param {boolean} enable True if cloud print should be used.
 * @param {string} cloudPrintUrl The URL to use for cloud print servers.
 */
function setUseCloudPrint(enable, cloudPrintURL) {
  useCloudPrint = enable;
  cloudprint.setBaseURL(cloudPrintURL);
}

/**
 * Take the PDF data handed to us and submit it to the cloud, closing the print
 * preview tab once the upload is successful.
 * @param {string} data Data to send as the print job.
 */
function printToCloud(data) {
  cloudprint.printToCloud(data, finishedCloudPrinting);
}

/**
 * Cloud print upload of the PDF file is finished, time to close the dialog.
 */
function finishedCloudPrinting() {
  window.location = cloudprint.getBaseURL();
}

/**
 * Checks whether the preview color setting is set to 'color' or not.
 *
 * @return {boolean} true if color is 'color'.
 */
function isColor() {
  return $('color').checked;
}

/**
 * Checks whether the specified settings are valid.
 *
 * @return {boolean} true if settings are valid, false if not.
 */
function areSettingsValid() {
  return pageSettings.isPageSelectionValid() &&
      (copiesSettings.isValid() ||
       getSelectedPrinterName() == PRINT_TO_PDF);
}

/**
 * Creates an object based on the values in the printer settings.
 *
 * @return {Object} Object containing print job settings.
 */
function getSettings() {
  var deviceName = getSelectedPrinterName();
  var printToPDF = (deviceName == PRINT_TO_PDF);

  var settings =
      {'deviceName': deviceName,
       'pageRange': pageSettings.selectedPageRanges,
       'printAll': pageSettings.allPagesRadioButton.checked,
       'duplex': copiesSettings.duplexMode,
       'copies': copiesSettings.numberOfCopies,
       'collate': copiesSettings.isCollated(),
       'landscape': layoutSettings.isLandscape(),
       'color': isColor(),
       'printToPDF': printToPDF,
       'requestID': 0};

  var printerList = $('printer-list');
  var selectedPrinter = printerList.selectedIndex;
  if (cloudprint.isCloudPrint(printerList.options[selectedPrinter])) {
    settings['cloudPrintID'] =
        printerList.options[selectedPrinter].value;
  }
  return settings;
}

/**
 * @return {number} The next unused preview request id.
 */
function generatePreviewRequestID() {
  return ++lastPreviewRequestID;
}

/**
 * @return {boolean} True iff a preview has been requested.
 */
function hasRequestedPreview() {
  return lastPreviewRequestID > -1;
}

/**
 * Returns the name of the selected printer or the empty string if no
 * printer is selected.
 * @return {string} The name of the currently selected printer.
 */
function getSelectedPrinterName() {
  var printerList = $('printer-list')
  var selectedPrinter = printerList.selectedIndex;
  if (selectedPrinter < 0)
    return '';
  return printerList.options[selectedPrinter].value;
}

/**
 * Asks the browser to print the preview PDF based on current print
 * settings. If the preview is still loading, printPendingFile() will get
 * called once the preview loads.
 */
function requestToPrintDocument() {
  hasPendingPrintDocumentRequest = hasPendingPreviewRequest;
  var printToPDF = getSelectedPrinterName() == PRINT_TO_PDF;

  if (hasPendingPrintDocumentRequest) {
    if (printToPDF) {
      // TODO(thestig) disable controls here.
     } else {
       isTabHidden = true;
       chrome.send('hidePreview');
     }
    return;
  }

  if (printToPDF) {
    sendPrintDocumentRequest();
  } else {
    removeEventListeners();
    window.setTimeout(function() { sendPrintDocumentRequest(); }, 1000);
  }
}

/**
 * Asks the browser to print the pending preview PDF that just finished
 * loading.
 */
function requestToPrintPendingDocument() {
  hasPendingPrintDocumentRequest = false;

  if (!areSettingsValid()) {
    if (isTabHidden)
      cancelPendingPrintRequest();
    return;
  }
  sendPrintDocumentRequest();
}

/**
 * Sends a message to cancel the pending print request.
 */
function cancelPendingPrintRequest() {
  chrome.send('cancelPendingPrintRequest');
}

/**
 * Sends a message to initiate print workflow.
 */
function sendPrintDocumentRequest() {
  var printerList = $('printer-list');
  var printer = printerList[printerList.selectedIndex];
  chrome.send('saveLastPrinter', [printer.textContent,
                                  cloudprint.getData(printer)]);
  chrome.send('print', [JSON.stringify(getSettings()),
                        cloudprint.getPrintTicketJSON(printer)]);
}

/**
 * Asks the browser to generate a preview PDF based on current print settings.
 */
function requestPrintPreview() {
  hasPendingPreviewRequest = true;
  removeEventListeners();
  printSettings.save();
  if (!isTabHidden)
    showLoadingAnimation();

  var settings = getSettings();
  settings.requestID = generatePreviewRequestID();
  chrome.send('getPreview', [JSON.stringify(settings)]);
}

/**
 * Called from PrintPreviewUI::OnFileSelectionCancelled to notify the print
 * preview tab regarding the file selection cancel event.
 */
function fileSelectionCancelled() {
  // TODO(thestig) re-enable controls here.
}

/**
 * Set the default printer. If there is one, generate a print preview.
 * @param {string} printer Name of the default printer. Empty if none.
 * @param {string} cloudPrintData Cloud print related data to restore if
 *                 the default printer is a cloud printer.
 */
function setDefaultPrinter(printer_name, cloudPrintData) {
  // Add a placeholder value so the printer list looks valid.
  addDestinationListOption('', '', true, true, true);
  if (printer_name) {
    defaultOrLastUsedPrinterName = printer_name;
    if (cloudPrintData) {
      cloudprint.setDefaultPrinter(printer_name,
                                   cloudPrintData,
                                   addCloudPrinters,
                                   doUpdateCloudPrinterCapabilities);
    } else {
      $('printer-list')[0].value = defaultOrLastUsedPrinterName;
      updateControlsWithSelectedPrinterCapabilities();
    }
  }
  chrome.send('getPrinters');
}

/**
 * Fill the printer list drop down.
 * Called from PrintPreviewHandler::SendPrinterList().
 * @param {Array} printers Array of printer info objects.
 */
function setPrinters(printers) {
  var printerList = $('printer-list');
  // Remove empty entry added by setDefaultPrinter.
  if (printerList[0] && printerList[0].textContent == '')
    printerList.remove(0);
  for (var i = 0; i < printers.length; ++i) {
    var isDefault = (printers[i].deviceName == defaultOrLastUsedPrinterName);
    addDestinationListOption(printers[i].printerName, printers[i].deviceName,
                             isDefault, false, false);
  }

  if (printers.length != 0)
    addDestinationListOption('', '', false, true, true);

  // Adding option for saving PDF to disk.
  addDestinationListOption(localStrings.getString('printToPDF'),
                           PRINT_TO_PDF,
                           defaultOrLastUsedPrinterName == PRINT_TO_PDF,
                           false,
                           false);
  addDestinationListOption('', '', false, true, true);

  // Add options to manage printers.
  if (!cr.isChromeOS) {
    addDestinationListOption(localStrings.getString('manageLocalPrinters'),
        MANAGE_LOCAL_PRINTERS, false, false, false);
  }
  if (useCloudPrint) {
    // Fetch recent printers.
    cloudprint.fetchPrinters(addCloudPrinters, false);
    // Fetch the full printer list.
    cloudprint.fetchPrinters(addCloudPrinters, true);
    addDestinationListOption(localStrings.getString('manageCloudPrinters'),
        MANAGE_CLOUD_PRINTERS, false, false, false);
  }

  printerList.disabled = false;

  if (!hasRequestedPreview())
    updateControlsWithSelectedPrinterCapabilities();
}

/**
 * Creates an option that can be added to the printer destination list.
 * @param {string} optionText specifies the option text content.
 * @param {string} optionValue specifies the option value.
 * @param {boolean} isDefault is true if the option needs to be selected.
 * @param {boolean} isDisabled is true if the option needs to be disabled.
 * @param {boolean} isSeparator is true if the option is a visual separator and
 *     needs to be disabled.
 * @return {Object} The created option.
 */
function createDestinationListOption(optionText, optionValue, isDefault,
    isDisabled, isSeparator) {
  var option = document.createElement('option');
  option.textContent = optionText;
  option.value = optionValue;
  option.selected = isDefault;
  option.disabled = isSeparator || isDisabled;
  // Adding attribute for improved accessibility.
  if (isSeparator)
    option.setAttribute("role", "separator");
  return option;
}

/**
 * Adds an option to the printer destination list.
 * @param {string} optionText specifies the option text content.
 * @param {string} optionValue specifies the option value.
 * @param {boolean} isDefault is true if the option needs to be selected.
 * @param {boolean} isDisabled is true if the option needs to be disabled.
 * @return {Object} The created option.
 */
function addDestinationListOption(optionText, optionValue, isDefault,
    isDisabled, isSeparator) {
  var option = createDestinationListOption(optionText,
                                           optionValue,
                                           isDefault,
                                           isDisabled,
                                           isSeparator);
  $('printer-list').add(option);
  return option;
}

/**
 * Adds an option to the printer destination list at a specified position.
 * @param {number} position The index in the printer-list wher the option
       should be created.
 * @param {string} optionText specifies the option text content.
 * @param {string} optionValue specifies the option value.
 * @param {boolean} isDefault is true if the option needs to be selected.
 * @param {boolean} isDisabled is true if the option needs to be disabled.
 * @return {Object} The created option.
 */
function addDestinationListOptionAtPosition(position,
                                            optionText,
                                            optionValue,
                                            isDefault,
                                            isDisabled,
                                            isSeparator) {
  var option = createDestinationListOption(optionText,
                                           optionValue,
                                           isDefault,
                                           isDisabled,
                                           isSeparator);
  var printerList = $('printer-list');
  var before = printerList[position];
  printerList.add(option, before);
  return option;
}

/**
 * Test if a particular cloud printer has already been added to the
 * printer dropdown.
 * @param {string} id A unique value to track this printer.
 * @return {boolean} True if this id has previously been passed to
 *     trackCloudPrinterAdded.
 */
function cloudPrinterAlreadyAdded(id) {
  return (addedCloudPrinters[id]);
}

/**
 * Record that a cloud printer will added to the printer dropdown.
 * @param {string} id A unique value to track this printer.
 * @return {boolean} False if adding this printer would exceed
 *     |maxCloudPrinters|.
 */
function trackCloudPrinterAdded(id) {
  if (Object.keys(addedCloudPrinters).length < maxCloudPrinters) {
    addedCloudPrinters[id] = true;
    return true;
  } else {
    return false;
  }
}


/**
 * Add cloud printers to the list drop down.
 * Called from the cloudprint object on receipt of printer information from the
 * cloud print server.
 * @param {Array} printers Array of printer info objects.
 * @return {Object} The currently selected printer.
 */
function addCloudPrinters(printers) {
  var isFirstPass = false;
  var showMorePrintersOption = false;
  var printerList = $('printer-list');

  if (firstCloudPrintOptionPos == lastCloudPrintOptionPos) {
    isFirstPass = true;
    // Remove empty entry added by setDefaultPrinter.
    if (printerList[0] && printerList[0].textContent == '')
      printerList.remove(0);
    var option = addDestinationListOptionAtPosition(
        lastCloudPrintOptionPos++,
        localStrings.getString('cloudPrinters'),
        'Label',
        false,
        true,
        false);
    cloudprint.setCloudPrint(option, null, null);
  }
  if (printers != null) {
    if (printers.length == 0) {
      addDestinationListOptionAtPosition(lastCloudPrintOptionPos++,
          localStrings.getString('addCloudPrinter'),
          ADD_CLOUD_PRINTER,
          false,
          false,
          false);
    } else {
      for (var i = 0; i < printers.length; i++) {
        if (!cloudPrinterAlreadyAdded(printers[i]['id'])) {
          var option = addDestinationListOptionAtPosition(
              lastCloudPrintOptionPos++,
              printers[i]['name'],
              printers[i]['id'],
              printers[i]['name'] == defaultOrLastUsedPrinterName,
              false,
              false);
          cloudprint.setCloudPrint(option,
                                   printers[i]['name'],
                                   printers[i]['id']);
          if (!trackCloudPrinterAdded(printers[i]['id'])) {
            showMorePrintersOption = true;
            break;
          }
        }
      }
      if (showMorePrintersOption) {
        addDestinationListOptionAtPosition(lastCloudPrintOptionPos++,
                                           '',
                                           '',
                                           false,
                                           true,
                                           true);
        addDestinationListOptionAtPosition(lastCloudPrintOptionPos++,
            localStrings.getString('morePrinters'),
            MORE_PRINTERS, false, false, false);
      }
    }
  } else {
    if (!cloudPrinterAlreadyAdded(SIGN_IN)) {
      addDestinationListOptionAtPosition(lastCloudPrintOptionPos++,
                                         localStrings.getString('signIn'),
                                         SIGN_IN,
                                         false,
                                         false,
                                         false);
      trackCloudPrinterAdded(SIGN_IN);
    }
  }
  if (isFirstPass) {
    addDestinationListOptionAtPosition(lastCloudPrintOptionPos,
                                       '',
                                       '',
                                       false,
                                       true,
                                       true);
    addDestinationListOptionAtPosition(lastCloudPrintOptionPos + 1,
                                       localStrings.getString('localPrinters'),
                                       '',
                                       false,
                                       true,
                                       false);
  }
  var selectedPrinter = printerList.selectedIndex;
  if (selectedPrinter < 0)
    return null;
  return printerList.options[selectedPrinter];
}

/**
 * Sets the color mode for the PDF plugin.
 * Called from PrintPreviewHandler::ProcessColorSetting().
 * @param {boolean} color is true if the PDF plugin should display in color.
 */
function setColor(color) {
  var pdfViewer = $('pdf-viewer');
  if (!pdfViewer) {
    return;
  }
  pdfViewer.grayscale(!color);
  var printerList = $('printer-list')
  cloudprint.setColor(printerList[printerList.selectedIndex], color);
}

/**
 * Display an error message in the center of the preview area.
 * @param {string} errorMessage The error message to be displayed.
 */
function displayErrorMessage(errorMessage) {
  $('print-button').disabled = true;
  $('overlay-layer').classList.remove('invisible');
  $('dancing-dots-text').classList.add('hidden');
  $('error-text').innerHTML = errorMessage;
  $('error-text').classList.remove('hidden');
  removeEventListeners();
  var pdfViewer = $('pdf-viewer');
  if (pdfViewer)
    $('mainview').removeChild(pdfViewer);

  if (isTabHidden)
    cancelPendingPrintRequest();
}

/**
 * Display an error message in the center of the preview area followed by a
 * button.
 * @param {string} errorMessage The error message to be displayed.
 * @param {string} buttonText The text to be displayed within the button.
 * @param {string} buttonListener The listener to be executed when the button is
 * clicked.
 */
function displayErrorMessageWithButton(
    errorMessage, buttonText, buttonListener) {
  var errorButton = $('error-button');
  errorButton.disabled = false;
  errorButton.textContent = buttonText;
  errorButton.onclick = buttonListener;
  errorButton.classList.remove('hidden');
  $('system-dialog-throbber').classList.add('hidden');
  $('native-print-dialog-throbber').classList.add('hidden');
  displayErrorMessage(errorMessage);
}

/**
 * Display an error message when print preview fails.
 * Called from PrintPreviewMessageHandler::OnPrintPreviewFailed().
 */
function printPreviewFailed() {
  displayErrorMessage(localStrings.getString('previewFailed'));
}

/**
 * Called when the PDF plugin loads its document.
 */
function onPDFLoad() {
  $('pdf-viewer').fitToHeight();
  setColor($('color').checked);
  hideLoadingAnimation();

  cr.dispatchSimpleEvent(document, 'PDFLoaded');
}

/**
 * Update the page count and check the page range.
 * Called from PrintPreviewUI::OnDidGetPreviewPageCount().
 * @param {number} pageCount The number of pages.
 */
function onDidGetPreviewPageCount(pageCount) {
  pageSettings.updateState(pageCount);
}

/**
 * Notification that a print preview page has been rendered.
 * Check if the settings have changed and request a regeneration if needed.
 * Called from PrintPreviewUI::OnDidPreviewPage().
 * @param {number} pageNumber The page number, 0-based.
 */
function onDidPreviewPage(pageNumber) {
  if (checkIfSettingsChangedAndRegeneratePreview())
    return;
  // TODO(thestig) Make use of |pageNumber| for pipelined preview generation.
}

/**
 * Update the print preview when new preview data is available.
 * Create the PDF plugin as needed.
 * Called from PrintPreviewUI::PreviewDataIsAvailable().
 * @param {string} jobTitle The print job title.
 * @param {boolean} modifiable If the preview is modifiable.
 * @param {string} previewUid Preview unique identifier.
 * @param {number} previewRequestId The preview request id that resulted in this
 *     response.
 */
function updatePrintPreview(jobTitle,
                            modifiable,
                            previewUid,
                            previewRequestId) {
  if (lastPreviewRequestID != previewRequestId)
    return;
  hasPendingPreviewRequest = false;

  if (checkIfSettingsChangedAndRegeneratePreview())
    return;

  previewModifiable = modifiable;
  document.title = localStrings.getStringF('printPreviewTitleFormat', jobTitle);

  createPDFPlugin(previewUid);
  cr.dispatchSimpleEvent(document, 'updateSummary');
  cr.dispatchSimpleEvent(document, 'updatePrintButton');
  addEventListeners();

  if (hasPendingPrintDocumentRequest)
    requestToPrintPendingDocument();
}

/**
 * Check if any print settings changed and regenerate the preview if needed.
 * @return {boolean} true if a new preview is required.
 */
function checkIfSettingsChangedAndRegeneratePreview() {
  var tempPrintSettings = new PrintSettings();
  tempPrintSettings.save();

  if (printSettings.deviceName != tempPrintSettings.deviceName) {
    updateControlsWithSelectedPrinterCapabilities();
    return true;
  }
  if (printSettings.isLandscape != tempPrintSettings.isLandscape) {
    setDefaultValuesAndRegeneratePreview();
    return true;
  }
  if (pageSettings.requestPrintPreviewIfNeeded())
    return true;

  return false;
}

/**
 * Create the PDF plugin or reload the existing one.
 * @param {string} previewUid Preview unique identifier.
 */
function createPDFPlugin(previewUid) {
  var pdfViewer = $('pdf-viewer');
  if (pdfViewer) {
    // Need to call this before the reload(), where the plugin resets its
    // internal page count.
    pdfViewer.goToPage('0');

    pdfViewer.reload();
    pdfViewer.grayscale(!isColor());
    return;
  }

  pdfViewer = document.createElement('embed');
  pdfViewer.setAttribute('id', 'pdf-viewer');
  pdfViewer.setAttribute('type',
                         'application/x-google-chrome-print-preview-pdf');
  pdfViewer.setAttribute('src', 'chrome://print/' + previewUid + '/print.pdf');
  pdfViewer.setAttribute('aria-live', 'polite');
  pdfViewer.setAttribute('aria-atomic', 'true');
  $('mainview').appendChild(pdfViewer);
  pdfViewer.onload('onPDFLoad()');
  pdfViewer.removePrintButton();
  pdfViewer.grayscale(true);
}

/**
 * @return {boolean} true if a compatible pdf plugin exists.
 */
function checkCompatiblePluginExists() {
  var dummyPlugin = $('dummy-viewer')
  return (dummyPlugin.onload &&
          dummyPlugin.goToPage &&
          dummyPlugin.removePrintButton);
}

window.addEventListener('DOMContentLoaded', onLoad);

/**
 * Sets the default values and sends a request to regenerate preview data.
 */
function setDefaultValuesAndRegeneratePreview() {
  pageSettings.resetState();
  requestPrintPreview();
}

/**
 * Class that represents the state of the print settings.
 */
function PrintSettings() {
  this.deviceName = '';
  this.isLandscape = '';
}

/**
 * Takes a snapshot of the print settings.
 */
PrintSettings.prototype.save = function() {
  this.deviceName = getSelectedPrinterName();
  this.isLandscape = layoutSettings.isLandscape();
}

