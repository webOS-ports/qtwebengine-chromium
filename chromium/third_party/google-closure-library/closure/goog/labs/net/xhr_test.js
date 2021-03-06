// Copyright 2011 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.module('goog.labs.net.xhrTest');
goog.setTestOnly('goog.labs.net.xhrTest');

const DefaultXmlHttpFactory = goog.require('goog.net.DefaultXmlHttpFactory');
const EventType = goog.require('goog.events.EventType');
const GoogPromise = goog.require('goog.Promise');
const MockClock = goog.require('goog.testing.MockClock');
const TestCase = goog.require('goog.testing.TestCase');
const WrapperXmlHttpFactory = goog.require('goog.net.WrapperXmlHttpFactory');
const XhrLike = goog.require('goog.net.XhrLike');
const XmlHttp = goog.require('goog.net.XmlHttp');
const events = goog.require('goog.events');
const testSuite = goog.require('goog.testing.testSuite');
const userAgent = goog.require('goog.userAgent');
const xhr = goog.require('goog.labs.net.xhr');
/** @suppress {extraRequire} Needed for G_testrunner */
goog.require('goog.testing.jsunit');

/** Path to a small download target used for testing binary requests. */
const TEST_IMAGE = 'testdata/cleardot.gif';


/** The expected bytes of the test image. */
const TEST_IMAGE_BYTES = [
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80,
  0xFF, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x21, 0xF9, 0x04,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3B
];

/**
 * @param {number} status HTTP status code.
 * @param {string=} opt_responseText
 * @param {number=} opt_latency
 *     Milliseconds between sending a request and receiving the response.
 * @return {!XhrLike}
 *     The XHR stub that will be returned from the factory.
 */
function stubXhrToReturn(status, opt_responseText, opt_latency) {
  if (opt_latency != null) {
    mockClock = new MockClock(true);
  }

  const stubXhr = {
    sent: false,
    aborted: false,
    status: 0,
    headers: {},
    open: function(method, url, async) {
      this.method = method;
      this.url = url;
      this.async = async;
    },
    setRequestHeader: function(key, value) {
      this.headers[key] = value;
    },
    overrideMimeType: function(mimeType) {
      this.mimeType = mimeType;
    },
    abort: function() {
      this.aborted = true;
      this.load(0);
    },
    send: function(data) {
      this.data = data;
      this.sent = true;

      // Fulfill the send asynchronously, or possibly with the MockClock.
      window.setTimeout(goog.bind(this.load, this, status), opt_latency || 0);
      if (mockClock) {
        mockClock.tick(opt_latency);
      }
    },
    load: function(status) {
      this.status = status;
      if (opt_responseText != null) {
        this.responseText = opt_responseText;
      }
      this.readyState = 4;
      if (this.onreadystatechange) this.onreadystatechange();
    }
  };

  stubXmlHttpWith(stubXhr);
  return stubXhr;
}

/**
 * @param {!Error} err Error to be thrown when sending the stub XHR.
 */
function stubXhrToThrow(err) {
  stubXmlHttpWith(buildThrowingStubXhr(err));
}

/**
 * @param {!Error} err Error to be thrown when sending this stub XHR.
 * @return {!XhrLike}
 *     The XHR stub that will be returned from the factory.
 */
function buildThrowingStubXhr(err) {
  return {
    sent: false,
    aborted: false,
    status: 0,
    headers: {},
    open: function(method, url, async) {
      this.method = method;
      this.url = url;
      this.async = async;
    },
    setRequestHeader: function(key, value) { this.headers[key] = value; },
    overrideMimeType: function(mimeType) { this.mimeType = mimeType; },
    send: function(data) { throw err; }
  };
}

/**
 * Replace XmlHttp with a function that returns a stub XHR.
 * @param {!XhrLike.OrNative} stubXhr
 */
function stubXmlHttpWith(stubXhr) {
  XmlHttp.setGlobalFactory({
    createInstance() {
      return stubXhr;
    },
    getOptions() {
      return {};
    }
  });
}

let mockClock;


/**
 * Tests whether the test was loaded from a file: protocol. Tests that use a
 * real network request cannot be run from the local file system due to
 * cross-origin restrictions, but will run if the tests are hosted on a server.
 * A log message is added to the test case to warn users that the a test was
 * skipped.
 *
 * @return {boolean} Whether the test is running on a local file system.
 */
