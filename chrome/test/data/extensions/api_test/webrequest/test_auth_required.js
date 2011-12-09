// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Generates a unique authentication URL so each test can run
// without hitting the HTTP authentication cache. Each test
// must use a unique realm, however.
function getURLAuthRequired(realm) {
  return getServerURL('auth-basic/' + realm + '/subpath?realm=' + realm);
}

runTests([
  // onAuthRequired is not a blocking function in this variant.
  function authRequiredNonBlocking() {
    var realm = 'nonblock';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          }
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]}, ["responseHeaders"]);
    navigateAndWait(url);
  },

  // onAuthRequired is a blocking function but takes no action in this variant.
  function authRequiredSyncNoAction() {
    var realm = 'syncnoaction';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          }
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]}, ["blocking", "responseHeaders"]);
    navigateAndWait(url);
  },

  // onAuthRequired is a blocking function that cancels the auth attempt.
  function authRequiredSyncCancelAuth() {
    var realm = 'synccancel';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          },
          retval: {}
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
          retval: {}
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          },
          retval: {cancel: true}
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]},
      ["responseHeaders", "blocking"]);
    navigateAndWait(url);
  },

  // onAuthRequired is a blocking function that sets authentication credentials.
  function authRequiredSyncSetAuth() {
    var realm = 'syncsetauth';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          },
          retval: {}
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
          retval: {}
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          },
          retval: {authCredentials: {username: "foo", password: "secret"}}
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 200,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 200 OK",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 200,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 200 OK",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]},
      ["responseHeaders", "blocking"]);
    navigateAndWait(url);
  },

  // onAuthRequired is an async function but takes no action in this variant.
  function authRequiredAsyncNoAction() {
    var realm = 'asyncnoaction';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          }
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]}, ["asyncBlocking", "responseHeaders"]);
    navigateAndWait(url);
  },

  // onAuthRequired is an async function that cancels the auth attempt.
  function authRequiredAsyncCancelAuth() {
    var realm = 'asynccancel';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          },
          retval: {}
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
          retval: {}
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          },
          retval: {cancel: true}
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 401,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]},
      ["responseHeaders", "asyncBlocking"]);
    navigateAndWait(url);
  },

  // onAuthRequired is an async function that sets authentication credentials.
  function authRequiredAsyncSetAuth() {
    var realm = 'asyncsetauth';
    var url = getURLAuthRequired(realm);
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: url,
            frameUrl: url
          },
          retval: {}
        },
        { label: "onBeforeSendHeaders",
          event: "onBeforeSendHeaders",
          details: {
            url: url,
            // Note: no requestHeaders because we don't ask for them.
          },
          retval: {}
        },
        { label: "onSendHeaders",
          event: "onSendHeaders",
          details: {
            url: url,
          }
        },
        { label: "onHeadersReceived",
          event: "onHeadersReceived",
          details: {
            url: url,
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized"
          }
        },
        { label: "onAuthRequired",
          event: "onAuthRequired",
          details: {
            url: url,
            isProxy: false,
            scheme: "basic",
            realm: realm,
            challenger: {host: testServer, port: testServerPort},
            responseHeadersExist: true,
            statusLine: "HTTP/1.0 401 Unauthorized",
          },
          retval: {authCredentials: {username: "foo", password: "secret"}}
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 200,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 200 OK",
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: url,
            fromCache: false,
            statusCode: 200,
            ip: "127.0.0.1",
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 200 OK",
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onBeforeSendHeaders", "onSendHeaders",
         "onHeadersReceived", "onAuthRequired", "onResponseStarted",
         "onCompleted"]
      ],
      {urls: ["<all_urls>"]},
      ["responseHeaders", "asyncBlocking"]);
    navigateAndWait(url);
  },
]);
