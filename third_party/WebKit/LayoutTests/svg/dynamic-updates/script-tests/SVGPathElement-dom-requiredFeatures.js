// [Name] SVGPathElement-dom-requiredFeatures.js
// [Expected rendering result] a series of PASS messages

createSVGTestCase();

var pathElement = createSVGElement("path");
pathElement.setAttribute("d", "M0 0 L 200 0 L 200 200 L 0 200");

rootSVGElement.appendChild(pathElement);

function repaintTest() {
    debug("Check that SVGPathElement is initially displayed");
    shouldBeEqualToString("document.defaultView.getComputedStyle(pathElement, null).display", "inline");
    debug("Check that setting requiredFeatures to something invalid makes it not render");
    pathElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#BogusFeature");
    shouldBeEqualToString("document.defaultView.getComputedStyle(pathElement, null).display", "");
    debug("Check that setting requiredFeatures to something valid makes it render again");
    pathElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#Shape");
    shouldBeEqualToString("document.defaultView.getComputedStyle(pathElement, null).display", "inline");
    debug("Check that adding something valid to requiredFeatures keeps rendering the element");
    pathElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#Gradient");
    shouldBeEqualToString("document.defaultView.getComputedStyle(pathElement, null).display", "inline");
    debug("Check that adding something invalid to requiredFeatures makes it not render");
    pathElement.setAttribute("requiredFeatures", "http://www.w3.org/TR/SVG11/feature#BogusFeature");
    shouldBeEqualToString("document.defaultView.getComputedStyle(pathElement, null).display", "");

    completeTest();
}

var successfullyParsed = true;
