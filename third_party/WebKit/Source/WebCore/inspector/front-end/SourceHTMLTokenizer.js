/* Generated by re2c 0.13.5 on Fri May  6 13:47:06 2011 */
/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

// Generate js file as follows:
//
// re2c -isc WebCore/inspector/front-end/SourceHTMLTokenizer.re2js \
// | sed 's|^yy\([^:]*\)*\:|case \1:|' \
// | sed 's|[*]cursor[+][+]|this._charAt(cursor++)|' \
// | sed 's|[[*][+][+]cursor|this._charAt(++cursor)|' \
// | sed 's|[*]cursor|this._charAt(cursor)|' \
// | sed 's|yych = \*\([^;]*\)|yych = this._charAt\1|' \
// | sed 's|{ gotoCase = \([^; continue; };]*\)|{ gotoCase = \1; continue; }|' \
// | sed 's|unsigned\ int|var|' \
// | sed 's|var\ yych|case 1: case 1: var yych|'

WebInspector.SourceHTMLTokenizer = function()
{
    WebInspector.SourceTokenizer.call(this);

    // The order is determined by the generated code.
    this._lexConditions = {
        INITIAL: 0,
        COMMENT: 1,
        DOCTYPE: 2,
        TAG: 3,
        DSTRING: 4,
        SSTRING: 5
    };
    this.case_INITIAL = 1000;
    this.case_COMMENT = 1001;
    this.case_DOCTYPE = 1002;
    this.case_TAG = 1003;
    this.case_DSTRING = 1004;
    this.case_SSTRING = 1005;

    this._parseConditions = {
        INITIAL: 0,
        ATTRIBUTE: 1,
        ATTRIBUTE_VALUE: 2,
        LINKIFY: 4,
        A_NODE: 8,
        SCRIPT: 16,
        STYLE: 32
    };

    this.condition = this.createInitialCondition();
}

