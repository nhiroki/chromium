importScripts('./js-test-pre.js');

var global = this;
global.jsTestIsAsync = true;

description('Test Promise.');

var thisInInit;
var resolver;
var promise = new Promise(function(r) {
  thisInInit = this;
  resolver = r;
});

shouldBeTrue('promise instanceof Promise');
shouldBe('promise.constructor', 'Promise');
shouldBe('thisInInit', 'promise');
shouldBeTrue('resolver instanceof PromiseResolver');
shouldBe('resolver.constructor', 'PromiseResolver');

shouldThrow('new Promise()', '"TypeError: Promise constructor takes a function argument"');
shouldThrow('new Promise(37)', '"TypeError: Promise constructor takes a function argument"');

shouldNotThrow('promise = new Promise(function() { throw Error("foo"); })');
promise.then(undefined, function(result) {
  global.result = result;
  shouldBeEqualToString('result.message', 'foo');
});

new Promise(function(resolver) {
  resolver.fulfill("hello");
  throw Error("foo");
}).then(function(result) {
  global.result = result;
  testPassed('fulfilled');
  shouldBeEqualToString('result', 'hello');
  finishJSTest();
}, function(result) {
  global.result = result;
  testFailed('rejected');
  finishJSTest();
});


