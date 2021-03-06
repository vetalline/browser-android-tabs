// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview_new', function() {
  /**
   * Helper functions for a select with timeout. Implemented by select settings
   * sections, so that the preview does not immediately begin generating and
   * freeze the dropdown when the value is changed.
   * Assumes that the elements implementing this behavior have no more than one
   * select element.
   * @polymerBehavior
   */
  const SelectBehavior = {
    properties: {
      selectedValue: {
        type: String,
        observer: 'onSelectedValueChange_',
      },
    },

    /**
     * @param {string} current
     * @param {?string} previous
     * @private
     */
    onSelectedValueChange_: function(current, previous) {
      // Don't trigger an extra preview request at startup.
      if (previous === undefined) {
        return;
      }

      this.debounce('select-change', () => {
        this.onProcessSelectChange(this.selectedValue);

        // For testing only
        this.fire('process-select-change');
      }, 100);
    },

    /**
     * Should be overridden by elements using this behavior to receive select
     * value updates.
     * @param {string} value The new select value to process.
     */
    onProcessSelectChange: function(value) {},
  };

  return {SelectBehavior: SelectBehavior};
});
