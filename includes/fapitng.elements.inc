<?php
// $Id$

/**
 * Basic behavior shared by forms and form elements.
 */
abstract class fapitng_FormBase extends elementstng_Element {
  public $attached = array();
  public $build_callbacks = array();
  public $submit_callbacks = array();

  function addChild(elementstng_Element $element) {
    $element->form = $this instanceof fapitng_Form ? $this : $this->form;
    parent::addChild($element);
  }

  /**
   * Execute an element's submit callbacks.
   */
  function submit() {
    foreach ($this->submit_callbacks as $callback) {
      call_user_func($callback, $this);
    }
    foreach ($this->children as $element) {
      $element->submit();
    }
  }
}

/**
 * A Drupal form.
 */
class fapitng_Form extends fapitng_FormBase {
  // @todo Implement form_state_defaults() and form_state_keys_no_cache().
  public $action = '';
  public $ajax = array();
  // @todo This was $form_state['build_info']['args'].
  public $arguments = array();
  public $attributes = array(
    'accept-charset' => 'UTF-8',
    'id' => '',
  );
  public $build_id = '';
  public $cache = FALSE;
  public $errors = array();
  public $expand_callbacks = array('fapitng_form_expand_form');
  // @todo This was $form_state['build_info']['files'].
  public $files = array();
  public $method = 'POST';
  public $programmed = FALSE;
  public $rebuild = FALSE;
  // Contains properties for this specific request that must not be cached.
  public $state = array(
    // @todo Confirm properties and their default values.
    // Public properties defined by form constructors and form handlers.
    'always_process' => NULL,
    'must_validate' => NULL,
    'no_redirect' => NULL,
    'rebuild' => FALSE,
    'rebuild_info' => array(),
    'redirect' => NULL,
    'temporary' => array(),
    // Internal properties defined by form processing.
    'buttons' => array(),
    'executed' => FALSE,
    'clicked_button' => NULL,
    'complete form' => NULL,
    'groups' => array(),
    'input' => NULL,
    'raw_input' => array(),
    'submit_handlers' => NULL,
    'submitted' => FALSE,
    'triggering_element' => NULL,
    'validate_handlers' => NULL,
    // References to the value properties of all input elements.
    'values' => NULL,
  );
  // Data storage intended for use by any module that needs to store something.
  public $storage = array();
  public $submit_element = NULL;
  public $theme_wrapper_callbacks = array('fapitng_form');
  public $token = '';
  public $type = 'form';
  public $valid = TRUE;
  public $validate_callbacks = array();
  protected $built = FALSE;

  function __construct($form_id, array $build_callbacks, array $arguments = array(), $programmed = FALSE) {
    global $user;

    $this->action = request_uri();
    $this->attributes['id'] = drupal_html_id('form-' . $form_id);
    $this->build_callbacks = $build_callbacks;
    $this->build_id = drupal_hash_base64(uniqid(mt_rand(), TRUE) . mt_rand());
    $this->id = $form_id;
    // Add a token to forms displayed to authenticated users. This ensures that
    // any submitted form was actually requested previously by the user and
    // protects against cross site request forgeries.
    // This does not apply to programmatically submitted forms. Furthermore,
    // since tokens are session-bound and forms displayed to anonymous users
    // are very likely cached, we cannot assign a token for them. During
    // installation, there is no $user yet.
    if (!empty($user->uid) && !$this->programmed) {
      $this->token = drupal_get_token($this->id);
    }

    // Set validate and submit callbacks based on build callbacks.
    foreach ($build_callbacks as $callback) {
      if (function_exists($callback . '_validate')) {
        $this->validate_callbacks[] = $callback . '_validate';
      }
      if (function_exists($callback . '_submit')) {
        $this->submit_callbacks[] = $callback . '_submit';
      }
    }
    // The first build callback defines the theme callback.
    $this->theme_callback = $build_callbacks[0];
  }

  /**
   * Prepare the form's basic elements.
   *
   * @return NULL
   */
  function prepare() {
    $this->addChild(new fapitng_FormHidden('form_build_id', array(
      'value' => $this->build_id,
      'attributes' => array(
        'id' => $this->build_id,
        'name' => 'form_build_id',
      ),
    )));
    $this->addChild(new fapitng_FormHidden('form_id', array(
      'value' => $this->id,
      'attributes' => array(
        'id' => 'form-element-' . $this->id,
        'name' => 'form_id',
      ),
    )));
    if ($this->token) {
      $this->addChild(new fapitng_FormHidden('form_token', array(
        'value' => $this->token,
      )));
    }
  }

  /**
   * Gather the form's elements.
   *
   * @return NULL
   */
  function build() {
    $this->built = TRUE;
    foreach ($this->build_callbacks as $callback) {
      call_user_func($callback, $this);
      drupal_alter("fapitng_form_$callback", $this);
    }
    drupal_alter('fapitng_form', $this);
  }

