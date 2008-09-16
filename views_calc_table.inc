<?php
// $Id$
/**
 * @file
 * Contains the table style plugin.
 */

/**
 * Style plugin to render each item as a row in a table.
 *
 * @ingroup views_style_plugins
 */
class views_calc_table extends views_plugin_style {
  function option_definition() {
    $options = parent::option_definition();

    $options['columns'] = array('default' => array());
    $options['default'] = array('default' => '');
    $options['info'] = array('default' => array());
    $options['override'] = array('default' => TRUE);
    $options['sticky'] = array('default' => FALSE);
    $options['order'] = array('default' => 'asc');

    return $options;
  }

  /**
   * Determine if we should provide sorting based upon $_GET inputs.
   */
  function build_sort() {
    if (!isset($_GET['order'])) {
      // check for a 'default' clicksort. If there isn't one, exit gracefully.
      if (empty($this->options['default'])) {
        return TRUE;
      }
      $sort = $this->options['default'];
      $this->order = !empty($this->options['order']) ? $this->options['order'] : 'asc';
    }
    else {
      $sort = $_GET['order'];
      // Store the $order for later use.
      $this->order = !empty($_GET['sort']) ? strtolower($_GET['sort']) : 'asc';
    }

    // If a sort we don't know anything about gets through, exit gracefully.
    if (empty($this->view->field[$sort])) {
      return TRUE;
    }

    // Ensure $this->order is valid.
    if ($this->order != 'asc' && $this->order != 'desc') {
      $this->order = 'asc';
    }

    // Store the $sort for later use.
    $this->active = $sort;

    // Tell the field to click sort.
    $this->view->field[$sort]->click_sort($this->order);

    // Let the builder know whether or not we're overriding the default sorts.
    return empty($this->options['override']);
  }

  /**
   * Normalize a list of columns based upon the fields that are
   * available.
   *
   * - Each field must be in a column.
   * - Each column must be based upon a field, and that field
   *   is somewhere in the column.
   * - Any fields not currently represented must be added.
   * - Columns must be re-ordered to match the fields.
   *
   * @param $columns
   *   An array of all fields; the key is the id of the field and the
   *   value is the id of the column the field should be in.
   * @param $fields
   *   The fields to use for the columns. If not provided, they will
   *   be requested from the current display. The running render should
   *   send the fields through, as they may be different than what the
   *   display has listed due to access control or other changes.
   */
  function sanitize_columns($columns, $fields = NULL) {
    $sanitized = array();
    if ($fields === NULL) {
      $fields = $this->display->handler->get_option('fields');
    }

    // Preconfigure the sanitized array so that the order is retained.
    foreach ($fields as $field => $info) {
      // Set to itself so that if it isn't touched, it gets column
      // status automatically.
      $sanitized[$field] = $field;
    }

    foreach ($columns as $field => $column) {
      // first, make sure the field still exists.
      if (!isset($sanitized[$field])) {
        continue;
      }

      // If the field is the column, mark it so, or the column
      // it's set to is a column, that's ok
      if ($field == $column || $columns[$column] == $column && !empty($sanitized[$column])) {
        $sanitized[$field] = $column;
      }
      // Since we set the field to itself initially, ignoring
      // the condition is ok; the field will get its column
      // status back.
    }

    return $sanitized;
  }

