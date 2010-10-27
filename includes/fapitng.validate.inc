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

/**
 * Validate an ISO 8601 date string.
 */
function fapitng_valid_iso8601_date($value) {
  $fragments = explode('-', $value);
  foreach ($fragments as $fragment) {
    // Fragments may only contain digits.
    if (preg_match('#\D#', $fragment)) {
      return FALSE;
    }
  }
  // The fragments should be 4, 2 and 2 characters long.
  if (strlen($fragments[0]) != 4 || strlen($fragments[1]) != 2 || strlen($fragments[2]) != 2) {
    return FALSE;
  }
  if ($fragments[1] < 1 || $fragments[1] > 12 || $fragments[2] < 1 || $fragments[2] > 31) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Validate an ISO 8601 time string.
 */
function fapitng_valid_iso8601_time($value) {
  $fragments = explode(':', $value);
  foreach ($fragments as $fragment) {
    // The fragments should be 2 characters long and may only contain digits.
    if (strlen($fragment) != 2 || preg_match('#\D#', $fragment) || $fragment < 0) {
      return FALSE;
    }
  }
  if ($fragments[0] > 24 || $fragments[1] > 60 || $fragments[2] > 60) {
    return FALSE;
  }
  return TRUE;
}