WebInspector.SourceHTMLTokenizer.prototype = {
    createInitialCondition: function()
    {
        return { lexCondition: this._lexConditions.INITIAL, parseCondition: this._parseConditions.INITIAL };
    },

    set line(line) {
        if (this._condition.internalJavaScriptTokenizerCondition) {
            var match = /<\/script/i.exec(line);
            if (match) {
                this._internalJavaScriptTokenizer.line = line.substring(0, match.index);
            } else
                this._internalJavaScriptTokenizer.line = line;
        } else if (this._condition.internalCSSTokenizerCondition) {
            var match = /<\/style/i.exec(line);
            if (match) {
                this._internalCSSTokenizer.line = line.substring(0, match.index);
            } else
                this._internalCSSTokenizer.line = line;
        }
        this._line = line;
    },

    _isExpectingAttribute: function()
    {
        return this._condition.parseCondition & this._parseConditions.ATTRIBUTE;
    },

    _isExpectingAttributeValue: function()
    {
        return this._condition.parseCondition & this._parseConditions.ATTRIBUTE_VALUE;
    },

    _setExpectingAttribute: function()
    {
        if (this._isExpectingAttributeValue())
            this._condition.parseCondition ^= this._parseConditions.ATTRIBUTE_VALUE;
        this._condition.parseCondition |= this._parseConditions.ATTRIBUTE;
    },

    _setExpectingAttributeValue: function()
    {
        if (this._isExpectingAttribute())
            this._condition.parseCondition ^= this._parseConditions.ATTRIBUTE;
        this._condition.parseCondition |= this._parseConditions.ATTRIBUTE_VALUE;
    },

    _stringToken: function(cursor, stringEnds)
    {
        if (!this._isExpectingAttributeValue()) {
            this.tokenType = null;
            return cursor;
        }
        this.tokenType = this._attrValueTokenType();
        if (stringEnds)
            this._setExpectingAttribute();
        return cursor;
    },

    _attrValueTokenType: function()
    {
        if (this._condition.parseCondition & this._parseConditions.LINKIFY) {
            if (this._condition.parseCondition & this._parseConditions.A_NODE)
                return "html-external-link";
            return "html-resource-link";
        }
        return "html-attribute-value";
    },

    get _internalJavaScriptTokenizer()
    {
        return WebInspector.SourceTokenizer.Registry.getInstance().getTokenizer("text/javascript");
    },

    get _internalCSSTokenizer()
    {
        return WebInspector.SourceTokenizer.Registry.getInstance().getTokenizer("text/css");
    },

    scriptStarted: function(cursor)
    {
        this._condition.internalJavaScriptTokenizerCondition = this._internalJavaScriptTokenizer.createInitialCondition();
    },

    scriptEnded: function(cursor)
    {
    },

    styleSheetStarted: function(cursor)
    {
        this._condition.internalCSSTokenizerCondition = this._internalCSSTokenizer.createInitialCondition();
    },

    styleSheetEnded: function(cursor)
    {
    },

    nextToken: function(cursor)
    {
        if (this._condition.internalJavaScriptTokenizerCondition) {
            // Re-set line to force </script> detection first.
            this.line = this._line;
            if (cursor !== this._internalJavaScriptTokenizer._line.length) {
                // Tokenizer is stateless, so restore its condition before tokenizing and save it after.
                this._internalJavaScriptTokenizer.condition = this._condition.internalJavaScriptTokenizerCondition;
                var result = this._internalJavaScriptTokenizer.nextToken(cursor);
                this.tokenType = this._internalJavaScriptTokenizer.tokenType;
                this._condition.internalJavaScriptTokenizerCondition = this._internalJavaScriptTokenizer.condition;
                return result;
            } else if (cursor !== this._line.length)
                delete this._condition.internalJavaScriptTokenizerCondition;
        } else if (this._condition.internalCSSTokenizerCondition) {
            // Re-set line to force </style> detection first.
            this.line = this._line;
            if (cursor !== this._internalCSSTokenizer._line.length) {
                // Tokenizer is stateless, so restore its condition before tokenizing and save it after.
                this._internalCSSTokenizer.condition = this._condition.internalCSSTokenizerCondition;
                var result = this._internalCSSTokenizer.nextToken(cursor);
                this.tokenType = this._internalCSSTokenizer.tokenType;
                this._condition.internalCSSTokenizerCondition = this._internalCSSTokenizer.condition;
                return result;
            } else if (cursor !== this._line.length)
                delete this._condition.internalCSSTokenizerCondition;
        }

        var cursorOnEnter = cursor;
        var gotoCase = 1;
        while (1) {
            switch (gotoCase)
            // Following comment is replaced with generated state machine.
            
        {
            case 1: var yych;
            var yyaccept = 0;
            if (this.getLexCondition() < 3) {
                if (this.getLexCondition() < 1) {
                    { gotoCase = this.case_INITIAL; continue; };
                } else {
                    if (this.getLexCondition() < 2) {
                        { gotoCase = this.case_COMMENT; continue; };
                    } else {
                        { gotoCase = this.case_DOCTYPE; continue; };
                    }
                }
            } else {
                if (this.getLexCondition() < 4) {
                    { gotoCase = this.case_TAG; continue; };
                } else {
                    if (this.getLexCondition() < 5) {
                        { gotoCase = this.case_DSTRING; continue; };
                    } else {
                        { gotoCase = this.case_SSTRING; continue; };
                    }
                }
            }
/* *********************************** */
case this.case_COMMENT:

            yych = this._charAt(cursor);
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 4; continue; };
                { gotoCase = 3; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 4; continue; };
                if (yych == '-') { gotoCase = 6; continue; };
                { gotoCase = 3; continue; };
            }
case 2:
            { this.tokenType = "html-comment"; return cursor; }
case 3:
            yyaccept = 0;
            yych = this._charAt(YYMARKER = ++cursor);
            { gotoCase = 9; continue; };
case 4:
            ++cursor;
case 5:
            { this.tokenType = null; return cursor; }
case 6:
            yyaccept = 1;
            yych = this._charAt(YYMARKER = ++cursor);
            if (yych != '-') { gotoCase = 5; continue; };
case 7:
            ++cursor;
            yych = this._charAt(cursor);
            if (yych == '>') { gotoCase = 10; continue; };
case 8:
            yyaccept = 0;
            YYMARKER = ++cursor;
            yych = this._charAt(cursor);
case 9:
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 2; continue; };
                { gotoCase = 8; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 2; continue; };
                if (yych == '-') { gotoCase = 12; continue; };
                { gotoCase = 8; continue; };
            }
case 10:
            ++cursor;
            this.setLexCondition(this._lexConditions.INITIAL);
            { this.tokenType = "html-comment"; return cursor; }
case 12:
            ++cursor;
            yych = this._charAt(cursor);
            if (yych == '-') { gotoCase = 7; continue; };
            cursor = YYMARKER;
            if (yyaccept <= 0) {
                { gotoCase = 2; continue; };
            } else {
                { gotoCase = 5; continue; };
            }
