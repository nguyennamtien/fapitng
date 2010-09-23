<?php
// $Id$

/**
 * Define renderable elements.
 *
 * Even though the basic implementation only requires a machine name and a
 * class, a format has been chosen to allow other modules to extend the
 * information by adding other properties, like titles, as you can see in
 * hook_elementstng_element_info_alter();
 */
function hook_elementstng_element_info() {
  $elements['form'] = array(
    'class' => 'fapitng_Form'
  );
  $elements['markup'] = array(
    'class' => 'elementstng_Markup'
  );
}

function hook_elementstng_element_info_alter(array &$elements) {
  $elements['markup']['class'] = 'foobar_Markup';
  $elements['markup']['title'] = t('Markup');
}