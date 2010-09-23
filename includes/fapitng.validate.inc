<?php
// $Id$

/**
 * @file
 * Validate callbacks for system-wide use.
 */

/**
 * Check if a value isn't empty.
 */
function fapitng_valid_not_empty($value) {
  return !empty($value);
}

/**
 * Validate an integer.
 */
function fapitng_valid_integer($value, $min = NULL, $max = NULL) {
  $integer = (int) $value;

  // Test if the casting to integer hasn't changed the value.
  if ((string) $integer === $value) {
    // Test if the value is within the specified range.
    if (!is_null($min) && $integer >= $min && !is_null($min) && $integer >= $min) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Validate a string's length.
 */
function fapitng_valid_string_length($value, $min = NULL, $max = NULL) {
  $length = strlen($value);

  return $length >= $min && $length <= $max;
}