  /**
   * Execute the form's construction stages.
   *
   * @return NULL
   */
  function construct() {
    $this->receive();
    if (!$this->cacheGet()) {
      $this->prepare();
      $this->build();
      $this->expand();
      $this->populate();
      $this->cacheSet();
    }
  }

  /**
   * Gather user inputted data and populate the form with it.
   *
   * @todo Allow input to be passed on directly as well.
   *
   * @return NULL
   */
  function receive() {
    // Collect inputted data. Forms can receive input in three different ways:
    // - The form is set to accept POST and there is POST data.
    // - The form is set to accept POST and there is GET data (prepopulate).
    // - The form is set to accept GET and there is GET data.
    if ($this->method == 'POST' && isset($_POST['form_id']) && $_POST['form_id'] == $this->id) {
      $this->state['raw_input'] = $_POST;
    }
    else {
      $this->state['raw_input'] = $_GET;
    }
  }

  /**
   * Populate the form's input elements.
   *
   * @return NULL
   */
  function populate() {
    if ($this->state['raw_input']) {
      $this->populateInputElements($this->children);
    }
  }
  
  /**
   * Populate the form's input elements.
   *
   * This is a recursive function, which is called from $this::populate().
   *
   * @param $elements array
   *   The elements to populate.
   *
   * @return NULL
   */
  private function populateInputElements(array $elements) {
    foreach ($elements as $element) {
      if ($element instanceof fapitng_FormInput) {
        if (isset($this->state['raw_input'][$element->attributes['name']])) {
          // @todo We inject client input directly into the form. What about
          // security issues?
          $element->value = $this->state['raw_input'][$element->attributes['name']];
          $this->state['values'][$element->attributes['name']] = $element->value;
        }
      }
      $this->populateInputElements($element->children);
    }
  }

  /**
   * Validate the form and its input elements.
   *
   * @return boolean
   *   Whether the form input is valid or not.
   */
  function validate() {
    // @todo Remove module_load_include() call when moving to Drupal core.
    module_load_include('inc', 'fapitng', '/includes/fapitng.validate');
    foreach ($this->validate_callbacks as $callback => $info) {
      if (!call_user_func_array($callback, array_merge(array($this->value), $info['arguments']))) {
        $this->errors[] = $info['message'];
      }
    }
    $this->validateInputElements($this->children);

    if (!$this->valid) {
      $this->state['rebuild'] = TRUE;
    }

    return $this->valid;
  }

  /**
   * Validate the form's input elements.
   *
   * This is a recursive function, which is called from $this::validate().
   *
   * @param $elements array
   *   The elements to validate.
   *
   * @return NULL
   */
  private function validateInputElements(array $elements) {
    foreach ($elements as $element) {
      if ($element instanceof fapitng_FormInput) {
        if (!$element->validate()) {
          $this->valid = FALSE;
        }
      }
      $this->validate_input_elements($element->children);
    }
  }

  /**
   * Cache the form.
   *
   * @return NULL
   */
  function cacheSet() {
    // @todo Do we need a #cache_token?
    if ($this->token) {
      // Don't cache the form's state.
      $state = $this->state;
      unset($this->state);
      cache_set($this->build_id, $this, 'fapitng_cache_form', REQUEST_TIME + 21600);
      $this->state = $state;
    }
  }

  /**
   * Get the form from the cache
   *
   * @return boolean
   *   Whether the form was successfully loaded from the cache or not.
   */
  function cacheGet() {
    global $user;

    // @todo Fix build ID variable.
    if (isset($this->state['raw_input']['build_id']) && $cached = cache_get($this->state['raw_input']['build_id'], 'fapitng_cache_form')) {
      $form_cached = $cached->data;

      if ($user->uid && drupal_valid_token($form_cached->token, $this->id)) {
        foreach ($form_cached as $property => $value) {
          $this->$property = $value;
        }

        // If the original form is contained in include files, load the files.
        foreach ($this->files as $file) {
          if (is_array($file)) {
            $file += array(
              'type' => 'inc',
              'name' => $file['module'],
            );
            module_load_include($file['type'], $file['module'], $file['name']);
          }
          elseif (file_exists($file)) {
            require_once DRUPAL_ROOT . '/' . $file;
          }
        }
      }
      return TRUE;
    }
    return FALSE;
  }
}

/**
 * Base class for all form elements.
 */
