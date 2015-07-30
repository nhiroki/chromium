// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
* @fileoverview Partial definitions for ECMAScript 7. To compile Files.app, some
*     definitions are defined in this file. They should be removed once ES7
*     definitions are ready in closure compiler by default.
* @externs
*/

/**
 * @param {!Object} obj
 * @param {function(string, !Object, string, *=)} callback
 * @param {!Array<string>=} acceptList
 */
Object.observe = function(obj, callback, acceptList) {};
