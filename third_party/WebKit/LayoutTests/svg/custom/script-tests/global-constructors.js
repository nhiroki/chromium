description("Test to make sure we expose all the global constructor objects requested by http://www.w3.org/TR/SVG11/ecmascript-binding.html");

function shouldBeDefined(a)
{
    var constructorString = "'function " + a + "() { [native code] }'";
    shouldBe("" + a + ".toString()", constructorString);
}

shouldBeDefined("SVGElement");
shouldBeDefined("SVGAnimatedBoolean");
shouldBeDefined("SVGAnimatedString");
shouldBeDefined("SVGStringList");
shouldBeDefined("SVGAnimatedEnumeration");
shouldBeDefined("SVGAnimatedInteger");
shouldBeDefined("SVGNumber");
shouldBeDefined("SVGAnimatedNumber");
shouldBeDefined("SVGNumberList");
shouldBeDefined("SVGAnimatedNumberList");
shouldBeDefined("SVGLength");
shouldBeDefined("SVGAnimatedLength");
shouldBeDefined("SVGLengthList");
shouldBeDefined("SVGAnimatedLengthList");
shouldBeDefined("SVGAngle");
shouldBeDefined("SVGAnimatedAngle");
shouldBeDefined("SVGColor");
shouldBeDefined("SVGICCColor");
shouldBeDefined("SVGRect");
shouldBeDefined("SVGAnimatedRect");
shouldBeDefined("SVGStylable");
shouldBeDefined("SVGLocatable");
shouldBeDefined("SVGTransformable");
shouldBeDefined("SVGTests");
shouldBeDefined("SVGLangSpace");
shouldBeDefined("SVGViewSpec");
shouldBeDefined("SVGURIReference");
shouldBeDefined("SVGCSSRule");
shouldBeDefined("SVGDocument");
shouldBeDefined("SVGSVGElement");
shouldBeDefined("SVGGElement");
shouldBeDefined("SVGDefsElement");
shouldBeDefined("SVGDescElement");
shouldBeDefined("SVGTitleElement");
shouldBeDefined("SVGSymbolElement");
shouldBeDefined("SVGUseElement");
shouldBeDefined("SVGElementInstance");
shouldBeDefined("SVGElementInstanceList");
shouldBeDefined("SVGImageElement");
shouldBeDefined("SVGSwitchElement");
shouldBeDefined("SVGStyleElement");
shouldBeDefined("SVGPoint");
shouldBeDefined("SVGPointList");
shouldBeDefined("SVGMatrix");
shouldBeDefined("SVGTransform");
shouldBeDefined("SVGTransformList");
shouldBeDefined("SVGAnimatedTransformList");
shouldBeDefined("SVGPreserveAspectRatio");
shouldBeDefined("SVGAnimatedPreserveAspectRatio");
shouldBeDefined("SVGPathSeg");
shouldBeDefined("SVGPathSegClosePath");
shouldBeDefined("SVGPathSegMovetoAbs");
shouldBeDefined("SVGPathSegMovetoRel");
shouldBeDefined("SVGPathSegLinetoAbs");
shouldBeDefined("SVGPathSegLinetoRel");
shouldBeDefined("SVGPathSegCurvetoCubicAbs");
shouldBeDefined("SVGPathSegCurvetoCubicRel");
shouldBeDefined("SVGPathSegCurvetoQuadraticAbs");
shouldBeDefined("SVGPathSegCurvetoQuadraticRel");
shouldBeDefined("SVGPathSegArcAbs");
shouldBeDefined("SVGPathSegArcRel");
shouldBeDefined("SVGPathSegLinetoHorizontalAbs");
shouldBeDefined("SVGPathSegLinetoHorizontalRel");
shouldBeDefined("SVGPathSegLinetoVerticalAbs");
shouldBeDefined("SVGPathSegLinetoVerticalRel");
shouldBeDefined("SVGPathSegCurvetoCubicSmoothAbs");
shouldBeDefined("SVGPathSegCurvetoCubicSmoothRel");
shouldBeDefined("SVGPathSegCurvetoQuadraticSmoothAbs");
shouldBeDefined("SVGPathSegCurvetoQuadraticSmoothRel");
shouldBeDefined("SVGPathSegList");
shouldBeDefined("SVGAnimatedPathData");
shouldBeDefined("SVGPathElement");
shouldBeDefined("SVGRectElement");
shouldBeDefined("SVGCircleElement");
shouldBeDefined("SVGEllipseElement");
shouldBeDefined("SVGLineElement");
shouldBeDefined("SVGAnimatedPoints");
shouldBeDefined("SVGPolylineElement");
shouldBeDefined("SVGPolygonElement");
shouldBeDefined("SVGTextContentElement");
shouldBeDefined("SVGTextPositioningElement");
shouldBeDefined("SVGTextElement");
shouldBeDefined("SVGTSpanElement");
shouldBeDefined("SVGTRefElement");
shouldBeDefined("SVGTextPathElement");
shouldBeDefined("SVGAltGlyphElement");
shouldBeDefined("SVGAltGlyphDefElement");
shouldBeDefined("SVGAltGlyphItemElement");
shouldBeDefined("SVGGlyphRefElement");
shouldBeDefined("SVGPaint");
shouldBeDefined("SVGMarkerElement");
shouldBeDefined("SVGColorProfileElement");
shouldBeDefined("SVGColorProfileRule");
shouldBeDefined("SVGGradientElement");
shouldBeDefined("SVGLinearGradientElement");
shouldBeDefined("SVGRadialGradientElement");
shouldBeDefined("SVGStopElement");
shouldBeDefined("SVGPatternElement");
shouldBeDefined("SVGClipPathElement");
shouldBeDefined("SVGMaskElement");
shouldBeDefined("SVGFilterElement");
shouldBeDefined("SVGFilterPrimitiveStandardAttributes");
shouldBeDefined("SVGFEBlendElement");
shouldBeDefined("SVGFEColorMatrixElement");
shouldBeDefined("SVGFEComponentTransferElement");
shouldBeDefined("SVGComponentTransferFunctionElement");
shouldBeDefined("SVGFEFuncRElement");
shouldBeDefined("SVGFEFuncGElement");
shouldBeDefined("SVGFEFuncBElement");
shouldBeDefined("SVGFEFuncAElement");
shouldBeDefined("SVGFECompositeElement");
shouldBeDefined("SVGFEConvolveMatrixElement");
shouldBeDefined("SVGFEDiffuseLightingElement");
shouldBeDefined("SVGFEDistantLightElement");
shouldBeDefined("SVGFEPointLightElement");
shouldBeDefined("SVGFESpotLightElement");
shouldBeDefined("SVGFEDisplacementMapElement");
shouldBeDefined("SVGFEFloodElement");
shouldBeDefined("SVGFEGaussianBlurElement");
shouldBeDefined("SVGFEImageElement");
shouldBeDefined("SVGFEMergeElement");
shouldBeDefined("SVGFEMergeNodeElement");
shouldBeDefined("SVGFEMorphologyElement");
shouldBeDefined("SVGFEOffsetElement");
shouldBeDefined("SVGFESpecularLightingElement");
shouldBeDefined("SVGFETileElement");
shouldBeDefined("SVGFETurbulenceElement");
shouldBeDefined("SVGCursorElement");
shouldBeDefined("SVGAElement");
shouldBeDefined("SVGViewElement");
shouldBeDefined("SVGScriptElement");
shouldBeDefined("SVGEvent");
shouldBeDefined("SVGZoomEvent");
shouldBeDefined("SVGAnimationElement");
shouldBeDefined("SVGAnimateElement");
shouldBeDefined("SVGSetElement");
shouldBeDefined("SVGAnimateMotionElement");
shouldBeDefined("SVGMPathElement");
shouldBeDefined("SVGAnimateColorElement");
shouldBeDefined("SVGAnimateTransformElement");
shouldBeDefined("SVGFontElement");
shouldBeDefined("SVGGlyphElement");
shouldBeDefined("SVGMissingGlyphElement");
shouldBeDefined("SVGHKernElement");
shouldBeDefined("SVGVKernElement");
shouldBeDefined("SVGFontFaceElement");
shouldBeDefined("SVGFontFaceSrcElement");
shouldBeDefined("SVGFontFaceUriElement");
shouldBeDefined("SVGFontFaceFormatElement");
shouldBeDefined("SVGFontFaceNameElement");
shouldBeDefined("SVGDefinitionSrcElement");
shouldBeDefined("SVGMetadataElement");
shouldBeDefined("SVGForeignObjectElement");

var successfullyParsed = true;