class fapitng_FormElement extends fapitng_FormBase {
  public $access = TRUE;
  public $form = NULL;
  public $parent = NULL;
  public $prefix = '';
  public $states = array();
  public $suffix = '';
  public $theme_wrapper_callbacks = array('fapitng_form_element');
  public $tree = FALSE;
  public $wrapper_prefix = '';
  public $wrapper_suffix = '';
  protected $printed = FALSE;

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
  }

  function settle() {
    parent::settle();
    if (empty($this->attributes['name'])) {
      $this->attributes['name'] = $this->parent->id . '-' . $this->id;
    }
    if (empty($this->attributes['id'])) {
      $this->attributes['id'] = $this->form->id . '-' . $this->id;
    }
  }
}

/**
 * Base class for all form elements that accept input.
 */
class fapitng_FormInput extends fapitng_FormElement {
  public $attributes = array(
    'name' => '',
  );
  public $default_value = NULL;
  public $disabled = FALSE;
  public $errors = array();
  public $expand_callbacks = array('fapitng_form_expand_input', 'fapitng_form_expand_required');
  public $validate_callbacks = array();
  public $required = FALSE;
  public $value = NULL;

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->attributes['value'] = '';
  }

  /**
   * Check user input for the input element.
   *
   * @return boolean
   *   Whether the element's value is valid or not.
   */
  function validate() {
    foreach ($this->validate_callbacks as $callback => $info) {
      if (!call_user_func_array($callback, array_merge(array($this->value), $info['arguments']))) {
        $this->errors[] = $info['message'];
      }
    }

    return empty($this->errors);
  }
}

class fapitng_FormFieldset extends fapitng_FormElement {
  public $collapsed = FALSE;
  public $collapsible = FALSE;
  public $group = NULL;
  // @todo Replace form_pre_render_fieldset().
  public $pre_render_callbacks = array('form_pre_render_fieldset');
  public $type = 'fieldset';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_expand_fieldset';
    $this->theme_wrapper_callbacks[] = 'fapitng_form_fieldset';
  }
}

class fapitng_FormVerticalTabs extends fapitng_FormElement {
  public $default_tab = '';
  public $type = 'vertical_tabs';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_expand_vertical_tabs';
    $this->theme_wrapper_callbacks[] = 'fapitng_form_vertical_tabs';
  }
}

/**
 * Base class for elements that accept text input.
 */
class fapitng_FormTextInput extends fapitng_FormInput {
  public $expand_callbacks = array('fapitng_form_expand_text_input');
  public $max_length = 0;
  public $min_length = 0;
}

class fapitng_FormTextField extends fapitng_FormTextInput {
  public $autocomplete_path = '';
  public $expand_callbacks = array('fapitng_form_expand_textfield');
  public $max_length = 128;
  public $theme_callback = 'fapitng_form_textfield';
  public $type = 'text';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->attributes['size'] = 60;
  }
}

class fapitng_FormPassword extends fapitng_FormTextField {
  public $type = 'password';
}

class fapitng_FormPasswordConfirm extends fapitng_FormPassword {
  public $type = 'password_confirm';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    // @todo Merge form_process_password_confirm() and
    // user_form_process_password_confirm().
      $this->expand_callbacks[] = 'fapitng_form_expand_password_confirm';
  }
}

class fapitng_FormTextarea extends fapitng_FormTextInput {
  public $autocomplete_path = '';
  public $cols = 60;
  public $resizable = TRUE;
  public $rows = 5;
  public $theme_callback = 'fapitng_form_textarea';
  public $type = 'textarea';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_textarea';
  }
}

class fapitng_FormTableselect extends fapitng_FormInput {
  public $empty = '';
  public $header = array();
  public $js_select = FALSE;
  public $multiple = TRUE;
  public $options = array();
  public $theme_callback = 'fapitng_form_tableselect';
  public $type = 'table_select';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_tableselect';
  }
}

class fapitng_FormButton extends fapitng_FormInput {
  // @todo Implement $execute_submit_handler.
  public $execute_submit_handler = FALSE;
  // @todo Implement $limit_validation ($limit_validation_errors in Drupal 7).
  public $limit_validation = array();
  public $return_value = '';
  public $theme_callback = 'fapitng_form_button';
  public $type = 'button';
}

class fapitng_FormSubmit extends fapitng_FormButton {
  public $execute_submit_handler = TRUE;
  public $type = 'submit';
}

class fapitng_FormImageButton extends fapitng_FormButton {
  public $source = '';
  public $theme_callback = 'fapitng_form_image_button';
  public $type = 'image_button';
}

class fapitng_FormSelect extends fapitng_FormInput {
  public $multiple = FALSE;
  public $options = array();
  // @todo Copy from the comment in Drupal 7. Check validity.
  // In order to be able to determine whether a select list needs an empty
  // default option, #required has to be NULL by default, as form_builder()
  // preemptively sets #required to FALSE for all elements.
  // @see form_process_select()
  public $required = NULL;
  public $theme_callback = 'fapitng_form_select';
  public $type = 'select';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    // @todo Replace form_process_select().
      $this->expand_callbacks[] = 'fapitng_form_expand_select';
  }
}