/* *********************************** */
case this.case_DOCTYPE:
            yych = this._charAt(cursor);
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 18; continue; };
                { gotoCase = 17; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 18; continue; };
                if (yych == '>') { gotoCase = 20; continue; };
                { gotoCase = 17; continue; };
            }
case 16:
            { this.tokenType = "html-doctype"; return cursor; }
case 17:
            yych = this._charAt(++cursor);
            { gotoCase = 23; continue; };
case 18:
            ++cursor;
            { this.tokenType = null; return cursor; }
case 20:
            ++cursor;
            this.setLexCondition(this._lexConditions.INITIAL);
            { this.tokenType = "html-doctype"; return cursor; }
case 22:
            ++cursor;
            yych = this._charAt(cursor);
case 23:
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 16; continue; };
                { gotoCase = 22; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 16; continue; };
                if (yych == '>') { gotoCase = 16; continue; };
                { gotoCase = 22; continue; };
            }
/* *********************************** */
case this.case_DSTRING:
            yych = this._charAt(cursor);
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 28; continue; };
                { gotoCase = 27; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 28; continue; };
                if (yych == '"') { gotoCase = 30; continue; };
                { gotoCase = 27; continue; };
            }
case 26:
            { return this._stringToken(cursor); }
case 27:
            yych = this._charAt(++cursor);
            { gotoCase = 34; continue; };
case 28:
            ++cursor;
            { this.tokenType = null; return cursor; }
case 30:
            ++cursor;
case 31:
            this.setLexCondition(this._lexConditions.TAG);
            { return this._stringToken(cursor, true); }
case 32:
            yych = this._charAt(++cursor);
            { gotoCase = 31; continue; };
case 33:
            ++cursor;
            yych = this._charAt(cursor);
case 34:
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 26; continue; };
                { gotoCase = 33; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 26; continue; };
                if (yych == '"') { gotoCase = 32; continue; };
                { gotoCase = 33; continue; };
            }
/* *********************************** */
case this.case_INITIAL:
            yych = this._charAt(cursor);
            if (yych == '<') { gotoCase = 39; continue; };
            ++cursor;
            { this.tokenType = null; return cursor; }
case 39:
            yyaccept = 0;
            yych = this._charAt(YYMARKER = ++cursor);
            if (yych <= '/') {
                if (yych == '!') { gotoCase = 44; continue; };
                if (yych >= '/') { gotoCase = 41; continue; };
            } else {
                if (yych <= 'S') {
                    if (yych >= 'S') { gotoCase = 42; continue; };
                } else {
                    if (yych == 's') { gotoCase = 42; continue; };
                }
            }
case 40:
            this.setLexCondition(this._lexConditions.TAG);
            {
                    if (this._condition.parseCondition & (this._parseConditions.SCRIPT | this._parseConditions.STYLE)) {
                        // Do not tokenize script and style tag contents, keep lexer state, even though processing "<".
                        this.setLexCondition(this._lexConditions.INITIAL);
                        this.tokenType = null;
                        return cursor;
                    }

                    this._condition.parseCondition = this._parseConditions.INITIAL;
                    this.tokenType = "html-tag";
                    return cursor;
                }
case 41:
            yyaccept = 0;
            yych = this._charAt(YYMARKER = ++cursor);
            if (yych == 'S') { gotoCase = 73; continue; };
            if (yych == 's') { gotoCase = 73; continue; };
            { gotoCase = 40; continue; };
case 42:
            yych = this._charAt(++cursor);
            if (yych <= 'T') {
                if (yych == 'C') { gotoCase = 62; continue; };
                if (yych >= 'T') { gotoCase = 63; continue; };
            } else {
                if (yych <= 'c') {
                    if (yych >= 'c') { gotoCase = 62; continue; };
                } else {
                    if (yych == 't') { gotoCase = 63; continue; };
                }
            }
case 43:
            cursor = YYMARKER;
            { gotoCase = 40; continue; };
case 44:
            yych = this._charAt(++cursor);
            if (yych <= 'C') {
                if (yych != '-') { gotoCase = 43; continue; };
            } else {
                if (yych <= 'D') { gotoCase = 46; continue; };
                if (yych == 'd') { gotoCase = 46; continue; };
                { gotoCase = 43; continue; };
            }
            yych = this._charAt(++cursor);
            if (yych == '-') { gotoCase = 54; continue; };
            { gotoCase = 43; continue; };
case 46:
            yych = this._charAt(++cursor);
            if (yych == 'O') { gotoCase = 47; continue; };
            if (yych != 'o') { gotoCase = 43; continue; };
