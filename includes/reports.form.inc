<?php

define('UPITT_ISLANDORA_INVENTORY_AUDIT_REPORT_PAGESIZE', 100);

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
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_islandora']) . ' Islandora objects',
      '#description' => l('What is not in Islandora?','/admin/islandora/inventory_audit/reports/islandora/false') . "<br>" .
        l('What is ONLY in Islandora?','/admin/islandora/inventory_audit/reports/islandora/true'),
    ),
    'workflow' => array(
      '#type' => 'fieldset',
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_workflow_record']) . ' legacy django workflow records',
      '#description' => l('What is ONLY in workflow?','/admin/islandora/inventory_audit/reports/workflow_records/true'),
    ),
    'bigfoot' => array(
      '#type' => 'fieldset',
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_bigfoot']) . ' bigfoot object files',
      '#description' => l('What is ONLY on bigfoot?','/admin/islandora/inventory_audit/reports/bigfoot/true'),
    ),
    'equinox' => array(
      '#type' => 'fieldset',
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_equinox']) . ' equinox object files',
      '#description' => l('What is ONLY on equinox?','/admin/islandora/inventory_audit/reports/equinox/true'),
    ),
  );
  return $form;
}

/**
 * Will create the output of a given report.
 *
 * @param string $table
 * @param string $boolean_string
 * @return HTML markup
 */
function upitt_islandora_inventory_audit_reports_report($table, $boolean_string) {
  $boolean = ($boolean_string == 'true') ? TRUE : FALSE;

  // Check for ?page= value -- to support pagination.
  $page = (isset($_GET['page'])) ? $_GET['page'] : 0;

  // Get records -- based on the offset - considering pagination.
  $report = upitt_islandora_inventory_audit_get_report_table($table, $boolean, $page);

  // Set full record count
  $count_rows = $report['count'];

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
  $human_readable_table = (($boolean) ? '(what is ONLY in) ' : '(objects that are NOT in) ') . $human_readable_table;
  
  $links = array(); //l('test','/admin'),l('testlink2','/islandora'), );

  return theme('upitt_islandora_inventory_audit_report',
      array('table' => $human_readable_table, 'pager' => $report['pager'],
        'table_markup' => $table_markup, 'count_rows' => $count_rows, 'links' => $links));
}

/**
 * This will perform the database query for the given table and other parameters.
 *
 * @param string $table
 * @param boolean $boolean
 * @param integer $page
 * @return array
 *   'count' integer represents the total number of records for the given report.
 *   'header' array of fieldnames to be used in creating the table markup in the
 *     calling procedure.
 *   'rows' array of values that correspond to the header fields - also used in
 *     creating the table markup in the calling procedure.
 */
function upitt_islandora_inventory_audit_get_report_table($table, $boolean, $page) {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $real_tablename = 'upitt_islandora_audit_items_' . $table;

  $rows = array();

  $fields = upitt_islandora_inventory_audit_get_fields($real_tablename);
  $sql = 'SELECT ' . implode(',', $fields['fields']) . ' ' .
         'FROM ' . $real_tablename . ' ';
  $result = db_query($sql);
  $result->execute();
  $count = $result->rowCount();

  $ps = UPITT_ISLANDORA_INVENTORY_AUDIT_REPORT_PAGESIZE;
  $sql .= 'LIMIT ' . (($page) ? $page * $ps . ', ' . $ps : $ps);

  $result = db_query($sql);
  $result->execute();
  
  pager_default_initialize($count, UPITT_ISLANDORA_INVENTORY_AUDIT_REPORT_PAGESIZE, $element = 0);
  $pager = theme('pager',array("quantity"=>$count));

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
      $header[] = array('data' => str_replace("_", " ", $field), 'class' => array('numeric'));
    } else {
      $header[] = str_replace("_", " ", $field);
    }
  }
  
  return array('count' => $count, 'header' => $header, 'rows' => $rows, 'pager' => $pager);
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
    case 'upitt_islandora_audit_items_islandora':
      $return['fields'][] = 'islandora_pid';
      $return['fields'][] = 'barcode';
      $return['fields'][] = 'parent_pids';
      $return['fields'][] = 'datastreams_latestsize';
      $return['fields'][] = 'datastreams_totalsize';
      $return['integer_fields'] = array('datastreams_latestsize', 'datastreams_totalsize');
      break;

    default:
      break;
  }
  return $return;
}