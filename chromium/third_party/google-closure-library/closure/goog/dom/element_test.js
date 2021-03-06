// Copyright 2019 The Closure Library Authors. All Rights Reserved.
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

goog.module('goog.dom.elementTest');
goog.setTestOnly();

const DomHelper = goog.require('goog.dom.DomHelper');
const TagName = goog.require('goog.dom.TagName');
const element = goog.require('goog.dom.element');
const testSuite = goog.require('goog.testing.testSuite');

/**
 * @param {function():!Document} getDocument
 * @return {!Object<string, function()>}
 */
const getTestsObject = (getDocument) => {
  let domHelper;
  let text;
  let div;
  let a;
  let table;
  let svg;

  return {
    setUp() {
      const doc = getDocument();
      domHelper = new DomHelper(doc);
      text = domHelper.createTextNode('foo');
      div = domHelper.createElement(TagName.DIV);
      a = domHelper.createElement(TagName.A);
      table = domHelper.createElement(TagName.TABLE);
      if (doc.createElementNS) {
        svg = doc.createElementNS('http://www.w3.org/2000/svg', 'svg');
      }
    },

    testIsElement() {
      assertFalse(element.isElement(text));

      assertTrue(element.isElement(div));
      assertTrue(element.isElement(a));
      assertTrue(element.isElement(table));

      if (svg) {
        assertTrue(element.isElement(svg));
      }
    },

    testIsHtmlElement() {
      assertFalse(element.isHtmlElement(text));

      assertTrue(element.isHtmlElement(div));
      assertTrue(element.isHtmlElement(a));
      assertTrue(element.isHtmlElement(table));

      if (svg) {
        assertFalse(element.isHtmlElement(svg));
      }
    },

    testIsHtmlElementOfType() {
      assertFalse(element.isHtmlElementOfType(text, TagName.DIV));

      assertTrue(element.isHtmlElementOfType(div, TagName.DIV));
      assertFalse(element.isHtmlElementOfType(div, TagName.A));

      assertTrue(element.isHtmlElementOfType(a, TagName.A));
      assertFalse(element.isHtmlElementOfType(a, TagName.DIV));

      assertTrue(element.isHtmlElementOfType(table, TagName.TABLE));
      assertFalse(element.isHtmlElementOfType(table, TagName.DIV));

      if (svg) {
        assertFalse(element.isHtmlElementOfType(svg, TagName.DIV));
      }
    },

    testIsHtmlAnchorElement() {
      assertTrue(element.isHtmlAnchorElement(a));

      assertFalse(element.isHtmlAnchorElement(div));
      assertFalse(element.isHtmlAnchorElement(table));

      if (svg) {
        assertFalse(element.isHtmlAnchorElement(svg));
      }
    },
  };
};

/**
 * Gets a secondary document to help expose differences in DOM ownership.
 * @return {!Document}
 */
const getRemoteDocument = () => {
  const domHelper = new DomHelper();
  const iframe = domHelper.createElement(TagName.IFRAME);
  domHelper.appendChild(document.body, iframe);
  const doc = iframe.contentWindow.document;
  domHelper.removeNode(iframe);
  return doc;
};

testSuite({
  testLocalDocument: getTestsObject(() => document),
  testRemoteDocument: getTestsObject(getRemoteDocument),
});