case 47:
            yych = this._charAt(++cursor);
            if (yych == 'C') { gotoCase = 48; continue; };
            if (yych != 'c') { gotoCase = 43; continue; };
case 48:
            yych = this._charAt(++cursor);
            if (yych == 'T') { gotoCase = 49; continue; };
            if (yych != 't') { gotoCase = 43; continue; };
case 49:
            yych = this._charAt(++cursor);
            if (yych == 'Y') { gotoCase = 50; continue; };
            if (yych != 'y') { gotoCase = 43; continue; };
case 50:
            yych = this._charAt(++cursor);
            if (yych == 'P') { gotoCase = 51; continue; };
            if (yych != 'p') { gotoCase = 43; continue; };
case 51:
            yych = this._charAt(++cursor);
            if (yych == 'E') { gotoCase = 52; continue; };
            if (yych != 'e') { gotoCase = 43; continue; };
case 52:
            ++cursor;
            this.setLexCondition(this._lexConditions.DOCTYPE);
            { this.tokenType = "html-doctype"; return cursor; }
case 54:
            ++cursor;
            yych = this._charAt(cursor);
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 57; continue; };
                { gotoCase = 54; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 57; continue; };
                if (yych != '-') { gotoCase = 54; continue; };
            }
            ++cursor;
            yych = this._charAt(cursor);
            if (yych == '-') { gotoCase = 59; continue; };
            { gotoCase = 43; continue; };
case 57:
            ++cursor;
            this.setLexCondition(this._lexConditions.COMMENT);
            { this.tokenType = "html-comment"; return cursor; }
case 59:
            ++cursor;
            yych = this._charAt(cursor);
            if (yych != '>') { gotoCase = 54; continue; };
            ++cursor;
            { this.tokenType = "html-comment"; return cursor; }
case 62:
            yych = this._charAt(++cursor);
            if (yych == 'R') { gotoCase = 68; continue; };
            if (yych == 'r') { gotoCase = 68; continue; };
            { gotoCase = 43; continue; };
case 63:
            yych = this._charAt(++cursor);
            if (yych == 'Y') { gotoCase = 64; continue; };
            if (yych != 'y') { gotoCase = 43; continue; };
case 64:
            yych = this._charAt(++cursor);
            if (yych == 'L') { gotoCase = 65; continue; };
            if (yych != 'l') { gotoCase = 43; continue; };
case 65:
            yych = this._charAt(++cursor);
            if (yych == 'E') { gotoCase = 66; continue; };
            if (yych != 'e') { gotoCase = 43; continue; };
case 66:
            ++cursor;
            this.setLexCondition(this._lexConditions.TAG);
            {
                    if (this._condition.parseCondition & this._parseConditions.STYLE) {
                        // Do not tokenize style tag contents, keep lexer state, even though processing "<".
                        this.setLexCondition(this._lexConditions.INITIAL);
                        this.tokenType = null;
                        return cursor;
                    }
                    this.tokenType = "html-tag";
                    this._condition.parseCondition = this._parseConditions.STYLE;
                    this._setExpectingAttribute();
                    return cursor;
                }
case 68:
            yych = this._charAt(++cursor);
            if (yych == 'I') { gotoCase = 69; continue; };
            if (yych != 'i') { gotoCase = 43; continue; };
case 69:
            yych = this._charAt(++cursor);
            if (yych == 'P') { gotoCase = 70; continue; };
            if (yych != 'p') { gotoCase = 43; continue; };
case 70:
            yych = this._charAt(++cursor);
            if (yych == 'T') { gotoCase = 71; continue; };
            if (yych != 't') { gotoCase = 43; continue; };
case 71:
            ++cursor;
            this.setLexCondition(this._lexConditions.TAG);
            {
                    if (this._condition.parseCondition & this._parseConditions.SCRIPT) {
                        // Do not tokenize script tag contents, keep lexer state, even though processing "<".
                        this.setLexCondition(this._lexConditions.INITIAL);
                        this.tokenType = null;
                        return cursor;
                    }
                    this.tokenType = "html-tag";
                    this._condition.parseCondition = this._parseConditions.SCRIPT;
                    this._setExpectingAttribute();
                    return cursor;
                }
