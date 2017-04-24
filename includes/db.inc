<?php

define('UPITT_ISLANDORA_INVENTORY_SOLR_LIMIT', 2000000);

/**
 * @file
 * Database functions for the Islandora Inventory Audit module.
 */

function upitt_islandora_inventory_html_escape_string($html_escape) {
  $html_escape =  htmlspecialchars($html_escape, ENT_QUOTES | ENT_HTML5, 'UTF-8');
  return $html_escape;
}

function upitt_islandora_inventory_get_record_by_pid($pid) {
  // strip the namespace from the PID value and look for that barcode in all tables.
  $barcode = substr($pid, strpos($pid, ':') + 1);

  // don't even try to use the drupal database objects to get this query right ...
  $query = db_query('SELECT ai.islandora_pid, aw.barcode `workflow_record` ' .
                    'FROM {upitt_islandora_audit_items_islandora} ai ' .
                    'LEFT JOIN {upitt_islandora_audit_items_workflow_record} aw ON (aw.barcode = ai.barcode) ' .
                    'LEFT JOIN {upitt_islandora_audit_items_bigfoot} ab ON (ab.barcode = ai.barcode) ' .
                    'LEFT JOIN {upitt_islandora_audit_items_equinox} ae ON (ae.barcode = ai.barcode) ' .
                    'WHERE ai.barcode = \'' . upitt_islandora_inventory_html_escape_string($barcode) . '\'');

  return $query->fetchAssoc();
}

function upitt_islandora_inventory_audit_populate() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');

  $times_info = array();
  $very_start_time = upitt_islandora_inventory_audit_microtime_float();
  $time_start = $very_start_time;

  $result = db_truncate('{upitt_islandora_audit_items_islandora}')->execute();
  $result = db_truncate('upitt_islandora_audit_items_workflow_record')->execute();
  $result = db_truncate('upitt_islandora_audit_items_bigfoot')->execute();
  $result = db_truncate('upitt_islandora_audit_items_equinox')->execute();

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
 * This will not need to check to see if a record exists.  It can just insert.
 *
 * @param array $name_value_pairs
 */
function upitt_islandora_inventory_audit_insert($table_suffix, $name_value_pairs) {
  $id_inserted = db_insert('upitt_islandora_audit_items_' . $table_suffix)
    ->fields($name_value_pairs)
    ->execute();
  return $id_inserted;
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
  $item_query = "SELECT DISTINCT(i.do_id) `do_id` " .
                "FROM `item` i " .
                // transaction_action_id of 3 is "ingest completed"
                "JOIN `transaction` t ON (t.item_id = i.id AND t.transaction_action_id = 3) LIMIT 5";

  $result = mysqli_query($link, $item_query);
  if (!$result) {
    upitt_workflow_sql_error_die($link, $item_query);
  }

  $replace_sql = array();
  while ($row = mysqli_fetch_assoc($result)) {
    upitt_islandora_inventory_audit_insert('workflow_record', array('barcode' => $row['do_id']));
  }

  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_workflow_record');
}

/**
 * Scan to pick up any folders that were not already added by workflow reference.
 */
function upitt_islandora_inventory_audit_populate_bigfoot_path() {
  // scan the directory -- finding all folders that qualify as an object on bigfoot...
  // while a book is only one object on the bigfoot system, it must create an entry for
  // every page *.tif / *.tiff file as an object record.

  $dir = variable_get('upitt_islandora_inventory_audit_bigfoot_mapped_path', '/ingest/holding');
  $cdir = scandir($dir);

  foreach ($cdir as $key => $value) {
    if (!in_array($value,array(".",".."))) {
      if (is_dir($dir . '/' . $value)) {
        drupal_set_message('OBJECT FOLDER ' . $key . ' = ' . $value);
        upitt_islandora_inventory_scan_subfolder($dir . '/' . $value, 'bigfoot');
      }
      else {
        drupal_set_message('NORMAL FILE ' . $key . ' = ' . $value);
      }
    }
  }
  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_bigfoot_path');
}

function upitt_islandora_inventory_scan_subfolder($subfolder, $inventory_table) {
  $cdir = scandir($subfolder);
  $barcode_found = FALSE;
  foreach ($cdir as $key => $value) {
    if (!$barcode_found && !in_array($value,array(".",".."))) {
      if (is_dir($subfolder . '/' . $value)) {
        upitt_islandora_inventory_scan_subfolder($subfolder . '/' . $value, 'bigfoot');
      }
      else {
        if (strstr($value, '.marcxml.xml') || strstr($value, '.mods.xml') || strstr($value, '.dc.xml')) {
          $barcode = substr($value, 0, strpos($value, '.'));
          $barcode_found = TRUE;
        }
      }
    }
  }
  if ($barcode_found) {
    upitt_islandora_inventory_audit_insert($inventory_table, array($inventory_table . '_path' => $subfolder, 'barcode' => $barcode));
  } else {
    drupal_set_message('no barcode found in ' . $subfolder . '"');
  }
  drupal_set_message('exiting upitt_islandora_inventory_scan_subfolder');
}

/**
 * Scan to update the equinox path values.
 */
function upitt_islandora_inventory_audit_populate_equinox_path() {
  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_equinox_path');
}

/**
 * Scan the islandora system - run a Solr query to get all PID values - and insert them
 */
function upitt_islandora_inventory_audit_populate_islandora_pid() {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $query_processor = new IslandoraSolrQueryProcessor();

  $ret_arr = array();
  // run two queries --
  // 1) to get the datastreams available on this object
  $query_processor->solrQuery = 'PID:pitt\:*';
  $query_processor->solrStart = 0;
  $query_processor->solrLimit = UPITT_ISLANDORA_INVENTORY_SOLR_LIMIT;
  $query_processor->solrParams = array('fl' => 'PID');

  $url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
  $solr->setCreateDocuments(FALSE);
  try {
    $search_results = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams, 'GET');
    $tmp = json_decode($search_results->getRawResponse(), TRUE);

    $results = array();
    $numFound = $tmp['response']['numFound'];
    if ($numFound == UPITT_ISLANDORA_INVENTORY_SOLR_LIMIT) {
      drupal_set_message(t('The number of objects returned is the same as the maximum limit.  It is likely that there are more objects than ' . UPITT_ISLANDORA_INVENTORY_SOLR_LIMIT));
    }

    if ($tmp['response']['numFound'] > 0) {
      foreach ($tmp['response']['docs'] as $k=>$rec) {
        $pid = $rec['PID'];
        $barcode = substr($pid, strpos($pid, ':') + 1);
        upitt_islandora_inventory_audit_insert('islandora', array('islandora_pid' => $pid, 'barcode' => $barcode));
      }
    }
  }
  catch (Exception $e) {
    error_log('EXCEPTION in _save_solr_search_session : called from ' . $_SERVER['SERVER_NAME'] .
' - ' . $_SERVER['REQUEST_URI'] . '
' . print_r($e, true));
  }
  
  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_islandora_pid');
}