  /**
   * Render the given style.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $handlers = $this->display->handler->get_handlers('field');
    if (empty($handlers)) {
      $form['error_markup'] = array(
        '#value' => t('You need at least one field before you can configure your table settings'),
        '#prefix' => '<div class="error form-item description">',
        '#suffix' => '</div>',
      );
      return;
    }

    $form['override'] = array(
      '#type' => 'checkbox',
      '#title' => t('Override normal sorting if click sorting is used'),
      '#default_value' => !empty($this->options['override']),
    );

    $form['sticky'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable Drupal style "sticky" table headers (Javascript)'),
      '#default_value' => !empty($this->options['sticky']),
      '#description' => t('(Sticky header effects will not be active for preview below, only on live output.)'),
    );

    $form['order'] = array(
      '#type' => 'select',
      '#title' => t('Default sort order'),
      '#options' => array('asc' => t('Ascending'), 'desc' => t('Descending')),
      '#default_value' => $this->options['order'],
      '#description' => t('If a default sort order is selected, what order should it use by default.'),
    );

    // Note: views UI registers this theme handler on our behalf. Your module
    // will have to register your theme handlers if you do stuff like this.
    $form['#theme'] = 'views_calc_ui_table';

    $columns = $this->sanitize_columns($this->options['columns']);

    // Create an array of allowed columns from the data we know:
    foreach ($handlers as $field => $handler) {
      if ($label = $handler->label()) {
        $field_names[$field] = $label;
      }
      else {
        $field_names[$field] = $handler->ui_name();
      }
    }

    if (isset($this->options['default'])) {
      $default = $this->options['default'];
      if (!isset($fields[$default])) {
        $default = -1;
      }
    }
    else {
      $default = -1;
    }

    foreach ($columns as $field => $column) {
      $safe = str_replace(array('][', '_', ' '), '-', $field);
      // the $id of the column for dependency checking.
      $id = 'edit-style-options-columns-' . $safe;

      $form['columns'][$field] = array(
        '#type' => 'select',
        '#options' => $field_names,
        '#default_value' => $column,
      );
      if ($handlers[$field]->click_sortable()) {
        $form['info'][$field]['sortable'] = array(
          '#type' => 'checkbox',
          '#default_value' => !empty($this->options['info'][$field]['sortable']),
          '#process' => array('views_process_dependency'),
          '#dependency' => array($id => array($field)),
        );
        // Provide an ID so we can have such things.
        $radio_id = form_clean_id('edit-default-' . $field);
        $form['default'][$field] = array(
          '#type' => 'radio',
          '#return_value' => $field,
          '#parents' => array('style_options', 'default'),
          '#id' => $radio_id,
          // because 'radio' doesn't fully support '#id' =(
          '#attributes' => array('id' => $radio_id),
          '#default_value' => $default,
          '#process' => array('views_process_dependency'),
          '#dependency' => array($id => array($field)),
        );
      }
      $form['info'][$field]['separator'] = array(
        '#type' => 'textfield',
        '#size' => 10,
        '#default_value' => isset($this->options['info'][$field]['separator']) ? $this->options['info'][$field]['separator'] : '',
        '#process' => array('views_process_dependency'),
        '#dependency' => array($id => array($field)),
      );

      $form['info'][$field]['has_calc'] = array(
        '#type' => 'checkbox',
        '#title' => t('Display calculation'),
        '#default_value' => isset($this->options['info'][$field]['has_calc']) ? $this->options['info'][$field]['has_calc'] : 0,
        '#process' => array('views_process_dependency'),
        '#dependency' => array($id => array($field)),
      );
      
      $options = _views_calc_calc_options();
      $form['info'][$field]['calc'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => isset($this->options['info'][$field]['calc']) ? $this->options['info'][$field]['calc'] : array(),
        '#process' => array('views_process_dependency'),
        '#dependency' => array('edit-style-options-info-'. $safe .'-has-calc' => array(TRUE)),
        '#multiple' => TRUE,
      );
      
      // markup for the field name
      $form['info'][$field]['name'] = array(
        '#value' => $field_names[$field],
      );
    }

    // Provide a radio for no default sort
    $form['default'][-1] = array(
      '#type' => 'radio',
      '#return_value' => -1,
      '#parents' => array('style_options', 'default'),
      '#id' => 'edit-default-0',
      '#default_value' => $default,
    );

    $form['description_markup'] = array(
      '#prefix' => '<div class="description form-item">',
      '#suffix' => '</div>',
      '#value' => t('Place fields into columns; you may combine multiple fields into the same column. If you do, the separator in the column specified will be used to separate the fields. Check the sortable box to make that column click sortable, and check the default sort radio to determine which column will be sorted by default, if any. You may control column order and field labels in the fields section.'),
    );
  }

  /**
   * TODO
   * figure out what changes are needed so Views field groups will work.
   */
  function pre_render($results) {
    // If there are no calc fields, do nothing.
    if (!$calc_fields = $this->get_calc_fields()) {
      return;
    }
    // If we're not getting a summary row, do nothing.
    if (!empty($this->view->views_calc_calculation)) {
      return;
    }
    
    $nodes = array();
    foreach ($this->view->result as $delta => $value) {
      $nids[] = $value->nid;
    }
    // Add summary rows to the results.
    $this->view->totals = array();
    foreach ($calc_fields as $calc => $field) {
      $summary_view = views_get_view($this->view->name);
      $display = $summary_view->add_display($this->view->current_display);
      $summary_view->set_display($display);
      $summary_view->views_calc_calculation = $calc;
      $summary_view->views_calc_nids = $nids;
      $summary_view->is_cacheable = FALSE;
      $summary_view->execute_display($display, $this->view->args);
      $this->view->totals[] = array_shift($summary_view->result);
    }
  }
  
  function query() {
    parent::query();
        
    // If we're not getting a summary row, do nothing.
    if (empty($this->view->views_calc_calculation)) {
      return;
    }
    // If there are no calc fields, do nothing.
    if (!$calc_fields = $this->get_calc_fields()) {
      return;
    }
    
    // Create summary rows.
    $calc = $this->view->views_calc_calculation;
    $fields = $calc_fields[$calc];
    foreach ($this->view->field as $field) {
      $query_field = $field->table .'.'. $field->field;
      $query_alias = $field->field_alias;
      if (isset($this->view->query->fields[$field->field_alias])) {
        unset($this->view->query->fields[$field->field_alias]);
      }
      if (in_array($field->field, $fields)) {
        // Calculated fields.
        $this->view->query->add_field(NULL, $calc .'('. $query_field .')', $field->field);
        $this->view->query->ensure_table($field->table);
      }
      else {
        // Empty fields that have no calculations.
        $this->view->query->add_field(NULL, "''", $field->field);
      }
      // Add a dummy field for the groupby.
      $this->view->query->add_field(NULL, "'". $calc ."'", "TOTAL_". $calc);
      $this->view->query->add_groupby("TOTAL_". $calc);
    }
    // TODO This won't work right with relationships, need a fix here.
    if (!empty($this->view->views_calc_nids)) {
      $this->view->query->add_where(NULL, "node.nid IN (%s)", implode(',', $this->view->views_calc_nids));
    }
  }

  function get_calc_fields() {
    $options  = $this->view->style_plugin->options;
    $handler  = $this->view->style_plugin;
    $fields   = $this->view->field;
    $columns  = $handler->sanitize_columns($options['columns'], $fields);
    
    $calc_fields = array();
    foreach ($columns as $field => $column) {
      if ($field == $column && empty($fields[$field]->options['exclude'])) {
        if ($options['info'][$field]['has_calc']) {
          foreach ($options['info'][$field]['calc'] as $calc) {
            $calc_fields[$calc][] = $field;
          }
        }
      }
    }
    return $calc_fields;
  }
}