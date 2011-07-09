/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

function FlakyLayoutTestDetector() {
    this._tests = {};
}

FlakyLayoutTestDetector.prototype = {
    incorporateTestResults: function(buildName, failingTests, tooManyFailures) {
        var newFlakyTests = [];

        if (tooManyFailures) {
            // Something was going horribly wrong during this test run. We shouldn't assume that any
            // passes/failures are due to flakiness.
            return newFlakyTests;
        }

        // Record failing tests.
        for (var testName in failingTests) {
            if (!(testName in this._tests)) {
                this._tests[testName] = {
                    state: this._states.LastSeenFailing,
                    history: [],
                };
            }

            var testData = this._tests[testName];
            testData.history.push({ build: buildName, result: failingTests[testName] });

            if (testData.state === this._states.LastSeenPassing) {
                testData.state = this._states.PossiblyFlaky;
                newFlakyTests.push(testName);
            }
        }

        // Record passing tests.
        for (var testName in this._tests) {
            if (testName in failingTests)
                continue;

            var testData = this._tests[testName];
            testData.history.push({ build: buildName, result: { failureType: 'pass' } });

            if (testData.state === this._states.LastSeenFailing)
                testData.state = this._states.LastSeenPassing;
        }

        return newFlakyTests;
    },

    flakinessExamples: function(testName) {
        if (!(testName in this._tests) || this._tests[testName].state !== this._states.PossiblyFlaky)
            return null;

        var history = this._tests[testName].history;

        var examples = [];
        for (var i = 0; i < history.length - 1; ++i) {
            var thisIsPassing = history[i].result.failureType === 'pass';
            var nextIsPassing = history[i + 1].result.failureType === 'pass';
            if (thisIsPassing === nextIsPassing)
                continue;
            var last = examples.last();
            if (!last || last.build !== history[i].build)
                examples.push(history[i]);
            examples.push(history[i + 1]);
        }

        // The list of examples can get quite long. Instead of showing the whole list, we abbreviate
        // by replacing the middle items with a separator.
        const startAndEndAbbreviatedExamplesCount = 3;
        console.assert(startAndEndAbbreviatedExamplesCount > 1);
        var abbreviatedExamplesToShow = 2 * startAndEndAbbreviatedExamplesCount;
        if (examples.length > abbreviatedExamplesToShow) {
            var examplesBeforeSeparator = examples.slice(0, startAndEndAbbreviatedExamplesCount);
            var examplesAfterSeparator = examples.slice(-startAndEndAbbreviatedExamplesCount);

            // There's no real use in having two "pass" examples in a row immediately next to the
            // separator.
            if (examplesBeforeSeparator[examplesBeforeSeparator.length - 1].result.failureType === 'pass' && examplesBeforeSeparator[examplesBeforeSeparator.length - 2].result.failureType === 'pass')
                examplesBeforeSeparator.splice(examplesBeforeSeparator.length - 1, 1);
            if (examplesAfterSeparator[0].result.failureType === 'pass' && examplesAfterSeparator[1].result.failureType === 'pass')
                examplesAfterSeparator.splice(0, 1);
            examples = examplesBeforeSeparator.concat({ isSeparator: true }, examplesAfterSeparator);
        }

        return examples;
    },

    get possiblyFlakyTests() {
        var self = this;
        return Object.keys(self._tests).filter(function(testName) { return self._tests[testName].state === self._states.PossiblyFlaky });
    },

    _states: {
        LastSeenFailing: 0,
        LastSeenPassing: 1,
        PossiblyFlaky: 2,
    },
};
