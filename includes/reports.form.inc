<?php

define('UPITT_ISLANDORA_INVENTORY_AUDIT_REPORT_PAGESIZE', 100);
define('UPITT_ISLANDORA_INVENTORY_AUDIT_PAGE_DETAIL_REPORT_PAGESIZE', 100);

/**
 * @file
 * Report forms for the Islandora Inventory Audit module.
 */

/*
 * -- objects on bigfoot that are not in Islandora
 * SELECT ab.barcode, ab.bigfoot_path
 * FROM upitt_islandora_audit_items_bigfoot ab
 * WHERE ab.barcode NOT IN (
 *   SELECT ai.barcode FROM upitt_islandora_audit_items_islandora ai);
 *
 * -- objects in workflow system that are in not Islandora
 * SELECT aw.barcode
 * FROM upitt_islandora_audit_items_workflow_record aw
 * WHERE aw.barcode NOT IN (
 *   SELECT ai.barcode FROM upitt_islandora_audit_items_islandora ai);
 *
 *
 */

function upitt_islandora_inventory_audit_reports_form() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
  $inventory_record_counts = upitt_islandora_inventory_all_record_counts();
  $form = array(
    'frame' => array(
      '#type' => 'item',
      '#title' => '<h3>Islandora Inventory Audit Reports</h3>',
    ),
    'islandora' => array(
      '#type' => 'fieldset',
      '#title' => 'Islandora objects',
      '#description' => l('Datastream Filespace by Depositor','/admin/islandora/inventory_audit/reports/islandora/dsfs_by_depositor') . "<br>" .
        l('Islandora inventory','/admin/islandora/inventory_audit/reports/islandora/all') . " (" . number_format($inventory_record_counts['upitt_islandora_audit_items_islandora']) . " digital objects)<br>" .
        l('Islandora inventory that is NOT in any other inventories','/admin/islandora/inventory_audit/reports/islandora/exclusive') . "<br>" .
        '<a href="/admin/islandora/inventory_audit/reports/islandora/page_detail">' . t('Bigfoot / Islandora page difference comparison.') . '</a>',
    ),
    'workflow' => array(
      '#type' => 'fieldset',
      '#title' => 'legacy django workflow records',
      '#description' => l('Workflow inventory','/admin/islandora/inventory_audit/reports/workflow_record/all') . 
        " (" . number_format($inventory_record_counts['upitt_islandora_audit_items_workflow_record']) . " records)<br>" .
        l('Workflow inventory that is NOT in Islandora','/admin/islandora/inventory_audit/reports/workflow_record/exclusive'),
    ),
    'bigfoot' => array(
      '#type' => 'fieldset',
      '#title' => 'bigfoot object files',
      '#description' => l('Bigfoot inventory','/admin/islandora/inventory_audit/reports/bigfoot/all') . 
        " (" . number_format($inventory_record_counts['upitt_islandora_audit_items_bigfoot']) . " files)<br>" .
        l('Bigfoot inventory that is NOT in Islandora','/admin/islandora/inventory_audit/reports/bigfoot/exclusive') . "<br>" .
        '<a href="/admin/islandora/inventory_audit/reports/bigfoot/exclusive?in_all_ingests=1">' . t('Bigfoot inventory DLXS! that is NOT in Islandora') . "</a>",
    ),
    'photodb' => array(
      '#type' => 'fieldset',
      '#title' => 'photodb objects',
      '#description' => l('PhotoDB inventory','/admin/islandora/inventory_audit/reports/photodb/all') . 
        " (" . number_format($inventory_record_counts['upitt_islandora_audit_items_photodb']) . " records)<br>" .
        l('PhotoDB inventory that is NOT in Islandora','/admin/islandora/inventory_audit/reports/photodb/exclusive'),
    ),
  );
  return $form;
}

/**
 * Will create the output of a given report.
 *
 * @param string $table
 * @param string $exclusive_string
 * @return HTML markup
 */