case 73:
            yych = this._charAt(++cursor);
            if (yych <= 'T') {
                if (yych == 'C') { gotoCase = 75; continue; };
                if (yych <= 'S') { gotoCase = 43; continue; };
            } else {
                if (yych <= 'c') {
                    if (yych <= 'b') { gotoCase = 43; continue; };
                    { gotoCase = 75; continue; };
                } else {
                    if (yych != 't') { gotoCase = 43; continue; };
                }
            }
            yych = this._charAt(++cursor);
            if (yych == 'Y') { gotoCase = 81; continue; };
            if (yych == 'y') { gotoCase = 81; continue; };
            { gotoCase = 43; continue; };
case 75:
            yych = this._charAt(++cursor);
            if (yych == 'R') { gotoCase = 76; continue; };
            if (yych != 'r') { gotoCase = 43; continue; };
case 76:
            yych = this._charAt(++cursor);
            if (yych == 'I') { gotoCase = 77; continue; };
            if (yych != 'i') { gotoCase = 43; continue; };
case 77:
            yych = this._charAt(++cursor);
            if (yych == 'P') { gotoCase = 78; continue; };
            if (yych != 'p') { gotoCase = 43; continue; };
case 78:
            yych = this._charAt(++cursor);
            if (yych == 'T') { gotoCase = 79; continue; };
            if (yych != 't') { gotoCase = 43; continue; };
case 79:
            ++cursor;
            this.setLexCondition(this._lexConditions.TAG);
            {
                    this.tokenType = "html-tag";
                    this._condition.parseCondition = this._parseConditions.INITIAL;
                    this.scriptEnded(cursor - 8);
                    return cursor;
                }
case 81:
            yych = this._charAt(++cursor);
            if (yych == 'L') { gotoCase = 82; continue; };
            if (yych != 'l') { gotoCase = 43; continue; };
case 82:
            yych = this._charAt(++cursor);
            if (yych == 'E') { gotoCase = 83; continue; };
            if (yych != 'e') { gotoCase = 43; continue; };
case 83:
            ++cursor;
            this.setLexCondition(this._lexConditions.TAG);
            {
                    this.tokenType = "html-tag";
                    this._condition.parseCondition = this._parseConditions.INITIAL;
                    this.styleSheetEnded(cursor - 7);
                    return cursor;
                }
/* *********************************** */
case this.case_SSTRING:
            yych = this._charAt(cursor);
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 89; continue; };
                { gotoCase = 88; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 89; continue; };
                if (yych == '\'') { gotoCase = 91; continue; };
                { gotoCase = 88; continue; };
            }
case 87:
            { return this._stringToken(cursor); }
case 88:
            yych = this._charAt(++cursor);
            { gotoCase = 95; continue; };
case 89:
            ++cursor;
            { this.tokenType = null; return cursor; }
case 91:
            ++cursor;
case 92:
            this.setLexCondition(this._lexConditions.TAG);
            { return this._stringToken(cursor, true); }
case 93:
            yych = this._charAt(++cursor);
            { gotoCase = 92; continue; };
case 94:
            ++cursor;
            yych = this._charAt(cursor);
case 95:
            if (yych <= '\f') {
                if (yych == '\n') { gotoCase = 87; continue; };
                { gotoCase = 94; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 87; continue; };
                if (yych == '\'') { gotoCase = 93; continue; };
                { gotoCase = 94; continue; };
            }
/* *********************************** */
case this.case_TAG:
            yych = this._charAt(cursor);
            if (yych <= '&') {
                if (yych <= '\r') {
                    if (yych == '\n') { gotoCase = 100; continue; };
                    if (yych >= '\r') { gotoCase = 100; continue; };
                } else {
                    if (yych <= ' ') {
                        if (yych >= ' ') { gotoCase = 100; continue; };
                    } else {
                        if (yych == '"') { gotoCase = 102; continue; };
                    }
                }
            } else {
                if (yych <= '>') {
                    if (yych <= ';') {
                        if (yych <= '\'') { gotoCase = 103; continue; };
                    } else {
                        if (yych <= '<') { gotoCase = 100; continue; };
                        if (yych <= '=') { gotoCase = 104; continue; };
                        { gotoCase = 106; continue; };
                    }
                } else {
                    if (yych <= '[') {
                        if (yych >= '[') { gotoCase = 100; continue; };
                    } else {
                        if (yych == ']') { gotoCase = 100; continue; };
                    }
                }
            }
            ++cursor;
            yych = this._charAt(cursor);
            { gotoCase = 119; continue; };
