<?php
// $Id$

abstract class elementstng_Element {
  // @todo Add process callback for AJAX settings.
  public $ajax = array();
  public $attributes = array();
  public $description = '';
  public $expand_callbacks = array();
  public $id = '';
  public $post_render_callbacks = array();
  public $pre_render_callbacks = array();
  public $rendered_content = '';
  public $theme_callback = '';
  public $theme_wrapper_callbacks = array();
  public $title = '';
  public $type = '';
  public $weight = 0;
  protected $children = array();
  protected $rendered = FALSE;

  function __construct($id, array $properties = array()) {
    $this->id = $id;
    foreach ($properties as $property => $value) {
      if (is_array($value)) {
        $this->$property = array_merge($this->$property, $value);
      }
      else {
        $this->$property = $value;
      }
    }
  }

  function children() {
    return $this->children;
  }

  function child($element_id) {
    return $this->children[$element_id];
  }

  function addChild(elementstng_Element $element) {
    $this->children[$element->id] = $element;
    $element->parent = $this;
    $element->settle();
  }

  function removeChild(elementstng_Element $element) {
    unset($this->children[$element->id]);
  }

  /**
   * Perform actions if the element is added to a parent.
   *
   * Empty function, because it's optional, but we don't want to check for its
   * existence every time we need it, and this way it documents itself.
   */
  function settle() {
  }

  /**
   * Expand the element's functionality, i.e. convert a password confirm element
   * to two separate fields with extra validation.
   */
  function expand() {
    foreach ($this->children as $element) {
      $element->expand();
    }
    foreach ($this->expand_callbacks as $callback) {
      call_user_func($callback, $this);
    }
  }

  function hide() {
    $this->rendered = TRUE;
  }

  function show() {
    $this->rendered = FALSE;
  }

  /**
   * Make the element ready for display on screen.
   */
  function render() {
    // @todo Return output if $this->rendered == FALSE, but only actually
    // render the element once. Also, keep this comment. It's useful. -XXX-.
    if (!$this->rendered) {
      foreach ($this->pre_render_callbacks as $callback) {
        call_user_func($callback, $this);
      }
      // Allow pre_render to abort rendering by calling $this->hide().
      if (!$this->rendered) {
        $this->rendered = TRUE;

        // Render the element itself.
        $this->rendered_content = theme($this->theme_callback, array(
          'element' => $this,
        ));


        // Render child elements.
        foreach ($this->children as $element) {
          $this->rendered_content .= $element->render();
        }
        foreach ($this->theme_wrapper_callbacks as $callback) {
          $this->rendered_content = theme($callback, array(
            'element' => $this,
          ));
        }
        foreach ($this->post_render_callbacks as $callback) {
          call_user_func($callback, $this);
        }
      }

      return $this->rendered_content;
    }
  }
}

class elementstng_Markup extends elementstng_Element {
  public $markup = '';
  // @todo callback cannot work with elementstng_Elements yet.
  public $pre_render_callbacks = array('drupal_pre_render_markup');
  public $type = 'markup';
}

class elementstng_Page extends elementstng_Element {
  public $show_messages = TRUE;
  public $theme = 'theme';
  public $theme_wrappers = array('html');
  public $type = 'page';
}

class elementstng_AJAX extends elementstng_Element {
  public $commands = array();
  public $error = NULL;
  public $header = TRUE;
  public $type = 'ajax';
}

class elementstng_HTMLTag extends elementstng_Element {
  // @todo Callback doesn't accept elementstng_Elements yet.
  public $pre_render_callbacks = array('drupal_pre_render_conditional_comments');
  public $theme_callback = 'elementstng_html_tag';
  public $type = 'html_tag';
  public $value = NULL;
}

class elementstng_Styles extends elementstng_Element {
  // @todo How do we implement group callbacks?
  public $group_callback = 'drupal_group_css';
  // @todo How do we implement aggregate callbacks?
  public $aggregate_callback = 'drupal_aggregate_css';
  public $items = array();
  // @todo Callback doesn't accept elementstng_Elements yet.
  public $pre_render_callbacks = array('drupal_pre_render_styles');
  public $type = 'styles';
  
}

class elementstng_Link extends elementstng_Markup {
  public $type = 'link';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
      // @todo Callback doesn't accept elementstng_Elements yet.
      $this->pre_render_callbacks[] = 'drupal_pre_render_link';
  }
}

class elementstng_Container extends elementstng_Element {
  public $expand_callbacks = array('elementstng_expand_container');
  public $theme_wrapper_callbacks = array('elementstng_container');
  public $type = 'container';
}

// @todo What does an Actions element do?
class elementstng_Actions extends elementstng_Container {
  public $type = 'actions';

  function __construct($id, array $properties = array()) {
    parent::__construct($id, $properties);
      // @todo Replace form_process_actions().
      $this->expand_callbacks[] = 'fapitng_expand_actions';
  }
}