/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.OverridesSupport = function()
{
    this._canForceCompositingMode = null;
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, this.updateCanForceCompositingMode.bind(this, null), this);

    WebInspector.settings.overrideUserAgent.addChangeListener(this._userAgentChanged, this);
    WebInspector.settings.userAgent.addChangeListener(this._userAgentChanged, this);

    WebInspector.settings.overrideDeviceMetrics.addChangeListener(this._deviceMetricsChanged, this);
    WebInspector.settings.deviceMetrics.addChangeListener(this._deviceMetricsChanged, this);
    WebInspector.settings.emulateViewport.addChangeListener(this._deviceMetricsChanged, this);
    WebInspector.settings.deviceFitWindow.addChangeListener(this._deviceMetricsChanged, this);

    WebInspector.settings.overrideGeolocation.addChangeListener(this._geolocationPositionChanged, this);
    WebInspector.settings.geolocationOverride.addChangeListener(this._geolocationPositionChanged, this);

    WebInspector.settings.overrideDeviceOrientation.addChangeListener(this._deviceOrientationChanged, this);
    WebInspector.settings.deviceOrientationOverride.addChangeListener(this._deviceOrientationChanged, this);

    WebInspector.settings.emulateTouchEvents.addChangeListener(this._emulateTouchEventsChanged, this);

    WebInspector.settings.overrideCSSMedia.addChangeListener(this._cssMediaChanged, this);
    WebInspector.settings.emulatedCSSMedia.addChangeListener(this._cssMediaChanged, this);
}

WebInspector.OverridesSupport.Events = {
    OverridesEnabledButImpossibleChanged: "OverridesEnabledButImpossibleChanged",
}

/**
 * @constructor
 * @param {number} width
 * @param {number} height
 * @param {number} deviceScaleFactor
 * @param {number} fontScaleFactor
 * @param {boolean} textAutosizing
 */
WebInspector.OverridesSupport.DeviceMetrics = function(width, height, deviceScaleFactor, fontScaleFactor, textAutosizing)
{
    this.width = width;
    this.height = height;
    this.deviceScaleFactor = deviceScaleFactor;
    this.fontScaleFactor = fontScaleFactor;
    this.textAutosizing = textAutosizing;
}

/**
 * Android computes a font scale factor fudged for improved legibility. This value is
 * not computed on non-Android builds so we duplicate the Android-specific logic here.
 * For a description of this algorithm, see:
 *     chrome/browser/chrome_content_browser_client.cc, GetFontScaleMultiplier(...)
 *
 * @param {number} width
 * @param {number} height
 * @param {number} deviceScaleFactor
 * @return {number} fontScaleFactor for Android.
 */
// FIXME(crbug.com/313410): Unify this calculation with the c++ side so we don't duplicate this logic.
WebInspector.OverridesSupport.DeviceMetrics._computeFontScaleFactorForAndroid = function(width, height, deviceScaleFactor)
{
    var minWidth = Math.min(width, height) / deviceScaleFactor;

    var kMinFSM = 1.05;
    var kWidthForMinFSM = 320;
    var kMaxFSM = 1.3;
    var kWidthForMaxFSM = 800;

    if (minWidth <= kWidthForMinFSM)
        return kMinFSM;
    if (minWidth >= kWidthForMaxFSM)
        return kMaxFSM;

    // The font scale multiplier varies linearly between kMinFSM and kMaxFSM.
    var ratio = (minWidth - kWidthForMinFSM) / (kWidthForMaxFSM - kWidthForMinFSM);
    var fontScaleFactor = ratio * (kMaxFSM - kMinFSM) + kMinFSM;
    return Math.round(fontScaleFactor * 1000) / 1000;
}

/**
 * @return {WebInspector.OverridesSupport.DeviceMetrics}
 */
WebInspector.OverridesSupport.DeviceMetrics.parseSetting = function(value)
{
    var width = 0;
    var height = 0;
    var deviceScaleFactor = 1;
    var fontScaleFactor = 1;
    var textAutosizing = false;
    if (value) {
        var splitMetrics = value.split("x");
        if (splitMetrics.length === 5) {
            width = parseInt(splitMetrics[0], 10);
            height = parseInt(splitMetrics[1], 10);
            deviceScaleFactor = parseFloat(splitMetrics[2]);
            fontScaleFactor = parseFloat(splitMetrics[3]);
            if (fontScaleFactor == 0)
                fontScaleFactor = WebInspector.OverridesSupport.DeviceMetrics._computeFontScaleFactorForAndroid(width, height, deviceScaleFactor);
            textAutosizing = splitMetrics[4] == 1;
        }
    }
    return new WebInspector.OverridesSupport.DeviceMetrics(width, height, deviceScaleFactor, fontScaleFactor, textAutosizing);
}

