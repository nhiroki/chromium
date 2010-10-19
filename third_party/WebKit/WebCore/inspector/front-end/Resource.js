/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.Resource = function(identifier, url)
{
    this.identifier = identifier;
    this.url = url;
    this._startTime = -1;
    this._endTime = -1;
    this._requestMethod = "";
    this._category = WebInspector.resourceCategories.other;
}

// Keep these in sync with WebCore::InspectorResource::Type
WebInspector.Resource.Type = {
    Document:   0,
    Stylesheet: 1,
    Image:      2,
    Font:       3,
    Script:     4,
    XHR:        5,
    Media:      6,
    WebSocket:  7,
    Other:      8,

    isTextType: function(type)
    {
        return (type === this.Document) || (type === this.Stylesheet) || (type === this.Script) || (type === this.XHR);
    },

    toUIString: function(type)
    {
        return WebInspector.UIString(WebInspector.Resource.Type.toString(type));
    },

    // Returns locale-independent string identifier of resource type (primarily for use in extension API).
    // The IDs need to be kept in sync with webInspector.resoureces.Types object in ExtensionAPI.js.
    toString: function(type)
    {
        switch (type) {
            case this.Document:
                return "document";
            case this.Stylesheet:
                return "stylesheet";
            case this.Image:
                return "image";
            case this.Font:
                return "font";
            case this.Script:
                return "script";
            case this.XHR:
                return "XHR";
            case this.Media:
                return "media";
            case this.WebSocket:
                return "WebSocket";
            case this.Other:
            default:
                return "other";
        }
    }
}

