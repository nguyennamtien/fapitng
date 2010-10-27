<?php
// $Id$

/**
 * Theme a form.
 */
function theme_fapitng_form(array $variables) {
  $element = $variables['element'];

  // Anonymous DIV to satisfy XHTML compliance.
  return '<form' . drupal_attributes($element->attributes) . '><div>' . $element->rendered_content . '</div></form>';
}

/**
 * Wrap a form element in extra HTML.
 */
function theme_fapitng_form_element(array $variables) {
  $element = $variables['element'];
  // This is also used in the installer, pre-database setup.
  $t = get_t();

  // Add element's #type and #name as class to aid with JS/CSS selectors.
  $attributes = array();
  $attributes['class'][] = 'form-element';
  $attributes['class'][] = 'form-element-type-' . $element->type;
  $attributes['class'][] = 'form-element-' . $element->attributes['name'];
  // Add a class for disabled input elements to facilitate cross-browser
  // styling.
  if ($element instanceof fapitng_FormInput && $element->disabled) {
    $attributes['class'][] = 'form-element-disabled';
  }
  $output = '<div' . drupal_attributes($attributes) . '>' . "\n";

  $prefix = !empty($element->wrapper_prefix) ? '<div class="wrapper-prefix">' . $element->wrapper_prefix . '</div> ' : '';
  $suffix = !empty($element->wrapper_suffix) ? '<div class="wrapper-suffix">' . $element->wrapper_suffix . '</div>' : '';

  $output .= ' ' . theme('fapitng_form_label', $variables);
  $output .= ' ' . $prefix . $element->rendered_content . $suffix . "\n";

  if (!empty($element->description)) {
    $output .= '<div class="description">' . $element->description . "</div>\n";
  }

  $output .= "</div>\n";

  return $output;
}

/**
 * Theme a form element's label.
 */
function theme_fapitng_form_label(array $variables) {
  $element = $variables['element'];

  // If there is no title, don't output a label.
  if (empty($element->title)) {
    return '';
  }

  // If the element is required, a required marker is appended to the label.
  $required = $element->required ? theme('form_required_marker', array('element' => $element)) : '';

  $title = filter_xss_admin($element->title);

  // The leading whitespace helps visually separate fields from inline labels.
  return ' <label for="' . $element->attributes['id'] . '">' . $title . $required . "</label>\n";
}

/**
 * Theme the marker that indicates a form input is required.
 */
function theme_fapitng_form_required_marker(array $variables) {
  // This is also used in the installer, pre-database setup.
  $t = get_t();
  $attributes = array(
    'class' => 'form-required',
    'title' => $t('!title is required.', array('!title' => filter_xss_admin($variables['element']->title))),
  );
  return '<span' . drupal_attributes($attributes) . '>*</span>';
}

/**
 * Theme a textarea.
 */
function theme_fapitng_form_textarea(array $variables) {
  $element = $variables['element'];

  $wrapper_attributes = array(
    'class' => array('form-textarea-wrapper'),
  );

  if ($element->resizable) {
    $wrapper_attributes['class'][] = 'resizable';
  }

  $output = '<div' . drupal_attributes($wrapper_attributes) . '>';
  $output .= '<textarea' . drupal_attributes($element->attributes) . '>' . check_plain($element->value) . '</textarea>';
  $output .= '</div>';

  return $output;
}

/**
 * Theme a text or password input.
 */
function theme_fapitng_form_text(array $variables) {
  $element = $variables['element'];
  $element_attributes = array_merge($element->attributes, array(
    'type' => $element->type,
    'value' => $element->value,
  ));

  $hidden = NULL;
  if (drupal_valid_path($element->autocomplete_path)) {
    // @todo Add JS and class in expand callback.
    drupal_add_js('misc/autocomplete.js');
    $element->attributes['class'][] = 'form-autocomplete';

    $hidden_attributes = array(
      'type' => 'hidden',
      'id' => $element->attributes['id'] . '-autocomplete',
      'value' => url($element->autocomplete_path, array('absolute' => TRUE)),
      'disabled' => 'disabled',
      'class' => array('autocomplete'),
    );
    $hidden = '<input' . drupal_attributes($hidden_attributes) . ' />';
  }

  $output = '<input' . drupal_attributes($element_attributes) . ' />';

  return $output . $hidden;

}

/**
 * Theme a hidden form input.
 */
function theme_fapitng_form_hidden(array $variables) {
  $element = $variables['element'];
  $element->attributes['type'] = 'hidden';

  return '<input' . drupal_attributes($element->attributes) . ' />';
}

/**
 * Theme a form button.
 */
function theme_fapitng_form_button(array $variables) {
  $element = $variables['element'];

  $attributes = $element->attributes;
  $attributes['type'] = 'submit';

// @todo We already have a class in theme_fapitng_form_element that can replace
// this, right?
//  $attributes['class'][] = 'form-' . $element['#button_type'];

  return '<input' . drupal_attributes($attributes) . ' />';
}

/**
 * Theme a radio button.
 */
function theme_fapitng_form_radio(array $variables) {
  $element = $variables['element'];
  $attributes = $element->attributes;
  $attributes['type'] = 'radio';
  if ($element->value) {
    $attributes['checked'] = 'checked';
  }

  return '<input' . drupal_attributes($attributes) . ' />';
}