case 99:
            {
                    if (this._condition.parseCondition === this._parseConditions.SCRIPT || this._condition.parseCondition === this._parseConditions.STYLE) {
                        // Fall through if expecting attributes.
                        this.tokenType = null;
                        return cursor;
                    }

                    if (this._condition.parseCondition === this._parseConditions.INITIAL) {
                        this.tokenType = "html-tag";
                        this._setExpectingAttribute();
                        var token = this._line.substring(cursorOnEnter, cursor);
                        if (token === "a")
                            this._condition.parseCondition |= this._parseConditions.A_NODE;
                        else if (this._condition.parseCondition & this._parseConditions.A_NODE)
                            this._condition.parseCondition ^= this._parseConditions.A_NODE;
                    } else if (this._isExpectingAttribute()) {
                        var token = this._line.substring(cursorOnEnter, cursor);
                        if (token === "href" || token === "src")
                            this._condition.parseCondition |= this._parseConditions.LINKIFY;
                        else if (this._condition.parseCondition |= this._parseConditions.LINKIFY)
                            this._condition.parseCondition ^= this._parseConditions.LINKIFY;
                        this.tokenType = "html-attribute-name";
                    } else if (this._isExpectingAttributeValue())
                        this.tokenType = this._attrValueTokenType();
                    else
                        this.tokenType = null;
                    return cursor;
                }
case 100:
            ++cursor;
            { this.tokenType = null; return cursor; }
case 102:
            yyaccept = 0;
            yych = this._charAt(YYMARKER = ++cursor);
            { gotoCase = 115; continue; };
case 103:
            yyaccept = 0;
            yych = this._charAt(YYMARKER = ++cursor);
            { gotoCase = 109; continue; };
case 104:
            ++cursor;
            {
                    if (this._isExpectingAttribute())
                        this._setExpectingAttributeValue();
                    this.tokenType = null;
                    return cursor;
                }
case 106:
            ++cursor;
            this.setLexCondition(this._lexConditions.INITIAL);
            {
                    this.tokenType = "html-tag";
                    if (this._condition.parseCondition & this._parseConditions.SCRIPT) {
                        this.scriptStarted(cursor);
                        // Do not tokenize script tag contents.
                        return cursor;
                    }

                    if (this._condition.parseCondition & this._parseConditions.STYLE) {
                        this.styleSheetStarted(cursor);
                        // Do not tokenize style tag contents.
                        return cursor;
                    }

                    this._condition.parseCondition = this._parseConditions.INITIAL;
                    return cursor;
                }
case 108:
            ++cursor;
            yych = this._charAt(cursor);
case 109:
            if (yych <= '\f') {
                if (yych != '\n') { gotoCase = 108; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 110; continue; };
                if (yych == '\'') { gotoCase = 112; continue; };
                { gotoCase = 108; continue; };
            }
case 110:
            ++cursor;
            this.setLexCondition(this._lexConditions.SSTRING);
            { return this._stringToken(cursor); }
case 112:
            ++cursor;
            { return this._stringToken(cursor, true); }
case 114:
            ++cursor;
            yych = this._charAt(cursor);
case 115:
            if (yych <= '\f') {
                if (yych != '\n') { gotoCase = 114; continue; };
            } else {
                if (yych <= '\r') { gotoCase = 116; continue; };
                if (yych == '"') { gotoCase = 112; continue; };
                { gotoCase = 114; continue; };
            }
case 116:
            ++cursor;
            this.setLexCondition(this._lexConditions.DSTRING);
            { return this._stringToken(cursor); }
case 118:
            ++cursor;
            yych = this._charAt(cursor);
case 119:
            if (yych <= '"') {
                if (yych <= '\r') {
                    if (yych == '\n') { gotoCase = 99; continue; };
                    if (yych <= '\f') { gotoCase = 118; continue; };
                    { gotoCase = 99; continue; };
                } else {
                    if (yych == ' ') { gotoCase = 99; continue; };
                    if (yych <= '!') { gotoCase = 118; continue; };
                    { gotoCase = 99; continue; };
                }
            } else {
                if (yych <= '>') {
                    if (yych == '\'') { gotoCase = 99; continue; };
                    if (yych <= ';') { gotoCase = 118; continue; };
                    { gotoCase = 99; continue; };
                } else {
                    if (yych <= '[') {
                        if (yych <= 'Z') { gotoCase = 118; continue; };
                        { gotoCase = 99; continue; };
                    } else {
                        if (yych == ']') { gotoCase = 99; continue; };
                        { gotoCase = 118; continue; };
                    }
                }
            }
        }

        }
    }
}

WebInspector.SourceHTMLTokenizer.prototype.__proto__ = WebInspector.SourceTokenizer.prototype;
