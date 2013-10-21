/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @extends {WebInspector.View}
 */
WebInspector.InspectorView = function()
{
    WebInspector.View.call(this);
    this.markAsRoot();
    this.element.classList.add("fill", "vbox");
    this.element.setAttribute("spellcheck", false);

    this._toolbar = new WebInspector.Toolbar(this);
    this._panelStatusBarElement = this.element.createChild("div", "panel-status-bar");
    this._panelsElement = this.element.createChild("div", "main-panels");
    this._drawer = new WebInspector.Drawer(this);

    this._history = [];
    this._historyIterator = -1;
    document.addEventListener("keydown", this._keyDown.bind(this), false);
    document.addEventListener("keypress", this._keyPress.bind(this), false);
    this._panelOrder = [];
    this._panelDescriptors = {};

    // Windows and Mac have two different definitions of '[' and ']', so accept both of each.
    this._openBracketIdentifiers = ["U+005B", "U+00DB"].keySet();
    this._closeBracketIdentifiers = ["U+005D", "U+00DD"].keySet();
    this._footerElementContainer = this.element.createChild("div", "inspector-footer status-bar hidden");
}

WebInspector.InspectorView.prototype = {
    /**
     * @return {WebInspector.Toolbar}
     */
    toolbar: function()
    {
        return this._toolbar;
    },

    /**
     * @return {WebInspector.Drawer}
     */
    drawer: function()
    {
        return this._drawer;
    },

    /**
     * @param {WebInspector.PanelDescriptor} panelDescriptor
     */
    addPanel: function(panelDescriptor)
    {
        this._panelOrder.push(panelDescriptor.name());
        this._panelDescriptors[panelDescriptor.name()] = panelDescriptor;
        this._toolbar.addPanel(panelDescriptor);
    },

    /**
     * @param {string} panelName
     * @return {?WebInspector.Panel}
     */
    panel: function(panelName)
    {
        var panelDescriptor = this._panelDescriptors[panelName];
        if (!panelDescriptor && this._panelOrder.length)
            panelDescriptor = this._panelDescriptors[this._panelOrder[0]];
        return panelDescriptor ? panelDescriptor.panel() : null;
    },

    /**
     * @param {string} panelName
     * @return {?WebInspector.Panel}
     */
    showPanel: function(panelName)
    {
        var panel = this.panel(panelName);
        if (panel)
            this.setCurrentPanel(panel);
        return panel;
    },

    /**
     * @return {WebInspector.Panel}
     */
    currentPanel: function()
    {
        return this._currentPanel;
    },

    /**
     * @return {WebInspector.Searchable}
     */
    getSearchProvider: function()
    {
        return this._currentPanel;
    },
    
    /**
     * @param {WebInspector.Panel} x
     */
    setCurrentPanel: function(x)
    {
        if (this._currentPanel === x)
            return;

        // FIXME: remove search controller.
        WebInspector.searchController.cancelSearch();

        if (this._currentPanel) {
            this._panelWillHide();
            this._currentPanel.detach();
        }

        this._currentPanel = x;

        if (x) {
            x.show(this._panelsElement);
            this._panelWasShown(x);
        }
        for (var panelName in WebInspector.panels) {
            if (WebInspector.panels[panelName] === x) {
                WebInspector.settings.lastActivePanel.set(panelName);
                this._pushToHistory(panelName);
                WebInspector.userMetrics.panelShown(panelName);
            }
        }
    },

    /**
     * @param {string} id
     * @param {string} title
     * @param {WebInspector.View} view
     */
    showCloseableViewInDrawer: function(id, title, view)
    {
        this._drawer.showCloseableView(id, title, view);
    },

    /**
     * @param {string} id
     * @param {string} title
     * @param {WebInspector.ViewFactory} factory
     */
    registerViewInDrawer: function(id, title, factory)
    {
        this._drawer.registerView(id, title, factory);
    },

    /**
     * @param {string} id
     */
    showViewInDrawer: function(id)
    {
        this._drawer.showView(id);
    },

    closeDrawer: function()
    {
        this._drawer.hide();
    },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function()
    {
        return this._currentPanel ? this._currentPanel.defaultFocusedElement() : null;
    },

    _keyPress: function(event)
    {
        // BUG 104250: Windows 7 posts a WM_CHAR message upon the Ctrl+']' keypress.
        // Any charCode < 32 is not going to be a valid keypress.
        if (event.charCode < 32 && WebInspector.isWin())
            return;
        clearTimeout(this._keyDownTimer);
        delete this._keyDownTimer;
    },

    _keyDown: function(event)
    {
        if (!WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event))
            return;

        // Ctrl/Cmd + 1-9 should show corresponding panel.
        var panelShortcutEnabled = WebInspector.settings.shortcutPanelSwitch.get();
        if (panelShortcutEnabled && !event.shiftKey && !event.altKey && event.keyCode > 0x30 && event.keyCode < 0x3A) {
            var panelName = this._panelOrder[event.keyCode - 0x31];
            if (panelName) {
                this.showPanel(panelName);
                event.consume(true);
            }
            return;
        }

        // BUG85312: On French AZERTY keyboards, AltGr-]/[ combinations (synonymous to Ctrl-Alt-]/[ on Windows) are used to enter ]/[,
        // so for a ]/[-related keydown we delay the panel switch using a timer, to see if there is a keypress event following this one.
        // If there is, we cancel the timer and do not consider this a panel switch.
        if (!WebInspector.isWin() || (!this._openBracketIdentifiers[event.keyIdentifier] && !this._closeBracketIdentifiers[event.keyIdentifier])) {
            this._keyDownInternal(event);
            return;
        }

        this._keyDownTimer = setTimeout(this._keyDownInternal.bind(this, event), 0);
    },

    _keyDownInternal: function(event)
    {
        if (this._openBracketIdentifiers[event.keyIdentifier]) {
            var isRotateLeft = !event.shiftKey && !event.altKey;
            if (isRotateLeft) {
                var index = this._panelOrder.indexOf(this.currentPanel().name);
                index = (index === 0) ? this._panelOrder.length - 1 : index - 1;
                this.showPanel(this._panelOrder[index]);
                event.consume(true);
                return;
            }

            var isGoBack = event.altKey;
            if (isGoBack && this._canGoBackInHistory()) {
                this._goBackInHistory();
                event.consume(true);
            }
            return;
        }

        if (this._closeBracketIdentifiers[event.keyIdentifier]) {
            var isRotateRight = !event.shiftKey && !event.altKey;
            if (isRotateRight) {
                var index = this._panelOrder.indexOf(this.currentPanel().name);
                index = (index + 1) % this._panelOrder.length;
                this.showPanel(this._panelOrder[index]);
                event.consume(true);
                return;
            }

            var isGoForward = event.altKey;
            if (isGoForward && this._canGoForwardInHistory()) {
                this._goForwardInHistory();
                event.consume(true);
            }
            return;
        }
    },

    _canGoBackInHistory: function()
    {
        return this._historyIterator > 0;
    },

    _goBackInHistory: function()
    {
        this._inHistory = true;
        this.setCurrentPanel(WebInspector.panels[this._history[--this._historyIterator]]);
        delete this._inHistory;
    },

    _canGoForwardInHistory: function()
    {
        return this._historyIterator < this._history.length - 1;
    },

    _goForwardInHistory: function()
    {
        this._inHistory = true;
        this.setCurrentPanel(WebInspector.panels[this._history[++this._historyIterator]]);
        delete this._inHistory;
    },

    _pushToHistory: function(panelName)
    {
        if (this._inHistory)
            return;

        this._history.splice(this._historyIterator + 1, this._history.length - this._historyIterator - 1);
        if (!this._history.length || this._history[this._history.length - 1] !== panelName)
            this._history.push(panelName);
        this._historyIterator = this._history.length - 1;
    },

    /**
     * @param {?Element} element
     */
    setFooterElement: function(element)
    {
        if (this._currentPanel && this._currentPanel.canSetFooterElement()) {
            this._currentPanel.setFooterElement(element);
            return;
        }
        if (element) {
            this._footerElementContainer.removeStyleClass("hidden");
            this._footerElementContainer.appendChild(element);
        } else {
            this._footerElementContainer.addStyleClass("hidden");
            this._footerElementContainer.removeChildren();
        }
        this.doResize();
    },

    onResize: function()
    {
        // FIXME: make toolbar and drawer views.
        this._toolbar.resize();
        this.doResize();
        this._drawer.resize();
    },

    /**
     * @param {WebInspector.Panel} panel
     */
    _panelWasShown: function(panel)
    {
        this._toolbar.panelSelected(panel);
        this._drawer.panelSelected(panel);

        var statusBarItems = panel.statusBarItems;
        if (statusBarItems) {
            for (var i = 0; i < statusBarItems.length; ++i)
                this._panelStatusBarElement.appendChild(statusBarItems[i]);
        }
        this._panelStatusBarElement.enableStyleClass("hidden", !statusBarItems);
        panel.focus();
    },

    _panelWillHide: function()
    {
        this._panelStatusBarElement.removeChildren();
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @type {WebInspector.InspectorView}
 */
WebInspector.inspectorView = null;
