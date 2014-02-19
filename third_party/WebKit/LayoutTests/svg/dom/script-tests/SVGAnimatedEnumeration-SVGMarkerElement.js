description("This test checks the use of SVGAnimatedEnumeration within SVGMarkerElement");

var markerElement = document.createElementNS("http://www.w3.org/2000/svg", "marker");
markerElement.setAttribute("markerUnits", "userSpaceOnUse");
markerElement.setAttribute("orient", "auto");

var svgElement = document.createElementNS("http://www.w3.org/2000/svg", "svg");

// markerUnits
debug("");
debug("Check initial 'markerUnits' value");
shouldBeEqualToString("markerElement.markerUnits.toString()", "[object SVGAnimatedEnumeration]");
shouldBeEqualToString("typeof(markerElement.markerUnits.baseVal)", "number");
shouldBe("markerElement.markerUnits.baseVal", "SVGMarkerElement.SVG_MARKERUNITS_USERSPACEONUSE");

debug("");
debug("Switch to 'strokeWidth'");
shouldBe("markerElement.markerUnits.baseVal = SVGMarkerElement.SVG_MARKERUNITS_STROKEWIDTH", "SVGMarkerElement.SVG_MARKERUNITS_STROKEWIDTH");
shouldBe("markerElement.markerUnits.baseVal", "SVGMarkerElement.SVG_MARKERUNITS_STROKEWIDTH");
shouldBeEqualToString("markerElement.getAttribute('markerUnits')", "strokeWidth");

debug("");
debug("Try setting invalid values");
shouldThrow("markerElement.markerUnits.baseVal = 3");
shouldBe("markerElement.markerUnits.baseVal", "SVGMarkerElement.SVG_MARKERUNITS_STROKEWIDTH");
shouldBeEqualToString("markerElement.getAttribute('markerUnits')", "strokeWidth");

shouldThrow("markerElement.markerUnits.baseVal = -1");
shouldBe("markerElement.markerUnits.baseVal", "SVGMarkerElement.SVG_MARKERUNITS_STROKEWIDTH");
shouldBeEqualToString("markerElement.getAttribute('markerUnits')", "strokeWidth");

shouldThrow("markerElement.markerUnits.baseVal = 0");
shouldBe("markerElement.markerUnits.baseVal", "SVGMarkerElement.SVG_MARKERUNITS_STROKEWIDTH");
shouldBeEqualToString("markerElement.getAttribute('markerUnits')", "strokeWidth");

debug("");
debug("Switch to 'userSpaceOnUse'");
shouldBe("markerElement.markerUnits.baseVal = SVGMarkerElement.SVG_MARKERUNITS_USERSPACEONUSE", "SVGMarkerElement.SVG_MARKERUNITS_USERSPACEONUSE");
shouldBe("markerElement.markerUnits.baseVal", "SVGMarkerElement.SVG_MARKERUNITS_USERSPACEONUSE");
shouldBeEqualToString("markerElement.getAttribute('markerUnits')", "userSpaceOnUse");

// orientType
debug("");
debug("Check initial 'orient' value");
shouldBeEqualToString("markerElement.orientType.toString()", "[object SVGAnimatedEnumeration]");
shouldBeEqualToString("typeof(markerElement.orientType.baseVal)", "number");
shouldBe("markerElement.orientAngle.baseVal.value", "0");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_UNSPECIFIED");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_AUTO");

debug("");
debug("Switch to 'Pi/2 rad' value - via setOrientToAngle()");
shouldBeUndefined("anglePiHalfRad = svgElement.createSVGAngle(); anglePiHalfRad.newValueSpecifiedUnits(SVGAngle.SVG_ANGLETYPE_RAD, (Math.PI / 2).toFixed(2))");
shouldBeUndefined("markerElement.setOrientToAngle(anglePiHalfRad)");
shouldBeEqualToString("markerElement.orientAngle.baseVal.value.toFixed(1)", "90.0");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_RAD");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBeEqualToString("markerElement.getAttribute('orient')", (Math.PI / 2).toFixed(2) + "rad");

debug("");
debug("Switch to 'auto' value - via setOrientToAuto()");
shouldBeUndefined("markerElement.setOrientToAuto()");
shouldBe("markerElement.orientAngle.baseVal.value", "0");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_UNSPECIFIED");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_AUTO");
shouldBeEqualToString("markerElement.getAttribute('orient')", "auto");

debug("");
debug("Switch to '20deg' value - via setOrientToAngle()");
shouldBeUndefined("angle20deg = svgElement.createSVGAngle(); angle20deg.newValueSpecifiedUnits(SVGAngle.SVG_ANGLETYPE_DEG, 20)");
shouldBeUndefined("markerElement.setOrientToAngle(angle20deg)");
shouldBe("markerElement.orientAngle.baseVal.value", "20");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_DEG");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBeEqualToString("markerElement.getAttribute('orient')", "20deg");

debug("");
debug("Switch to '10deg' value");
shouldBe("markerElement.orientAngle.baseVal.value = 10", "10");
shouldBe("markerElement.orientAngle.baseVal.value", "10");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_DEG");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBeEqualToString("markerElement.getAttribute('orient')", "10deg");

debug("");
debug("Switch to 'auto' value - by modifying orientType");
shouldBe("markerElement.orientType.baseVal = SVGMarkerElement.SVG_MARKER_ORIENT_AUTO", "SVGMarkerElement.SVG_MARKER_ORIENT_AUTO");
shouldBe("markerElement.orientAngle.baseVal.value", "0");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_UNSPECIFIED");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_AUTO");
shouldBeEqualToString("markerElement.getAttribute('orient')", "auto");

markerElement.setAttribute('orient', '10deg');

debug("");
debug("Try setting invalid values");
shouldThrow("markerElement.orientType.baseVal = 3");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBeEqualToString("markerElement.getAttribute('orient')", "10deg");

shouldThrow("markerElement.orientType.baseVal = -1");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBeEqualToString("markerElement.getAttribute('orient')", "10deg");

shouldThrow("markerElement.orientType.baseVal = 0");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBeEqualToString("markerElement.getAttribute('orient')", "10deg");

debug("");
debug("Switch back to 'auto' value");
shouldBe("markerElement.orientType.baseVal = SVGMarkerElement.SVG_MARKER_ORIENT_AUTO", "SVGMarkerElement.SVG_MARKER_ORIENT_AUTO");
shouldBe("markerElement.orientAngle.baseVal.value", "0");
shouldBe("markerElement.orientAngle.baseVal.unitType", "SVGAngle.SVG_ANGLETYPE_UNSPECIFIED");
shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_AUTO");
shouldBeEqualToString("markerElement.getAttribute('orient')", "auto");

successfullyParsed = true;
