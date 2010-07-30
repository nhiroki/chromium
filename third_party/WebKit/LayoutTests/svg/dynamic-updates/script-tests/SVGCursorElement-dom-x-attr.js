// [Name] SVGCursorElement-dom-x-attr.js
// [Expected rendering result] cursor image should hang from the upper-left corner of the cursor after clicking - and a series of PASS mesasges

description("Tests dynamic updates of the 'x' attribute of the SVGCursorElement object")
createSVGTestCase();

var cursorElement = createSVGElement("cursor");
cursorElement.setAttribute("id", "cursor");
cursorElement.setAttribute("x", "100");
cursorElement.setAttributeNS(xlinkNS, "xlink:href", "../W3C-SVG-1.1/resources/sphere.png");
rootSVGElement.appendChild(cursorElement);

var rectElement = createSVGElement("rect");
rectElement.setAttribute("cursor", "url(#cursor)");
rectElement.setAttribute("width", "200");
rectElement.setAttribute("height", "200");
rectElement.setAttribute("fill", "green");
rootSVGElement.appendChild(rectElement);

shouldBeEqualToString("cursorElement.getAttribute('x')", "100");

function executeTest() {
    cursorElement.setAttribute("x", "0");
    shouldBeEqualToString("cursorElement.getAttribute('x')", "0");

    completeTest();
}

startTest(rectElement, 150, 150);

var successfullyParsed = true;
