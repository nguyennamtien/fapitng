<?php
// $Id$

/**
 * Basic behavior shared by forms and form elements.
 */
abstract class fapitng_FormBase extends elementstng_RenderableElement {
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
  // @todo Confirm properties and their default values.
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
  public $destination = array(
    'path' => NULL,
    'options' => array(),
    'http_response_code' => 302
  );
  public $errors = array();
  public $expand_callbacks = array('fapitng_form_expand_form');
  // @todo This was $form_state['build_info']['files'].
  public $files = array();
  public $method = 'POST';
  public $no_cache = FALSE;
  public $programmed = FALSE;
  public $rebuild = FALSE;
  // Contains properties for this specific request that must not be cached.
  public $request = array(
    // @todo Confirm properties and their default values.
    'always_process' => NULL,
    'buttons' => array(),
    'complete form' => NULL,
    'executed' => FALSE,
    'groups' => array(),
    'input' => NULL,
    'invalid_elements' => array(),
    'must_validate' => NULL,
    'rebuild' => FALSE,
    'rebuild_info' => array(),
    'submit_handlers' => NULL,
    'submitted' => FALSE,
    'temporary' => array(),
    'raw_input' => array(),
    'trigger' => NULL,
    'validate_handlers' => NULL,
  );
  // Data storage intended for use by any module that needs to store something.
  public $storage = array();
  public $submit_element = NULL;
  public $theme_wrapper_callbacks = array('fapitng_form');
  public $token = '';
  public $type = 'form';
  public $validate_callbacks = array();
  protected $built = FALSE;

