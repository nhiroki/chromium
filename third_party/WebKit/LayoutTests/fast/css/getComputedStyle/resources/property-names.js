// We only test properties that are exposed in all ports.
var propertiesToTest = {
    "-webkit-animation-delay": true,
    "-webkit-animation-direction": true,
    "-webkit-animation-duration": true,
    "-webkit-animation-fill-mode": true,
    "-webkit-animation-iteration-count": true,
    "-webkit-animation-name": true,
    "-webkit-animation-play-state": true,
    "-webkit-animation-timing-function": true,
    "-webkit-appearance": true,
    "-webkit-backface-visibility": true,
    "-webkit-background-clip": true,
    "-webkit-background-composite": true,
    "-webkit-background-origin": true,
    "-webkit-background-size": true,
    "-webkit-border-fit": true,
    "-webkit-border-horizontal-spacing": true,
    "-webkit-border-image": true,
    "-webkit-border-vertical-spacing": true,
    "-webkit-box-align": true,
    "-webkit-box-decoration-break": true,
    "-webkit-box-direction": true,
    "-webkit-box-flex": true,
    "-webkit-box-flex-group": true,
    "-webkit-box-lines": true,
    "-webkit-box-ordinal-group": true,
    "-webkit-box-orient": true,
    "-webkit-box-pack": true,
    "-webkit-box-reflect": true,
    "-webkit-box-shadow": true,
    "-webkit-color-correction": true,
    "-webkit-column-axis": true,
    "-webkit-column-break-after": true,
    "-webkit-column-break-before": true,
    "-webkit-column-break-inside": true,
    "-webkit-column-count": true,
    "-webkit-column-gap": true,
    "-webkit-column-rule-color": true,
    "-webkit-column-rule-style": true,
    "-webkit-column-rule-width": true,
    "-webkit-column-span": true,
    "-webkit-column-width": true,
    "-webkit-font-smoothing": true,
    "-webkit-grid-column": true,
    "-webkit-grid-row": true,
    "-webkit-highlight": true,
    "-webkit-hyphenate-character": true,
    "-webkit-line-align": true,
    "-webkit-line-box-contain": true,
    "-webkit-line-break": true,
    "-webkit-line-clamp": true,
    "-webkit-line-grid": true,
    "-webkit-line-snap": true,
    "-webkit-locale": true,
    "-webkit-margin-after-collapse": true,
    "-webkit-margin-before-collapse": true,
    "-webkit-marquee-direction": true,
    "-webkit-marquee-increment": true,
    "-webkit-marquee-repetition": true,
    "-webkit-marquee-style": true,
    "-webkit-mask-attachment": true,
    "-webkit-mask-box-image": true,
    "-webkit-mask-box-image-outset": true,
    "-webkit-mask-box-image-repeat": true,
    "-webkit-mask-box-image-slice": true,
    "-webkit-mask-box-image-source": true,
    "-webkit-mask-box-image-width": true,
    "-webkit-mask-clip": true,
    "-webkit-mask-composite": true,
    "-webkit-mask-image": true,
    "-webkit-mask-origin": true,
    "-webkit-mask-position": true,
    "-webkit-mask-repeat": true,
    "-webkit-mask-size": true,
    "-webkit-nbsp-mode": true,
    "-webkit-perspective": true,
    "-webkit-perspective-origin": true,
    "-webkit-print-color-adjust": true,
    "-webkit-rtl-ordering": true,
    "-webkit-text-combine": true,
    "-webkit-text-decorations-in-effect": true,
    "-webkit-text-emphasis-color": true,
    "-webkit-text-emphasis-position": true,
    "-webkit-text-emphasis-style": true,
    "-webkit-text-fill-color": true,
    "-webkit-text-orientation": true,
    "-webkit-text-security": true,
    "-webkit-text-stroke-color": true,
    "-webkit-text-stroke-width": true,
    "-webkit-transform": true,
    "-webkit-transform-origin": true,
    "-webkit-transform-style": true,
    "-webkit-transition-delay": true,
    "-webkit-transition-duration": true,
    "-webkit-transition-property": true,
    "-webkit-transition-timing-function": true,
    "-webkit-user-drag": true,
    "-webkit-user-modify": true,
    "-webkit-user-select": true,
    "-webkit-writing-mode": true,
    "align-content": true,
    "align-items": true,
    "align-self": true,
    "alignment-baseline": true,
    "background-attachment": true,
    "background-clip": true,
    "background-color": true,
    "background-image": true,
    "background-origin": true,
    "background-position": true,
    "background-repeat": true,
    "background-size": true,
    "baseline-shift": true,
    "border-bottom-color": true,
    "border-bottom-left-radius": true,
    "border-bottom-right-radius": true,
    "border-bottom-style": true,
    "border-bottom-width": true,
    "border-collapse": true,
    "border-image-outset": true,
    "border-image-repeat": true,
    "border-image-slice": true,
    "border-image-source": true,
    "border-image-width": true,
    "border-left-color": true,
    "border-left-style": true,
    "border-left-width": true,
    "border-right-color": true,
    "border-right-style": true,
    "border-right-width": true,
    "border-top-color": true,
    "border-top-left-radius": true,
    "border-top-right-radius": true,
    "border-top-style": true,
    "border-top-width": true,
    "bottom": true,
    "box-shadow": true,
    "box-sizing": true,
    "caption-side": true,
    "clear": true,
    "clip": true,
    "clip-path": true,
    "clip-rule": true,
    "color": true,
    "color-interpolation": true,
    "color-interpolation-filters": true,
    "color-rendering": true,
    "cursor": true,
    "direction": true,
    "display": true,
    "dominant-baseline": true,
    "empty-cells": true,
    "fill": true,
    "fill-opacity": true,
    "fill-rule": true,
    "filter": true,
    "flex-direction": true,
    "flex-wrap": true,
    "float": true,
    "flood-color": true,
    "flood-opacity": true,
    "font-kerning": true,
    "font-size": true,
    "font-style": true,
    "font-variant": true,
    "font-variant-ligatures": true,
    "font-weight": true,
    "glyph-orientation-horizontal": true,
    "glyph-orientation-vertical": true,
    "height": true,
    "image-rendering": true,
    "justify-content": true,
    "kerning": true,
    "left": true,
    "letter-spacing": true,
    "lighting-color": true,
    "line-height": true,
    "list-style-image": true,
    "list-style-position": true,
    "list-style-type": true,
    "margin-bottom": true,
    "margin-left": true,
    "margin-right": true,
    "margin-top": true,
    "marker-end": true,
    "marker-mid": true,
    "marker-start": true,
    "mask": true,
    "max-height": true,
    "max-width": true,
    "min-height": true,
    "min-width": true,
    "opacity": true,
    "order": true,
    "orphans": true,
    "outline-color": true,
    "outline-style": true,
    "outline-width": true,
    "overflow-x": true,
    "overflow-y": true,
    "padding-bottom": true,
    "padding-left": true,
    "padding-right": true,
    "padding-top": true,
    "page-break-after": true,
    "page-break-before": true,
    "page-break-inside": true,
    "pointer-events": true,
    "position": true,
    "resize": true,
    "right": true,
    "shape-rendering": true,
    "speak": true,
    "stop-color": true,
    "stop-opacity": true,
    "stroke": true,
    "stroke-dasharray": true,
    "stroke-dashoffset": true,
    "stroke-linecap": true,
    "stroke-linejoin": true,
    "stroke-miterlimit": true,
    "stroke-opacity": true,
    "stroke-width": true,
    "tab-size": true,
    "table-layout": true,
    "text-align": true,
    "text-anchor": true,
    "text-decoration": true,
    "text-indent": true,
    "text-overflow": true,
    "text-rendering": true,
    "text-shadow": true,
    "text-transform": true,
    "top": true,
    "unicode-bidi": true,
    "vector-effect": true,
    "vertical-align": true,
    "visibility": true,
    "white-space": true,
    "widows": true,
    "width": true,
    "word-break": true,
    "word-spacing": true,
    "word-wrap": true,
    "writing-mode": true,
    "z-index": true,
    "zoom": true,
};

// There properties don't show up when iterating a computed style object,
// but we do want to dump their values in tests.
var hiddenComputedStyleProperties = [
    "background-position-x",
    "background-position-y",
    "border-spacing",
    "overflow",
    "-webkit-mask-position-x",
    "-webkit-mask-position-y",
];
