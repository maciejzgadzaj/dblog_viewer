<?php

/**
 * Implementation of hook_admin_settings().
 */
function dblog_viewer_admin_settings() {
  $form['dblog_viewer_date_format'] = array(
    '#type'           => 'select',
    '#title'          => t('Date format for recent log entries'),
    '#default_value'  => variable_get('dblog_viewer_date_format', 'medium'),
    '#options'        => array('small' => t('short'), 'medium' => t('medium'), 'large' => t('long')),
    '#description'    => t(''),
  );
  $form['dblog_viewer_truncate_message'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Length of error message to display on recent entries list'),
    '#default_value'  => variable_get('dblog_viewer_truncate_message', 200),
    '#description'    => t(''),
  );
  $form['dblog_viewer_entries_per_page'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Number of entries per page'),
    '#default_value'  => variable_get('dblog_viewer_entries_per_page', 50),
    '#description'    => t(''),
  );
  return system_settings_form($form);
}

/**
 * Menu callback; displays a listing of log messages.
 */
function dblog_viewer_page_recent_log_entries() {
  drupal_add_css(drupal_get_path('module', 'dblog_viewer') . '/dblog_viewer.css', 'module', 'all', FALSE);
  $filter = dblog_viewer_build_filter_query();
  $rows = array();
  $icons = array(
    WATCHDOG_DEBUG    => '',
    WATCHDOG_INFO     => '',
    WATCHDOG_NOTICE   => '',
    WATCHDOG_WARNING  => theme('image', 'misc/watchdog-warning.png', t('warning'), t('warning')),
    WATCHDOG_ERROR    => theme('image', 'misc/watchdog-error.png', t('error'), t('error')),
    WATCHDOG_CRITICAL => theme('image', 'misc/watchdog-error.png', t('critical'), t('critical')),
    WATCHDOG_ALERT    => theme('image', 'misc/watchdog-error.png', t('alert'), t('alert')),
    WATCHDOG_EMERG    => theme('image', 'misc/watchdog-error.png', t('emergency'), t('emergency')),
  );
  $classes = array(
    WATCHDOG_DEBUG    => 'dblog-debug',
    WATCHDOG_INFO     => 'dblog-info',
    WATCHDOG_NOTICE   => 'dblog-notice',
    WATCHDOG_WARNING  => 'dblog-warning',
    WATCHDOG_ERROR    => 'dblog-error',
    WATCHDOG_CRITICAL => 'dblog-critical',
    WATCHDOG_ALERT    => 'dblog-alert',
    WATCHDOG_EMERG    => 'dblog-emerg',
  );

  $output = drupal_get_form('dblog_viewer_filter_form');

  $header = array(
    ' ',
    array('data' => t('wid'), 'field' => 'w.wid'),
    array('data' => t('Type'), 'field' => 'w.type'),
    array('data' => t('Date'), 'field' => 'w.wid', 'sort' => 'desc'),
    t('Message'),
    array('data' => t('User'), 'field' => 'u.name'),
    array('data' => t('Operations')),
  );

  $sql = "SELECT w.wid, w.uid, w.severity, w.type, w.timestamp, w.message, w.variables, w.link, u.name FROM {watchdog} w INNER JOIN {users} u ON w.uid = u.uid";
  $tablesort = tablesort_sql($header);
  if (!empty($filter['where'])) {
    $result = pager_query($sql ." WHERE ". $filter['where'] . $tablesort, variable_get('dblog_viewer_entries_per_page', 50), 0, NULL, $filter['args']);
  }
  else {
    $result = pager_query($sql . $tablesort, variable_get('dblog_viewer_entries_per_page', 50));
  }

  while ($dblog = db_fetch_object($result)) {
    $rows[] = array('data' =>
      array(
        $icons[$dblog->severity],
        $dblog->wid,
        t($dblog->type),
        format_date($dblog->timestamp, variable_get('dblog_viewer_date_format', 'medium')),
        l(truncate_utf8(_dblog_viewer_format_message($dblog), variable_get('dblog_viewer_truncate_message', 200), TRUE, TRUE), 'admin/reports/event/'. $dblog->wid, array('html' => TRUE)),
        theme('username', $dblog),
        $dblog->link,
      ),
      // Attributes for tr
      'class' => "dblog-". preg_replace('/[^a-z]/i', '-', $dblog->type) .' '. $classes[$dblog->severity]
    );
  }

  if (!$rows) {
    $rows[] = array(array('data' => t('No log messages available.'), 'colspan' => 6));
  }

  $output .= theme('table', $header, $rows, array('id' => 'admin-dblog'));
  $output .= theme('pager', NULL, 50, 0);

  return $output;

}
