

 /** 
 * `iron-range-behavior` provides the behavior for something with a minimum to maximum range.
 *
 * @demo demo/index.html
 * @polymerBehavior 
 */
 Polymer.IronRangeBehavior = {

  properties: {

    /**
     * The number that represents the current value.
     */
    value: {
      type: Number,
      value: 0,
      notify: true,
      reflectToAttribute: true
    },

    /**
     * The number that indicates the minimum value of the range.
     */
    min: {
      type: Number,
      value: 0,
      notify: true
    },

    /**
     * The number that indicates the maximum value of the range.
     */
    max: {
      type: Number,
      value: 100,
      notify: true
    },

    /**
     * Specifies the value granularity of the range's value.
     */
    step: {
      type: Number,
      value: 1,
      notify: true
    },

    /**
     * Returns the ratio of the value.
     */
    ratio: {
      type: Number,
      value: 0,
      readOnly: true,
      notify: true
    },
  },

  observers: [
    '_update(value, min, max, step)'
  ],

  _calcRatio: function(value) {
    return (this._clampValue(value) - this.min) / (this.max - this.min);
  },

  _clampValue: function(value) {
    return Math.min(this.max, Math.max(this.min, this._calcStep(value)));
  },

  _calcStep: function(value) {
   /**
    * if we calculate the step using
    * `Math.round(value / step) * step` we may hit a precision point issue 
    * eg. 0.1 * 0.2 =  0.020000000000000004
    * http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html
    *
    * as a work around we can divide by the reciprocal of `step`
    */
    return this.step ? (Math.round(value / this.step) / (1 / this.step)) : value;
  },

  _validateValue: function() {
    var v = this._clampValue(this.value);
    this.value = this.oldValue = isNaN(v) ? this.oldValue : v;
    return this.value !== v;
  },

  _update: function() {
    this._validateValue();
    this._setRatio(this._calcRatio(this.value) * 100);
  }

};