class fapitng_FormWeight extends fapitng_FormSelect {
  public $delta = 10;
  public $type = 'weight';
  public $value = 0;

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    // @todo Replace form_process_select().
      $this->expand_callbacks[] = 'fapitng_form_expand_weight';
  }
}

class fapitng_FormDate extends fapitng_FormInput {
  // @todo Do we need a theme callback for this element?
  public $theme_callback = 'fapitng_form_date';
  public $type = 'date';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    // @todo Replace form_process_date().
      $this->expand_callbacks[] = 'fapitng_form_expand_date';
  }
}

class fapitng_FormRadio extends fapitng_FormInput {
  public $return_value = '';
  public $theme_callback = 'fapitng_form_radio';
  public $type = 'radio';
}

class fapitng_FormRadios extends fapitng_FormInput {
  public $expand_callbacks = array('fapitng_form_expand_radios');
  public $options = array();
  // @ todo Implement pre render callback.
  public $pre_render_callbacks = array('form_pre_render_conditional_form_element');
  public $type = 'radios';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    array_unshift($this->theme_wrapper_callbacks, 'fapitng_form_radios');
  }
}

class fapitng_FormCheckbox extends fapitng_FormInput {
  public $return_value = '';
  public $theme_callback = 'fapitng_form_checkbox';
  public $type = 'checkbox';
}

class fapitng_FormCheckboxes extends fapitng_FormInput {
  public $expand_callbacks = array('fapitng_form_expand_checkboxes');
  public $options = array();
  // @ todo Implement pre render callback.
  public $pre_render_callbacks = array('form_pre_render_conditional_form_element');
  public $type = 'checkboxes';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    array_unshift($this->theme_wrapper_callbacks, 'fapitng_form_checkboxes');
  }
}

class fapitng_FormFile extends fapitng_FormInput {
  public $theme_callback = 'fapitng_form_file';
  public $type = 'file';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->attributes['size'] = 60;
  }
}

class fapitng_FormHidden extends fapitng_FormInput {
  public $theme_callback = 'fapitng_form_hidden';
  public $type = 'hidden';
}

/**
 * A form token to validate the form's authenticity.
 *
 * Using more than one in the same form isn't useful, because they will both
 * execute exactly the same validation.
 */
class fapitng_FormToken extends fapitng_FormHidden {
  public $type = 'token';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->validate_callbacks['drupal_valid_token'] = array(
      'message' => t('This form is outdated. Reload the page and try again.'),
      'arguments' => array($this->form->id),
    );
  }
}

class fapitng_FormItem extends fapitng_FormElement {
  public $markup = '';
  // @todo callback cannot work with elementstng_Elements yet.
  public $pre_render_callbacks = array('drupal_pre_render_markup');
  public $type = 'item';
}

function fapitng_form_expand_form(fapitng_Form $form) {
  $form->attributes['method'] = $form->method;
  $form->attributes['action'] = drupal_strip_dangerous_protocols($form->action);
}

function fapitng_form_expand_input(fapitng_FormInput $element) {
  if ($element->disabled) {
    $element->attributes['class'][] = 'disabled';
  }
  $element->attributes['value'] = $element->value;
}

function fapitng_form_expand_required(fapitng_FormInput $element) {
  if ($element->required) {
    $element->validate_callbacks['fapitng_valid_not_empty'] = array(
      'message' => t('%title is required.', array('%title' => $element->title)),
    );
    $element->attributes['class'][] = 'required';
  }
}

function fapitng_form_expand_textarea(fapitng_FormTextarea $element) {
  if ($element->resizable) {
    $element->attached['js']['misc/textarea.js'] = array(
      'type' => 'file',
    );
  }
}

function fapitng_form_expand_text_input(fapitng_FormTextInput $element) {
  if ($element->min_length) {
    // Minimum and maximum lengths have been set.
    if ($element->max_length) {
      $message = t('%title must be !min to !max characters long.', array('%title' => $element->title, '!min' => $element->min_length, '!max' => $element->max_length));
    }
    // Only the minimum length has been set.
    else {
      $message = t('%title must be at least !min characters long.', array('%title' => $element->title, '!min' => $element->min_length));
    }
  }
  // Only the maximum length has been set.
  elseif ($element->max_length) {
    $message = t('%title cannot be longer than !max characters.', array('%title' => $element->title, '!max' => $element->max_length));
  }
  if (isset($message)) {
    $element->validate_callbacks['fapitng_valid_string_length'] = array(
      'message' => $message,
      'arguments' => array($element->min_length, $element->max_length),
    );
  }
}

function fapitng_form_expand_textfield(fapitng_FormTextfield $element) {
  if ($element->max_length) {
    $element->attributes['class']['maxlength'] = $element->max_length;
  }
}