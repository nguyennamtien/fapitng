<?php
// $Id$

/**
 * Allow modules to add properties to elements or overwrite existing ones.
 */
function hook_elementstng_element_properties() {
  $properties['form'] = array(
    'submit_callbacks' => array('foo_custom_submit_callback')
  );

  return $properties;
}