<?php

/**
 * @file theme.inc
 *
 * An array of preprocessors to fill variables for templates and helper
 * functions to make theming easier.
 */

/**
 * Theme the form for the table style plugin
 */
function theme_views_calc_ui_table($variables) {
  $form = $variables['form'];
  $output = drupal_render($form['description_markup']);

  $header = array(
    t('Field'),
    t('Justification'),
    t('Column calculations'),
    t('Column'),
    t('Separator'),
    array(
      'data' => t('Sortable'),
      'align' => 'center',
    ),
    array(
      'data' => t('Default sort'),
      'align' => 'center',
    ),
  );
  $rows = array();
  foreach (element_children($form['columns']) as $id) {
    $row = array();
    $row[] = drupal_render($form['info'][$id]['name']);
    $row[] = drupal_render($form['info'][$id]['align']);
    $row[] = drupal_render($form['info'][$id]['has_calc']) . drupal_render($form['info'][$id]['calc']);
    $row[] = drupal_render($form['columns'][$id]);
    $row[] = drupal_render($form['info'][$id]['separator']);
    if (!empty($form['info'][$id]['sortable'])) {
      $row[] = array(
        'data' => drupal_render($form['info'][$id]['sortable']),
        'align' => 'center',
      );
      $row[] = array(
        'data' => drupal_render($form['default'][$id]),
        'align' => 'center',
      );
    }
    else {
      $row[] = '';
      $row[] = '';
    }
    $rows[] = $row;
  }

  // Add the special 'None' row.
  $rows[] = array(t('None'), '', '', '', '', array(
      'align' => 'center',
      'data' => drupal_render($form['default'][-1]),
    ));

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= drupal_render_children($form);
  return $output;
}


/**
 * Display a view as a table style.
 */
function template_preprocess_views_calc_table(&$vars) {
  $view = $vars['view'];
  if (!empty($view->views_calc_calculation)) {
    $vars['rows'] = array();
    return;
  }

  drupal_add_css(drupal_get_path('module', 'views_calc') . '/views_calc.css');

  // We need the raw data for this grouping, which is passed in as $vars['rows'].
  // However, the template also needs to use for the rendered fields.  We
  // therefore swap the raw data out to a new variable and reset $vars['rows']
  // so that it can get rebuilt.
  $result   = $vars['rows'];
  $vars['rows'] = array();

  $options  = $view->style_plugin->options;
  $handler  = $view->style_plugin;
  $vars['options'] = $options;
  $hide_details = $options['detailed_values'];

  $fields   = &$view->field;
  $columns  = $handler->sanitize_columns($options['columns'], $fields);

  $active   = !empty($handler->active) ? $handler->active : '';
  $order    = !empty($handler->order) ? $handler->order : 'asc';

  $query    = tablesort_get_query_parameters();
  if ($query) {
    $query = '&' . $query;
  }

  // Render the header labels.
  foreach ($columns as $field => $column) {
    if ($field == $column && empty($fields[$field]->options['exclude'])) {
      $label = check_plain(!empty($fields[$field]) ? $fields[$field]->label() : '');
      if (empty($options['info'][$field]['sortable'])) {
        $vars['header'][$field] = $label;
      }
      else {
        // @todo -- make this a setting
        $initial = 'asc';

        if ($active == $field && $order == 'asc') {
          $initial = 'desc';
        }

        $image = theme('tablesort_indicator', array('style' => $initial));
        $title = t('sort by @s', array('@s' => $label));
        $link_options = array(
          'html' => true,
          'attributes' => array('title' => $title),
          'query' => 'order=' . urlencode($field) . '&sort=' . $initial . $query,
        );
        $vars['header'][$field] = l($label . $image, $_GET['q'], $link_options);
      }
    }

    // Create a second variable so we can easily find what fields we have and what the
    // CSS classes should be.
    $vars['fields'][$field] = drupal_clean_css_identifier($field);
    if ($active == $field) {
      $vars['fields'][$field] .= ' active';
    }
  }

  // Render each field into its appropriate column. Preserve rows.
  foreach ($result as $num => $row) {
    foreach ($columns as $field => $column) {
      $field_output = $fields[$field]->theme($row);
      if (!empty($fields[$field]) && empty($fields[$field]->options['exclude'])) {

        // Don't bother with separators and stuff if the field does not show up.
        if (!isset($field_output) && isset($vars['rows'][$num][$column])) {
          continue;
        }

        if ($hide_details) {
          continue;
        }

        // Place the field into the column, along with an optional separator.
        if (isset($vars['rows'][$num][$column])) {
          if (!empty($options['info'][$column]['separator'])) {
            $vars['rows'][$num][$column] .= filter_xss_admin($options['info'][$column]['separator']);
          }
        }
        else {
          $vars['rows'][$num][$column] = '';
        }

        $vars['rows'][$num][$column] .= $field_output;
      }
    }
  }

  // Add totals.
  $vars['totals'] = array();
  $vars['sub_totals'] = array();
  if ($view->query->pager->get_total_items() > $view->get_items_per_page()
      && $view->sub_totals) {
    views_calc_table_total($vars, 'sub_totals', $view->sub_totals);
  }
  if ($view->totals) {
    views_calc_table_total($vars, 'totals', $view->totals);
  }

  // Add classes.
  $vars['class'] = 'views-table';
  if (!empty($options['sticky'])) {
    drupal_add_js('misc/tableheader.js');
    $vars['class'] .= " sticky-enabled";
  }
}

