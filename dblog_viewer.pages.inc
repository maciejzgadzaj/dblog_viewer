<?php

######################
###  DRUPAL HOOKS  ###
######################

/**
 * Implementation of hook_admin_settings().
 */
function dblog_viewer_admin_settings() {
  /*
  $columns = array(
    'image'       => t('Severity image'),
    'severity'    => t('Severity'),
    'wid'         => 'wid',
    'type'        => t('Type'),
    'date'        => t('Date'),
    'message'     => t('Message'),
    'variables'   => t('Variables'),
    'user'        => t('User'),
    'operarions'  => t('Operations'),
  );
  $default_values = drupal_map_assoc(array('image', 'wid', 'type', 'date', 'message', 'user', 'operarions'));
  $form['dblog_viewer_display_columns'] = array(
    '#type'           => 'checkboxes',
    '#title'          => t('Display columns'),
    '#options'        => $columns,
    '#default_value'  => variable_get('dblog_viewer_display_columns', array('image', 'wid', 'type', 'date', 'message', 'user', 'operarions')),
  );
  */

  $form['dblog_viewer_entries_per_page'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Number of entries per page'),
    '#default_value'  => variable_get('dblog_viewer_entries_per_page', 50),
    '#description'    => t(''),
  );

  $form['dblog_viewer_truncate_message'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Message length'),
    '#default_value'  => variable_get('dblog_viewer_truncate_message', 200),
    '#description'    => t('Maximum length of processed message to display (after replacing all variables).'),
  );

  $form['dblog_viewer_date_format'] = array(
    '#type'           => 'select',
    '#title'          => t('Date format'),
    '#default_value'  => variable_get('dblog_viewer_date_format', 'medium'),
    '#options'        => array('small' => t('short'), 'medium' => t('medium'), 'large' => t('long')),
    '#description'    => t('As defined in !link configuration.', array('!link' => l('Date and Time', 'admin/settings/date-time'))),
  );

  return system_settings_form($form);
}


########################
###  MENU CALLBACKS  ###
########################

/**
 * Displays listing of log messages.
 */
function dblog_viewer_page_overview() {
  drupal_add_css(drupal_get_path('module', 'dblog_viewer') . '/dblog_viewer.css', 'module', 'all', FALSE);
  $filter = dblog_viewer_build_filter_query();
  $icons = _dblog_viewer_severity_icons();
  $classes = _dblog_viewer_severity_classes();

  $output = drupal_get_form('dblog_viewer_filter_form');

  $table_header = array(
    ' ',
    array('data' => t('wid'), 'field' => 'w.wid'),
    array('data' => t('Type'), 'field' => 'w.type'),
    array('data' => t('Date'), 'field' => 'w.wid', 'sort' => 'desc'),
    t('Message'),
    array('data' => t('User'), 'field' => 'u.name'),
    array('data' => t('Operations')),
  );

  $sql = "SELECT w.wid, w.uid, w.severity, w.type, w.timestamp, w.message, w.variables, w.link, u.name FROM {watchdog} w INNER JOIN {users} u ON w.uid = u.uid";
  $tablesort = tablesort_sql($table_header);
  if (!empty($filter['where'])) {
    $result = pager_query($sql ." WHERE ". $filter['where'] . $tablesort, variable_get('dblog_viewer_entries_per_page', 50), 0, NULL, $filter['args']);
  }
  else {
    $result = pager_query($sql . $tablesort, variable_get('dblog_viewer_entries_per_page', 50));
  }

  $table_rows = array();
  while ($dblog = db_fetch_object($result)) {
    $table_rows[] = array('data' =>
      array(
        $icons[$dblog->severity],
        $dblog->wid,
        t($dblog->type),
        format_date($dblog->timestamp, variable_get('dblog_viewer_date_format', 'medium')),
        l(truncate_utf8(_dblog_viewer_format_message($dblog), variable_get('dblog_viewer_truncate_message', 200), TRUE, TRUE), 'admin/reports/dblog_viewer_event/'. $dblog->wid, array('html' => TRUE)),
        theme('username', $dblog),
        $dblog->link,
      ),
      // Attributes for tr
      'class' => "dblog-". preg_replace('/[^a-z]/i', '-', $dblog->type) .' '. $classes[$dblog->severity]
    );
  }

  if (!$table_rows) {
    $table_rows[] = array(array('data' => t('No log messages available.'), 'colspan' => 6));
  }

  $output .= theme('table', $table_header, $table_rows, array('id' => 'dblog-viewer'));
  $output .= theme('pager', NULL, 50, 0);

  return $output;

}

/**
 * Displays details about specific log message.
 */
function dblog_viewer_page_event($id) {
  drupal_add_css(drupal_get_path('module', 'dblog_viewer') . '/dblog_viewer.css', 'module', 'all', FALSE);
  $severity = watchdog_severity_levels();
  $output = '';
  $result = db_query('SELECT w.*, u.name, u.uid FROM {watchdog} w INNER JOIN {users} u ON w.uid = u.uid WHERE w.wid = %d', $id);
  if ($dblog = db_fetch_object($result)) {
    $rows = array(
      array(
        'data' => array(
          array('data' => t('Severity'), 'header' => TRUE),
          $severity[$dblog->severity],
        ),
        'class' => _dblog_viewer_severity_classes($dblog->severity),
      ),
      array(
        array('data' => t('Type'), 'header' => TRUE),
        t($dblog->type),
      ),
      array(
        array('data' => t('Message'), 'header' => TRUE),
        _dblog_viewer_format_message($dblog),
      ),
      array(
        array('data' => t('Date'), 'header' => TRUE),
        format_date($dblog->timestamp, 'large'),
      ),
      array(
        array('data' => t('User'), 'header' => TRUE),
        theme('username', $dblog),
      ),
      array(
        array('data' => t('Location'), 'header' => TRUE),
        l($dblog->location, $dblog->location),
      ),
      array(
        array('data' => t('Referrer'), 'header' => TRUE),
        l($dblog->referer, $dblog->referer),
      ),
      array(
        array('data' => t('Hostname'), 'header' => TRUE),
        check_plain($dblog->hostname),
      ),
      array(
        array('data' => t('Operations'), 'header' => TRUE),
        $dblog->link,
      ),
    );
    $attributes = array('class' => 'dblog-viewer-event');
    $output = theme('table', array(), $rows, $attributes);

    $output .= '<h2 class="title">' . t('Core database values') . '</h2>';
    $rows = array(
      array(
        array('data' => t('Message not processed'), 'header' => TRUE),
        $dblog->message,
      ),
      array(
        array('data' => t('Variables serialized'), 'header' => TRUE),
        $dblog->variables,
      ),
      array(
        array('data' => t('Variables unserialized'), 'header' => TRUE),
        '<pre>' . var_export(unserialize($dblog->variables), true) . '</pre>',
      ),
    );
    $attributes = array('class' => 'dblog-viewer-event');
    $output .= theme('table', array(), $rows, $attributes);
  }
  return $output;
}


#################
###  ACTIONS  ###
#################

/**
 * Deletes filtered log messages from database.
 */
function _dblog_viewer_delete_selected_entries() {
  $filter = dblog_viewer_build_filter_query();
  $sql = "DELETE FROM {watchdog}";
  db_query($sql ." WHERE ". $filter['where'], $filter['args']);
  drupal_set_message('Selected log entries have been deleted.');
}
