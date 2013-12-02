function postResult(result, actual, expected)
{
    var message = result ? "PASS" : "FAIL";
    message += ": worker: " + actual + " is ";
    if (!result)
        message += "not ";
    message += expected;
    postMessage(message);
}

function testPassed(message)
{
    postMessage("PASS: " + message);
}

function testFailed(message)
{
    postMessage("FAIL: " + message);
}

function debug(message)
{
    postMessage(message);
}

function shouldBe(actual, expected)
{
    var _result = eval(actual + " == " + expected);
    postResult(_result, actual, expected);
}

function shouldBeTrue(actual)
{
    shouldBe(actual, "true");
}

function shouldBeFalse(actual)
{
    shouldBe(actual, "false");
}

var exceptionName;
var exceptionProto;
var closeEvent;
var code;
var reason;
var result;
var invalidAccessErr = "InvalidAccessError";
var syntaxErr = "SyntaxError";
var normalClosure = 1000;
var abnormalClosure = 1006;
var url = "ws://127.0.0.1:8880/close";
var ws;
var testId;

var codeTestCodes = [
    999, 1001, 2999, 5000, 65536 + 1000, 0x100000000 + 1000, 2999.9, NaN, "0", "100", 1/0, -1/0, 0/0,
    1000.0
];

var reasonTestReasons = [
    "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234", // 124 Byte
    "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012\u00a9", // length is 123, but 124 Byte in UTF-8
    "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123", // 123 Byte
];

var reasonTestResults = [
    false,
    false,
    true,
];

function handleOpen()
{
    testFailed("handleOpen() was called.");
};

function handleError()
{
    testFailed("handleError() was called.");
};

function handleClose()
{
    testFailed("handleClose() was called.");
};

function handleMessage()
{
    testFailed("handleMessage() was called.");
};

function setDefaultHandlers(ws)
{
    ws.onopen = handleOpen;
    ws.onerror = handleError;
    ws.onclose = handleClose;
    ws.onmessage = handleMessage;
}

function runCodeTest()
{
    ws = new WebSocket(url);
    setDefaultHandlers(ws);
    for (var id = 0; id < codeTestCodes.length; id++) {
        if (codeTestCodes[id] != normalClosure) {
            debug("Invalid code test: " + id);
            ws.onclose = handleClose;
        } else {
            ws.onclose = function (e)
            {
                debug("runCodeTest: onclose().");
                closeEvent = e;
                shouldBe("closeEvent.code", "abnormalClosure");
                if (closeEvent.code == abnormalClosure)
                    runInvalidStringTest();
            };
            ws.onerror = function ()
            {
                testPassed("onerror() was called.");
            };
        }
        try {
            ws.close(codeTestCodes[id]);
        } catch (e) {
            debug("Code " + codeTestCodes[id] + " must cause " + invalidAccessErr + '.');
            exceptionName = e.name;
            exceptionProto = Object.getPrototypeOf(e);
            shouldBe("exceptionName", "invalidAccessErr");
        }
    }
}

function runInvalidStringTest()
{
    // FIXME: unpaired surrogates throw SyntaxError
    debug("Skip invalid string test.");
    runReasonTest();
}

function runReasonTest()
{
    ws = new WebSocket(url);
    setDefaultHandlers(ws);
    for (var id = 0; id < reasonTestReasons.length; id++) {
        debug("Reason test: " + id);
        if (!reasonTestResults[id]) {
            debug("  with invalid reason: " + reasonTestReasons[id]);
            ws.onclose = handleClose;
        } else {
            ws.onclose = function (e)
            {
                debug("runReasonTest: onclose().");
                closeEvent = e;
                shouldBe("closeEvent.code", "abnormalClosure");
                if (closeEvent.code == abnormalClosure)
                    runCodeAndReasonTest();
            };
            ws.onerror = function ()
            {
                testPassed("onerror() was called.");
            };
        }
        try {
            ws.close(normalClosure, reasonTestReasons[id]);
        } catch (e) {
            debug("Reason " + reasonTestReasons[id] + " must cause " + syntaxErr + '.');
            result = reasonTestResults[id];
            exceptionName = e.name;
            exceptionProto = Object.getPrototypeOf(e);
            shouldBeFalse("result");
            shouldBe("exceptionName", "syntaxErr");
        }
    }
}

function runCodeAndReasonTest()
{
    var codes = [
        1000,
        3000,
        4000,
        4999
    ];
    var reasons = [
        "OK, Bye!",
        "3000",
        "code is 4000",
        "\u00a9 Google"
    ];
    (function test (id) {
        debug("Code and reason test: " + id);
        ws = new WebSocket(url);
        setDefaultHandlers(ws);
        ws.onopen = function ()
        {
            ws.close(codes[id], reasons[id]);
        };
        ws.onclose = function (e)
        {
            closeEvent = e;
            code = codes[id];
            reason = reasons[id];
            debug("Code and reason must be");
            debug("  code  : " + code);
            debug("  reason: " + reason);
            shouldBeTrue("closeEvent.wasClean");
            shouldBe("closeEvent.code", "code");
            shouldBe("closeEvent.reason", "reason");
            if (++id != codes.length)
                test(id);
            else
                postMessage("DONE");
        };
    })(0);
}

runCodeTest();
