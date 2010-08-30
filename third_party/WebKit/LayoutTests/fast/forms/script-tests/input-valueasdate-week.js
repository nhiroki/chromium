description('Tests for .valueAsDate with &lt;input type=week>.');

var input = document.createElement('input');
input.type = 'week';

function valueAsDateFor(stringValue) {
    input.value = stringValue;
    return input.valueAsDate;
}

function setValueAsDateAndGetValue(year, month, day) {
    var date = new Date();
    date.setTime(Date.UTC(year, month, day));
    if (year < 100)
        date.setUTCFullYear(year);
    input.valueAsDate = date;
    return input.value;
}

shouldBe('valueAsDateFor("")', 'null');
// January 1st is Monday. W01 starts on January 1nd.
shouldBe('valueAsDateFor("2007-W01").getTime()', 'Date.UTC(2007, 0, 1)');
// January 1st is Tuesday. W01 starts on December 31th.
shouldBe('valueAsDateFor("2008-W01").getTime()', 'Date.UTC(2007, 11, 31)');
// January 1st is Wednesday. W01 starts on December 29th.
shouldBe('valueAsDateFor("2003-W01").getTime()', 'Date.UTC(2002, 11, 30)');
// January 1st is Thursday. W01 starts on December 29th.
shouldBe('valueAsDateFor("2004-W01").getTime()', 'Date.UTC(2003, 11, 29, 0, 0, 0, 0)');
// January 1st is Friday. W01 starts on January 4th.
shouldBe('valueAsDateFor("2010-W01").getTime()', 'Date.UTC(2010, 0, 4)');
// January 1st is Saturday. W01 starts on January 3rd.
shouldBe('valueAsDateFor("2005-W01").getTime()', 'Date.UTC(2005, 0, 3)');
// January 1st is Sunday. W01 starts on January 2nd.
shouldBe('valueAsDateFor("2006-W01").getTime()', 'Date.UTC(2006, 0, 2)');

debug('Various January 1st:');
shouldBe('setValueAsDateAndGetValue(1, 0, 1)', '"0001-W01"');
shouldBe('setValueAsDateAndGetValue(2007, 0, 1)', '"2007-W01"');
shouldBe('setValueAsDateAndGetValue(2008, 0, 1)', '"2008-W01"');
shouldBe('setValueAsDateAndGetValue(2003, 0, 1)', '"2003-W01"');
shouldBe('setValueAsDateAndGetValue(2004, 0, 1)', '"2004-W01"');
shouldBe('setValueAsDateAndGetValue(2010, 0, 1)', '"2009-W53"');
shouldBe('setValueAsDateAndGetValue(2005, 0, 1)', '"2004-W53"');
shouldBe('setValueAsDateAndGetValue(2006, 0, 1)', '"2005-W52"');

debug('Normal cases:');
shouldBe('setValueAsDateAndGetValue(2010, 0, 3)', '"2009-W53"');
shouldBe('setValueAsDateAndGetValue(2010, 0, 4)', '"2010-W01"');
shouldBe('setValueAsDateAndGetValue(2010, 0, 10)', '"2010-W01"');
shouldBe('setValueAsDateAndGetValue(2010, 0, 11)', '"2010-W02"');
shouldBe('setValueAsDateAndGetValue(2010, 0, 17)', '"2010-W02"');
shouldBe('setValueAsDateAndGetValue(2010, 11, 31)', '"2010-W52"');

debug('Around the minimum value:');
// Gregorian calendar started in 1582. We don't support that year.
shouldBe('setValueAsDateAndGetValue(0, 11, 31)', '""');
shouldBe('setValueAsDateAndGetValue(0, 0, 1)', '""');

var successfullyParsed = true;
