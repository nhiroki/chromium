﻿#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endregion

using System;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using UnitTest.Issues.TestProtos;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf
{
    /// <summary>
    /// Tests for the JSON formatter. Note that in these tests, double quotes are replaced with apostrophes
    /// for the sake of readability (embedding \" everywhere is painful). See the AssertJson method for details.
    /// </summary>
    public class JsonFormatterTest
    {
        [Test]
        public void DefaultValues_WhenOmitted()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(formatDefaultValues: false));

            AssertJson("{ }", formatter.Format(new ForeignMessage()));
            AssertJson("{ }", formatter.Format(new TestAllTypes()));
            AssertJson("{ }", formatter.Format(new TestMap()));
        }

        [Test]
        public void DefaultValues_WhenIncluded()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(formatDefaultValues: true));
            AssertJson("{ 'c': 0 }", formatter.Format(new ForeignMessage()));
        }

        [Test]
        public void AllSingleFields()
        {
            var message = new TestAllTypes
            {
                SingleBool = true,
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleDouble = 23.5,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleFloat = 12.25f,
                SingleForeignEnum = ForeignEnum.FOREIGN_BAR,
                SingleForeignMessage = new ForeignMessage { C = 10 },
                SingleImportEnum = ImportEnum.IMPORT_BAZ,
                SingleImportMessage = new ImportMessage { D = 20 },
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.FOO,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 35 },
                SinglePublicImportMessage = new PublicImportMessage { E = 54 },
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleString = "test\twith\ttabs",
                SingleUint32 = uint.MaxValue,
                SingleUint64 = ulong.MaxValue,
            };
            var actualText = JsonFormatter.Default.Format(message);

            // Fields in numeric order
            var expectedText = "{ " +
                "'singleInt32': 100, " +
                "'singleInt64': '3210987654321', " +
                "'singleUint32': 4294967295, " +
                "'singleUint64': '18446744073709551615', " +
                "'singleSint32': -456, " +
                "'singleSint64': '-12345678901235', " +
                "'singleFixed32': 23, " +
                "'singleFixed64': '1234567890123', " +
                "'singleSfixed32': -123, " +
                "'singleSfixed64': '-12345678901234', " +
                "'singleFloat': 12.25, " +
                "'singleDouble': 23.5, " +
                "'singleBool': true, " +
                "'singleString': 'test\\twith\\ttabs', " +
                "'singleBytes': 'AQIDBA==', " +
                "'singleNestedMessage': { 'bb': 35 }, " +
                "'singleForeignMessage': { 'c': 10 }, " +
                "'singleImportMessage': { 'd': 20 }, " +
                "'singleNestedEnum': 'FOO', " +
                "'singleForeignEnum': 'FOREIGN_BAR', " +
                "'singleImportEnum': 'IMPORT_BAZ', " +
                "'singlePublicImportMessage': { 'e': 54 }" +
                " }";
            AssertJson(expectedText, actualText);
        }

        [Test]
        public void RepeatedField()
        {
            AssertJson("{ 'repeatedInt32': [ 1, 2, 3, 4, 5 ] }",
                JsonFormatter.Default.Format(new TestAllTypes { RepeatedInt32 = { 1, 2, 3, 4, 5 } }));
        }

        [Test]
        public void MapField_StringString()
        {
            AssertJson("{ 'mapStringString': { 'with spaces': 'bar', 'a': 'b' } }",
                JsonFormatter.Default.Format(new TestMap { MapStringString = { { "with spaces", "bar" }, { "a", "b" } } }));
        }

        [Test]
        public void MapField_Int32Int32()
        {
            // The keys are quoted, but the values aren't.
            AssertJson("{ 'mapInt32Int32': { '0': 1, '2': 3 } }",
                JsonFormatter.Default.Format(new TestMap { MapInt32Int32 = { { 0, 1 }, { 2, 3 } } }));
        }

        [Test]
        public void MapField_BoolBool()
        {
            // The keys are quoted, but the values aren't.
            AssertJson("{ 'mapBoolBool': { 'false': true, 'true': false } }",
                JsonFormatter.Default.Format(new TestMap { MapBoolBool = { { false, true }, { true, false } } }));
        }

        [TestCase(1.0, "1")]
        [TestCase(double.NaN, "'NaN'")]
        [TestCase(double.PositiveInfinity, "'Infinity'")]
        [TestCase(double.NegativeInfinity, "'-Infinity'")]
        public void DoubleRepresentations(double value, string expectedValueText)
        {
            var message = new TestAllTypes { SingleDouble = value };
            string actualText = JsonFormatter.Default.Format(message);
            string expectedText = "{ 'singleDouble': " + expectedValueText + " }";
            AssertJson(expectedText, actualText);
        }

        [Test]
        public void UnknownEnumValueOmitted_SingleField()
        {
            var message = new TestAllTypes { SingleForeignEnum = (ForeignEnum) 100 };
            AssertJson("{ }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void UnknownEnumValueOmitted_RepeatedField()
        {
            var message = new TestAllTypes { RepeatedForeignEnum = { ForeignEnum.FOREIGN_BAZ, (ForeignEnum) 100, ForeignEnum.FOREIGN_FOO } };
            AssertJson("{ 'repeatedForeignEnum': [ 'FOREIGN_BAZ', 'FOREIGN_FOO' ] }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void UnknownEnumValueOmitted_MapField()
        {
            // This matches the C++ behaviour.
            var message = new TestMap { MapInt32Enum = { { 1, MapEnum.MAP_ENUM_FOO }, { 2, (MapEnum) 100 }, { 3, MapEnum.MAP_ENUM_BAR } } };
            AssertJson("{ 'mapInt32Enum': { '1': 'MAP_ENUM_FOO', '3': 'MAP_ENUM_BAR' } }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void UnknownEnumValueOmitted_RepeatedField_AllEntriesUnknown()
        {
            // *Maybe* we should hold off on writing the "[" until we find that we've got at least one value to write...
            // but this is what happens at the moment, and it doesn't seem too awful.
            var message = new TestAllTypes { RepeatedForeignEnum = { (ForeignEnum) 200, (ForeignEnum) 100 } };
            AssertJson("{ 'repeatedForeignEnum': [ ] }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void NullValueForMessage()
        {
            var message = new TestMap { MapInt32ForeignMessage = { { 10, null } } };
            AssertJson("{ 'mapInt32ForeignMessage': { '10': null } }", JsonFormatter.Default.Format(message));
        }

        [Test]
        [TestCase("a\u17b4b", "a\\u17b4b")] // Explicit
        [TestCase("a\u0601b", "a\\u0601b")] // Ranged
        [TestCase("a\u0605b", "a\u0605b")] // Passthrough (note lack of double backslash...)
        public void SimpleNonAscii(string text, string encoded)
        {
            var message = new TestAllTypes { SingleString = text };
            AssertJson("{ 'singleString': '" + encoded + "' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void SurrogatePairEscaping()
        {
            var message = new TestAllTypes { SingleString = "a\uD801\uDC01b" };
            AssertJson("{ 'singleString': 'a\\ud801\\udc01b' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void InvalidSurrogatePairsFail()
        {
            // Note: don't use TestCase for these, as the strings can't be reliably represented 
            // See http://codeblog.jonskeet.uk/2014/11/07/when-is-a-string-not-a-string/

            // Lone low surrogate
            var message = new TestAllTypes { SingleString = "a\uDC01b" };
            Assert.Throws<ArgumentException>(() => JsonFormatter.Default.Format(message));

            // Lone high surrogate
            message = new TestAllTypes { SingleString = "a\uD801b" };
            Assert.Throws<ArgumentException>(() => JsonFormatter.Default.Format(message));
        }

        [Test]
        [TestCase("foo_bar", "fooBar")]
        [TestCase("bananaBanana", "bananaBanana")]
        [TestCase("BANANABanana", "bananaBanana")]
        public void ToCamelCase(string original, string expected)
        {
            Assert.AreEqual(expected, JsonFormatter.ToCamelCase(original));
        }

        [Test]
        [TestCase(null, "{ }")]
        [TestCase("x", "{ 'fooString': 'x' }")]
        [TestCase("", "{ 'fooString': '' }")]
        [TestCase(null, "{ }")]
        public void Oneof(string fooStringValue, string expectedJson)
        {
            var message = new TestOneof();
            if (fooStringValue != null)
            {
                message.FooString = fooStringValue;
            }

            // We should get the same result both with and without "format default values".
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false));
            AssertJson(expectedJson, formatter.Format(message));
            formatter = new JsonFormatter(new JsonFormatter.Settings(true));
            AssertJson(expectedJson, formatter.Format(message));
        }

        [Test]
        public void WrapperFormatting_Single()
        {
            // Just a few examples, handling both classes and value types, and
            // default vs non-default values
            var message = new TestWellKnownTypes
            {
                Int64Field = 10,
                Int32Field = 0,
                BytesField = ByteString.FromBase64("ABCD"),
                StringField = ""
            };
            var expectedJson = "{ 'int64Field': '10', 'int32Field': 0, 'stringField': '', 'bytesField': 'ABCD' }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
        }

        [Test]
        public void WrapperFormatting_IncludeNull()
        {
            // The actual JSON here is very large because there are lots of fields. Just test a couple of them.
            var message = new TestWellKnownTypes { Int32Field = 10 };
            var formatter = new JsonFormatter(new JsonFormatter.Settings(true));
            var actualJson = formatter.Format(message);
            Assert.IsTrue(actualJson.Contains("\"int64Field\": null"));
            Assert.IsFalse(actualJson.Contains("\"int32Field\": null"));
        }

        [Test]
        public void OutputIsInNumericFieldOrder_NoDefaults()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false));
            var message = new TestJsonFieldOrdering { PlainString = "p1", PlainInt32 = 2 };
            AssertJson("{ 'plainString': 'p1', 'plainInt32': 2 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1Int32 = 5, O2String = "o2", PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o2String': 'o2', 'plainInt32': 10, 'o1Int32': 5 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1String = "", O2Int32 = 0, PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o1String': '', 'plainInt32': 10, 'o2Int32': 0 }", formatter.Format(message));
        }

        [Test]
        public void OutputIsInNumericFieldOrder_WithDefaults()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(true));
            var message = new TestJsonFieldOrdering();
            AssertJson("{ 'plainString': '', 'plainInt32': 0 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1Int32 = 5, O2String = "o2", PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o2String': 'o2', 'plainInt32': 10, 'o1Int32': 5 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1String = "", O2Int32 = 0, PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o1String': '', 'plainInt32': 10, 'o2Int32': 0 }", formatter.Format(message));
        }

        [Test]
        public void TimestampStandalone()
        {
            Assert.AreEqual("1970-01-01T00:00:00Z", new Timestamp().ToString());
            Assert.AreEqual("1970-01-01T00:00:00.100Z", new Timestamp { Nanos = 100000000 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.120Z", new Timestamp { Nanos = 120000000 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123Z", new Timestamp { Nanos = 123000000 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123400Z", new Timestamp { Nanos = 123400000 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123450Z", new Timestamp { Nanos = 123450000 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123456Z", new Timestamp { Nanos = 123456000 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123456700Z", new Timestamp { Nanos = 123456700 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123456780Z", new Timestamp { Nanos = 123456780 }.ToString());
            Assert.AreEqual("1970-01-01T00:00:00.123456789Z", new Timestamp { Nanos = 123456789 }.ToString());

            // One before and one after the Unix epoch
            Assert.AreEqual("1673-06-19T12:34:56Z",
                new DateTime(1673, 6, 19, 12, 34, 56, DateTimeKind.Utc).ToTimestamp().ToString());
            Assert.AreEqual("2015-07-31T10:29:34Z",
                new DateTime(2015, 7, 31, 10, 29, 34, DateTimeKind.Utc).ToTimestamp().ToString());
        }

        [Test]
        public void TimestampField()
        {
            var message = new TestWellKnownTypes { TimestampField = new Timestamp() };
            AssertJson("{ 'timestampField': '1970-01-01T00:00:00Z' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        [TestCase(0, 0, "0s")]
        [TestCase(1, 0, "1s")]
        [TestCase(-1, 0, "-1s")]
        [TestCase(0, 100000000, "0.100s")]
        [TestCase(0, 120000000, "0.120s")]
        [TestCase(0, 123000000, "0.123s")]
        [TestCase(0, 123400000, "0.123400s")]
        [TestCase(0, 123450000, "0.123450s")]
        [TestCase(0, 123456000, "0.123456s")]
        [TestCase(0, 123456700, "0.123456700s")]
        [TestCase(0, 123456780, "0.123456780s")]
        [TestCase(0, 123456789, "0.123456789s")]
        [TestCase(0, -100000000, "-0.100s")]
        [TestCase(1, 100000000, "1.100s")]
        [TestCase(-1, -100000000, "-1.100s")]
        // Non-normalized examples
        [TestCase(1, 2123456789, "3.123456789s")]
        [TestCase(1, -100000000, "0.900s")]
        public void DurationStandalone(long seconds, int nanoseconds, string expected)
        {
            Assert.AreEqual(expected, new Duration { Seconds = seconds, Nanos = nanoseconds }.ToString());
        }

        [Test]
        public void DurationField()
        {
            var message = new TestWellKnownTypes { DurationField = new Duration() };
            AssertJson("{ 'durationField': '0s' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void StructSample()
        {
            var message = new Struct
            {
                Fields =
                {
                    { "a", new Value { NullValue = new NullValue() } },
                    { "b", new Value { BoolValue = false } },
                    { "c", new Value { NumberValue = 10.5 } },
                    { "d", new Value { StringValue = "text" } },
                    { "e", new Value { ListValue = new ListValue { Values = { new Value { StringValue = "t1" }, new Value { NumberValue = 5 } } } } },
                    { "f", new Value { StructValue = new Struct { Fields = { { "nested", new Value { StringValue = "value" } } } } } }
                }
            };
            AssertJson("{ 'a': null, 'b': false, 'c': 10.5, 'd': 'text', 'e': [ 't1', 5 ], 'f': { 'nested': 'value' } }", message.ToString());
        }

        [Test]
        public void FieldMaskStandalone()
        {
            var fieldMask = new FieldMask { Paths = { "", "single", "with_underscore", "nested.field.name", "nested..double_dot" } };
            Assert.AreEqual(",single,withUnderscore,nested.field.name,nested..doubleDot", fieldMask.ToString());

            // Invalid, but we shouldn't create broken JSON...
            fieldMask = new FieldMask { Paths = { "x\\y" } };
            Assert.AreEqual(@"x\\y", fieldMask.ToString());
        }

        [Test]
        public void FieldMaskField()
        {
            var message = new TestWellKnownTypes { FieldMaskField = new FieldMask { Paths = { "user.display_name", "photo" } } };
            AssertJson("{ 'fieldMaskField': 'user.displayName,photo' }", JsonFormatter.Default.Format(message));
        }

        /// <summary>
        /// Checks that the actual JSON is the same as the expected JSON - but after replacing
        /// all apostrophes in the expected JSON with double quotes. This basically makes the tests easier
        /// to read.
        /// </summary>
        private static void AssertJson(string expectedJsonWithApostrophes, string actualJson)
        {
            var expectedJson = expectedJsonWithApostrophes.Replace("'", "\"");
            Assert.AreEqual(expectedJson, actualJson);
        }
    }
}