function upitt_islandora_inventory_audit_reports_report($table, $exclusive_string) {
  $exclusive = ($exclusive_string == 'exclusive') ? TRUE : FALSE;

  // Check for ?page= value -- to support pagination.
  $page = filter_input(INPUT_GET, 'page');;

  // Check for any filter variable values.
  $barcode = filter_input(INPUT_GET, 'barcode');
  $depositor = filter_input(INPUT_GET, 'depositor');
  $model = filter_input(INPUT_GET, 'REQUEST_URI');
  $order = filter_input(INPUT_GET, 'order');
  $sort = filter_input(INPUT_GET, 'sort');
  $in_all_ingests = filter_input(INPUT_GET, 'in_all_ingests');
  $csv_download = filter_input(INPUT_GET, 'csv');

  $filters = array();
  if ($barcode) { $filters['ta.barcode'] = $barcode; }
  if ($depositor) { $filters['ta.depositor'] = $depositor; }
  if ($model) { $filters['ta.fedora_model'] = $model; }
  if ($in_all_ingests > -1 && $in_all_ingests < 2) { $filters['ta.in_all_ingests'] = $in_all_ingests; }
  // Get records -- based on the offset - considering pagination.
  switch ($exclusive_string) {
    case 'dsfs_by_depositor':
      $report = upitt_islandora_inventory_audit_get_datastreamfilesize_report($sort, $order, $csv_download);
      break;

    case 'page_detail':
      $report = upitt_islandora_inventory_audit_get_page_detail_table($page, $filters, $barcode, $csv_download);
      break;

    default:
      $report = upitt_islandora_inventory_audit_get_report_table($table, $exclusive, $page, $filters, $csv_download);
      break;
  }

  // Set full record count
  $count_rows = $report['count'];

  foreach ($report['rows'] as $k => $row) {
    if (isset($row['barcode'])) {
      $report['rows'][$k]['barcode'] = '<a href="/islandora/object/pitt:' . $row['barcode'] . '">' . $row['barcode'] . '</a>';
    }
  }
  // Build table from records and theme it for output.
  $table_markup = theme('table', array('header' => $report['header'],
    'rows' => $report['rows']));

  switch ($table) {
    case 'workflow_records':
      $human_readable_table = 'Workflow Records';
    break;

    default:
      $human_readable_table = ucwords($table);
    break;
  }

  $request_uri = filter_input(INPUT_SERVER, 'REQUEST_URI');
  $get_from_uri = (strstr($request_uri, '?') ?
        substr($request_uri, 0, strpos($request_uri, '?'))
          : $request_uri);

  $csv_download_link = $get_from_uri . '?' . $_SERVER['QUERY_STRING'] . '&csv=1';
  if ($exclusive_string == 'page_detail') {
    $filter_controls = '';
    $human_readable_table = 'Page Detail Comparison' . ($barcode ? ' for ' . l('Islandora: pitt:' . $barcode, '/islandora/object/pitt:' . $barcode . '/manage') . ' | ' .
            l('Workflow: ' . $barcode, '/workflow_legacy/object/' . $barcode) : '');
    $get_from_uri = '';
  }
  elseif ($exclusive_string == 'dsfs_by_depositor' && $table == 'islandora') {
    $filter_controls = '';
    $human_readable_table = 'Datastream filesizes by Depositor';
    $get_from_uri = '';
  }
  else {
    $remaining_tables = ($table == 'islandora') ? 'any other inventories' : 'Islandora';
    $human_readable_table = (($exclusive) ? 'What is in ' . $human_readable_table . ' that is NOT in ' . $remaining_tables:
        $human_readable_table . ' inventory'
      );

    $request_uri = filter_input(INPUT_SERVER, 'REQUEST_URI');

    $get_from_uri = (strstr($request_uri, '?') ?
          substr($request_uri, 0, strpos($request_uri, '?'))
            : $request_uri);
    // Filter data form
    $filter_controls_form = array(
      'frame' => array(
        '#type' => 'item',
        '#title' => 'Filter',
          '#name' => '',
        'barcode' => array(
          '#type' => 'textfield',
          '#name' => 'barcode',
          '#title' => 'Barcode',
          '#value' => $barcode,
        ),
      ),
    );
    if ($table == 'islandora') {
      $filter_controls_form['frame']['depositor'] = array(
        '#type' => 'textfield',
        '#name' => 'depositor',
        '#title' => 'Depositor',
        '#value' => $depositor,
      );
      $filter_controls_form['frame']['model'] = array(
        '#type' => 'textfield',
        '#name' => 'model',
        '#title' => 'Model',
        '#value' => $model,
      );
    }
    elseif ($table == 'bigfoot') {
      $filter_controls_form['frame']['in_all_ingests'] = array(
        '#type' => 'textfield',
        '#name' => 'in_all_ingests',
        '#title' => 'In all_ingests',
        '#value' => $in_all_ingests,
      );
    }

    // The form is complete -- slap the Submit button to the bottom of the filtering form.
    $filter_controls_form['frame']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Filter'),
    );

    // This is not conventional, but the way this report is used and handling of $_GET
    // parameter values makes this form handle various table sources - based on the
    // $table and $exclusive values passed in.  This will render the form that allows
    // filtering of the fields for THIS table specifically.
    //
    // If the table is islandora, expose model and depositor.
    $filter_controls = drupal_render($filter_controls_form);
  }

  $markup = theme('upitt_islandora_inventory_audit_report',
      array('title' => $human_readable_table, 'filter_controls' => $filter_controls,
        'pager' => $report['pager'], 'get_from_uri' => $get_from_uri,
        'table_markup' => $table_markup, 'count_rows' => $count_rows, 'csv_download_link' => $csv_download_link,
        'total_object_count' => $report['total_object_count'],
        'report_detail' => $report['report_detail']));

  return $markup;
}