WebInspector.Resource.prototype = {
    get url()
    {
        return this._url;
    },

    set url(x)
    {
        if (this._url === x)
            return;

        this._url = x;
        delete this._parsedQueryParameters;

        var parsedURL = x.asParsedURL();
        this.domain = parsedURL ? parsedURL.host : "";
        this.path = parsedURL ? parsedURL.path : "";
        this.lastPathComponent = "";
        if (parsedURL && parsedURL.path) {
            var lastSlashIndex = parsedURL.path.lastIndexOf("/");
            if (lastSlashIndex !== -1)
                this.lastPathComponent = parsedURL.path.substring(lastSlashIndex + 1);
        }
        this.lastPathComponentLowerCase = this.lastPathComponent.toLowerCase();
    },

    get documentURL()
    {
        return this._documentURL;
    },

    set documentURL(x)
    {
        this._documentURL = x;
    },

    get displayName()
    {
        if (this._displayName)
            return this._displayName;
        this._displayName = this.lastPathComponent;
        if (!this._displayName)
            this._displayName = this.displayDomain;
        if (!this._displayName && this.url)
            this._displayName = this.url.trimURL(WebInspector.mainResource ? WebInspector.mainResource.domain : "");
        if (this._displayName === "/")
            this._displayName = this.url;
        return this._displayName;
    },

    get displayDomain()
    {
        // WebInspector.Database calls this, so don't access more than this.domain.
        if (this.domain && (!WebInspector.mainResource || (WebInspector.mainResource && this.domain !== WebInspector.mainResource.domain)))
            return this.domain;
        return "";
    },

    get startTime()
    {
        return this._startTime || -1;
    },

    set startTime(x)
    {
        this._startTime = x;
    },

    get responseReceivedTime()
    {
        if (this.timing && this.timing.requestTime) {
            // Calculate responseReceivedTime from timing data for better accuracy.
            // Timing's requestTime is a baseline in seconds, rest of the numbers there are ticks in millis.
            return this.timing.requestTime + this.timing.receiveHeadersEnd / 1000.0;
        }
        return this._responseReceivedTime || -1;
    },

    set responseReceivedTime(x)
    {
        this._responseReceivedTime = x;
    },

    get endTime()
    {
        return this._endTime || -1;
    },

    set endTime(x)
    {
        // In case of fast load (or in case of cached resources), endTime on network stack
        // can be less then m_responseReceivedTime measured in WebCore. Normalize it here,
        // prefer actualEndTime to m_responseReceivedTime.
        if (x < this.responseReceivedTime)
            this.responseReceivedTime = x;

        this._endTime = x;
    },

    get duration()
    {
        if (this._endTime === -1 || this._startTime === -1)
            return -1;
        return this._endTime - this._startTime;
    },

    get latency()
    {
        if (this._responseReceivedTime === -1 || this._startTime === -1)
            return -1;
        return this._responseReceivedTime - this._startTime;
    },

    get receiveDuration()
    {
        if (this._endTime === -1 || this._responseReceivedTime === -1)
            return -1;
        return this._endTime - this._responseReceivedTime;
    },

    get resourceSize()
    {
        return this._resourceSize || 0;
    },

    set resourceSize(x)
    {
        this._resourceSize = x;
    },

    get transferSize()
    {
        // FIXME: this is wrong for chunked-encoding resources.
        return this.cached ? 0 : Number(this.responseHeaders["Content-Length"] || this.resourceSize || 0);
    },

    get expectedContentLength()
    {
        return this._expectedContentLength || 0;
    },

    set expectedContentLength(x)
    {
        this._expectedContentLength = x;
    },

    get finished()
    {
        return this._finished;
    },

    set finished(x)
    {
        if (this._finished === x)
            return;

        this._finished = x;

        if (x) {
            this._checkWarnings();
            this.dispatchEventToListeners("finished");
        }
    },

    get failed()
    {
        return this._failed;
    },

    set failed(x)
    {
        this._failed = x;
    },

    get category()
    {
        return this._category;
    },

    set category(x)
    {
        if (this._category === x)
            return;

        var oldCategory = this._category;
        if (oldCategory)
            oldCategory.removeResource(this);

        this._category = x;

        if (this._category)
            this._category.addResource(this);
    },

    get cached()
    {
        return this._cached;
    },

    set cached(x)
    {
        this._cached = x;
    },

    get mimeType()
    {
        return this._mimeType;
    },

    set mimeType(x)
    {
        this._mimeType = x;
    },

    get type()
    {
        return this._type;
    },

    set type(x)
    {
        if (this._type === x)
            return;

        this._type = x;

        switch (x) {
            case WebInspector.Resource.Type.Document:
                this.category = WebInspector.resourceCategories.documents;
                break;
            case WebInspector.Resource.Type.Stylesheet:
                this.category = WebInspector.resourceCategories.stylesheets;
                break;
            case WebInspector.Resource.Type.Script:
                this.category = WebInspector.resourceCategories.scripts;
                break;
            case WebInspector.Resource.Type.Image:
                this.category = WebInspector.resourceCategories.images;
                break;
            case WebInspector.Resource.Type.Font:
                this.category = WebInspector.resourceCategories.fonts;
                break;
            case WebInspector.Resource.Type.XHR:
                this.category = WebInspector.resourceCategories.xhr;
                break;
            case WebInspector.Resource.Type.WebSocket:
                this.category = WebInspector.resourceCategories.websocket;
                break;
            case WebInspector.Resource.Type.Other:
            default:
                this.category = WebInspector.resourceCategories.other;
                break;
        }
    },

    get requestHeaders()
    {
        return this._requestHeaders || {};
    },

    set requestHeaders(x)
    {
        this._requestHeaders = x;
        delete this._sortedRequestHeaders;

        this.dispatchEventToListeners("requestHeaders changed");
    },

    get sortedRequestHeaders()
    {
        if (this._sortedRequestHeaders !== undefined)
            return this._sortedRequestHeaders;

        this._sortedRequestHeaders = [];
        for (var key in this.requestHeaders)
            this._sortedRequestHeaders.push({header: key, value: this.requestHeaders[key]});
        this._sortedRequestHeaders.sort(function(a,b) { return a.header.localeCompare(b.header) });

        return this._sortedRequestHeaders;
    },

    requestHeaderValue: function(headerName)
    {
        return this._headerValue(this.requestHeaders, headerName);
    },

    get requestFormData()
    {
        return this._requestFormData;
    },

    set requestFormData(x)
    {
        this._requestFormData = x;
        delete this._parsedFormParameters;
    },

    get responseHeaders()
    {
        return this._responseHeaders || {};
    },

    set responseHeaders(x)
    {
        this._responseHeaders = x;
        delete this._sortedResponseHeaders;

        this.dispatchEventToListeners("responseHeaders changed");
    },

    get sortedResponseHeaders()
    {
        if (this._sortedResponseHeaders !== undefined)
            return this._sortedResponseHeaders;

        this._sortedResponseHeaders = [];
        for (var key in this.responseHeaders)
            this._sortedResponseHeaders.push({header: key, value: this.responseHeaders[key]});
        this._sortedResponseHeaders.sort(function(a,b) { return a.header.localeCompare(b.header) });

        return this._sortedResponseHeaders;
    },

    responseHeaderValue: function(headerName)
    {
        return this._headerValue(this.responseHeaders, headerName);
    },

    get queryParameters()
    {
        if (this._parsedQueryParameters)
            return this._parsedQueryParameters;
        var queryString = this.url.split("?", 2)[1];
        if (!queryString)
            return;
        this._parsedQueryParameters = this._parseParameters(queryString);
        return this._parsedQueryParameters;
    },

    get formParameters()
    {
        if (this._parsedFormParameters)
            return this._parsedFormParameters;
        if (!this.requestFormData)
            return;
        var requestContentType = this.requestHeaderValue("Content-Type");
        if (!requestContentType || !requestContentType.match(/^application\/x-www-form-urlencoded\s*(;.*)?$/i))
            return;
        this._parsedFormParameters = this._parseParameters(this.requestFormData);
        return this._parsedFormParameters;
    },

    _parseParameters: function(queryString)
    {
        function parseNameValue(pair)
        {
            var parameter = {};
            var splitPair = pair.split("=", 2);

            parameter.name = splitPair[0];
            if (splitPair.length === 1)
                parameter.value = "";
            else
                parameter.value = splitPair[1];
            return parameter;
        }
        return queryString.split("&").map(parseNameValue);
    },

    _headerValue: function(headers, headerName)
    {
        headerName = headerName.toLowerCase();
        for (var header in headers) {
            if (header.toLowerCase() === headerName)
                return headers[header];
        }
    },

    get scripts()
    {
        if (!("_scripts" in this))
            this._scripts = [];
        return this._scripts;
    },

    addScript: function(script)
    {
        if (!script)
            return;
        this.scripts.unshift(script);
        script.resource = this;
    },

    removeAllScripts: function()
    {
        if (!this._scripts)
            return;

        for (var i = 0; i < this._scripts.length; ++i) {
            if (this._scripts[i].resource === this)
                delete this._scripts[i].resource;
        }

        delete this._scripts;
    },

    removeScript: function(script)
    {
        if (!script)
            return;

        if (script.resource === this)
            delete script.resource;

        if (!this._scripts)
            return;

        this._scripts.remove(script);
    },

    get errors()
    {
        return this._errors || 0;
    },

    set errors(x)
    {
        this._errors = x;
    },

    get warnings()
    {
        return this._warnings || 0;
    },

    set warnings(x)
    {
        this._warnings = x;
    },

    _mimeTypeIsConsistentWithType: function()
    {
        // If status is an error, content is likely to be of an inconsistent type,
        // as it's going to be an error message. We do not want to emit a warning
        // for this, though, as this will already be reported as resource loading failure.
        if (this.statusCode >= 400)
            return true;

        if (typeof this.type === "undefined"
         || this.type === WebInspector.Resource.Type.Other
         || this.type === WebInspector.Resource.Type.XHR
         || this.type === WebInspector.Resource.Type.WebSocket)
            return true;

        if (this.mimeType in WebInspector.MIMETypes)
            return this.type in WebInspector.MIMETypes[this.mimeType];

        return false;
    },

    _checkWarnings: function()
    {
        for (var warning in WebInspector.Warnings)
            this._checkWarning(WebInspector.Warnings[warning]);
    },

    _checkWarning: function(warning)
    {
        var msg;
        switch (warning.id) {
            case WebInspector.Warnings.IncorrectMIMEType.id:
                if (!this._mimeTypeIsConsistentWithType())
                    msg = new WebInspector.ConsoleMessage(WebInspector.ConsoleMessage.MessageSource.Other,
                        WebInspector.ConsoleMessage.MessageType.Log, 
                        WebInspector.ConsoleMessage.MessageLevel.Warning,
                        -1,
                        this.url,
                        null,
                        1,
                        String.sprintf(WebInspector.Warnings.IncorrectMIMEType.message, WebInspector.Resource.Type.toUIString(this.type), this.mimeType),
                        null,
                        null);
                break;
        }

        if (msg)
            WebInspector.console.addMessage(msg);
    },

    getContents: function(callback)
    {
        // FIXME: eventually, cached resources will have no identifiers.
        if (this.frameID)
            InspectorBackend.resourceContent(this.frameID, this.url, callback);
        else
            InspectorBackend.getResourceContent(this.identifier, false, callback);
    }
}

WebInspector.Resource.prototype.__proto__ = WebInspector.Object.prototype;

WebInspector.Resource.CompareByStartTime = function(a, b)
{
    return a.startTime - b.startTime;
}

WebInspector.Resource.CompareByResponseReceivedTime = function(a, b)
{
    var aVal = a.responseReceivedTime;
    var bVal = b.responseReceivedTime;
    if (aVal === -1 ^ bVal === -1)
        return bVal - aVal;
    return aVal - bVal;
}

WebInspector.Resource.CompareByEndTime = function(a, b)
{
    var aVal = a.endTime;
    var bVal = b.endTime;
    if (aVal === -1 ^ bVal === -1)
        return bVal - aVal;
    return aVal - bVal;
}

WebInspector.Resource.CompareByDuration = function(a, b)
{
    return a.duration - b.duration;
}

WebInspector.Resource.CompareByLatency = function(a, b)
{
    return a.latency - b.latency;
}

WebInspector.Resource.CompareBySize = function(a, b)
{
    return a.resourceSize - b.resourceSize;
}

WebInspector.Resource.CompareByTransferSize = function(a, b)
{
    return a.transferSize - b.transferSize;
}