function isRunningLocally() {
  if (window.location.protocol == 'file:') {
    const testCase = goog.global['G_testRunner'].testCase;
    testCase.saveMessage('Test skipped while running on local file system.');
    return true;
  }
  return false;
}

testSuite({
  setUpPage() {
    TestCase.getActiveTestCase().promiseTimeout = 10000;  // 10s
  },

  tearDown() {
    if (mockClock) {
      mockClock.dispose();
      mockClock = null;
    }

    XmlHttp.setGlobalFactory(new DefaultXmlHttpFactory());
  },

  testSimpleRequest() {
    if (isRunningLocally()) return;

    return xhr.send('GET', 'testdata/xhr_test_text.data').then(function(xhr) {
      assertEquals('Just some data.', xhr.responseText);
      assertEquals(200, xhr.status);
    });
  },

  testGetText() {
    if (isRunningLocally()) return;

    return xhr.get('testdata/xhr_test_text.data').then(function(responseText) {
      assertEquals('Just some data.', responseText);
    });
  },

  testGetTextWithJson() {
    if (isRunningLocally()) return;

    return xhr.get('testdata/xhr_test_json.data').then(function(responseText) {
      assertEquals('while(1);\n{"stat":"ok","count":12345}\n', responseText);
    });
  },

  testPostText() {
    if (isRunningLocally()) return;

    return xhr.post('testdata/xhr_test_text.data', 'post-data')
        .then(function(responseText) {
          // No good way to test post-data gets transported.
          assertEquals('Just some data.', responseText);
        });
  },

  testGetJson() {
    if (isRunningLocally()) return;

    return xhr
        .getJson('testdata/xhr_test_json.data', {xssiPrefix: 'while(1);\n'})
        .then(function(responseObj) {
          assertEquals('ok', responseObj['stat']);
          assertEquals(12345, responseObj['count']);
        });
  },

  testGetBlob() {
    if (isRunningLocally()) return;

    // IE9 and earlier do not support blobs.
    if (!('Blob' in goog.global)) {
      const err = assertThrows(function() {
        xhr.getBlob(TEST_IMAGE);
      });
      assertEquals(
          'Assertion failed: getBlob is not supported in this browser.',
          err.message);
      return;
    }

    const options = {withCredentials: true};

    return xhr.getBlob(TEST_IMAGE, options)
        .then(function(blob) {
          const reader = new FileReader();
          return new GoogPromise(function(resolve, reject) {
            events.listenOnce(reader, EventType.LOAD, resolve);
            reader.readAsArrayBuffer(blob);
          });
        })
        .then(function(e) {
          assertElementsEquals(
              TEST_IMAGE_BYTES, new Uint8Array(e.target.result));
          assertObjectEquals(
              'input options should not have mutated.', {withCredentials: true},
              options);
        });
  },

  testGetBytes() {
    if (isRunningLocally()) return;

    // IE8 requires a VBScript fallback to read the bytes from the response.
    if (userAgent.IE && !userAgent.isDocumentMode(9)) {
      const err = assertThrows(function() {
        xhr.getBytes(TEST_IMAGE);
      });
      assertEquals(
          'Assertion failed: getBytes is not supported in this browser.',
          err.message);
      return;
    }

    const options = {withCredentials: true};

    return xhr.getBytes(TEST_IMAGE).then(function(bytes) {
      assertElementsEquals(TEST_IMAGE_BYTES, bytes);
      assertObjectEquals(
          'input options should not have mutated.', {withCredentials: true},
          options);
    });
  },

  testSerialRequests() {
    if (isRunningLocally()) return;

    return xhr.get('testdata/xhr_test_text.data')
        .then(function(response) {
          return xhr.getJson(
              'testdata/xhr_test_json.data', {xssiPrefix: 'while(1);\n'});
        })
        .then(function(responseObj) {
          // Data that comes through to callbacks should be from the 2nd
          // request.
          assertEquals('ok', responseObj['stat']);
          assertEquals(12345, responseObj['count']);
        });
  },

  testBadUrlDetectedAsError() {
    if (isRunningLocally()) return;

    return xhr.getJson('unknown-file.dat')
        .then(fail /* opt_onFulfilled */, function(err) {
          assertTrue(
              'Error should be an HTTP error', err instanceof xhr.HttpError);
          assertEquals(404, err.status);
          assertNotNull(err.xhr);
        });
  },

  testBadOriginTriggersOnErrorHandler() {
    if (userAgent.EDGE) return;  // failing b/62677027

    return xhr.get('http://www.google.com')
        .then(
            function() {
              fail(
                  'XHR to http://www.google.com should\'ve failed due to ' +
                  'same-origin policy.');
            } /* opt_onFulfilled */,
            function(err) {
              // In IE this will be a goog.labs.net.xhr.Error since it is thrown
              //  when calling xhr.open(), other browsers will raise an
              //  HttpError.
              assertTrue(
                  'Error should be an xhr error', err instanceof xhr.Error);
              assertNotNull(err.xhr);
            });
  },

  //============================================================================
  // The following tests use a stubbed out XMLHttpRequest.
  //============================================================================

  testSendNoOptions() {
    let called = false;
    stubXhrToReturn(200);
    assertFalse('Callback should not yet have been called', called);
    return xhr.send('GET', 'test-url', null).then(function(stubXhr) {
      called = true;
      assertEquals('GET', stubXhr.method);
      assertEquals('test-url', stubXhr.url);
    });
  },

  testSendPostSetsDefaultHeader() {
    stubXhrToReturn(200);
    return xhr.send('POST', 'test-url', null).then(function(stubXhr) {
      assertEquals('POST', stubXhr.method);
      assertEquals('test-url', stubXhr.url);
      assertEquals(
          'application/x-www-form-urlencoded;charset=utf-8',
          stubXhr.headers['Content-Type']);
    });
  },

  testSendPostDoesntSetHeaderWithFormData() {
    if (!goog.global['FormData']) {
      return;
    }
    const formData = new goog.global['FormData']();
    formData.append('name', 'value');

    stubXhrToReturn(200);
    return xhr.send('POST', 'test-url', formData).then(function(stubXhr) {
      assertEquals('POST', stubXhr.method);
      assertEquals('test-url', stubXhr.url);
      assertEquals(undefined, stubXhr.headers['Content-Type']);
    });
  },

  testSendPostHeaders() {
    stubXhrToReturn(200);
    return xhr
        .send(
            'POST', 'test-url', null,
            {headers: {'Content-Type': 'text/plain', 'X-Made-Up': 'FooBar'}})
        .then(function(stubXhr) {
          assertEquals('POST', stubXhr.method);
          assertEquals('test-url', stubXhr.url);
          assertEquals('text/plain', stubXhr.headers['Content-Type']);
          assertEquals('FooBar', stubXhr.headers['X-Made-Up']);
        });
  },

  testSendPostHeadersWithFormData() {
    if (!goog.global['FormData']) {
      return;
    }
    const formData = new goog.global['FormData']();
    formData.append('name', 'value');

    stubXhrToReturn(200);
    return xhr
        .send(
            'POST', 'test-url', formData,
            {headers: {'Content-Type': 'text/plain', 'X-Made-Up': 'FooBar'}})
        .then(function(stubXhr) {
          assertEquals('POST', stubXhr.method);
          assertEquals('test-url', stubXhr.url);
          assertEquals('text/plain', stubXhr.headers['Content-Type']);
          assertEquals('FooBar', stubXhr.headers['X-Made-Up']);
        });
  },

  testSendNullPostHeaders() {
    stubXhrToReturn(200);
    return xhr
        .send('POST', 'test-url', null, {
          headers:
              {'Content-Type': null, 'X-Made-Up': 'FooBar', 'Y-Made-Up': null}
        })
        .then(function(stubXhr) {
          assertEquals('POST', stubXhr.method);
          assertEquals('test-url', stubXhr.url);
          assertEquals(undefined, stubXhr.headers['Content-Type']);
          assertEquals('FooBar', stubXhr.headers['X-Made-Up']);
          assertEquals(undefined, stubXhr.headers['Y-Made-Up']);
        });
  },

  testSendNullPostHeadersWithFormData() {
    if (!goog.global['FormData']) {
      return;
    }
    const formData = new goog.global['FormData']();
    formData.append('name', 'value');

    stubXhrToReturn(200);
    return xhr
        .send('POST', 'test-url', formData, {
          headers:
              {'Content-Type': null, 'X-Made-Up': 'FooBar', 'Y-Made-Up': null}
        })
        .then(function(stubXhr) {
          assertEquals('POST', stubXhr.method);
          assertEquals('test-url', stubXhr.url);
          assertEquals(undefined, stubXhr.headers['Content-Type']);
          assertEquals('FooBar', stubXhr.headers['X-Made-Up']);
          assertEquals(undefined, stubXhr.headers['Y-Made-Up']);
        });
  },

  testSendWithCredentials() {
    stubXhrToReturn(200);
    return xhr.send('POST', 'test-url', null, {withCredentials: true})
        .then(function(stubXhr) {
          assertTrue('XHR should have been sent', stubXhr.sent);
          assertTrue(stubXhr.withCredentials);
        });
  },

  testSendWithMimeType() {
    stubXhrToReturn(200);
    return xhr.send('POST', 'test-url', null, {mimeType: 'text/plain'})
        .then(function(stubXhr) {
          assertTrue('XHR should have been sent', stubXhr.sent);
          assertEquals('text/plain', stubXhr.mimeType);
        });
  },

  testSendWithHttpError() {
    stubXhrToReturn(500);
    return xhr.send('POST', 'test-url', null)
        .then(fail /* opt_onResolved */, function(err) {
          assertTrue(err instanceof xhr.HttpError);
          assertTrue(err.xhr.sent);
          assertEquals(500, err.status);
        });
  },

  testSendWithTimeoutNotHit() {
    stubXhrToReturn(200, null /* opt_responseText */, 1400 /* opt_latency */);
    return xhr.send('POST', 'test-url', null, {timeoutMs: 1500})
        .then(function(stubXhr) {
          assertTrue(mockClock.getTimeoutsMade() > 0);
          assertTrue('XHR should have been sent', stubXhr.sent);
          assertFalse('XHR should not have been aborted', stubXhr.aborted);
        });
  },

  testSendWithTimeoutHit() {
    stubXhrToReturn(200, null /* opt_responseText */, 50 /* opt_latency */);
    return xhr.send('POST', 'test-url', null, {timeoutMs: 50})
        .then(fail /* opt_onResolved */, function(err) {
          assertTrue('XHR should have been sent', err.xhr.sent);
          assertTrue('XHR should have been aborted', err.xhr.aborted);
          assertTrue(err instanceof xhr.TimeoutError);
        });
  },

  testCancelRequest() {
    const request = stubXhrToReturn(200);
    const promise =
        xhr.send('GET', 'test-url')
            .then(fail /* opt_onResolved */, function(error) {
              assertTrue(error instanceof GoogPromise.CancellationError);
              assertTrue('XHR should have been aborted', request.aborted);
              return null;  // Return a non-error value for the test runner.
            });
    promise.cancel();
    return promise;
  },

  testGetJson_stubbed() {
    stubXhrToReturn(200, '{"a": 1, "b": 2}');
    xhr.getJson('test-url').then(function(responseObj) {
      assertObjectEquals({a: 1, b: 2}, responseObj);
    });
  },

  testGetJsonWithXssiPrefix() {
    stubXhrToReturn(200, 'while(1);\n{"a": 1, "b": 2}');
    return xhr.getJson('test-url', {xssiPrefix: 'while(1);\n'})
        .then(function(responseObj) {
          assertObjectEquals({a: 1, b: 2}, responseObj);
        });
  },

  testSendWithClientException() {
    stubXhrToThrow(new Error('CORS XHR with file:// schemas not allowed.'));
    return xhr.send('POST', 'file://test-url', null)
        .then(fail /* opt_onResolved */, function(err) {
          assertFalse('XHR should not have been sent', err.xhr.sent);
          assertTrue(err instanceof Error);
          assertTrue(
              /CORS XHR with file:\/\/ schemas not allowed./.test(err.message));
        });
  },

  testSendWithFactory() {
    stubXhrToReturn(200);
    const options = {
      xmlHttpFactory: new WrapperXmlHttpFactory(
          goog.partial(buildThrowingStubXhr, new Error('Bad factory')),
          XmlHttp.getOptions)
    };
    return xhr.send('POST', 'file://test-url', null, options)
        .then(fail /* opt_onResolved */, function(err) {
          assertTrue(err instanceof Error);
        });
  },

});