/**
 * Build total var line.
 */
function views_calc_table_total(&$vars, $key, $totals) {

  $view = $vars['view'];
  $options  = $view->style_plugin->options;
  $handler  = $view->style_plugin;
  $fields   = &$view->field;
  $columns  = $handler->sanitize_columns($options['columns'], $fields);

  $vars[$key] = array();
  $added_label = array();
  // Build a data baserow with the default fields of the view
  // Grundstruktur für normale viewszeile erstellen
  $baserow = new StdClass();
  foreach ($view->field as $field) {
    if (isset($field->aliases['entity_type'])) {
      $query_alias = $field->aliases['entity_type'];
    }
    else {
      $query_alias = $field->field_alias;
    }
    $baserow->{$query_alias} = $totals->{$query_alias};
  }
  // Build aggregation rows, one per function.
  foreach ($view->views_calc_fields as $calc => $calc_fields) {
    // Rebuild row as if it where single queries.
    $row = clone $baserow;
    // Zeile mit aggregationswerten füllen

    foreach ($view->field as $field) {
      if (isset($field->aliases['entity_type'])) {
        $query_alias = $field->aliases['entity_type'];
      }
      else {
        $query_alias = $field->field_alias;
      }
      $ext_alias = strtolower($calc) . '__' . $query_alias;
      if (in_array($field->field, $calc_fields)) {
        $row->{$query_alias} = $totals->{$ext_alias};
      }
    }
    // Build row output data.
    foreach ($columns as $field => $column) {
      //$field_alias = $fields[$field]->field_alias;
      if ((array_key_exists('entity_type',$fields[$field]->aliases))
      && isset($fields[$field]->aliases['entity_type'])) {
        //$field_alias = strtolower($calc) . '__' . $fields[$field]->aliases['entity_type'];
        $field_alias = $fields[$field]->aliases['entity_type'];
      }
      else {
        $field_alias = $fields[$field]->field_alias;
      }
      if ($field == $column && empty($fields[$field]->options['exclude'])) {
        // process only calculated fields
        if (in_array($field, $calc_fields) && isset($row->$field_alias)) {
          // COUNT is always a numeric value, no matter what kind of field it is.
          if ($calc == 'COUNT') {
            $vars[$key][$calc][$column] =  $row->$field_alias;
          }
          // Calculations other than COUNT should run the value through the field's theme.
          // This will allow dates and numeric values to apply the right formatting to the result.
          else {
            // TODO: Theme the output, see comment above and behind the statement
            $vars[$key][$calc][$column] = $row->$field_alias;//$fields[$field]->theme($row);
          }
        }
        else {
          // Add the calc type label into the first empty column.
          // Identify which is the sub total and which the grand total
          // when both are provided.
          if (empty($added_label[$calc])) {
            if ($key == 'sub_totals') {
              $label = t("Page !Calculation", array("!Calculation" => $calc));
            }
            else {
              $label = t("Total !Calculation", array("!Calculation" => $calc));
            }
            $vars[$key][$calc][$column] = $label;
            $added_label[$calc] = TRUE;
          }
          else {
            $vars[$key][$calc][$column] = '';
          }
        }
      }
    }
  }
}
