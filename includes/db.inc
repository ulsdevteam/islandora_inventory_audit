<?php

/**
 * @file
 * Database functions for the Islandora Inventory Audit module.
 */

function upitt_islandora_inventory_get_record_by_pid($pid) {
  $query = db_select('upitt_islandora_audit_items', 'ai')
    ->fields('ai')
    ->condition('ai.islandora_pid', $pid);

  $record = $query->execute();
  $return = array();
  foreach($record as $node){
    $return = (array) $node;
  }
  return $return;
}

function upitt_islandora_inventory_audit_populate() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');

  $times_info = array();
  $very_start_time = upitt_islandora_inventory_audit_microtime_float();
  $time_start = $very_start_time;

  $result = db_truncate('upitt_islandora_audit_items');
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> truncate {upitt_islandora_audit_items} table done';

  // populate by workflow records -- this could also update bigfoot_path entries.
  $time_start = upitt_islandora_inventory_audit_microtime_float();
  upitt_islandora_inventory_audit_populate_workflow_record();
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> upitt_islandora_inventory_audit_populate_workflow_record done';

  // scan to pick up any folders that were not already added by workflow reference.
  $time_start = upitt_islandora_inventory_audit_microtime_float();
  upitt_islandora_inventory_audit_populate_bigfoot_path();
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> upitt_islandora_inventory_audit_populate_bigfoot_path done';

  // scan to update the equinox path values.
  $time_start = upitt_islandora_inventory_audit_microtime_float();
  upitt_islandora_inventory_audit_populate_equinox_path();
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> upitt_islandora_inventory_audit_populate_equinox_path done';

  // scan the islandora system to see if the objects exist.
  $time_start = upitt_islandora_inventory_audit_microtime_float();
  upitt_islandora_inventory_audit_populate_islandora_pid();
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> upitt_islandora_inventory_audit_populate_islandora_pid done';

  $time = $time_end - $very_start_time;
  $times_info[] = '<b>' . round($time, 2) . '</b> Total time';

  drupal_set_message(implode('<br>', $times_info));
}

/**
 * This will check to see if a record exists with this unique field value already
 * -- and update it if it does, else it will insert the new record.
 *
 * @param array $name_value_pairs
 */
function upitt_islandora_inventory_audit_insert_or_update($name_value_pairs) {
  $result = db_select('upitt_islandora_audit_items', 'ai', array('target' => 'default'))
    ->fields('ai', array('id'));

  foreach ($name_value_pairs as $field => $value) {
    $result->condition($field, $value);
  }

  $row = $result->execute()->fetchAssoc();
  if (!empty($row['id'])) {
    // UPDATE $row['id']
    $num_updated = db_update('upitt_islandora_audit_items')
      ->fields($name_value_pairs)
      ->condition('id', $row['id'])
      ->execute();
  }
  else {
    // INSERT
    $id_inserted = db_insert('upitt_islandora_audit_items')
      ->fields($name_value_pairs)
      ->execute();
  }
}

/**
 * Populate upitt_islandora_audit_items record by scanning workflow records --
 * this could also update bigfoot_path entries.
 *
 * Since this reads the islandora_workflow database, the method to get the database
 * link is borrowed from the upitt_workflow module.
 *
 * This is populating for the items that have a transaction record
 * with "ingest completed" status.
 */
function upitt_islandora_inventory_audit_populate_workflow_record() {
  module_load_include('inc', 'upitt_workflow', 'includes/utilities');
  module_load_include('module', 'upitt_workflow', 'upitt_workflow');

  $link = upitt_workflow_get_databaselink('mysql_new_workflow');
  $item_query = "SELECT DISTINCT(i.do_id) `do_id`, CONCAT('pitt:', i.do_id) `pid` " .
                "FROM `item` i " .
                // transaction_action_id of 3 is "ingest completed"
                "JOIN `transaction` t ON (t.item_id = i.id AND t.transaction_action_id = 3) LIMIT 5";

  $result = mysqli_query($link, $item_query);
  if (!$result) {
    upitt_workflow_sql_error_die($link, $item_query);
  }

  $replace_sql = array();
  while ($row = mysqli_fetch_assoc($result)) {
    upitt_islandora_inventory_audit_insert_or_update(array('islandora_pid' => $row['pid'], 'workflow_record' => $row['do_id']));
  }

  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_workflow_record');
}

/**
 * Scan to pick up any folders that were not already added by workflow reference.
 */
function upitt_islandora_inventory_audit_populate_bigfoot_path() {
  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_bigfoot_path');

  // scan the directory -- finding all folders that qualify as an object on bigfoot...
  // while a book is only one object on the bigfoot system, it must create an entry for
  // every page *.tif / *.tiff file as an object record.

  $dir = variable_get('upitt_islandora_inventory_audit_bigfoot_mapped_path', '/ingest/holding');
  $cdir = scandir($dir);
  foreach ($cdir as $key => $value) {
    if (!in_array($value,array(".",".."))) {
      if (is_dir($dir . '/' . $value)) {
        drupal_set_message('OBJECT FOLDER ' . $key . ' = ' . $value);

      }
      else {
        drupal_set_message('NORMAL FILE ' . $key . ' = ' . $value);
      }
    }
  }
  return FALSE;
}

/**
 * Scan to update the equinox path values.
 */
function upitt_islandora_inventory_audit_populate_equinox_path() {
  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_equinox_path');
}

/**
 * Scan the islandora system to see if the objects exist.
 */
function upitt_islandora_inventory_audit_populate_islandora_pid() {
  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_islandora_pid');
}