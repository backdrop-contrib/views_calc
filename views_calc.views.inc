<?php
/**
 *  Implementation of hook_views_tables.
 *
 *  Add calc fields to views field list.
 */
function views_calc_views_tables() {
  $tables['views_calc'] = array(
    'name' => 'node',
    'join' => array(
      'left' => array(
        'table' => 'node',
        'field' => 'nid'
      ),
      'right' => array(
        'field' => 'nid'
      ),
    ),
  );
  // insert all available views_calc fields into field list as placeholders
  // using NULL for table name so views doesn't try to prepend the calc with a tablename
  $results = _views_calc_fields();
  while($field = db_fetch_array($results)) {
    $tables[NULL]['fields']['cid'. $field['cid']] = array(
      'name'          => t($field['label']),
      'notafield'     => true,
      'sortable'      => true,
      'query_handler' => 'views_calc_query_handler',
      'handler'       => 'views_calc_field_handler',
      'cid'           => $field['cid'],
      'format'        => $field['format'],
      'custom'        => $field['custom'],
      );
  }
  return $tables;
}

/**
 *  Query handler to update query
 *  use handler insert a real field with the calc into the query
 */
function views_calc_query_handler(&$field, &$fieldinfo, &$query) {
  $results = _views_calc_fields();
  while ($calc_field = db_fetch_array($results)) {
    if ($fieldinfo['cid'] == $calc_field['cid']) {
      foreach (explode(',', $calc_field['tablelist']) as $table) {
        $query->ensure_table($table);
      }
      $query->add_field("(". $calc_field['calc'] .") AS _cid". $calc_field['cid'], NULL);
    }
  }
}

/**
 *  The field handler.
 *  Used to format the calculation results
 */
function views_calc_field_handler(&$field, &$fieldinfo, $value, $data) {
  return _views_calc_format($value, $field['format'], $field['custom']);
}

function _views_calc_format($value, $format, $custom) {
  $formats = _views_calc_format_options();
  $format = $formats[$format];
  $tmp = explode(':', $format);
  $function = trim($tmp[0]);
  $vars     = $tmp[1];
  if ($function == 'custom') {
    $tmp = explode(':', $custom);
    $function = trim($tmp[0]);
    $vars     = $tmp[1];
  }
  if (empty($function) || $function == 'none') {
    $function = 'check_plain';
  }
  return $function($value, $vars);
}

/**
 * Implementation of hook_views_plugins
 */
function views_calc_views_plugins() {
  $path = drupal_get_path('module', 'views_calc');
  $views_path = drupal_get_path('module', 'views');
  require_once "./$path/theme.inc";

  $data = array(
    'module' => 'views_calc', // This just tells our themes are elsewhere.
    'style' => array(
      'parent' => array(
        // this isn't really a display but is necessary so the file can
        // be included.
        'no ui' => TRUE,
        'handler' => 'views_plugin_style',
        'path' => "$views_path/plugins",
        'parent' => '',
      ),
      // Style plugin for the navigation.
      'views_calc' => array(
        'title' => t('Views Calc Table'),
        'help' => t('Creates a table with column calculations.'),
        'handler' => 'views_calc_table',
        'path' => "$path",
        'parent' => 'parent',
        'theme' => 'views_calc_table',
        'theme file' => 'theme.inc',
        'theme path' => "$path",
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
        'even empty' => FALSE,
      ),
    ),
  );
  return $data;
}