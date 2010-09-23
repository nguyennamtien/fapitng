<?php
// $Id$

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
 * Contains the entire form structure.
 */
class fapitng_Form extends fapitng_FormBase {
  public $action = '';
  public $ajax = array();
  public $attributes = array(
    'accept-charset' => 'UTF-8',
    'id' => '',
  );
  public $build_id = '';
  public $errors = array();
  public $expand_callbacks = array('fapitng_form_expand_form');
  public $method = 'POST';
	public $state = array();
  public $theme_wrapper_callbacks = array('fapitng_form');
	public $token = '';
  public $type = 'form';
	public $valid = TRUE;
  public $validate_callbacks = array();
  protected $built = FALSE;

	function __construct($form_id, array $build_callbacks, fapitng_FormState $form_state) {
	  global $user;

    $this->action = request_uri();
	  $this->attributes['id'] = drupal_html_id('form-' . $form_id);
		$this->build_callbacks = $build_callbacks;
		$this->build_id = drupal_hash_base64(uniqid(mt_rand(), TRUE) . mt_rand());
	  $this->id = $form_id;
		$this->state = $form_state;
    // Add a token to forms displayed to authenticated users. This ensures that
    // any submitted form was actually requested previously by the user and
    // protects against cross site request forgeries.
    // This does not apply to programmatically submitted forms. Furthermore,
    // since tokens are session-bound and forms displayed to anonymous users
    // are very likely cached, we cannot assign a token for them. During
    // installation, there is no $user yet.
		if (!empty($user->uid) &&  !$this->state->programmed) {
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
	 */
	function build() {
	  if (!$this->built) {
	    $this->built = TRUE;
  		foreach ($this->build_callbacks as $callback) {
  			call_user_func($callback, $this);
  			drupal_alter("fapitng_form_$callback", $this);
  		}
  		drupal_alter('fapitng_form', $this);
		}
	}

  /**
   * Gather user inputted data and populate the form with it.
   */
	function populate() {
	  // Collect inputted data. Forms can receive input in three different ways:
	  // - The form is set to accept POST and there is POST data.
	  // - The form is set to accept POST and there is GET data (prepopulate).
	  // - The form is set to accept GET and there is GET data.
    if ($this->method == 'POST' && isset($_POST['form_id']) && $_POST['form_id'] == $this->id) {
      $this->state->raw_input = $_POST;
    }
    else {
      $this->state->raw_input = $_GET;
    }
    print_r($this->state->raw_input);

    // @todo Populate elements using (sanitized?) input and the default values.
	}

	/**
	 * Validate the form and its input elements.
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
	    $this->state->rebuild = TRUE;
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
}

/**
 * Base class for all form elements.
 */
class fapitng_FormElement extends fapitng_FormBase {
  public $access = TRUE;
  public $ajax = array();
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

/**
 * Base class for elements that accept text input.
 */
class fapitng_FormTextInput extends fapitng_FormInput {
  public $expand_callbacks = array('fapitng_form_expand_text_input');
  public $max_length = 0;
  public $min_length = 0;
}

class fapitng_FormFieldset extends fapitng_FormElement {
  public $collapsed = FALSE;
  public $collapsible = FALSE;
  public $group = NULL;
  public $type = 'fieldset';
}

class fapitng_FormVerticalTabs extends fapitng_FormElement {
  public $default_tab = '';
  public $type = 'vertical_tabs';
}

class fapitng_FormPassword extends fapitng_FormTextInput {
  public $expand_callbacks = array('fapitng_form_expand_text');
  public $max_length = 128;
  public $size = 60;
  public $type = 'password';
}

class fapitng_FormText extends fapitng_FormTextInput {
  public $autocomplete_path = '';
  public $expand_callbacks = array('fapitng_form_expand_text');
  public $max_length = 128;
  public $size = 60;
  public $text_format = FILTER_FORMAT_DEFAULT;
  public $type = 'text';
}

class fapitng_FormTextarea extends fapitng_FormTextInput {
  public $autocomplete_path = '';
  public $cols = 60;
  public $resizable = TRUE;
  public $rows = 5;
  public $theme_callback = 'fapitng_form_element_textarea';
  public $type = 'textarea';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
    $this->expand_callbacks[] = 'fapitng_form_expand_textarea';
  }
}

class fapitng_FormWeight extends fapitng_FormInput {
  public $delta = 10;
  public $type = 'weight';
}

class fapitng_FormTableSelect extends fapitng_FormInput {
  public $empty = '';
  public $header = array();
  public $js_select = FALSE;
  public $multiple = FALSE;
  public $options = array();
  public $type = 'table_select';
}

class fapitng_FormButton extends fapitng_FormInput {
  public $execute_submit_handler = TRUE;
  public $limit_validation_errors = array();
  public $return_value = '';
  public $theme_callback = 'fapitng_form_element_button';
  public $type = 'button';
}

class fapitng_FormImageButton extends fapitng_FormButton {
  public $source = '';
  public $type = 'image_button';
}

class fapitng_FormSelect extends fapitng_FormInput {
  public $multiple = FALSE;
  public $options = array();
  public $type = 'select';
}

class fapitng_FormCheckboxes extends fapitng_FormInput {
  public $options = array();
  public $type = 'checkboxes';
}

class fapitng_FormRadios extends fapitng_FormInput {
  public $options = array();
  public $type = 'radios';
}

class fapitng_FormCheckbox extends fapitng_FormInput {
  public $return_value = '';
  public $type = 'checkbox';
}

class fapitng_FormRadio extends fapitng_FormInput {
  public $return_value = '';
  public $type = 'radio';
}

class fapitng_FormDate extends fapitng_FormInput {
  public $type = 'date';
}

class fapitng_FormFile extends fapitng_FormInput {
  public $type = 'file';
}

class fapitng_FormManagedFile extends fapitng_FormInput {
  public $type = 'managed_file';
}

class fapitng_FormPasswordConfirm extends fapitng_FormInput {
  public $type = 'password_confirm';
}

class fapitng_FormHidden extends fapitng_FormInput {
  public $theme_callback = 'fapitng_form_element_hidden';
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

function fapitng_form_expand_text(fapitng_FormTextfield $element) {
  if ($element->max_length) {
    $element->attributes['class']['maxlength'] = $element->max_length;
  }
}