/**
 * @return {?WebInspector.OverridesSupport.DeviceMetrics}
 */
WebInspector.OverridesSupport.DeviceMetrics.parseUserInput = function(widthString, heightString, deviceScaleFactorString, fontScaleFactorString, textAutosizing)
{
    function isUserInputValid(value, isInteger)
    {
        if (!value)
            return true;
        return isInteger ? /^[0]*[1-9][\d]*$/.test(value) : /^[0]*([1-9][\d]*(\.\d+)?|\.\d+)$/.test(value);
    }

    if (!widthString ^ !heightString)
        return null;

    var isWidthValid = isUserInputValid(widthString, true);
    var isHeightValid = isUserInputValid(heightString, true);
    var isDeviceScaleFactorValid = isUserInputValid(deviceScaleFactorString, false);
    var isFontScaleFactorValid = isUserInputValid(fontScaleFactorString, false);

    if (!isWidthValid && !isHeightValid && !isDeviceScaleFactorValid && !isFontScaleFactorValid)
        return null;

    var width = isWidthValid ? parseInt(widthString || "0", 10) : -1;
    var height = isHeightValid ? parseInt(heightString || "0", 10) : -1;
    var deviceScaleFactor = isDeviceScaleFactorValid ? parseFloat(deviceScaleFactorString) : -1;
    var fontScaleFactor = isFontScaleFactorValid ? parseFloat(fontScaleFactorString) : -1;

    return new WebInspector.OverridesSupport.DeviceMetrics(width, height, deviceScaleFactor, fontScaleFactor, textAutosizing);
}

WebInspector.OverridesSupport.DeviceMetrics.prototype = {
    /**
     * @return {boolean}
     */
    isValid: function()
    {
        return this.isWidthValid() && this.isHeightValid() && this.isDeviceScaleFactorValid() && this.isFontScaleFactorValid();
    },

    /**
     * @return {boolean}
     */
    isWidthValid: function()
    {
        return this.width >= 0;
    },

    /**
     * @return {boolean}
     */
    isHeightValid: function()
    {
        return this.height >= 0;
    },

    /**
     * @return {boolean}
     */
    isDeviceScaleFactorValid: function()
    {
        return this.deviceScaleFactor > 0;
    },

    /**
     * @return {boolean}
     */
    isFontScaleFactorValid: function()
    {
        return this.fontScaleFactor > 0;
    },

    /**
     * @return {boolean}
     */
    isTextAutosizingValid: function()
    {
        return true;
    },

    /**
     * @return {string}
     */
    toSetting: function()
    {
        if (!this.isValid())
            return "";

        return this.width && this.height ? this.width + "x" + this.height + "x" + this.deviceScaleFactor + "x" + this.fontScaleFactor + "x" + (this.textAutosizing ? "1" : "0") : "";
    },

    /**
     * @return {string}
     */
    widthToInput: function()
    {
        return this.isWidthValid() && this.width ? String(this.width) : "";
    },

    /**
     * @return {string}
     */
    heightToInput: function()
    {
        return this.isHeightValid() && this.height ? String(this.height) : "";
    },

    /**
     * @return {string}
     */
    deviceScaleFactorToInput: function()
    {
        return this.isDeviceScaleFactorValid() && this.deviceScaleFactor ? String(this.deviceScaleFactor) : "";
    },

    /**
     * @return {string}
     */
    fontScaleFactorToInput: function()
    {
        return this.isFontScaleFactorValid() && this.fontScaleFactor ? String(this.fontScaleFactor) : "";
    }
}

/**
 * @constructor
 * @param {number} latitude
 * @param {number} longitude
 */
WebInspector.OverridesSupport.GeolocationPosition = function(latitude, longitude, error)
{
    this.latitude = latitude;
    this.longitude = longitude;
    this.error = error;
}

WebInspector.OverridesSupport.GeolocationPosition.prototype = {
    /**
     * @return {string}
     */
    toSetting: function()
    {
        return (typeof this.latitude === "number" && typeof this.longitude === "number" && typeof this.error === "string") ? this.latitude + "@" + this.longitude + ":" + this.error : "";
    }
}

/**
 * @return {WebInspector.OverridesSupport.GeolocationPosition}
 */
WebInspector.OverridesSupport.GeolocationPosition.parseSetting = function(value)
{
    if (value) {
        var splitError = value.split(":");
        if (splitError.length === 2) {
            var splitPosition = splitError[0].split("@")
            if (splitPosition.length === 2)
                return new WebInspector.OverridesSupport.GeolocationPosition(parseFloat(splitPosition[0]), parseFloat(splitPosition[1]), splitError[1]);
        }
    }
    return new WebInspector.OverridesSupport.GeolocationPosition(0, 0, "");
}