/**
 * Function that will get the datastreams filesize report of the islandora inventory table.
 *
 * @param string $sort   direction of the sort ASC or DESC
 * @param string $order  field name to be sorted upon
 * @return associative array of table info that can be themed to output HTML of table.
 */
function upitt_islandora_inventory_audit_get_datastreamfilesize_report($sort, $order, $csv_download = 0) {
  $order = str_replace('% ', 'pct ', $order);
  module_load_include('inc', 'islandora', 'includes/utilities');

  $real_tablename = 'upitt_islandora_audit_items_islandora';

  $rows = array();

  $fields = array(
    'fields' => array('depositor', 'datastreams latest size', 'datastreams total size',
      'pct of All', '"extra" versions size', 'pct of depositor due to "extra" versions',
      'pct of total "extra" size'),
    'pct_fields' => array('pct of All', 'pct of depositor due to "extra" versions', 'pct of total "extra" size'),
    'integer_fields' => array('datastreams latest size', 'datastreams total size', '"extra" versions size'));

  // Get the total datastream filesize
  $result = db_query("SELECT SUM(datastreams_totalsize) `total` FROM upitt_islandora_audit_items_islandora");
  $result->execute();
  if ($result->rowCount()) {
    $row = $result->fetchAssoc();
    $total_size = $row['total'];
  }
  else {
    $total_size = 1;
  }

  $sql = "SELECT depositor, count(depositor) `depositors count`, SUM(datastreams_latestsize) `datastreams latest size`, " .
         "SUM(datastreams_totalsize) `datastreams total size`, " .
         "ROUND(SUM(datastreams_totalsize) / " . $total_size . " * 100, 4) `pct of All`, " .
         "SUM(datastreams_totalsize) - SUM(datastreams_latestsize) `\"extra\" versions size`, " .
         "ROUND((SUM(datastreams_totalsize) - SUM(datastreams_latestsize)) / SUM(datastreams_totalsize) * 100, 4) `pct of depositor due to \"extra\" versions`, " .
         "ROUND((SUM(datastreams_totalsize) - SUM(datastreams_latestsize)) / " . $total_size . " * 100, 4) `pct of total \"extra\" size` " .
         "FROM upitt_islandora_audit_items_islandora ai " .
         "GROUP BY ai.depositor " .
         ($order ? "ORDER BY `" . $order . "` " .$sort : '');

  $result = db_query($sql);
  $result->execute();

  $count = $result->rowCount();

  $result = db_query($sql);
  $result->execute();

  $total_object_count = 0;
  while($record = $result->fetchAssoc()) {
    // loop through the elements of the record - and if the field matches one of
    // the integer_fields, then use islandora_convert_bytes_to_human_readable() to format
    // - and set a class that will right-align the value.
    foreach ($record as $fieldname => $value) {
      if (!(array_search($fieldname, $fields['integer_fields']) === FALSE)) {
        $record[$fieldname] = array('data' => islandora_convert_bytes_to_human_readable($value) . "<br><span class='raw_integer'>" . number_format($value) . " bytes</span>", 'class' => array('numeric')) ;
      }
      if (!(array_search($fieldname, $fields['pct_fields']) === FALSE)) {
        $record[$fieldname] = array('data' => $value, 'class' => array('pct_fields')) ;
      }
    }
    $record['depositor'] = $record['depositor'] . '<br><span class="raw_integer">' . number_format($record['depositors count']) . ' object' . ($record['depositors count'] == 1 ? '' : 's') . '</span>';
    $total_object_count += $record['depositors count'];
    unset($record['depositors count']);
    $rows[] = $record;
  }

  $header = array();
  foreach ($fields['fields'] as $field) {
    $header_text = str_replace(array("_", "pct"), array(" ", "%"), $field);
    // Also, add the class to right-align the header for integer columns.
    if (!(array_search($field, $fields['integer_fields']) === FALSE)) {
      $header[] = array('data' => $header_text, 'class' => array('numeric'), 'field' => $field);
    } else {
      $header[] = array('data' => $header_text, 'field' => $field);
    }
  }
  if ($csv_download) {
    header("Content-type: text/csv");
    header("Content-Disposition: attachment; filename=datastream_filesize_report.csv");
    header("Pragma: no-cache");
    header("Expires: 0");
    $use_header = array();
    foreach ($header as $i => $heading) {
      $use_header[$i] = strip_tags(isset($heading['data']) ? $heading['data'] : $heading);
    }
    print implode(",", $use_header) . "\n";
    foreach ($rows as $row) {
      $use_row = array();
      foreach ($row as $i => $cell) {
        $use_row[$i] = '"' . strip_tags(isset($cell['data']) ? str_replace("<br>", " ", $cell['data']) : $cell) . '"';
      }
      print implode(",", $use_row) . "\n";
    }
    exit;
  }
  else {
    return array('count' => $count, 'header' => $header, 'rows' => $rows, 'pager' => '', 'total_object_count' => $total_object_count, 'report_detail' => '');
  }
}

