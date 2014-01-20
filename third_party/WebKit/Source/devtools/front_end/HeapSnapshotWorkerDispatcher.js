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
 */
WebInspector.HeapSnapshotWorkerDispatcher = function(globalObject, postMessage)
{
    this._objects = [];
    this._global = globalObject;
    this._postMessage = postMessage;
}

WebInspector.HeapSnapshotWorkerDispatcher.prototype = {
    _findFunction: function(name)
    {
        var path = name.split(".");
        var result = this._global;
        for (var i = 0; i < path.length; ++i)
            result = result[path[i]];
        return result;
    },

    /**
     * @param {string} name
     * @param {*} data
     */
    sendEvent: function(name, data)
    {
        this._postMessage({eventName: name, data: data});
    },

    dispatchMessage: function(event)
    {
        var data = /** @type {!WebInspector.HeapSnapshotCommon.WorkerCommand } */(event.data);
        var response = {callId: data.callId};
        try {
            switch (data.disposition) {
                case "create": {
                    var constructorFunction = this._findFunction(data.methodName);
                    this._objects[data.objectId] = new constructorFunction(this);
                    break;
                }
                case "dispose": {
                    delete this._objects[data.objectId];
                    break;
                }
                case "getter": {
                    var object = this._objects[data.objectId];
                    var result = object[data.methodName];
                    response.result = result;
                    break;
                }
                case "factory": {
                    var object = this._objects[data.objectId];
                    var result = object[data.methodName].apply(object, data.methodArguments);
                    if (result)
                        this._objects[data.newObjectId] = result;
                    response.result = !!result;
                    break;
                }
                case "method": {
                    var object = this._objects[data.objectId];
                    response.result = object[data.methodName].apply(object, data.methodArguments);
                    break;
                }
                case "evaluateForTest": {
                    try {
                        response.result = eval(data.source)
                    } catch (e) {
                        response.result = e.toString();
                    }
                    break;
                }
                case "enableAllocationProfiler": {
                    WebInspector.HeapSnapshot.enableAllocationProfiler = true;
                    return;
                }
            }
        } catch (e) {
            response.error = e.toString();
            response.errorCallStack = e.stack;
            if (data.methodName)
                response.errorMethodName = data.methodName;
        }
        this._postMessage(response);
    }
};