/**
 * @return {?WebInspector.OverridesSupport.GeolocationPosition}
 */
WebInspector.OverridesSupport.GeolocationPosition.parseUserInput = function(latitudeString, longitudeString, errorStatus)
{
    function isUserInputValid(value)
    {
        if (!value)
            return true;
        return /^[-]?[0-9]*[.]?[0-9]*$/.test(value);
    }

    if (!latitudeString ^ !latitudeString)
        return null;

    var isLatitudeValid = isUserInputValid(latitudeString);
    var isLongitudeValid = isUserInputValid(longitudeString);

    if (!isLatitudeValid && !isLongitudeValid)
        return null;

    var latitude = isLatitudeValid ? parseFloat(latitudeString) : -1;
    var longitude = isLongitudeValid ? parseFloat(longitudeString) : -1;

    return new WebInspector.OverridesSupport.GeolocationPosition(latitude, longitude, errorStatus ? "PositionUnavailable" : "");
}

WebInspector.OverridesSupport.GeolocationPosition.clearGeolocationOverride = function()
{
    PageAgent.clearGeolocationOverride();
}

/**
 * @constructor
 * @param {number} alpha
 * @param {number} beta
 * @param {number} gamma
 */
WebInspector.OverridesSupport.DeviceOrientation = function(alpha, beta, gamma)
{
    this.alpha = alpha;
    this.beta = beta;
    this.gamma = gamma;
}

WebInspector.OverridesSupport.DeviceOrientation.prototype = {
    /**
     * @return {string}
     */
    toSetting: function()
    {
        return JSON.stringify(this);
    }
}

/**
 * @return {WebInspector.OverridesSupport.DeviceOrientation}
 */
WebInspector.OverridesSupport.DeviceOrientation.parseSetting = function(value)
{
    if (value) {
        var jsonObject = JSON.parse(value);
        return new WebInspector.OverridesSupport.DeviceOrientation(jsonObject.alpha, jsonObject.beta, jsonObject.gamma);
    }
    return new WebInspector.OverridesSupport.DeviceOrientation(0, 0, 0);
}

/**
 * @return {?WebInspector.OverridesSupport.DeviceOrientation}
 */
WebInspector.OverridesSupport.DeviceOrientation.parseUserInput = function(alphaString, betaString, gammaString)
{
    function isUserInputValid(value)
    {
        if (!value)
            return true;
        return /^[-]?[0-9]*[.]?[0-9]*$/.test(value);
    }

    if (!alphaString ^ !betaString ^ !gammaString)
        return null;

    var isAlphaValid = isUserInputValid(alphaString);
    var isBetaValid = isUserInputValid(betaString);
    var isGammaValid = isUserInputValid(gammaString);

    if (!isAlphaValid && !isBetaValid && !isGammaValid)
        return null;

    var alpha = isAlphaValid ? parseFloat(alphaString) : -1;
    var beta = isBetaValid ? parseFloat(betaString) : -1;
    var gamma = isGammaValid ? parseFloat(gammaString) : -1;

    return new WebInspector.OverridesSupport.DeviceOrientation(alpha, beta, gamma);
}

WebInspector.OverridesSupport.DeviceOrientation.clearDeviceOrientationOverride = function()
{
    PageAgent.clearDeviceOrientationOverride();
}