function upitt_islandora_inventory_audit_get_page_details_for_barcode($barcode, $csv_download) {
  global $databases;
  $drupal_database_name = $databases['default']['default']['database'];
  $workflow_database_name = $databases['default']['mysql_new_workflow']['database'];

  $sql_wf = "SELECT iwf.name " .
            "FROM `" . $workflow_database_name . "`.`item` iwi " .
            "JOIN `" . $workflow_database_name . "`.`item_file` iwf  ON (iwf.item_id = iwi.id AND iwf.`use` = 'MASTER') " .
            "WHERE iwi.do_id = '" . $barcode . "' " . 
            "ORDER BY iwf.name";
  $result_wf = db_query($sql_wf);
  $result_wf->execute();

  $header_wf_io = array('bigfoot TIF filename', 'islandora_pid');

  while($record = $result_wf->fetchAssoc()) {
    $pid = 'pitt:' . $barcode . '-' . str_replace(".tif", "", $record['name']);
    $rows[$pid] = array('bigfoot TIF filename' => $record['name'], 'islandora_pid' => '&nbsp;');
  }

  $sql_io = "SELECT barcode, islandora_pid " .
            "FROM `upitt_islandora_audit_items_islandora` " .
            "WHERE barcode like CONCAT('" . $barcode . "', '-%') " .
            "ORDER BY islandora_pid";
  $result_io = db_query($sql_io);
  $result_io->execute();
  while($record = $result_io->fetchAssoc()) {
    $pid = $record['islandora_pid'];
    $rows[$pid]['islandora_pid'] = $pid;
  }

  $not_in_bigfoot = $not_in_islandora = array();
  foreach ($rows as $pid => $row) {
    if ($row['islandora_pid'] == '&nbsp;') {
      $not_in_islandora[] = $pid;
    }
    if (!isset($row['bigfoot TIF filename'])) {
      $not_in_bigfoot[] = $pid;
    }
  }
  $report_detail = '';
  if (count($not_in_islandora) > 0) {
    $report_detail .= '<h4>Missing pages in Islandora for</h4><ul><li>' . implode("</li><li>", $not_in_islandora) . "</li></ul>";
  }
  if (count($not_in_bigfoot) > 0) {
    $report_detail .= '<h4>Missing files in Bigfoot for</h4><ul><li>' . implode("</li><li>", $not_in_bigfoot) . "</li></ul>";
  }

  $count = count($rows);
  return array('count' => $count, 'header' => $header_wf_io, 'rows' => $rows, 'pager' => '', 'total_object_count' => '', 'report_detail' => $report_detail);
}

