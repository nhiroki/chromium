if (!window.eventSender || !window.layoutTestController) {
    alert('This test needs to be run in DRT, to get results!');
}

// Map 'point' into absolute coordinates, usable for eventSender
function toAbsoluteCoordinates(point, element) {
    return point.matrixTransform(document.rootElement.getTransformToElement(element));
}

// Select a range of characters in text element 'id', from the start position of the 'start' character to the end position of the 'end' character
function selectRange(id, start, end, expectedText) {
    var element = document.getElementById(id);
    var startExtent = element.getExtentOfChar(start);
    var endExtent = element.getExtentOfChar(end);

    if (window.eventSender) {
        var startPos = element.getStartPositionOfChar(start);
        var endPos = element.getEndPositionOfChar(end);

        // Trigger 'partial glyph selection' code, by adjusting the end x position by half glyph width
        endPos.x -= endExtent.width / 2 - 1;

        var absStartPos = toAbsoluteCoordinates(startPos, element);
        var absEndPos = toAbsoluteCoordinates(endPos, element);

        // Move to selection origin and hold down mouse
        eventSender.mouseMoveTo(absStartPos.x, absStartPos.y);
        eventSender.mouseDown();

        // Move again to start selection
        eventSender.mouseMoveTo(absStartPos.x, absStartPos.y);

        // Move to end location and release mouse
        eventSender.mouseMoveTo(absEndPos.x, absEndPos.y);
        eventSender.mouseUp();
    }

    // Highlight rect that we've selected
    var rectElement = document.createElementNS("http://www.w3.org/2000/svg", "svg:rect");
    rectElement.setAttribute("x", startExtent.x);
    rectElement.setAttribute("y", endExtent.y);
    rectElement.setAttribute("width", endExtent.x + endExtent.width - startExtent.x);
    rectElement.setAttribute("height", endExtent.height);
    rectElement.setAttribute("fill-opacity", "0.4");
    rectElement.setAttribute("fill", "red");
    document.getElementById("container").appendChild(rectElement);

    // Check selection worked properly, otherwhise report error
    var actualText = window.getSelection().toString();
    if (actualText != expectedText) {
        var textElement = document.createElementNS("http://www.w3.org/2000/svg", "svg:text");
        textElement.setAttribute("x", "0");
        textElement.setAttribute("y", "35");
        textElement.setAttribute("fill", "red");
        textElement.setAttribute("transform", "scale(0.5)");
        textElement.setAttribute("font-size", "8");
        textElement.textContent = "Expected '" + expectedText + "' to be selected, got: '" + actualText + "'";
        document.getElementById("container").appendChild(textElement);
    }
}