WebInspector.OverridesSupport.prototype = {
    /**
     * @param {string} deviceMetrics
     * @param {string} userAgent
     */
    emulateDevice: function(deviceMetrics, userAgent)
    {
        WebInspector.settings.deviceMetrics.set(deviceMetrics);
        WebInspector.settings.userAgent.set(userAgent);
        WebInspector.settings.overrideDeviceMetrics.set(true);
        WebInspector.settings.overrideUserAgent.set(true);
        WebInspector.settings.emulateTouchEvents.set(true);
        WebInspector.settings.emulateViewport.set(true);
    },

    reset: function()
    {
        WebInspector.settings.overrideDeviceMetrics.set(false);
        WebInspector.settings.overrideUserAgent.set(false);
        WebInspector.settings.emulateTouchEvents.set(false);
        WebInspector.settings.overrideDeviceOrientation.set(false);
        WebInspector.settings.overrideGeolocation.set(false);
        WebInspector.settings.overrideCSSMedia.set(false);
        WebInspector.settings.emulateViewport.set(false);
    },

    applyInitialOverrides: function()
    {
        if (this._anyOverrideIsEnabled())
            this.updateCanForceCompositingMode(this._updateAllOverrides.bind(this));
        else
            this._updateAllOverrides();
    },

    _updateAllOverrides: function()
    {
        this._userAgentChanged();
        this._deviceMetricsChanged();
        this._deviceOrientationChanged();
        this._geolocationPositionChanged();
        this._emulateTouchEventsChanged();
        this._cssMediaChanged();
    },

    _userAgentChanged: function()
    {
        NetworkAgent.setUserAgentOverride(WebInspector.settings.overrideUserAgent.get() && this._canForceCompositingMode ? WebInspector.settings.userAgent.get() : "");
        this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
    },

    _deviceMetricsChanged: function()
    {
        var metrics = WebInspector.OverridesSupport.DeviceMetrics.parseSetting(WebInspector.settings.overrideDeviceMetrics.get() && this._canForceCompositingMode ? WebInspector.settings.deviceMetrics.get() : "");
        if (metrics.isValid()) {
            var active = metrics.width > 0 && metrics.height > 0;
            var dipWidth = Math.round(metrics.width / metrics.deviceScaleFactor);
            var dipHeight = Math.round(metrics.height / metrics.deviceScaleFactor);
            PageAgent.setDeviceMetricsOverride(dipWidth, dipHeight, metrics.deviceScaleFactor, WebInspector.settings.emulateViewport.get(), WebInspector.settings.deviceFitWindow.get(), metrics.textAutosizing, metrics.fontScaleFactor);
        }
        this._revealOverridesTabIfNeeded();
        this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
    },

    _geolocationPositionChanged: function()
    {
        if (!WebInspector.settings.overrideGeolocation.get() || !this._canForceCompositingMode) {
            PageAgent.clearGeolocationOverride();
            return;
        }
        var geolocation = WebInspector.OverridesSupport.GeolocationPosition.parseSetting(WebInspector.settings.geolocationOverride.get());
        if (geolocation.error)
            PageAgent.setGeolocationOverride();
        else
            PageAgent.setGeolocationOverride(geolocation.latitude, geolocation.longitude, 150);
        this._revealOverridesTabIfNeeded();
        this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
    },

    _deviceOrientationChanged: function()
    {
        if (!WebInspector.settings.overrideDeviceOrientation.get() || !this._canForceCompositingMode) {
            PageAgent.clearDeviceOrientationOverride();
            return;
        }
        var deviceOrientation = WebInspector.OverridesSupport.DeviceOrientation.parseSetting(WebInspector.settings.deviceOrientationOverride.get());
        PageAgent.setDeviceOrientationOverride(deviceOrientation.alpha, deviceOrientation.beta, deviceOrientation.gamma);
        this._revealOverridesTabIfNeeded();
        this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
    },

    _emulateTouchEventsChanged: function()
    {
        WebInspector.domAgent.emulateTouchEventObjects(WebInspector.settings.emulateTouchEvents.get() && this._canForceCompositingMode);
        this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
    },

    _cssMediaChanged: function()
    {
        PageAgent.setEmulatedMedia(WebInspector.settings.overrideCSSMedia.get() && this._canForceCompositingMode ? WebInspector.settings.emulatedCSSMedia.get() : "");
        WebInspector.cssModel.mediaQueryResultChanged();
        this._revealOverridesTabIfNeeded();
        this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
    },

    _anyOverrideIsEnabled: function()
    {
        return WebInspector.settings.overrideUserAgent.get() || WebInspector.settings.overrideDeviceMetrics.get() ||
            WebInspector.settings.overrideGeolocation.get() || WebInspector.settings.overrideDeviceOrientation.get() ||
            WebInspector.settings.emulateTouchEvents.get() || WebInspector.settings.overrideCSSMedia.get();
    },

    _revealOverridesTabIfNeeded: function()
    {
        if (this._canForceCompositingMode && this._anyOverrideIsEnabled()) {
            if (!WebInspector.settings.showEmulationViewInDrawer.get())
                WebInspector.settings.showEmulationViewInDrawer.set(true);
            WebInspector.inspectorView.showViewInDrawer("emulation");
        }
    },

    updateCanForceCompositingMode: function(callback)
    {
        function apiCallback(error, result)
        {
            if (!error && this._canForceCompositingMode !== result) {
                this._canForceCompositingMode = result;
                if (this._anyOverrideIsEnabled())
                    this._updateAllOverrides();
                this.dispatchEventToListeners(WebInspector.OverridesSupport.Events.OverridesEnabledButImpossibleChanged);
            }
            if (callback)
                callback();
        }
        PageAgent.canForceCompositingMode(apiCallback.bind(this));
    },

    overridesEnabledButImpossible: function()
    {
        return this._anyOverrideIsEnabled() && !this._canForceCompositingMode;
    },

    __proto__: WebInspector.Object.prototype
}


/**
 * @type {WebInspector.OverridesSupport}
 */
WebInspector.overridesSupport;