function upitt_islandora_inventory_audit_get_page_detail_table($page, $filters, $barcode, $csv_download) {
  if ($barcode) {
    return upitt_islandora_inventory_audit_get_page_details_for_barcode($barcode, $csv_download);
  }
  module_load_include('inc', 'islandora', 'includes/utilities');
  $sql = "SELECT pc.barcode, pc.bigfoot_page_count `bigfoot pages`, pc.islandora_page_count `islandora pages` " .
         "FROM upitt_islandora_audit_page_counts pc " .
         "WHERE bigfoot_page_count <> islandora_page_count ";

  $fields = upitt_islandora_inventory_audit_get_fields('page_detail_report');
  
  if (!$csv_download) {
    $result = db_query($sql);
    $count = $result->rowCount();
    $ps = UPITT_ISLANDORA_INVENTORY_AUDIT_PAGE_DETAIL_REPORT_PAGESIZE;
    $sql .= 'LIMIT ' . (($page) ? $page * $ps . ', ' . $ps : $ps);

    $result = db_query($sql);
    $result->execute();

    pager_default_initialize($count, UPITT_ISLANDORA_INVENTORY_AUDIT_PAGE_DETAIL_REPORT_PAGESIZE, $element = 0);
    $pager = theme('pager',array("quantity"=>10));
  } else {
    $result = db_query($sql);
    $result->execute();
  }

  while($record = $result->fetchAssoc()) {
    // loop through the elements of the record - and if the field matches one of
    // the wrap fields,
    // - and set a class that will right-align the value.
    foreach ($record as $fieldname => $value) {
      if (!(array_search($fieldname, $fields['wrap_fields']) === FALSE)) {
        $value = str_replace(",", ",\n", $value);
        $record[$fieldname] = array('data' => $value, 'class' => array('wrap_cell'));
      }
    }
    $record['options'] = array('data' => '<a href="/admin/islandora/inventory_audit/reports/islandora/page_detail?barcode=' . $record['barcode'] . '">' . t('Details') . '</a>');

    $rows[] = $record;
  }

  $header = array();
  foreach ($fields['fields'] as $field) {
    // Also, add the class to right-align the header for integer columns.
    if (!(array_search($field, $fields['integer_fields']) === FALSE)) {
      $header[] = array('data' => str_replace(array("_", "pc."), " ", $field), 'class' => array('numeric'));
    } else {
      $header[] = str_replace(array("_", "pc."), " ", $field);
    }
  }
  if ($csv_download) {
    header("Content-type: text/csv");
    header("Content-Disposition: attachment; filename=" . $table . '-' . implode('-', $csv_name) . ".csv");
    header("Pragma: no-cache");
    header("Expires: 0");
    print implode(",", $header) . "\n";
    foreach ($rows as $row) {
      print implode(",", $row) . "\n";
    }
    exit;
  }
  else {
    return array('count' => $count, 'header' => $header, 'rows' => $rows, 'pager' => $pager, 'total_object_count' => '', 'report_detail' => '');
  }
}

/**
 * This will perform the database query for the given table and other parameters.
 *
 * @param string $table
 * @param boolean $exclusive
 * @param integer $page
 * @param array $filters
 *   array keys correspond to the fieldname, the array
 *     value is the filter value for that field.
 * @return array
 *   'count' integer represents the total number of records for the given report.
 *   'header' array of fieldnames to be used in creating the table markup in the
 *     calling procedure.
 *   'rows' array of values that correspond to the header fields - also used in
 *     creating the table markup in the calling procedure.
 */