  function __construct($form_id, array $build_callbacks, array $arguments = array(), $programmed = FALSE) {
    global $user;

    parent::__construct($form_id);

    $this->action = request_uri();
    $this->attributes['id'] = drupal_html_id('form-' . $form_id);
    $this->build_callbacks = $build_callbacks;
    $this->build_id = drupal_hash_base64(uniqid(mt_rand(), TRUE) . mt_rand());
    if (isset($_GET['destination'])) {
      $this->destination['path'] = $_GET['destination'];
    }
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
    )));
    $this->addChild(new fapitng_FormHidden('form_id', array(
      'value' => $this->id,
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
    if (!$this->cacheGet()) {
      $this->prepare();
      $this->build();
      $this->expand();
      $this->populate();
    }
  }

  /**
   * Gather user inputted data and populate the form with it.
   *
   * Forms can receive input in different ways:
   * 1) The form is set to accept POST and there is POST data.
   * 2) The form is set to accept POST and there is GET data (prepopulate).
   * 3) The form is set to accept GET and there is GET data.
   *
   * @return NULL
   */
  function receive() {
    // Input type 1.
    if ($this->method == 'POST' && isset($_POST[$this->id . '-form_id']) && $_POST[$this->id . '-form_id'] == $this->id) {
      $this->request['raw_input'] = $_POST;
    }
    // Input types 2 and 3.
    else {
      $this->request['raw_input'] = $_GET;
    }
  }

  /**
   * Populate the form's input elements and request data.
   *
   * Population needs to be performed *after* the construction stage, so
   * elements can populate their children, which have been created later.
   *
   * @return NULL
   */
  function populate() {
    $this->populateInputElements($this->children);
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
        $element->populate();
      }
      $this->populateInputElements($element->children);
    }
  }

  function render() {
    // Set system messages for validation errors.
    foreach ($this->request['invalid_elements'] as $element) {
      foreach ($element->errors as $error) {
        drupal_set_message($error, 'error');
      }
    }

    return parent::render();
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
    if ($this->errors) {
      $this->request['invalid_elements'][$this->id] = $this;
    }
    $this->validateInputElements($this->children);

    if (!empty($this->request['invalid_elements'])) {
      $this->request['rebuild'] = TRUE;
      return FALSE;
    }
    return TRUE;
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
          $this->request['valid'] = FALSE;
          $this->request['invalid_children'][] = $element;
        }
      }
      $this->validateInputElements($element->children);
    }
  }

  /**
   * Redirect the user based on form data.
   *
   * @return NULL
   */
  function redirect() {
    if (!$this->programmed && !$this->request['rebuild'] && $this->destination) {
      if (drupal_installation_attempted()) {
        install_goto($this->destination['path']);
      }
      else {
        drupal_goto($this->destination['path'], $this->destination['options'], $this->destination['http_response_code']);
      }
    }
  }

  /**
   * Cache the form.
   *
   * @see fapitng_FormElement::__sleep()
   *
   * @return NULL
   */
  function cacheSet() {
    // @todo Do we need a #cache_token next to the regular token?
    if ($this->token && $this->cache && !$this->no_cache) {
      // Don't cache the form's request data.
      $request = $this->request;
      unset($this->request);
      cache_set($this->build_id, get_object_vars($this), 'fapitng_cache_form', REQUEST_TIME + 21600);
      $this->request = $request;
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

    if (isset($this->request['raw_input'][$this->id . '-form_build_id']) && $cached = cache_get($this->request['raw_input'][$this->id . '-form_build_id'], 'fapitng_cache_form')) {
      $properties = $cached->data;

      if ($user->uid && drupal_valid_token($properties['token'], $this->id)) {
        foreach ($properties as $property => $value) {
          $this->$property = $value;
        }
        $this->cacheGetRestoreRelations($this);

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

  /**
   * Restore relations between the form and child elements after getting a form
   * from the cache.
   *
   * @param $parent fapitng_FormBase
   *   A form or a form element whose children to restore the relations for.
   */
  private function cacheGetRestoreRelations(fapitng_FormBase $parent) {
    foreach ($parent->children as $element) {
      $element->form = $this;
      $element->form = $parent;
      $this->cacheGetRestoreRelations($element);
    }
  }
}

/**
 * Base class for all form elements.
 */
class fapitng_FormElement extends fapitng_FormBase {
  public $access = TRUE;
  public $form = NULL;
  public $prefix = '';
  public $states = array();
  public $suffix = '';
  public $theme_wrapper_callbacks = array('fapitng_form_element');
  public $tree = FALSE;
  // @todo Are these wrapper properties going to be used?
  public $wrapper_prefix = '';
  public $wrapper_suffix = '';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
  }

  function __sleep() {
    // Unset (possible) references to the form, because the form object will no
    // longer exist after caching.
    return array_diff(array_keys(get_object_vars($this)), array('form', 'parent'));
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
  public $disabled = FALSE;
  public $errors = array();
  public $expand_callbacks = array('fapitng_form_expand_input', 'fapitng_form_expand_required');
  public $validate_callbacks = array();
  public $required = FALSE;
  public $value = NULL;

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->attributes['name'] = '';
    $this->attributes['value'] = '';
  }

  /**
   * Set the value of this input element.
   *
   * By default calling this method equals setting $this->value directly.
   * Subclasses can however implement their own value() method to perform extra
   * actions. This is particularly useful for elements that exist of multiple
   * child elements that need to have their value set as soon as the parent's
   * value is set.
   *
   * @param $value mixed
   *   The value to give to $this->value.
   *
   * @return NULL
   */
  function populate() {
    // @todo We inject client input directly into the form. What about
    // security issues?
    if (isset($this->form->request['raw_input'][$this->attributes['name']])) {
      // Disabled input elements do not accept new input.
      if (!$this->disabled) {
        $this->value = $this->form->request['raw_input'][$this->attributes['name']];
      }
    }
  }

  /**
   * Check user input for the input element.
   *
   * @return boolean
   *   Whether the element's value is valid or not.
   */
  function validate() {
    foreach ($this->validate_callbacks as $callback => $info) {
      $info += array(
        'arguments' => array(),
      );
      if (!call_user_func_array($callback, array_merge(array($this->value), $info['arguments']))) {
        $this->errors[] = $info['message'];
        $this->form->request['invalid_elements'][$this->id] = $this;
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
abstract class fapitng_FormTextInput extends fapitng_FormInput {
  public $max_length = 0;
  public $min_length = 0;

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_text_input';
  }
}

class fapitng_FormText extends fapitng_FormTextInput {
  public $autocomplete_path = '';
  public $max_length = 128;
  public $theme_callback = 'fapitng_form_text';
  public $type = 'text';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_text';
    $this->attributes['size'] = 60;
  }
}

class fapitng_FormPassword extends fapitng_FormText {
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
  // Regular buttons do not trigger form submissions without JavaScript, so
  // hide if there is no JavaScript support.
  // @todo Implement $limit_validation ($limit_validation_errors in Drupal 7).
  public $limit_validation = array();
  public $return_value = '';
  public $theme_callback = 'fapitng_form_button';
  public $type = 'button';
}

class fapitng_FormSubmit extends fapitng_FormButton {
  public $type = 'submit';

  function populate() {
    parent::populate();
    if (isset($this->form->request['raw_input'][$this->attributes['name']]) && $this->form->request['raw_input'][$this->attributes['name']] == $this->value) {
      $this->form->request['trigger'] = $this;
      $this->form->request['submitted'] = TRUE;
    }
  }
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

/**
 * A date picker.
 *
 * @todo Can we support more calendars than the Gregorian?
 * @todo Add support for BCE.
 */
class fapitng_FormDate extends fapitng_FormText {
  public $max_length = 10;
  public $type = 'date';

  function __construct($id, array $properties = array()) {
    // First set the description, then call the parent constructor, so it can
    // be overridden manually.
    $this->description = t('Use the format YYYY-MM-DD.');
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_date';
    $this->attributes['size'] = 10;
  }
}

/**
 * A time picker.
 *
 * @todo Add support for 12-hour notations.
 */
class fapitng_FormTime extends fapitng_FormText {
  public $max_length = 8;
  public $type = 'time';

  function __construct($id, array $properties = array()) {
    // First set the description, then call the parent constructor, so it can
    // be overridden manually.
    $this->description = t('Use the format HH:MM:SS.');
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_time';
    $this->attributes['size'] = 8;
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
  public $type = 'radios';

  function populate() {
    parent::populate();
    foreach ($this->children as $element) {
      $element->value = $element->attributes['value'] == $this->value;
    }
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

function fapitng_form_expand_options(fapitng_FormInput $element) {
  $element->validate_callbacks['in_array'] = array(
    'message' => t('%title has a value that does not appear in its available options.', array('%title' => $element->title)),
    'arguments' => $element->options,
  );
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

function fapitng_form_expand_text(fapitng_FormText $element) {
  if ($element->max_length) {
    $element->attributes['maxlength'] = $element->max_length;
  }
}

function fapitng_form_expand_radios(fapitng_FormRadios $element) {
  foreach ($element->options as $key => $value) {
    $radio = new fapitng_FormRadio($key, array(
      'title' => $value,
      // Radios handle boolean values.
      'value' => $element->value == $key,
    ));
    $radio->attributes['name'] = $element->attributes['name'];
    // Although radios handle boolean values, HTML forms don't work that way.
    // Set the value attribute to make the radio submit a unique value which we
    // can convert to a boolean.
    $radio->attributes['value'] = $key;
    $element->addChild($radio);
  }
}

function fapitng_form_expand_date(elementstng_Element $element) {
    $element->validate_callbacks['fapitng_valid_iso8601_date'] = array(
      'message' => t('%title must be of the format YYYY-MM-DD.', array('%title' => $element->title)),
    );
}

function fapitng_form_expand_time(elementstng_Element $element) {
    $element->validate_callbacks['fapitng_valid_iso8601_time'] = array(
      'message' => t('%title must be of the format HH:MM:SS.', array('%title' => $element->title)),
    );
}