function upitt_islandora_inventory_audit_get_report_table($table, $exclusive, $page, $filters, $csv_download = 0) {
  global $databases;
  $drupal_database_name = $databases['default']['default']['database'];
  $workflow_database_name = $databases['default']['mysql_new_workflow']['database'];  

  module_load_include('inc', 'islandora', 'includes/utilities');

  $real_tablename = 'upitt_islandora_audit_items_' . $table;

  $rows = $csv_name = array();

  $sql_where = $sql_joins = $sql_select = $where_add = '';
  if (count($filters) > 0) {
    $where_add_parts = array();
    foreach ($filters as $fieldname => $value) {
      $csv_name[] = $fieldname . '_' . $value;
      $where_add_parts[] = $fieldname . ' like \'%' . addslashes(filter_xss($value)) . '%\'';
    }
    $where_add = 'WHERE ' . implode(' AND ', $where_add_parts);
  }

  $fields = upitt_islandora_inventory_audit_get_fields($real_tablename);
  $sql = 'SELECT ' . implode(', ', $fields['fields']) . ' ' .
         'FROM ' . $drupal_database_name . '.' . $real_tablename . ' ta ';
  if ($exclusive) {
    $csv_name[] = 'exclusive';
    switch ($table) {
      case 'islandora':
        $sql .= 'LEFT JOIN upitt_islandora_audit_items_bigfoot ab ON (ta.barcode = ab.barcode) ' .
                'LEFT JOIN upitt_islandora_audit_items_workflow_record aw ON (ta.barcode = aw.barcode) ' .
                'LEFT JOIN upitt_islandora_audit_items_photodb ap ON (ta.barcode = ap.barcode) ' .
                'WHERE ab.barcode IS NULL AND aw.barcode IS NULL ';
        $sql = str_replace("WHERE ", ($where_add ? $where_add . " AND " : "WHERE "), $sql);
        break;
      case 'bigfoot':
        $sql_select = $sql;
        $sql_joins = 'LEFT JOIN ' . $drupal_database_name . '.upitt_islandora_audit_items_islandora ai ON (ta.barcode = ai.barcode) ';
        $sql_where = 'WHERE ai.barcode IS NULL ';
        $sql_where = str_replace("WHERE ", ($where_add ? $where_add . " AND " : "WHERE "), $sql_where);
        $sql .= 'LEFT JOIN ' . $drupal_database_name . '.upitt_islandora_audit_items_islandora ai ON (ta.barcode = ai.barcode) ' .
                'JOIN ' . $workflow_database_name . '.item iwi ON (iwi.do_id = ta.barcode) ' .
                'JOIN ' . $workflow_database_name . '.item_file iwf ON (iwf.item_id = iwi.id AND iwf.`use` = \'MASTER\') ' .
                'JOIN ' . $workflow_database_name . '.`transaction` t ON (t.item_id = iwi.id AND t.transaction_action_id = 3) ' .
                $sql_where;
        break;

      default:
        $sql .= 'LEFT JOIN ' . $drupal_database_name . '.upitt_islandora_audit_items_islandora ai ON (ta.barcode = ai.barcode) ' .
                'WHERE ai.barcode IS NULL GROUP BY iwi.id ';
        $sql = str_replace("WHERE ", ($where_add ? $where_add . " AND " : "WHERE "), $sql);
        break;
    }
  }
  else {
    $csv_name[] = 'all';
    if ($table == 'bigfoot') {
      $sql_select = $sql;
      $sql_joins = 'LEFT JOIN ' . $drupal_database_name . '.upitt_islandora_audit_items_islandora ai ON (ta.barcode = ai.barcode) ';
      $sql .= $sql_joins .
              'JOIN ' . $workflow_database_name . '.item iwi ON (iwi.do_id = ta.barcode) ' .
              'JOIN ' . $workflow_database_name . '.item_file iwf ON (iwf.item_id = iwi.id AND iwf.`use` = \'MASTER\') ';
      $sql_where = $where_add;
    }
    $sql .= $where_add;
  }
  // if this is the bigfoot table, the sql must be a UNION
  if ($table == 'bigfoot' && $sql_select) {
    // replace the field in sql_select with an empty string
    $sql_select = str_replace(', iwf.name', ', \'\'', $sql_select);
    $sql = $sql . ' UNION ' . $sql_select . $sql_joins . $sql_where;
  }
  $result = db_query($sql);

  $result->execute();
  if (!$csv_download) {
    $count = $result->rowCount();
    $ps = UPITT_ISLANDORA_INVENTORY_AUDIT_REPORT_PAGESIZE;
    $sql .= 'LIMIT ' . (($page) ? $page * $ps . ', ' . $ps : $ps);

    $result = db_query($sql);
    $result->execute();
  
    pager_default_initialize($count, UPITT_ISLANDORA_INVENTORY_AUDIT_REPORT_PAGESIZE, $element = 0);
    $pager = theme('pager',array("quantity"=>10));
  }

  while($record = $result->fetchAssoc()) {
    // loop through the elements of the record - and if the field matches one of
    // the integer_fields, then use islandora_convert_bytes_to_human_readable() to format
    // - and set a class that will right-align the value.
    foreach ($record as $fieldname => $value) {
      if (!(array_search($fieldname, $fields['integer_fields']) === FALSE)) {
        $record[$fieldname] = array('data' => islandora_convert_bytes_to_human_readable($value), 'class' => array('numeric'));
      }
    }

    $rows[] = $record;
  }

  $header = array();
  foreach ($fields['fields'] as $field) {
    // Also, add the class to right-align the header for integer columns.
    if (!(array_search($field, $fields['integer_fields']) === FALSE)) {
      $header[] = array('data' => str_replace(array("_", "ta."), " ", $field), 'class' => array('numeric'));
    } else {
      $header[] = str_replace(array("_", "ta."), " ", $field);
    }
  }

  if ($csv_download) {
    header("Content-type: text/csv");
    header("Content-Disposition: attachment; filename=" . $table . '-' . implode('-', $csv_name) . ".csv");
    header("Pragma: no-cache");
    header("Expires: 0");
    print implode(",", $header) . "\n";
    foreach ($rows as $row) {
      print implode(",", $row) . "\n";
    }
    exit;
  }
  else {
    return array('count' => $count, 'header' => $header, 'rows' => $rows, 'pager' => $pager, 'total_object_count' => '', 'report_detail' => '');
  }
}

/**
 * Given the table name, this will return the table's field names that are to be
 * used for building the query.
 * 
 * @param string $real_tablename
 * @return array
 */
function upitt_islandora_inventory_audit_get_fields($real_tablename) {
  $return = array();
  switch ($real_tablename) {
    case 'page_detail_report':
      $return['fields'][] = 'pc.barcode';
//      $return['fields'][] = 'pc.bigfoot_page_count';
      $return['fields'][] = 'bigfoot pages';
//      $return['fields'][] = 'bigfoot tif files';
      $return['fields'][] = 'islandora pages';
//      $return['fields'][] = 'islandora objects';
      $return['fields'][] = 'options';
      $return['wrap_fields'] = array ('islandora objects', 'bigfoot tif files');
      $return['integer_fields'] = array('datastreams_latestsize', 'datastreams_totalsize');
      break;
    
    case 'upitt_islandora_audit_items_islandora':
      $return['fields'][] = 'ta.islandora_pid';
      $return['fields'][] = 'ta.barcode';
      $return['fields'][] = 'ta.fedora_model';
      $return['fields'][] = 'ta.depositor';
      $return['fields'][] = 'ta.datastreams_latestsize';
      $return['fields'][] = 'ta.datastreams_totalsize';
      $return['integer_fields'] = array('datastreams_latestsize', 'datastreams_totalsize');
      break;

    case 'upitt_islandora_audit_items_bigfoot':
      $return['fields'][] = 'ta.barcode';
      $return['fields'][] = 'ta.bigfoot_path';
      $return['fields'][] = 'ta.in_all_ingests';
      $return['fields'][] = 'iwf.name';
      $return['integer_fields'] = array('');
      break;

    case 'upitt_islandora_audit_items_workflow_record':
      $return['fields'][] = 'ta.barcode';
      $return['integer_fields'] = array();
      break;

    case 'upitt_islandora_audit_items_photodb':
      $return['fields'][] = 'ta.barcode';
      $return['integer_fields'] = array();
      break;

    default:
      break;
  }
  return $return;
}
