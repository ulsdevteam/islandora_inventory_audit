<?php

define('BIGFOOT_UPDATE_CHUNK_SIZE', 1000);
define('UPITT_ISLANDORA_INVENTORY_SOLR_LIMIT', 2000000);
define('UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE', 2000);

/**
 * @file
 * Database functions for the Islandora Inventory Audit module.
 */

function upitt_islandora_inventory_audit_html_escape_string($html_escape) {
  // ENT_HTML5 introduced with php 5.4, so check before using.
  if (defined('ENT_HTML5')) {
    $html_escape =  htmlspecialchars($html_escape, ENT_QUOTES | ENT_HTML5, 'UTF-8');
  }
  else {
    $html_escape =  htmlspecialchars($html_escape, ENT_QUOTES, 'UTF-8');
  }
  return $html_escape;
}

function upitt_islandora_inventory_audit_get_record_by_pid($pid) {
  // strip the namespace from the PID value and look for that barcode in all tables.
  $barcode = substr($pid, strpos($pid, ':') + 1);

  // don't even try to use the drupal database objects to get this query right ...
  $query = db_query('SELECT ai.islandora_pid, aw.barcode `workflow_record`, ab.bigfoot_path, ap.barcode `photodb_identifier` ' .
                    'FROM {upitt_islandora_audit_items_islandora} ai ' .
                    'LEFT JOIN {upitt_islandora_audit_items_workflow_record} aw ON (aw.barcode = ai.barcode) ' .
                    'LEFT JOIN {upitt_islandora_audit_items_bigfoot} ab ON (ab.barcode = ai.barcode) ' .
                    'LEFT JOIN {upitt_islandora_audit_items_equinox} ae ON (ae.barcode = ai.barcode) ' .
                    'LEFT JOIN {upitt_islandora_audit_items_photodb} ap ON (ap.barcode = ai.barcode) ' .
                    'WHERE ai.barcode = \'' . upitt_islandora_inventory_audit_html_escape_string($barcode) . '\'');

  return $query->fetchAssoc();
}

/**
 * This will sync the inventory with what is in Islandora.
 */
function upitt_islandora_inventory_audit_sync_islandora() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');

  $times_info = array();
  $very_start_time = upitt_islandora_inventory_audit_microtime_float();
  $time_start = $very_start_time;

  // $result = db_truncate('{upitt_islandora_audit_items_parents_islandora}')->execute();
  // $result = db_truncate('{upitt_islandora_audit_items_islandora}')->execute();

  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> truncate {upitt_islandora_audit_items_islandora} table done';

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
 * This will sync the inventory based on records in the django workflow tables.
 */
function upitt_islandora_inventory_audit_sync_workflow() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');

  $times_info = array();
  $very_start_time = upitt_islandora_inventory_audit_microtime_float();
  $time_start = $very_start_time;

  $result = db_truncate('upitt_islandora_audit_items_workflow_record')->execute();

  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> truncate {upitt_islandora_audit_items_workflow_record} table done';

  // populate by workflow records -- this could also update bigfoot_path entries.
  $time_start = upitt_islandora_inventory_audit_microtime_float();
  upitt_islandora_inventory_audit_populate_workflow_record();
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> upitt_islandora_inventory_audit_populate_workflow_record done';

  $time = $time_end - $very_start_time;
  $times_info[] = '<b>' . round($time, 2) . '</b> Total time';

  drupal_set_message(implode('<br>', $times_info));
}

/**
 * This will sync the inventory of the files on bigfoot.
 */
function upitt_islandora_inventory_audit_sync_bigfoot() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');

  $times_info = array();
  $very_start_time = upitt_islandora_inventory_audit_microtime_float();
  $time_start = $very_start_time;

  $result = db_truncate('upitt_islandora_audit_items_bigfoot')->execute();

  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> truncate {upitt_islandora_audit_items_bigfoot} table done';

  // scan to pick up any folders that were not already added by workflow reference.
  $time_start = upitt_islandora_inventory_audit_microtime_float();
  upitt_islandora_inventory_audit_populate_bigfoot_path();
  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $time_start;
  $times_info[] = '<b>' . round($time, 2) . ' seconds</b> upitt_islandora_inventory_audit_populate_bigfoot_path done';

  $time = $time_end - $very_start_time;
  $times_info[] = '<b>' . round($time, 2) . '</b> Total time';

  drupal_set_message(implode('<br>', $times_info));
}

/**
 * To populate the inventory records for the photoDB table.
 * @return boolean
 */
function upitt_islandora_inventory_audit_sync_photodb() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');

  $times_info = array();
  $very_start_time = upitt_islandora_inventory_audit_microtime_float();

  // populate the table here...

  $time_end = upitt_islandora_inventory_audit_microtime_float();
  $time = $time_end - $very_start_time;
  $times_info[] = '<b>' . round($time, 2) . '</b> Total time';

  drupal_set_message(implode('<br>', $times_info));
  return TRUE;
}

/**
 * This will update the page count values for all book objects from the islandora table --
 * and gets a matching value from the workflow database by looking at the item_file table.
 */
function upitt_islandora_inventory_pagecounts($pid = NULL) {
  global $databases;
  $drupal_database_name = $databases['default']['default']['database'];
  $workflow_database_name = $databases['default']['mysql_new_workflow']['database'];

  set_time_limit(0);
  module_load_include('inc', 'upitt_workflow', 'includes/utilities');
  module_load_include('module', 'upitt_workflow', 'upitt_workflow');
  $link = upitt_workflow_get_databaselink('default');

  // Step 1 - get all book records - with two subqueries to count page objects
  // on islandora and to count page records in bigfoot.
  $booki_sql = "SELECT ii.barcode, ii.islandora_pid, " .
               " (SELECT COUNT(*) FROM `" .$drupal_database_name . "`.`upitt_islandora_audit_items_islandora` " .
               "  WHERE islandora_pid like CONCAT(ii.islandora_pid, '-%')) as `islandora_page_count` " .
               "FROM `" . $drupal_database_name . "`.`upitt_islandora_audit_items_islandora` ii " .
               "WHERE ii.fedora_model IN ('bookCModel', 'manuscriptCModel', 'newspaperCModel')";
  $bookb_sql = "SELECT ii.barcode, ii.islandora_pid, " .
               "(SELECT COUNT(*) " .
               " FROM `" . $workflow_database_name . "`.`item` iwi " .
               " JOIN `" . $workflow_database_name . "`.`item_file` iwf  ON (iwf.item_id = iwi.id AND iwf.`use` = 'MASTER') " .
               " WHERE iwi.do_id = ii.barcode) as `bigfoot_page_count` " .
               "FROM `" . $drupal_database_name . "`.`upitt_islandora_audit_items_islandora` ii " .
               "WHERE ii.fedora_model IN ('bookCModel', 'manuscriptCModel', 'newspaperCModel')";

  if (!is_null($pid)) {
    $booki_sql .= " AND ii.islandora_pid = '" . $pid . "'";
    $bookb_sql .= " AND ii.islandora_pid = '" . $pid . "'";
  }
  else {
    // Step 2 - clear the table for the new results.
    $truncate_result = mysqli_query($link, "TRUNCATE TABLE `upitt_islandora_audit_page_counts`");
  }

  // Step 3 - populate the page_count table with the results of $booki_sql from Step 1.
  $result_i = mysqli_query($link, $booki_sql);
  if (!$result_i) {
    upitt_workflow_sql_error_die($link, $booki_sql);
  }
  while ($row = mysqli_fetch_assoc($result_i)) {
    $insert_sql = "REPLACE INTO `upitt_islandora_audit_page_counts` (`barcode`, `islandora_page_count`) " .
                  "VALUES ('" . upitt_islandora_inventory_audit_html_escape_string($row['barcode']) . "', " .
                  $row['islandora_page_count'] . ")";
    $result = mysqli_query($link, $insert_sql);
  }

  // Step 4 - update the page_count table with the results of $bookb_sql from Step 1.
  $result_b = mysqli_query($link, $bookb_sql);
  if (!$result_b) {
    upitt_workflow_sql_error_die($link, $bookb_sql);
  }
  while ($row = mysqli_fetch_assoc($result_b)) {
    $insert_sql = "UPDATE `upitt_islandora_audit_page_counts` SET `bigfoot_page_count` = " .
                  $row['bigfoot_page_count'] . " WHERE `barcode` = '" . upitt_islandora_inventory_audit_html_escape_string($row['barcode']) . "'";
    $result = mysqli_query($link, $insert_sql);
  }


  mysqli_close($link);

}


/**
 * This process will update the "in_all_ingests" field based on whether or not the
 * barcode is in the all_ingests.txt file -- requires a upitt_workflow function call.
 */
function upitt_islandora_inventory_updatebigfoottable() {
  // TODO - this may be best if it just loops through the all_ingests.txt file and updates only those records.
  set_time_limit(0);
  module_load_include('inc', 'upitt_workflow', 'includes/utilities');
  module_load_include('module', 'upitt_workflow', 'upitt_workflow');
  $offset = 0;
  $link = upitt_workflow_get_databaselink('default');

  $all_ingests_file = file(drupal_get_path('module', 'upitt_workflow') . '/includes/all_ingests.txt');

  while ($num_updated = upitt_islandora_inventory_updatebigfoottable_chunk($offset, $link, $all_ingests_file) > 0) {
    $offset += BIGFOOT_UPDATE_CHUNK_SIZE;
  }
}

function upitt_islandora_inventory_updatebigfoottable_chunk($offset, $link, $all_ingests_file) {
  $max = $offset + BIGFOOT_UPDATE_CHUNK_SIZE;
  $max = min($max, count($all_ingests_file));

  $d = date('H:i:s');
  error_log('[' . $d . '] '. $offset . ' updating upitt_islandora_audit_items_bigfoot.in_all_ingests');

  for ($i = $offset; $i < $max; $i++) {
    $updated = 0;
    $barcode = trim($all_ingests_file[$i]);
    $update_sql = 'UPDATE upitt_islandora_audit_items_bigfoot SET in_all_ingests = 1 WHERE barcode = \'' . $barcode . '\'';
    $result = mysqli_query($link, $update_sql);
    $updated ++;
  }
  return $updated;
}

/**
 * This will perform all four areas "sync" functions.
 */
function upitt_islandora_inventory_audit_populate() {
  upitt_islandora_inventory_audit_sync_islandora();
  upitt_islandora_inventory_audit_sync_workflow();
  upitt_islandora_inventory_audit_sync_bigfoot();
  upitt_islandora_inventory_audit_sync_photodb();
}

/**
 * This will not need to check to see if a record exists.  It can just insert.
 *
 * @param array $name_value_pairs
 */
function upitt_islandora_inventory_audit_insert($table_suffix, $name_value_pairs) {
  if ($table_suffix == 'islandora') {
    db_query("DELETE FROM upitt_islandora_audit_items_islandora WHERE islandora_pid = '" . $name_value_pairs['islandora_pid'] . "'")->execute();
  }
  $id_inserted = db_insert('upitt_islandora_audit_items_' . $table_suffix)
    ->fields($name_value_pairs)
    ->execute();
  return $id_inserted;
}

function upitt_islandora_inventory_audit_insert_islandora_parents($pid, $parents) {
  foreach ($parents as $parent) {
    if (is_object(islandora_object_load($parent))) {
      db_query("DELETE FROM upitt_islandora_audit_items_parents_islandora WHERE " .
              "islandora_pid = '" . $pid . "' AND parent_islandora_pid = '" . $parent . "'")->execute();
      db_insert('upitt_islandora_audit_items_parents_islandora')
        ->fields(array(
          'islandora_pid' => $pid,
          'parent_islandora_pid' => $parent,)
        )
        ->execute();
    }
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
  set_time_limit(0);

  module_load_include('inc', 'upitt_workflow', 'includes/utilities');
  module_load_include('module', 'upitt_workflow', 'upitt_workflow');

  $link = upitt_workflow_get_databaselink('mysql_new_workflow');
  $item_query = "SELECT DISTINCT(i.do_id) `do_id` " .
                "FROM `item` i " .
                // transaction_action_id of 3 is "ingest completed"
                "JOIN `transaction` t ON (t.item_id = i.id AND t.transaction_action_id = 3)";

  $result = mysqli_query($link, $item_query);
  if (!$result) {
    upitt_workflow_sql_error_die($link, $item_query);
  }

  while ($row = mysqli_fetch_assoc($result)) {
    upitt_islandora_inventory_audit_insert('workflow_record', array('barcode' => $row['do_id']));
  }

  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_workflow_record');
}

/**
 * Scan to pick up any folders that were not already added by workflow reference.
 */
function upitt_islandora_inventory_audit_populate_bigfoot_path() {
  set_time_limit(0);

  // scan the directory -- finding all folders that qualify as an object on bigfoot...
  // while a book is only one object on the bigfoot system, it must create an entry for
  // every page *.tif / *.tiff file as an object record.
  $files_list_path = variable_get('upitt_islandora_inventory_audit_bigfoot_files_mapped_path');
  if (!$files_list_path) {
    drupal_set_message('The "Bigfoot files list file" is not configured.  Configure it: ' . l('/admin/islandora/inventory_audit', '/admin/islandora/inventory_audit'), 'error');
    return;
  }

  $ignore_these_upcased_filenames = array('TARGET.TIF');
  $handle = fopen($files_list_path, "r");
  if ($handle) {
    $last_barcode = '';
    while (($file = fgets($handle)) !== FALSE) {
      $file = trim($file);
      $path_info = pathinfo($file);
      $path = (isset($path_info['dirname']) ? $path_info['dirname'] : '/');
      $fname_only = $path_info['filename'];

      if (strstr($file, '.mods.xml')/* || strstr($file, '.dc.xml') */) {
        $barcode = str_replace(array($path . '/', '.mods.xml'/*, '.dc.xml'*/),
                "", $file);
        $upcase_barcode = strtoupper($barcode);
        if ($last_barcode <> $upcase_barcode) {
          upitt_islandora_inventory_audit_insert('bigfoot', array('bigfoot_path' => $path, 'barcode' => $barcode));
          $last_barcode = $upcase_barcode;
        }
      }
//       elseif (isset($fn_parts[0]) && isset($fn_parts[1]) && ($fn_parts[1] == 'tif' || $fn_parts[1] == 'tiff') && is_numeric($fn_parts[0])) {
      elseif (($path_info['extension'] == 'tif') || ($path_info['extension'] == 'tiff')) {
        // For this section, the barcode can come from the filename
        $path_parts = explode("/", $path);
        $barcode = (count($path_parts) > 0) ? $path_parts[count($path_parts) - 1] : '';
        // Only set this as the barcode if the two parts are not the same...
        $pseudo_barcode = ($barcode <> $fname_only) ? $barcode . '-' . $fname_only : $fname_only;

        // shift off the last element (the extension) -- and reimplode to get just the filename portion
        // Look at the filename -- if it looks like it is a page object (####.tif), then add that reference as
        // a barcode value like 31234567809-0041.
        $upcased_fname = strtoupper($fname_only);
        if (array_search($upcased_fname, $ignore_these_upcased_filenames) === FALSE) {
          upitt_islandora_inventory_audit_insert('bigfoot', array('bigfoot_path' => $path, 'barcode' => $pseudo_barcode));
        }
      }
    }
    fclose($handle);
  } else {
    drupal_set_message('There was a problem opening the file "' . $files_list_path . '".  No inventory could be determined from the bigfoot location.', 'error');
  }
}

/**
 *
 * @param string $subfolder
 * @param string $inventory_table
 * @return string -- will return the barcode value of the folder that was scanned --
 * that value can be used in recursive calls.
 */
function upitt_islandora_inventory_scan_subfolder($subfolder, $inventory_table) {
  $cdir = scandir($subfolder);
  $barcode_found = $barcode = FALSE;
  $pages = array();
  foreach ($cdir as $key => $value) {
    if (!in_array($value,array(".",".."))) {
      if (is_dir($subfolder . '/' . $value)) {
        // SKIP folders that are inside the object folders
        // $barcode_found = upitt_islandora_inventory_scan_subfolder($subfolder . '/' . $value, 'bigfoot');
      }
      elseif (!$barcode_found) {
        if (strstr($value, '.marcxml.xml') || strstr($value, '.mods.xml') || strstr($value, '.dc.xml')) {
          $barcode = substr($value, 0, strpos($value, '.'));
          $barcode_found = TRUE;
        }
      }
      // Look at the filename -- if it looks like it is a page object (####.tif), then add that reference as
      // a barcode value like 31234567809-0041.
      $parts = explode(".", $value);
      if (isset($parts[0]) && isset($parts[1]) && ($parts[1] == 'tif' || $parts[1] == 'tiff') && is_numeric($parts[0])) {
        $pages[] = '' . $parts[0];
      }
    }
  }
  if ($barcode_found) {
    upitt_islandora_inventory_audit_insert($inventory_table, array($inventory_table . '_path' => $subfolder, 'barcode' => $barcode));
    foreach ($pages as $page) {
      $pseudo_barcode = $barcode . '-' . $page;
      upitt_islandora_inventory_audit_insert($inventory_table, array($inventory_table . '_path' => $subfolder, 'barcode' => $pseudo_barcode));
    }
  } else {
    drupal_set_message('no barcode found in ' . $subfolder . '"');
  }
  drupal_set_message('exiting upitt_islandora_inventory_scan_subfolder');
  return $barcode;
}

/**
 * Scan the islandora system - run a Solr query to get all PID values - and insert them
 */
function upitt_islandora_inventory_audit_populate_islandora_pid() {
  set_time_limit(0);

  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $query_processor = new IslandoraSolrQueryProcessor();

  $use_offset = intval(variable_get('upitt_islandora_inventory_audit_islandora_offset', '0') + 0);

  $ret_arr = array();
  // run two queries --
  // 1) to get the datastreams available on this object
  $query_processor->solrQuery = 'PID:pitt\:*';
  $query_processor->solrStart = 0;
  $query_processor->solrLimit = 0;
  $query_processor->solrParams = array('fl' => 'PID');
  $starting_date_filter = (variable_get('upitt_islandora_inventory_audit_islandora_start_date', '')) ?
      '[' . variable_get('upitt_islandora_inventory_audit_islandora_start_date', '') . ' TO NOW]' : '';
  if ($starting_date_filter) {
    $query_processor->solrParams['fq'] = 'fgs_createdDate_dt:' . $starting_date_filter;
  }

  $url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
  $solr->setCreateDocuments(FALSE);
  try {
    $search_results = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams, 'GET');
    $tmp = json_decode($search_results->getRawResponse(), TRUE);
    $results = array();
    // IF the process faults for some reason, and need to restart, remove the two
    // calls to db_truncate in upitt_islandora_inventory_audit_sync_islandora and set
    // the initial offset here after determining where to start again.  The objects are
    // pulled from Solr in the order of their PID value, so a newly inserted object would be
    // starting at the index that is +1 from the previously occuring PID value when
    // sorted alphabetically by PIDs.
    $offset = $use_offset;
    $total = 0;
    $numFound = $tmp['response']['numFound'] - $offset;
    // repeatedly call this to process this many records each time:
    //   UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE
    while ($total < $numFound) {
      error_log('.' . $total . ", " . $offset);
      $total = upitt_islandora_inventory_audit_populate_achunkof_islandora_pid($offset);
      $offset = $offset + $total;
    }
  }
  catch (Exception $e) {
    error_log('EXCEPTION in upitt_islandora_inventory_audit_populate_islandora_pid : called from ' . $_SERVER['SERVER_NAME'] .
' - ' . $_SERVER['REQUEST_URI'] . '
' . print_r($e, true));
  }

  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_islandora_pid');
}

/**
 * Process to update the depositor values for all child objects to all paged_content
 * models.
 *
 * DO THIS ALL AT ONCE AFTER THE TWO TABLES ARE CALCULATED - since this is
 * more than 1.5 million records, the JOIN ~ UPDATE statement will run out of memory
 * making a temporary index... so a script was used to update to set the depositor
 * value for the page objects.
 *
 */
function upitt_islandora_inventory_audit_update_depositors() {
  set_time_limit(0);
  $message = '';
  while ($processed = upitt_islandora_inventory_audit_update_depositors_do_some()) {
    $message .= ".";
  }
  $message .= "<br>Each dot above represents 50,000 updated records.";
  drupal_set_message($message);
}

function upitt_islandora_inventory_audit_update_depositors_do_some() {
  $result = db_query("SELECT ai.depositor, ai.islandora_pid, p.parent_islandora_pid as parent_pid, ai2.depositor `set` " .
    "FROM upitt_islandora_audit_items_islandora ai " .
    "JOIN upitt_islandora_audit_items_parents_islandora p ON (p.islandora_pid = ai.islandora_pid) " .
    "JOIN upitt_islandora_audit_items_islandora ai2 ON (ai2.islandora_pid = p.parent_islandora_pid) " .
    "WHERE ai.depositor = '' AND ai.fedora_model IN ('pageCModel', 'newspaperPageCModel', 'manuscriptPageCModel') " .
    " AND ai2.depositor <> '' LIMIT 50000");
  $result ->execute();
  $processed = 0;
  while($record = $result->fetchAssoc()) {
    $r = db_query("UPDATE upitt_islandora_audit_items_islandora SET depositor = '" . addslashes($record['set']) . "' WHERE islandora_pid = '" . $record['islandora_pid'] . "'")->execute();
    $processed++;
  }
  return $processed;
}

/**
 * This will get UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE islandora PID records at a time.
 */
function upitt_islandora_inventory_audit_populate_achunkof_islandora_pid($offset) {
  module_load_include('inc', 'upitt_workflow', 'includes/utilities');
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $query_processor = new IslandoraSolrQueryProcessor();

  $ret_arr = array();
  // run two queries --
  // 1) to get the datastreams available on this object
  $query_processor->solrQuery = 'PID:pitt\:*';
  $query_processor->solrStart = $offset;
  $query_processor->solrLimit = UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE;
  $query_processor->solrParams = array('fl' => 'PID', 'sort' => 'PID ASC');
  $starting_date_filter = (variable_get('upitt_islandora_inventory_audit_islandora_start_date', '')) ?
      variable_get('upitt_islandora_inventory_audit_islandora_start_date', '') . ' TO NOW' : '';
  if ($starting_date_filter) {
    /*
     * f%5B0%5D=mods_originInfo_type_sort_dateOther_dt%3A%5B1884-01-01T00%3A00%3A00Z%20TO%201914-01-01T00%3A00%3A00Z%5D
     * fgs_createdDate_dt:[2017-03-29T00%3A00%3A00Z TO NOW]
     * "fgs_createdDate_dt:[1884-01-01T00:00:00Z TO NOW]",
     */
    $query_processor->solrParams['fq'] = 'fgs_createdDate_dt:[' . $starting_date_filter . ']';
  }

  $url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
  $solr->setCreateDocuments(FALSE);
  try {
    $search_results = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams, 'GET');
    $tmp = json_decode($search_results->getRawResponse(), TRUE);

    $results = array();
    $numAdded = 0;
    $numFound = $tmp['response']['numFound'];
//    $pids = upitt_islandora_inventory_audit_missedpids();
    if ($tmp['response']['numFound'] > 0) {
//      foreach ($pids as $pid) {
//        $pid = trim($pid);
      foreach ($tmp['response']['docs'] as $k=>$rec) {
        $pid = $rec['PID'];
        $datastream_sizes = upitt_workflow_get_depositor_model_and_datastreams_sizes($pid);
        $datastream_size_sums = upitt_workflow_sum_datastream_sizes($datastream_sizes);
        $barcode = substr($pid, strpos($pid, ':') + 1);
        $obj = islandora_object_load($pid);
        if (is_object($obj)) {
          $parents = upitt_islandora_inventory_get_parent_values($obj);
          upitt_islandora_inventory_audit_insert('islandora', array('islandora_pid' => $pid,
              'barcode' => $barcode,
              'datastreams_latestsize' => $datastream_size_sums['datastreams_latestsize'],
              'datastreams_totalsize' => $datastream_size_sums['datastreams_totalsize'],
              'fedora_model' => (array_key_exists('RELS_EXT_hasModel_uri_s', $datastream_sizes) === TRUE) ? str_replace('info:fedora/islandora:', '', $datastream_sizes['RELS_EXT_hasModel_uri_s']) : '',
              'depositor' => isset($datastream_sizes['mods_name_depositor_namePart_s']) ? $datastream_sizes['mods_name_depositor_namePart_s'] : '',
              'create_dt' => isset($datastream_sizes['fgs_createdDate_dt']) ? 
                date('Y-m-d H:i:s', strtotime(str_replace(array("Z", "T"), "", $datastream_sizes['fgs_createdDate_dt']))) :
                'NULL',
              ));
          // add records for the object to parent relationhips
          upitt_islandora_inventory_audit_insert_islandora_parents($pid, $parents);
        }
        $numAdded++;
      }
    }
  }
  catch (Exception $e) {
    error_log('EXCEPTION in upitt_islandora_inventory_audit_populate_achunkof_islandora_pid : called from ' . $_SERVER['SERVER_NAME'] .
' - ' . $_SERVER['REQUEST_URI'] . '
' . print_r($e, true));
  }
  return $numAdded;
}

function upitt_islandora_inventory_all_record_counts() {
  $results = db_select('upitt_islandora_audit_items_islandora', 'ai')
            ->fields('ai')
            ->execute();
  $num_of_upitt_islandora_audit_items_islandora = $results->rowCount();
  $results = db_select('upitt_islandora_audit_items_workflow_record', 'aw')
            ->fields('aw')
            ->execute();
  $num_of_upitt_islandora_audit_items_workflow_record = $results->rowCount();
  $results = db_select('upitt_islandora_audit_items_bigfoot', 'ab')
            ->fields('ab')
            ->execute();
  $num_of_upitt_islandora_audit_items_bigfoot = $results->rowCount();
  $results = db_select('upitt_islandora_audit_items_equinox', 'ae')
            ->fields('ae')
            ->execute();
  $num_of_upitt_islandora_audit_items_equinox = $results->rowCount();
  $results = db_select('upitt_islandora_audit_items_photodb', 'ab')
            ->fields('ab')
            ->execute();
  $num_of_upitt_islandora_audit_items_photodb = $results->rowCount();
  return array('upitt_islandora_audit_items_islandora' => $num_of_upitt_islandora_audit_items_islandora,
      'upitt_islandora_audit_items_workflow_record' => $num_of_upitt_islandora_audit_items_workflow_record,
      'upitt_islandora_audit_items_bigfoot' => $num_of_upitt_islandora_audit_items_bigfoot,
      'upitt_islandora_audit_items_photodb' => $num_of_upitt_islandora_audit_items_photodb,
      'upitt_islandora_audit_items_equinox' => $num_of_upitt_islandora_audit_items_equinox,
      );
}

function upitt_islandora_inventory_audit_missedpids() {
  return array('pitt:00a303183m-0001', 'pitt:00a303183m-0008', 'pitt:00a303183m-0009', 'pitt:00a303183m-0010', 'pitt:00a303183m-0011', 'pitt:00a303183m-0012', 'pitt:00a303183m-0013', 'pitt:00a303183m-0014', 'pitt:00a303183m-0015', 'pitt:00a303183m-0016', 'pitt:00a303183m-0017', 'pitt:00a303183m-0018', 'pitt:00a303183m-0019', 'pitt:00a303183m-0020', 'pitt:00a303183m-0021', 'pitt:00a303183m-0022', 'pitt:00a303183m-0023', 'pitt:00a303183m-0024', 'pitt:00a303183m-0025', 'pitt:00a303183m-0026', 'pitt:00a303183m-0027', 'pitt:00a303183m-0028', 'pitt:00a303183m-0029', 'pitt:00a303183m-0030', 'pitt:00a303183m-0031', 'pitt:00a303183m-0032', 'pitt:00a303183m-0033', 'pitt:00a303183m-0034', 'pitt:00a303183m-0035', 'pitt:00a303183m-0036', 'pitt:00a303183m-0037', 'pitt:00a303183m-0038', 'pitt:00a303183m-0039', 'pitt:00a303183m-0040', 'pitt:00a303183m-0041', 'pitt:00a303183m-0042', 'pitt:00a303183m-0043', 'pitt:00a303183m-0044', 'pitt:00a303183m-0045', 'pitt:00a303183m-0046', 'pitt:00a303183m-0047', 'pitt:00a303183m-0048', 'pitt:00a303183m-0049', 'pitt:00a303183m-0050', 'pitt:00a303183m-0051', 'pitt:00a303183m-0052', 'pitt:00a303183m-0053', 'pitt:00a303183m-0054', 'pitt:00a303183m-0055', 'pitt:00a303183m-0056', 'pitt:00a303183m-0057', 'pitt:00a303183m-0058', 'pitt:00a303183m-0059', 'pitt:00a303183m-0060', 'pitt:00a303183m-0061', 'pitt:00a303183m-0062', 'pitt:00a303183m-0063', 'pitt:00a303183m-0064', 'pitt:00adj9508m-0001', 'pitt:00afh9773m-0034', 'pitt:00afh9773m-0035', 'pitt:00afh9773m-0335', 'pitt:00afh9773m-0336', 'pitt:00agc4820m-0001', 'pitt:00agc4820m-0010', 'pitt:00agc4820m-0012', 'pitt:00agc4820m-0013', 'pitt:00agc4820m-0014', 'pitt:00agc4820m-0015', 'pitt:00agc4820m-0100', 'pitt:00agc4820m-0101', 'pitt:00agc4820m-0102', 'pitt:00agc4820m-0103', 'pitt:00agc4820m-0104', 'pitt:00agc4820m-0105', 'pitt:00agc4820m-0106', 'pitt:00agc4820m-0107', 'pitt:00agc4820m-0108', 'pitt:00agc4820m-0117', 'pitt:00agc4820m-0118', 'pitt:00agc4820m-0119', 'pitt:00agc4820m-0120', 'pitt:00agc4820m-0121', 'pitt:00agc4820m-0122', 'pitt:00agc4820m-0123', 'pitt:00agc4820m-0124', 'pitt:00agc4820m-0125', 'pitt:00agc4820m-0126', 'pitt:00agc4820m-0127', 'pitt:00agc4820m-0128', 'pitt:00agc4820m-0129', 'pitt:00agc4820m-0130', 'pitt:00agc4820m-0131', 'pitt:00agc4820m-0132', 'pitt:00agc4820m-0133', 'pitt:00agc4820m-0134', 'pitt:00agc4820m-0135', 'pitt:00agc4820m-0136', 'pitt:00agc4820m-0137', 'pitt:00agc4820m-0138', 'pitt:00agc4820m-0139', 'pitt:00agc4820m-0140', 'pitt:00agc4820m-0141', 'pitt:00agc4820m-0142', 'pitt:00agc4820m-0143', 'pitt:00agc4820m-0144', 'pitt:00agc4820m-0145', 'pitt:00agc4820m-0146', 'pitt:00agc4820m-0147', 'pitt:00agc4820m-0148', 'pitt:00agc4820m-0149', 'pitt:00agc4820m-0150', 'pitt:00agc4820m-0151', 'pitt:00agc4820m-0152', 'pitt:00agc4820m-0153', 'pitt:00agc4820m-0154', 'pitt:00age8892m-0012', 'pitt:00age8892m-0013', 'pitt:00age8892m-0120', 'pitt:00age8892m-0121', 'pitt:00age8892m-0122', 'pitt:00age8892m-0123', 'pitt:00age8892m-0124', 'pitt:00age8892m-0125', 'pitt:00age8892m-0126', 'pitt:00age8892m-0127', 'pitt:00age8892m-0128', 'pitt:00age8892m-0129', 'pitt:00age8892m-0130', 'pitt:00age8892m-0131', 'pitt:00age8892m-0132', 'pitt:00age8892m-0133', 'pitt:00awn7796m-0076', 'pitt:00hc01976m', 'pitt:00hc05616m', 'pitt:00hc06822m', 'pitt:00hc06847m', 'pitt:00hc08750m-0433', 'pitt:00hc12053m', 'pitt:00hc12174m', 'pitt:00hc12653m', 'pitt:01awn7797m-0001', 'pitt:01awn7797m-0010', 'pitt:01awn7797m-0100', 'pitt:01awn7797m-0101', 'pitt:01awn7797m-0102', 'pitt:01awn7797m-0103', 'pitt:01awn7797m-0104', 'pitt:01awn7797m-0105', 'pitt:01awn7797m-0106', 'pitt:04awn7797m-0414', 'pitt:111', 'pitt:1910e36913-0045', 'pitt:1918e49702-0221', 'pitt:1918e49702-0222', 'pitt:1918e49702-0223', 'pitt:1918e49702-0224', 'pitt:1918e49702-0225', 'pitt:1918e49702-0226', 'pitt:1918e49702-0227', 'pitt:1918e49702-0228', 'pitt:1918e49702-0229', 'pitt:1918e49702-0230', 'pitt:1918e49702-0231', 'pitt:1918e49702-0232', 'pitt:1918e49702-0233', 'pitt:1918e49702-0234', 'pitt:1918e49702-0235', 'pitt:1918e49702-0236', 'pitt:1918e49702-0237', 'pitt:1918e49702-0238', 'pitt:1918e49702-0239', 'pitt:1918e49702-0240', 'pitt:1918e49702-0241', 'pitt:1918e49702-0242', 'pitt:1918e49702-0243', 'pitt:1918e49702-0244', 'pitt:1918e49702-0245', 'pitt:1918e49702-0246', 'pitt:1918e49702-0254', 'pitt:1918e49702-0255', 'pitt:1918e49702-0256', 'pitt:1918e49702-0257', 'pitt:1918e49702-0258', 'pitt:1918e49702-0259', 'pitt:1918e49702-0260', 'pitt:1918e49702-0261', 'pitt:1918e49702-0262', 'pitt:1918e49702-0263', 'pitt:1918e49702-0264', 'pitt:1918e49702-0265', 'pitt:1918e49702-0266', 'pitt:1918e49702-0267', 'pitt:1918e49702-0268', 'pitt:1918e49702-0269', 'pitt:1918e49702-0270', 'pitt:1918e49702-0271', 'pitt:1918e49702-0272', 'pitt:1918e49702-0273', 'pitt:1918e49702-0274', 'pitt:1918e49702-0275', 'pitt:1918e49702-0276', 'pitt:1918e49702-0277', 'pitt:1918e49702-0278', 'pitt:1918e49702-0279', 'pitt:1918e49702-0280', 'pitt:1918e49702-0281', 'pitt:1918e49702-0282', 'pitt:1918e49702-0283', 'pitt:1918e49702-0284', 'pitt:1918e49702-0285', 'pitt:1918e49702-0286', 'pitt:1918e49702-0287', 'pitt:1918e49702-0288', 'pitt:1918e49702-0289', 'pitt:1918e49702-0290', 'pitt:1918e49702-0291', 'pitt:1918e49702-0292', 'pitt:1918e49702-0293', 'pitt:1918e49702-0294', 'pitt:1918e49702-0295', 'pitt:1918e49702-0296', 'pitt:1918e49702-0297', 'pitt:1918e49702-0298', 'pitt:1918e49702-0299', 'pitt:1918e49702-0300', 'pitt:1918e49702-0301', 'pitt:1918e49702-0302', 'pitt:1918e49702-0303', 'pitt:1918e49702-0304', 'pitt:1918e49702-0305', 'pitt:1918e49702-0306', 'pitt:1918e49702-0307', 'pitt:1918e49702-0308', 'pitt:1918e49702-0309', 'pitt:1918e49702-0310', 'pitt:1918e49702-0311', 'pitt:1918e49702-0312', 'pitt:1918e49702-0313', 'pitt:1918e49702-0314', 'pitt:1918e49702-0315', 'pitt:1918e49702-0316', 'pitt:1918e49702-0317', 'pitt:1918e49702-0318', 'pitt:1918e49702-0319', 'pitt:1918e49702-0320', 'pitt:1918e49702-0321', 'pitt:1918e49702-0322', 'pitt:1918e49702-0323', 'pitt:1918e49702-0324', 'pitt:1918e49702-0325', 'pitt:1918e49702-0326', 'pitt:1918e49702-0327', 'pitt:1918e49702-0328', 'pitt:1918e49702-0329', 'pitt:1918e49702-0330', 'pitt:1918e49702-0331', 'pitt:1918e49702-0332', 'pitt:1918e49702-0333', 'pitt:1918e49702-0334', 'pitt:1918e49702-0335', 'pitt:1918e49702-0336', 'pitt:1918e49702-0337', 'pitt:2000.07.087-0002', 'pitt:2000.07.117-0002', 'pitt:20090803-rosensteel-0010', 'pitt:20090803-rosensteel-0011', 'pitt:230', 'pitt:231', 'pitt:232', 'pitt:233', 'pitt:234', 'pitt:235', 'pitt:237', 'pitt:238', 'pitt:239', 'pitt:240', 'pitt:241', 'pitt:242', 'pitt:27awn8131m-0014', 'pitt:27awn8131m-0132', 'pitt:27awn8131m-0133', 'pitt:27awn8131m-0134', 'pitt:27awn8131m-0135', 'pitt:27awn8131m-0136', 'pitt:27awn8131m-0137', 'pitt:27awn8131m-0138', 'pitt:27awn8131m-0139', 'pitt:27awn8131m-0140', 'pitt:27awn8131m-0141', 'pitt:27awn8131m-0142', 'pitt:31735029657974-0016', 'pitt:31735029657974-0064', 'pitt:31735029687732-0002', 'pitt:31735029687732-0003', 'pitt:31735029687732-0018', 'pitt:31735029687732-0019', 'pitt:31735029687732-0020', 'pitt:31735029687732-0021', 'pitt:31735029687732-0022', 'pitt:31735029687732-0023', 'pitt:31735029687732-0024', 'pitt:31735029687732-0025', 'pitt:31735029687732-0026', 'pitt:31735029687732-0027', 'pitt:31735029687732-0028', 'pitt:31735029687732-0029', 'pitt:31735029687732-0030', 'pitt:31735029687732-0031', 'pitt:31735029687732-0047', 'pitt:31735029687732-0048', 'pitt:31735029687732-0135', 'pitt:31735029687732-0136', 'pitt:31735029687732-0137', 'pitt:31735029687732-0138', 'pitt:31735029687732-0171', 'pitt:31735029687732-0172', 'pitt:31735033433537-0018', 'pitt:31735033434006-0022', 'pitt:31735033434006-0032', 'pitt:31735033434048-0028', 'pitt:31735033434048-0065', 'pitt:31735033434071-0002', 'pitt:31735033434121-0042', 'pitt:31735033434139-0010', 'pitt:31735033434139-0012', 'pitt:31735033434220-0009', 'pitt:31735033440441-0014', 'pitt:31735033440474-0003', 'pitt:31735033447313-0044', 'pitt:31735033448337-0017', 'pitt:31735033448394-0051', 'pitt:31735033448584-0029', 'pitt:31735033448584-0071', 'pitt:31735033448667-0011', 'pitt:31735033448667-0036', 'pitt:31735033448691-0004', 'pitt:31735033448717-0042', 'pitt:31735033448766-0042', 'pitt:31735033448774-0018', 'pitt:31735033448774-0041', 'pitt:31735033464896-0086', 'pitt:31735033464979-0035', 'pitt:31735033464995-0054', 'pitt:31735033465018-0043', 'pitt:31735033465125-0041', 'pitt:31735033465133-0025', 'pitt:31735033465133-0029', 'pitt:31735033465166-0057', 'pitt:31735033465208-0067', 'pitt:31735033465281-0044', 'pitt:31735033465299-0059', 'pitt:31735033465299-0105', 'pitt:31735033465299-0125', 'pitt:31735033465356-0051', 'pitt:31735033465364-0053', 'pitt:31735033465406-0060', 'pitt:31735033465406-0079', 'pitt:31735033465414-0007', 'pitt:31735033465414-0045', 'pitt:31735033466214-0083', 'pitt:31735033466313-0034', 'pitt:31735033466487-0076', 'pitt:31735037969825-0015', 'pitt:31735037969825-0111', 'pitt:31735037969866-0003', 'pitt:31735037969890-0010', 'pitt:31735037969890-0036', 'pitt:31735037969890-0082', 'pitt:31735037969908-0116', 'pitt:31735037969932-0026', 'pitt:31735037969973-0028', 'pitt:31735037969973-0109', 'pitt:31735037969973-0111', 'pitt:31735037969973-0120', 'pitt:31735037969999-0050', 'pitt:31735037970120-0060', 'pitt:31735037970120-0106', 'pitt:31735037970120-0130', 'pitt:31735037970146-0006', 'pitt:31735037970146-0072', 'pitt:31735037970179-0074', 'pitt:31735037970195-0073', 'pitt:31735037970195-0099', 'pitt:31735037970195-0104', 'pitt:31735037970195-0158', 'pitt:31735037970195-0170', 'pitt:31735037970195-0184', 'pitt:31735037970195-0224', 'pitt:31735037970195-0413', 'pitt:31735037970195-0429', 'pitt:31735037970203-0046', 'pitt:31735037970203-0054', 'pitt:31735037973223-0049', 'pitt:31735037973223-0132', 'pitt:31735037973231-0154', 'pitt:31735037973231-0162', 'pitt:31735037973231-0184', 'pitt:31735037973348-0055', 'pitt:31735037973462-0024', 'pitt:31735037973462-0132', 'pitt:31735037973462-0196', 'pitt:31735037973587-0021', 'pitt:31735037973629-0012', 'pitt:31735037973629-0060', 'pitt:31735037981390-0086', 'pitt:31735037981390-0146', 'pitt:31735037981416-0053', 'pitt:31735037981416-0067', 'pitt:31735037981416-0134', 'pitt:31735037981424-0033', 'pitt:31735037981424-0104', 'pitt:31735037981523-0019', 'pitt:31735037981523-0021', 'pitt:31735037981572-0019', 'pitt:31735037981671-0058', 'pitt:31735037981846-0003', 'pitt:31735037981861-0056', 'pitt:31735037981929-0007', 'pitt:31735037984253-0001',
  'pitt:31735037984261-0005', 'pitt:31735037984303-0002', 'pitt:31735038276188-0423', 'pitt:31735038276188-0424', 'pitt:31735038276188-0425', 'pitt:31735041719364-0003', 'pitt:31735041719364-0021', 'pitt:31735041719364-0022', 'pitt:31735041719364-0023', 'pitt:31735041719364-0024', 'pitt:31735041719364-0025', 'pitt:31735041719364-0027', 'pitt:31735041719364-0028', 'pitt:31735041719364-0029', 'pitt:31735041719364-0030', 'pitt:31735041719364-0031', 'pitt:31735041719364-0208', 'pitt:31735041719364-0209', 'pitt:31735041719364-0210', 'pitt:31735041719364-0211', 'pitt:31735041719364-0212', 'pitt:31735041719364-0213', 'pitt:31735041719364-0214', 'pitt:31735041719364-0215', 'pitt:31735041719364-0216', 'pitt:31735041719364-0217', 'pitt:31735041719364-0218', 'pitt:31735041719364-0219', 'pitt:31735041719364-0220', 'pitt:31735041719364-0221', 'pitt:31735041719364-0222', 'pitt:31735041719364-0223', 'pitt:31735041719364-0224', 'pitt:31735041719364-0225', 'pitt:31735041719364-0226', 'pitt:31735041719364-0227', 'pitt:31735041719364-0228', 'pitt:31735041719364-0229', 'pitt:31735041719364-0230', 'pitt:31735041719364-0231', 'pitt:31735041719364-0232', 'pitt:31735041719364-0233', 'pitt:31735041719364-0234', 'pitt:31735041719364-0235', 'pitt:31735041719364-0236', 'pitt:31735041719364-0237', 'pitt:31735041719364-0238', 'pitt:31735041719364-0239', 'pitt:31735041719364-0240', 'pitt:31735041719364-0241', 'pitt:31735041719364-0242', 'pitt:31735041719364-0243', 'pitt:31735041719364-0244', 'pitt:31735041719364-0245', 'pitt:31735041719364-0246', 'pitt:31735041719364-0247', 'pitt:31735041719364-0248', 'pitt:31735041719364-0249', 'pitt:31735041719364-0250', 'pitt:31735041719364-0251', 'pitt:31735041719364-0252', 'pitt:31735041719364-0253', 'pitt:31735041719364-0254', 'pitt:31735041719364-0255', 'pitt:31735041719364-0256', 'pitt:31735041719364-0257', 'pitt:31735041719364-0258', 'pitt:31735041719364-0263', 'pitt:31735041719364-0264', 'pitt:31735041719364-0265', 'pitt:31735041719364-0266', 'pitt:31735041719364-0267', 'pitt:31735041719364-0268', 'pitt:31735041719364-0269', 'pitt:31735041719364-0270', 'pitt:31735041719364-0271', 'pitt:31735041719364-0272', 'pitt:31735041719364-0273', 'pitt:31735041719364-0274', 'pitt:31735041719364-0275', 'pitt:31735041719364-0276', 'pitt:31735041719364-0277', 'pitt:31735041719364-0278', 'pitt:31735041719364-0279', 'pitt:31735041719364-0280', 'pitt:31735041719364-0281', 'pitt:31735041719364-0282', 'pitt:31735041719364-0283', 'pitt:31735041719364-0284', 'pitt:31735041719364-0285', 'pitt:31735041719364-0286', 'pitt:31735041719364-0287', 'pitt:31735041719364-0288', 'pitt:31735041719364-0289', 'pitt:31735041719364-0290', 'pitt:31735041719364-0291', 'pitt:31735041719364-0292', 'pitt:31735041719364-0293', 'pitt:31735041719364-0294', 'pitt:31735041719364-0296', 'pitt:31735041719364-0297', 'pitt:31735041719364-0298', 'pitt:31735041719364-0299', 'pitt:31735041719364-0300', 'pitt:31735041719364-0301', 'pitt:31735041719364-0302', 'pitt:31735041719364-0303', 'pitt:31735041719364-0304', 'pitt:31735041719364-0305', 'pitt:31735041719364-0306', 'pitt:31735041719364-0307', 'pitt:31735041719364-0308', 'pitt:31735041719364-0309', 'pitt:31735041719364-0310', 'pitt:31735041719364-0311', 'pitt:31735044221517-0012', 'pitt:31735044221517-0034', 'pitt:31735044221640-0003', 'pitt:31735044221723-0015', 'pitt:31735044221806-0030', 'pitt:31735044222267-0006', 'pitt:31735044598922-0145', 'pitt:31735045177304-0004', 'pitt:31735047439355-0037', 'pitt:31735051649592-0003', 'pitt:31735051649592-0004', 'pitt:31735051649592-0005', 'pitt:31735051649592-0006', 'pitt:31735051649592-0007', 'pitt:31735051649592-0008', 'pitt:31735051649592-0009', 'pitt:31735051649592-0021', 'pitt:31735051649592-0022', 'pitt:31735051649592-0023', 'pitt:31735051649592-0024', 'pitt:31735051649592-0025', 'pitt:31735051649592-0026', 'pitt:31735051649592-0027', 'pitt:31735051649592-0028', 'pitt:31735051649592-0029', 'pitt:31735051649592-0030', 'pitt:31735051649592-0031', 'pitt:31735051649592-0032', 'pitt:31735051649592-0033', 'pitt:31735051649592-0034', 'pitt:31735051649592-0035', 'pitt:31735051649592-0036', 'pitt:31735051649592-0037', 'pitt:31735051649592-0038', 'pitt:31735051649592-0039', 'pitt:31735051649592-0040', 'pitt:31735051649592-0041', 'pitt:31735051649592-0042', 'pitt:31735051649592-0043', 'pitt:31735051649592-0044', 'pitt:31735051649592-0045', 'pitt:31735051649592-0046', 'pitt:31735051649592-0047', 'pitt:31735051649592-0048', 'pitt:31735051649592-0049', 'pitt:31735051649592-0050', 'pitt:31735051649592-0051', 'pitt:31735051649592-0052', 'pitt:31735051649592-0053', 'pitt:31735051649592-0054', 'pitt:31735051649592-0055', 'pitt:31735051649592-0056', 'pitt:31735051649592-0057', 'pitt:31735051649592-0058', 'pitt:31735051649592-0059', 'pitt:31735051649592-0060', 'pitt:31735051649592-0061', 'pitt:31735051649592-0062', 'pitt:31735051649592-0063', 'pitt:31735051649592-0064', 'pitt:31735051649592-0065', 'pitt:31735051649592-0066', 'pitt:31735051649592-0067', 'pitt:31735051649592-0068', 'pitt:31735051649592-0069', 'pitt:31735051649592-0070', 'pitt:31735051649592-0071', 'pitt:31735051649592-0072', 'pitt:31735051649592-0073', 'pitt:31735051649592-0074', 'pitt:31735051649592-0075', 'pitt:31735051649592-0076', 'pitt:31735051649592-0077', 'pitt:31735051649592-0078', 'pitt:31735051649592-0079', 'pitt:31735051649592-0080', 'pitt:31735051649592-0081', 'pitt:31735051649592-0082', 'pitt:31735051649592-0083', 'pitt:31735051649592-0084', 'pitt:31735051649592-0085', 'pitt:31735051649592-0086', 'pitt:31735051649592-0087', 'pitt:31735051649592-0088', 'pitt:31735051649592-0089', 'pitt:31735051649592-0090', 'pitt:31735051649592-0091', 'pitt:31735051649592-0092', 'pitt:31735051649592-0093', 'pitt:31735051649592-0094', 'pitt:31735051649592-0095', 'pitt:31735051649592-0096', 'pitt:31735051649592-0097', 'pitt:31735051649592-0098', 'pitt:31735051649592-0099', 'pitt:31735051649592-0203', 'pitt:31735051649592-0204', 'pitt:31735051649592-0205', 'pitt:31735051649592-0206', 'pitt:31735051649592-0207', 'pitt:31735051649592-0208', 'pitt:31735051649592-0209', 'pitt:31735051649592-0210', 'pitt:31735051649592-0211', 'pitt:31735051649592-0212', 'pitt:31735051649592-0213', 'pitt:31735051649592-0214', 'pitt:31735051649592-0215', 'pitt:31735051649592-0216', 'pitt:31735051649592-0217', 'pitt:31735051649592-0218', 'pitt:31735051649592-0219', 'pitt:31735051649592-0220', 'pitt:31735051649592-0221', 'pitt:31735051649592-0222', 'pitt:31735051649592-0223', 'pitt:31735051649592-0224', 'pitt:31735051649592-0225', 'pitt:31735051649592-0226', 'pitt:31735051649592-0227', 'pitt:31735051649592-0228', 'pitt:31735051649592-0229', 'pitt:31735051649592-0230', 'pitt:31735051649592-0231', 'pitt:31735051649592-0232', 'pitt:31735051649592-0233', 'pitt:31735051649592-0234', 'pitt:31735051649592-0235', 'pitt:31735051649592-0236', 'pitt:31735051649592-0237', 'pitt:31735051649592-0238', 'pitt:31735051649592-0239', 'pitt:31735051649592-0240', 'pitt:31735051649592-0241', 'pitt:31735051649592-0242', 'pitt:31735051649592-0243', 'pitt:31735051649592-0244', 'pitt:31735051649592-0245', 'pitt:31735051649592-0246', 'pitt:31735051649592-0247', 'pitt:31735051649592-0248', 'pitt:31735051649592-0249', 'pitt:31735051649592-0250', 'pitt:31735051649592-0251', 'pitt:31735051649592-0252', 'pitt:31735051649592-0253', 'pitt:31735051649592-0254', 'pitt:31735051649592-0255', 'pitt:31735051649592-0256', 'pitt:31735051649592-0257', 'pitt:31735051649592-0258', 'pitt:31735051649592-0259', 'pitt:31735051649592-0260', 'pitt:31735051649592-0261', 'pitt:31735051649592-0262', 'pitt:31735051649592-0263', 'pitt:31735051649592-0264', 'pitt:31735051649592-0265', 'pitt:31735051649592-0266', 'pitt:31735051649592-0267', 'pitt:31735051649592-0268', 'pitt:31735051649592-0269', 'pitt:31735051649592-0270', 'pitt:31735051649592-0271', 'pitt:31735051649592-0272', 'pitt:31735051649592-0273', 'pitt:31735051649592-0274', 'pitt:31735051649592-0275', 'pitt:31735051649592-0276', 'pitt:31735051649592-0277', 'pitt:31735051649592-0278', 'pitt:31735051649592-0279', 'pitt:31735051649592-0280', 'pitt:31735051649592-0281', 'pitt:31735051649592-0282', 'pitt:31735051649592-0283', 'pitt:31735051649592-0284', 'pitt:31735051649592-0285', 'pitt:31735051649592-0286', 'pitt:31735051649592-0287', 'pitt:31735051649592-0288', 'pitt:31735051649592-0289', 'pitt:31735051649592-0290', 'pitt:31735051649592-0291', 'pitt:31735051649592-0292', 'pitt:31735051649592-0293', 'pitt:31735051649592-0294', 'pitt:31735051649592-0295', 'pitt:31735051649592-0296', 'pitt:31735051649592-0297', 'pitt:31735051649592-0298', 'pitt:31735051649592-0299', 'pitt:31735051649592-0300', 'pitt:31735051649592-0301', 'pitt:31735051649592-0302', 'pitt:31735051649592-0303', 'pitt:31735051649592-0304', 'pitt:31735051649592-0305', 'pitt:31735051649592-0306', 'pitt:31735051649592-0307', 'pitt:31735051649592-0308', 'pitt:31735051649592-0309', 'pitt:31735051649592-0310', 'pitt:31735051649592-0311', 'pitt:31735051649592-0312', 'pitt:31735051649592-0313', 'pitt:31735051649592-0314', 'pitt:31735051649592-0315', 'pitt:31735051649592-0316', 'pitt:31735051649592-0317', 'pitt:31735051649592-0318', 'pitt:31735051649592-0319', 'pitt:31735051649592-0320', 'pitt:31735051649592-0321', 'pitt:31735051649592-0322', 'pitt:31735051649592-0323', 'pitt:31735051649592-0324', 'pitt:31735051649592-0325', 'pitt:31735051649592-0326', 'pitt:31735051649592-0327', 'pitt:31735051649592-0328', 'pitt:31735051649592-0329', 'pitt:31735051649592-0330', 'pitt:31735051649592-0331', 'pitt:31735051649592-0332', 'pitt:31735051649592-0333', 'pitt:31735051649592-0334', 'pitt:31735051649592-0335', 'pitt:31735051649592-0336', 'pitt:31735051649592-0337', 'pitt:31735051649592-0338', 'pitt:31735051649592-0339', 'pitt:31735051649592-0340', 'pitt:31735051649592-0341', 'pitt:31735051649592-0342', 'pitt:31735051649592-0343', 'pitt:31735051649592-0344', 'pitt:31735051649592-0345', 'pitt:31735051649592-0346', 'pitt:31735051649592-0347', 'pitt:31735051649592-0348', 'pitt:31735051649592-0349', 'pitt:31735051649592-0350', 'pitt:31735051649592-0351', 'pitt:31735051649592-0352', 'pitt:31735051649592-0353', 'pitt:31735051649592-0354', 'pitt:31735051649592-0355', 'pitt:31735051649592-0356', 'pitt:31735051649592-0357', 'pitt:31735051649592-0358', 'pitt:31735051649592-0359', 'pitt:31735051649592-0360', 'pitt:31735051649592-0361', 'pitt:31735051649592-0362', 'pitt:31735051649592-0363', 'pitt:31735051649592-0364', 
  'pitt:31735051649592-0365', 'pitt:31735051649592-0366', 'pitt:31735051649592-0367', 'pitt:31735051649592-0368', 'pitt:31735051649592-0369', 'pitt:31735051649592-0370', 'pitt:31735051649592-0371', 'pitt:31735051649592-0372', 'pitt:31735051649592-0373', 'pitt:31735051649592-0374', 'pitt:31735051649592-0375', 'pitt:31735051649592-0376', 'pitt:31735051649592-0377', 'pitt:31735051649592-0378', 'pitt:31735051649592-0379', 'pitt:31735051649592-0380', 'pitt:31735051649592-0381', 'pitt:31735051649592-0382', 'pitt:31735051649592-0383', 'pitt:31735051649592-0384', 'pitt:31735051649592-0385', 'pitt:31735051649592-0386', 'pitt:31735051649592-0387', 'pitt:31735051649592-0388', 'pitt:31735051649592-0389', 'pitt:31735051649592-0390', 'pitt:31735051649592-0391', 'pitt:31735051649592-0392', 'pitt:31735051649592-0393', 'pitt:31735051649592-0394', 'pitt:31735051649592-0395', 'pitt:31735051649592-0396', 'pitt:31735051649592-0397', 'pitt:31735051649592-0398', 'pitt:31735051649592-0399', 'pitt:31735051649592-0400', 'pitt:31735051649592-0401', 'pitt:31735051649592-0402', 'pitt:31735051649592-0403', 'pitt:31735051649592-0404', 'pitt:31735051649592-0405', 'pitt:31735051649592-0406', 'pitt:31735051649592-0407', 'pitt:31735051649592-0408', 'pitt:31735051649592-0409', 'pitt:31735051649592-0410', 'pitt:31735051649592-0411', 'pitt:31735051649592-0412', 'pitt:31735051649592-0413', 'pitt:31735051649592-0414', 'pitt:31735051649592-0415', 'pitt:31735051649592-0416', 'pitt:31735051649592-0417', 'pitt:31735051649592-0418', 'pitt:31735051649592-0419', 'pitt:31735051649592-0420', 'pitt:31735051649592-0421', 'pitt:31735051649592-0422', 'pitt:31735051649592-0423', 'pitt:31735051649592-0424', 'pitt:31735051649592-0425', 'pitt:31735051649592-0426', 'pitt:31735051649592-0427', 'pitt:31735051649592-0428', 'pitt:31735051649592-0429', 'pitt:31735051649592-0430', 'pitt:31735051649592-0431', 'pitt:31735051649592-0432', 'pitt:31735051649592-0433', 'pitt:31735051649592-0434', 'pitt:31735051649592-0435', 'pitt:31735051649592-0436', 'pitt:31735051649592-0437', 'pitt:31735051649592-0438', 'pitt:31735051649592-0439', 'pitt:31735051649592-0440', 'pitt:31735051649592-0441', 'pitt:31735051649592-0442', 'pitt:31735051649592-0443', 'pitt:31735051649592-0444', 'pitt:31735051649592-0445', 'pitt:31735051649592-0446', 'pitt:31735051649592-0447', 'pitt:31735051649592-0448', 'pitt:31735051649592-0449', 'pitt:31735051650285-0003', 'pitt:31735051650285-0004', 'pitt:31735051650285-0005', 'pitt:31735051650285-0006', 'pitt:31735051650285-0007', 'pitt:31735051650285-0008', 'pitt:31735051650285-0009', 'pitt:31735051650285-0028', 'pitt:31735051650285-0029', 'pitt:31735051650285-0030', 'pitt:31735051650285-0031', 'pitt:31735051650285-0032', 'pitt:31735051650285-0033', 'pitt:31735051650285-0034', 'pitt:31735051650285-0035', 'pitt:31735051650285-0036', 'pitt:31735051650285-0037', 'pitt:31735051650285-0038', 'pitt:31735051650285-0039', 'pitt:31735051650285-0040', 'pitt:31735051650285-0041', 'pitt:31735051650285-0042', 'pitt:31735051650285-0043', 'pitt:31735051650285-0044', 'pitt:31735051650285-0045', 'pitt:31735051650285-0046', 'pitt:31735051650285-0047', 'pitt:31735051650285-0048', 'pitt:31735051650285-0049', 'pitt:31735051650285-0050', 'pitt:31735051650285-0051', 'pitt:31735051650285-0052', 'pitt:31735051650285-0053', 'pitt:31735051650285-0054', 'pitt:31735051650285-0055', 'pitt:31735051650285-0056', 'pitt:31735051650285-0057', 'pitt:31735051650285-0058', 'pitt:31735051650285-0059', 'pitt:31735051650285-0060', 'pitt:31735051650285-0061', 'pitt:31735051650285-0062', 'pitt:31735051650285-0063', 'pitt:31735051650285-0064', 'pitt:31735051650285-0065', 'pitt:31735051650285-0066', 'pitt:31735051650285-0067', 'pitt:31735051650285-0068', 'pitt:31735051650285-0069', 'pitt:31735051650285-0070', 'pitt:31735051650285-0071', 'pitt:31735051650285-0072', 'pitt:31735051650285-0073', 'pitt:31735051650285-0074', 'pitt:31735051650285-0075', 'pitt:31735051650285-0076', 'pitt:31735051650285-0077', 'pitt:31735051650285-0078', 'pitt:31735051650285-0079', 'pitt:31735051650285-0080', 'pitt:31735051650285-0081', 'pitt:31735051650285-0082', 'pitt:31735051650285-0083', 'pitt:31735051650285-0084', 'pitt:31735051650285-0085', 'pitt:31735051650285-0086', 'pitt:31735051650285-0087', 'pitt:31735051650285-0088', 'pitt:31735051650285-0089', 'pitt:31735051650285-0090', 'pitt:31735051650285-0091', 'pitt:31735051650285-0092', 'pitt:31735051650285-0093', 'pitt:31735051650566-0066', 'pitt:31735051651929-0041', 'pitt:31735051653495-0009', 'pitt:31735051653495-0032', 'pitt:31735051653495-0041', 'pitt:31735051653495-0056', 'pitt:31735051653495-0062', 'pitt:31735051654345-0001', 'pitt:31735051654394-0055', 'pitt:31735051654394-0085', 'pitt:31735051654436-0037', 'pitt:31735051654477-0003', 'pitt:31735051654725-0007', 'pitt:31735051654725-0008', 'pitt:31735051654725-0127', 'pitt:31735051654725-0231', 'pitt:31735051654725-0244', 'pitt:31735051654725-0260', 'pitt:31735051654725-0268', 'pitt:31735051654766-0114', 'pitt:31735051654766-0117', 'pitt:31735051654881-0005', 'pitt:31735051655011-0005', 'pitt:31735051655052-0031', 'pitt:31735051655052-0068', 'pitt:31735051655052-0075', 'pitt:31735051655052-0115', 'pitt:31735051655094-0082', 'pitt:31735051655136-0094', 'pitt:31735051655136-0203', 'pitt:31735051655136-0216', 'pitt:31735051655177-0007', 'pitt:31735051656530-0001', 'pitt:31735051656639-0075', 'pitt:31735051656639-0078', 'pitt:31735051656670-0041', 'pitt:31735051657181-0002', 'pitt:31735051657694-0007', 'pitt:31735051657777-0124', 'pitt:31735051657777-0137', 'pitt:31735051657777-0212', 'pitt:31735051657777-0217', 'pitt:31735051657777-0356', 'pitt:31735051657801-0004', 'pitt:31735051657819-0093', 'pitt:31735051657819-0154', 'pitt:31735051657819-0232', 'pitt:31735051657868-0002', 'pitt:31735051658130-0036', 'pitt:31735051658130-0074', 'pitt:31735051658163-0076', 'pitt:31735051658171-0041', 'pitt:31735051658171-0110', 'pitt:31735051658171-0181', 'pitt:31735051658171-0220', 'pitt:31735051658171-0264', 'pitt:31735051658189-0013', 'pitt:31735051658205-0032', 'pitt:31735051658239-0002', 'pitt:31735051658411-0008', 'pitt:31735051658486-0010', 'pitt:31735051658502-0003', 'pitt:31735051658650-0011', 'pitt:31735051658809-0009', 'pitt:31735051658858-0138', 'pitt:31735051658858-0141', 'pitt:31735051658858-0142', 'pitt:31735051658932-0071', 'pitt:31735051658932-0124', 'pitt:31735051659013-0042', 'pitt:31735051659013-0103', 'pitt:31735051659013-0230', 'pitt:31735051659013-0258', 'pitt:31735051659096-0112', 'pitt:31735051659138-0121', 'pitt:31735051659187-0024', 'pitt:31735051659187-0051', 'pitt:31735051659302-0042', 'pitt:31735051659971-0003', 'pitt:31735053221911-0007', 'pitt:31735053221911-0009', 'pitt:31735053221911-0064', 'pitt:31735053221911-0065', 'pitt:31735053221911-0066', 'pitt:31735053221911-0067', 'pitt:31735053221911-0068', 'pitt:31735053221911-0069', 'pitt:31735053221911-0086', 'pitt:31735053221911-0087', 'pitt:31735053221911-0088', 'pitt:31735053221911-0089', 'pitt:31735053221911-0090', 'pitt:31735053221911-0091', 'pitt:31735053221911-0092', 'pitt:31735053221911-0093', 'pitt:31735053221911-0094', 'pitt:31735053221911-0095', 'pitt:31735053221911-0096', 'pitt:31735053221911-0097', 'pitt:31735053221911-0098', 'pitt:31735053221911-0099', 'pitt:31735054778596-0008', 'pitt:31735054778596-0009', 'pitt:31735054778596-0013', 'pitt:31735054778596-0014', 'pitt:31735054778596-0015', 'pitt:31735054778596-0016', 'pitt:31735054778596-0025', 'pitt:31735054778596-0078', 'pitt:31735054778596-0079', 'pitt:31735054778596-0080', 'pitt:31735054778596-0081', 'pitt:31735054778596-0082', 'pitt:31735054778596-0083', 'pitt:31735054778596-0084', 'pitt:31735054778596-0085', 'pitt:31735054778596-0086', 'pitt:31735054778596-0087', 'pitt:31735054778596-0088', 'pitt:31735054778596-0089', 'pitt:31735054778596-0121', 'pitt:31735054778596-0122', 'pitt:31735054778596-0123', 'pitt:31735054778596-0124', 'pitt:31735054778596-0125', 'pitt:31735054778596-0126', 'pitt:31735054778596-0127', 'pitt:31735054778596-0128', 'pitt:31735054778596-0129', 'pitt:31735054778596-0130', 'pitt:31735054778596-0131', 'pitt:31735054778596-0132', 'pitt:31735054778596-0133', 'pitt:31735054778596-0134', 'pitt:31735054778596-0135', 'pitt:31735054778596-0136', 'pitt:31735054778596-0137', 'pitt:31735054778596-0138', 'pitt:31735054778596-0139', 'pitt:31735054778596-0140', 'pitt:31735054778596-0141', 'pitt:31735054778596-0142', 'pitt:31735054778596-0143', 'pitt:31735054778596-0144', 'pitt:31735054778596-0145', 'pitt:31735054778596-0146', 'pitt:31735054778596-0147', 'pitt:31735054778596-0148', 'pitt:31735054778596-0149', 'pitt:31735054778596-0150', 'pitt:31735054778596-0151', 'pitt:31735054778596-0152', 'pitt:31735054778596-0153', 'pitt:31735054778596-0154', 'pitt:31735054778596-0155', 'pitt:31735054778596-0156', 'pitt:31735054778596-0157', 'pitt:31735054778596-0158', 'pitt:31735054778596-0159', 'pitt:31735054778596-0160', 'pitt:31735054778596-0161', 'pitt:31735054778596-0162', 'pitt:31735054778596-0163', 'pitt:31735054778596-0164', 'pitt:31735054778596-0244', 'pitt:31735054778596-0245', 'pitt:31735054778596-0246', 'pitt:31735054778596-0247', 'pitt:31735054778596-0248', 'pitt:31735054778596-0249', 'pitt:31735054853613-0056', 'pitt:31735054853613-0057', 'pitt:31735054853613-0065', 'pitt:31735054853613-0068', 'pitt:31735054853613-0556', 'pitt:31735054853613-0557', 'pitt:31735054853613-0558', 'pitt:31735054853613-0559', 'pitt:31735054853613-0560', 'pitt:31735054853613-0561', 'pitt:31735054853613-0562', 'pitt:31735054853613-0563', 'pitt:31735054853613-0564', 'pitt:31735054853613-0565', 'pitt:31735054853613-0566', 'pitt:31735054853613-0567', 'pitt:31735054853613-0568', 'pitt:31735054853613-0569', 'pitt:31735054853613-0570', 'pitt:31735054853613-0571', 'pitt:31735054853613-0642', 'pitt:31735054853613-0643', 'pitt:31735054853613-0644', 'pitt:31735054853613-0645', 'pitt:31735054853613-0646', 'pitt:31735054853613-0647', 'pitt:31735054853613-0648', 'pitt:31735054853613-0649', 'pitt:31735054853613-0650', 'pitt:31735054853613-0651', 'pitt:31735054853613-0652', 'pitt:31735054853613-0653', 'pitt:31735054853613-0654', 'pitt:31735054853613-0672', 'pitt:31735054853613-0673', 'pitt:31735054853613-0674', 'pitt:31735054853613-0675', 'pitt:31735054853613-0676', 'pitt:31735054853613-0677', 'pitt:31735054853613-0678', 'pitt:31735054853613-0679', 'pitt:31735054853613-0680', 'pitt:31735054853613-0681', 'pitt:31735054853613-0682', 
  'pitt:31735054853613-0683', 'pitt:31735054853613-0684', 'pitt:31735054853613-0685', 'pitt:31735054853613-0686', 'pitt:31735054853613-0687', 'pitt:31735054853613-0688', 'pitt:31735054854017-0004', 'pitt:31735054854017-0038', 'pitt:31735054854017-0039', 'pitt:31735054854017-0040', 'pitt:31735054854017-0041', 'pitt:31735054854017-0042', 'pitt:31735054854017-0043', 'pitt:31735054854017-0044', 'pitt:31735054854017-0045', 'pitt:31735054854017-0046', 'pitt:31735054854017-0048', 'pitt:31735054854017-0378', 'pitt:31735054854017-0379', 'pitt:31735054854017-0380', 'pitt:31735054854017-0381', 'pitt:31735054854017-0382', 'pitt:31735054854017-0383', 'pitt:31735054854017-0384', 'pitt:31735054854017-0385', 'pitt:31735054854017-0386', 'pitt:31735054854017-0387', 'pitt:31735054854017-0388', 'pitt:31735054854017-0389', 'pitt:31735054854017-0390', 'pitt:31735054854017-0391', 'pitt:31735054854017-0392', 'pitt:31735054854017-0393', 'pitt:31735054854017-0394', 'pitt:31735054854017-0395', 'pitt:31735054854017-0396', 'pitt:31735054854017-0397', 'pitt:31735054854017-0398', 'pitt:31735054854017-0399', 'pitt:31735054854017-0400', 'pitt:31735054854017-0401', 'pitt:31735054854017-0402', 'pitt:31735054854017-0403', 'pitt:31735054854017-0404', 'pitt:31735054854017-0405', 'pitt:31735054854017-0406', 'pitt:31735054854017-0407', 'pitt:31735054854017-0408', 'pitt:31735054854017-0409', 'pitt:31735054854017-0410', 'pitt:31735054854017-0411', 'pitt:31735054854017-0412', 'pitt:31735054854017-0413', 'pitt:31735054854017-0414', 'pitt:31735054854017-0415', 'pitt:31735054854017-0416', 'pitt:31735054854017-0417', 'pitt:31735054854017-0418', 'pitt:31735054854017-0419', 'pitt:31735054854017-0420', 'pitt:31735054854017-0421', 'pitt:31735054854017-0422', 'pitt:31735054854017-0423', 'pitt:31735054854017-0424', 'pitt:31735054854017-0425', 'pitt:31735054854017-0426', 'pitt:31735054854017-0427', 'pitt:31735054854017-0428', 'pitt:31735054854017-0429', 'pitt:31735054854017-0430', 'pitt:31735054854017-0431', 'pitt:31735054854017-0432', 'pitt:31735054854017-0433', 'pitt:31735054854017-0434', 'pitt:31735054854017-0435', 'pitt:31735054854017-0436', 'pitt:31735054854017-0437', 'pitt:31735054854017-0438', 'pitt:31735054854017-0439', 'pitt:31735054854017-0440', 'pitt:31735054854017-0441', 'pitt:31735054854017-0442', 'pitt:31735054854017-0443', 'pitt:31735054854017-0444', 'pitt:31735054854017-0445', 'pitt:31735054854017-0446', 'pitt:31735054854017-0447', 'pitt:31735054854017-0448', 'pitt:31735054854017-0449', 'pitt:31735054854017-0450', 'pitt:31735054854017-0451', 'pitt:31735054854017-0452', 'pitt:31735054854017-0453', 'pitt:31735054854017-0454', 'pitt:31735054854017-0455', 'pitt:31735054854017-0456', 'pitt:31735054854017-0457', 'pitt:31735054854017-0458', 'pitt:31735054854017-0459', 'pitt:31735054854017-0460', 'pitt:31735054854017-0470', 'pitt:31735054854017-0471', 'pitt:31735054854017-0472', 'pitt:31735054854017-0473', 'pitt:31735054854017-0474', 'pitt:31735054854017-0475', 'pitt:31735054854017-0476', 'pitt:31735054854017-0477', 'pitt:31735054854017-0478', 'pitt:31735054854017-0479', 'pitt:31735054854017-0480', 'pitt:31735054854017-0481', 'pitt:31735054854017-0482', 'pitt:31735054854017-0483', 'pitt:31735054854173-0003', 'pitt:31735054854173-0004', 'pitt:31735054854173-0005', 'pitt:31735054854173-0006', 'pitt:31735054854173-0007', 'pitt:31735054854173-0008', 'pitt:31735054854173-0011', 'pitt:31735054854173-0012', 'pitt:31735054854173-0028', 'pitt:31735054854173-0029', 'pitt:31735054854173-0030', 'pitt:31735054854173-0031', 'pitt:31735054854173-0032', 'pitt:31735054854173-0033', 'pitt:31735054854173-0034', 'pitt:31735054854173-0035', 'pitt:31735054854173-0036', 'pitt:31735054854173-0037', 'pitt:31735054854173-0038', 'pitt:31735054854173-0039', 'pitt:31735054854173-0040', 'pitt:31735054854173-0041', 'pitt:31735054854173-0042', 'pitt:31735054854173-0043', 'pitt:31735054854173-0044', 'pitt:31735054854173-0045', 'pitt:31735054854173-0046', 'pitt:31735054854173-0047', 'pitt:31735054854173-0048', 'pitt:31735054854173-0049', 'pitt:31735054854173-0050', 'pitt:31735054854173-0051', 'pitt:31735054854173-0052', 'pitt:31735054854173-0053', 'pitt:31735054854173-0054', 'pitt:31735054854173-0055', 'pitt:31735054854173-0056', 'pitt:31735054854173-0057', 'pitt:31735054854173-0058', 'pitt:31735054854173-0059', 'pitt:31735054854173-0060', 'pitt:31735054854173-0061', 'pitt:31735054854173-0062', 'pitt:31735054854173-0063', 'pitt:31735054854173-0064', 'pitt:31735054854173-0065', 'pitt:31735054854173-0066', 'pitt:31735054854173-0067', 'pitt:31735054854173-0068', 'pitt:31735054854173-0069', 'pitt:31735054854173-0070', 'pitt:31735054854173-0071', 'pitt:31735054854173-0072', 'pitt:31735054854173-0073', 'pitt:31735054854173-0074', 'pitt:31735054854173-0075', 'pitt:31735054854173-0076', 'pitt:31735054854173-0077', 'pitt:31735054854173-0078', 'pitt:31735054854173-0079', 'pitt:31735054854173-0080', 'pitt:31735054854173-0081', 'pitt:31735054854173-0082', 'pitt:31735054854173-0083', 'pitt:31735054854173-0084', 'pitt:31735054854173-0085', 'pitt:31735054854173-0086', 'pitt:31735054854173-0087', 'pitt:31735054854173-0106', 'pitt:31735054854173-0107', 'pitt:31735054854173-0108', 'pitt:31735054854173-0109', 'pitt:31735054854173-0110', 'pitt:31735054854173-0111', 'pitt:31735054854173-0112', 'pitt:31735054854173-0113', 'pitt:31735054854173-0114', 'pitt:31735054854173-0115', 'pitt:31735054854173-0116', 'pitt:31735054854173-0117', 'pitt:31735054854173-0118', 'pitt:31735054854173-0119', 'pitt:31735054854173-0120', 'pitt:31735054854173-0121', 'pitt:31735054854173-0273', 'pitt:31735054854173-0274', 'pitt:31735054854173-0275', 'pitt:31735054854173-0276', 'pitt:31735054854173-0277', 'pitt:31735054854173-0278', 'pitt:31735054854173-0279', 'pitt:31735054854173-0280', 'pitt:31735054854173-0281', 'pitt:31735054854173-0282', 'pitt:31735054854173-0283', 'pitt:31735054854173-0284', 'pitt:31735054854173-0285', 'pitt:31735054854173-0286', 'pitt:31735054854173-0287', 'pitt:31735054854173-0288', 'pitt:31735054854173-0289', 'pitt:31735054854173-0290', 'pitt:31735054854173-0291', 'pitt:31735054854173-0292', 'pitt:31735054854173-0293', 'pitt:31735054854173-0294', 'pitt:31735054854173-0295', 'pitt:31735054854603-0440', 'pitt:31735054855907-0593', 'pitt:31735054856426-0025', 'pitt:31735054856426-0026', 'pitt:31735054856426-0027', 'pitt:31735054856426-0028', 'pitt:31735054856426-0029', 'pitt:31735054856426-0192', 'pitt:31735054856426-0193', 'pitt:31735054856426-0241', 'pitt:31735054856426-0242', 'pitt:31735054856426-0243', 'pitt:31735054856426-0244', 'pitt:31735054856426-0245', 'pitt:31735054856426-0246', 'pitt:31735054856426-0247', 'pitt:31735054856426-0248', 'pitt:31735054856426-0249', 'pitt:31735054856426-0250', 'pitt:31735054856426-0251', 'pitt:31735054856426-0252', 'pitt:31735054856426-0253', 'pitt:31735054856426-0254', 'pitt:31735054856426-0255', 'pitt:31735054856426-0256', 'pitt:31735054856426-0257', 'pitt:31735054856426-0258', 'pitt:31735054856426-0259', 'pitt:31735054856426-0260', 'pitt:31735054856426-0261', 'pitt:31735054856426-0262', 'pitt:31735054856426-0263', 'pitt:31735054856426-0264', 'pitt:31735054856426-0265', 'pitt:31735054856426-0266', 'pitt:31735054856426-0267', 'pitt:31735054856426-0268', 'pitt:31735054856426-0269', 'pitt:31735054856426-0270', 'pitt:31735054856426-0271', 'pitt:31735054856426-0272', 'pitt:31735054856426-0273', 'pitt:31735054856426-0274', 'pitt:31735054856426-0275', 'pitt:31735054856426-0276', 'pitt:31735054856426-0277', 'pitt:31735054856426-0278', 'pitt:31735054856426-0279', 'pitt:31735054856426-0280', 'pitt:31735054856426-0281', 'pitt:31735054856426-0282', 'pitt:31735054856426-0283', 'pitt:31735054856426-0284', 'pitt:31735054856426-0285', 'pitt:31735054856426-0286', 'pitt:31735054856426-0287', 'pitt:31735054856426-0288', 'pitt:31735054856426-0289', 'pitt:31735054856426-0290', 'pitt:31735054856426-0291', 'pitt:31735054856426-0292', 'pitt:31735054858174-0546', 'pitt:31735054858596-0107', 'pitt:31735054859529-0022', 'pitt:31735054859529-0025', 'pitt:31735054859529-0026', 'pitt:31735054859529-0214', 'pitt:31735054859529-0215', 'pitt:31735054859529-0216', 'pitt:31735054859529-0217', 'pitt:31735054859529-0218', 'pitt:31735054859529-0219', 'pitt:31735054859529-0220', 'pitt:31735054859529-0221', 'pitt:31735054859529-0222', 'pitt:31735054859529-0223', 'pitt:31735054859529-0249', 'pitt:31735054859529-0250', 'pitt:31735054859529-0251', 'pitt:31735054859529-0252', 'pitt:31735054859529-0253', 'pitt:31735054859529-0254', 'pitt:31735054859529-0255', 'pitt:31735054859529-0256', 'pitt:31735054859529-0257', 'pitt:31735054859529-0258', 'pitt:31735054859529-0259', 'pitt:31735054859529-0260', 'pitt:31735054859529-0261', 'pitt:31735054859529-0262', 'pitt:31735054859529-0263', 'pitt:31735054859529-0264', 'pitt:31735054859529-0265', 'pitt:31735054860592-0034', 'pitt:31735055592285-0002', 'pitt:31735055592285-0003', 'pitt:31735055592285-0004', 'pitt:31735055592285-0005', 'pitt:31735055592285-0006', 'pitt:31735055592285-0013', 'pitt:31735055592285-0014', 'pitt:31735055592285-0015', 'pitt:31735055592285-0016', 'pitt:31735055592285-0017', 'pitt:31735055592285-0018', 'pitt:31735055592285-0019', 'pitt:31735055592285-0020', 'pitt:31735055592285-0021', 'pitt:31735055592285-0022', 'pitt:31735055592285-0023', 'pitt:31735055592285-0024', 'pitt:31735055592285-0025', 'pitt:31735055592285-0026', 'pitt:31735055592285-0027', 'pitt:31735055592285-0028', 'pitt:31735055592285-0029', 'pitt:31735055592285-0030', 'pitt:31735055592285-0031', 'pitt:31735055592285-0032', 'pitt:31735055592285-0033', 'pitt:31735055592285-0034', 'pitt:31735055592285-0035', 'pitt:31735055592285-0036', 'pitt:31735055592285-0037', 'pitt:31735055592285-0038', 'pitt:31735055592285-0039', 'pitt:31735055592285-0040', 'pitt:31735055592285-0041', 'pitt:31735055592285-0042', 'pitt:31735055592285-0043', 'pitt:31735055592285-0044', 'pitt:31735055592285-0045', 'pitt:31735055592285-0046', 'pitt:31735055592285-0047', 'pitt:31735055592285-0048', 'pitt:31735055592285-0049', 'pitt:31735055592285-0050', 'pitt:31735055592285-0051', 'pitt:31735055592285-0052', 'pitt:31735055592285-0053', 'pitt:31735055592285-0054', 'pitt:31735055592285-0055', 'pitt:31735055592285-0056', 'pitt:31735055592285-0057', 'pitt:31735055592285-0058', 'pitt:31735055592285-0059', 'pitt:31735055592285-0128', 'pitt:31735055592285-0129', 'pitt:31735055592285-0130', 'pitt:31735055592285-0131', 'pitt:31735055592285-0132', 
  'pitt:31735055592285-0133', 'pitt:31735055592285-0134', 'pitt:31735055592285-0135', 'pitt:31735055592285-0136', 'pitt:31735055592285-0137', 'pitt:31735055592285-0138', 'pitt:31735055592285-0139', 'pitt:31735055592285-0140', 'pitt:31735055592285-0141', 'pitt:31735055592285-0142', 'pitt:31735055592285-0143', 'pitt:31735055592285-0144', 'pitt:31735055592285-0145', 'pitt:31735055592285-0146', 'pitt:31735055592285-0147', 'pitt:31735055592285-0148', 'pitt:31735055592285-0149', 'pitt:31735055592285-0150', 'pitt:31735055592285-0151', 'pitt:31735055592285-0152', 'pitt:31735055592285-0153', 'pitt:31735055592285-0154', 'pitt:31735055592285-0155', 'pitt:31735055592285-0156', 'pitt:31735055592285-0157', 'pitt:31735055592285-0158', 'pitt:31735055592285-0159', 'pitt:31735055592285-0160', 'pitt:31735055592285-0161', 'pitt:31735055592285-0162', 'pitt:31735055592285-0163', 'pitt:31735055592285-0164', 'pitt:31735055592285-0165', 'pitt:31735055592285-0166', 'pitt:31735055592285-0167', 'pitt:31735055592285-0168', 'pitt:31735055592285-0169', 'pitt:31735055592285-0170', 'pitt:31735055592285-0171', 'pitt:31735055592285-0172', 'pitt:31735055592285-0173', 'pitt:31735055592285-0174', 'pitt:31735055592285-0175', 'pitt:31735055592285-0176', 'pitt:31735055592285-0177', 'pitt:31735055592285-0178', 'pitt:31735055592285-0179', 'pitt:31735055592285-0180', 'pitt:31735055592285-0181', 'pitt:31735055592285-0182', 'pitt:31735055592285-0183', 'pitt:31735055592285-0184', 'pitt:31735055592285-0185', 'pitt:31735055592285-0186', 'pitt:31735055592285-0187', 'pitt:31735055592285-0188', 'pitt:31735055592285-0189', 'pitt:31735055592285-0190', 'pitt:31735055592285-0191', 'pitt:31735055592285-0192', 'pitt:31735055592285-0193', 'pitt:31735055592285-0194', 'pitt:31735055592285-0195', 'pitt:31735055592285-0196', 'pitt:31735055592285-0197', 'pitt:31735055592285-0198', 'pitt:31735055592285-0199', 'pitt:31735055592285-0200', 'pitt:31735055592285-0201', 'pitt:31735055592285-0202', 'pitt:31735055592285-0203', 'pitt:31735055592285-0204', 'pitt:31735055592285-0205', 'pitt:31735055592285-0206', 'pitt:31735055592285-0207', 'pitt:31735055592285-0208', 'pitt:31735055592285-0209', 'pitt:31735055592285-0210', 'pitt:31735055592285-0211', 'pitt:31735055592285-0212', 'pitt:31735055592285-0213', 'pitt:31735055592285-0214', 'pitt:31735055592285-0215', 'pitt:31735055592285-0216', 'pitt:31735055592285-0217', 'pitt:31735055592285-0218', 'pitt:31735055592285-0219', 'pitt:31735055592285-0220', 'pitt:31735055592285-0221', 'pitt:31735055592285-0222', 'pitt:31735055592285-0223', 'pitt:31735055592285-0224', 'pitt:31735055592285-0225', 'pitt:31735055592285-0226', 'pitt:31735055592285-0227', 'pitt:31735055592285-0228', 'pitt:31735055592285-0229', 'pitt:31735055592285-0230', 'pitt:31735055592285-0231', 'pitt:31735055592285-0232', 'pitt:31735055592285-0233', 'pitt:31735055592285-0234', 'pitt:31735055592285-0235', 'pitt:31735055592285-0236', 'pitt:31735055592285-0237', 'pitt:31735055592285-0238', 'pitt:31735055592285-0239', 'pitt:31735055592285-0240', 'pitt:31735055592285-0241', 'pitt:31735055592285-0242', 'pitt:31735055592285-0243', 'pitt:31735055592285-0244', 'pitt:31735055723393-0006', 'pitt:31735055723393-0007', 'pitt:31735055723393-0058', 'pitt:31735055723393-0059', 'pitt:31735055723393-0060', 'pitt:31735055723393-0061', 'pitt:31735055723393-0062', 'pitt:31735055723393-0063', 'pitt:31735055723393-0064', 'pitt:31735055723393-0065', 'pitt:31735055723393-0066', 'pitt:31735055723393-0067', 'pitt:31735055723393-0068', 'pitt:31735055723393-0069', 'pitt:31735055723393-0070', 'pitt:31735055723393-0071', 'pitt:31735055723393-0102', 'pitt:31735055723393-0577', 'pitt:31735055723393-0578', 'pitt:31735055723393-0579', 'pitt:31735055723393-0580', 'pitt:31735055723393-0581', 'pitt:31735055723393-0582', 'pitt:31735055723393-0583', 'pitt:31735055723393-0584', 'pitt:31735055723393-0585', 'pitt:31735055723393-0586', 'pitt:31735055723393-0587', 'pitt:31735055723393-0588', 'pitt:31735055723393-0589', 'pitt:31735055723393-0590', 'pitt:31735055723393-0591', 'pitt:31735055723393-0592', 'pitt:31735055723393-0593', 'pitt:31735055723393-0594', 'pitt:31735055723393-0595', 'pitt:31735055723393-0596', 'pitt:31735055723393-0597', 'pitt:31735055723393-0598', 'pitt:31735055723393-0599', 'pitt:31735055723393-0600', 'pitt:31735055723393-0601', 'pitt:31735055723393-0602', 'pitt:31735055723393-0603', 'pitt:31735055723393-0604', 'pitt:31735055723393-0605', 'pitt:31735055723393-0606', 'pitt:31735055723393-0607', 'pitt:31735055723393-0608', 'pitt:31735055723393-0609', 'pitt:31735055723393-0610', 'pitt:31735055723393-0611', 'pitt:31735055723393-0612', 'pitt:31735055723393-0613', 'pitt:31735055723393-0614', 'pitt:31735055723393-0615', 'pitt:31735055723393-0616', 'pitt:31735055723393-0617', 'pitt:31735055723393-0618', 'pitt:31735055723393-0619', 'pitt:31735055723393-0620', 'pitt:31735055723393-0621', 'pitt:31735055723393-0622', 'pitt:31735055723393-0623', 'pitt:31735055723393-0624', 'pitt:31735055723393-0625', 'pitt:31735055723393-0626', 'pitt:31735055723393-0627', 'pitt:31735055723393-0628', 'pitt:31735055723393-0629', 'pitt:31735055723393-0630', 'pitt:31735055723393-0631', 'pitt:31735055723393-0632', 'pitt:31735055723393-0633', 'pitt:31735055723393-0634', 'pitt:31735055723393-0635', 'pitt:31735055723393-0636', 'pitt:31735055723393-0637', 'pitt:31735055723393-0638', 'pitt:31735055723393-0639', 'pitt:31735055723393-0640', 'pitt:31735055723393-0641', 'pitt:31735055723393-0642', 'pitt:31735055723393-0643', 'pitt:31735055723393-0644', 'pitt:31735055723393-0645', 'pitt:31735055723393-0646', 'pitt:31735055723393-0647', 'pitt:31735055723393-0648', 'pitt:31735055723393-0649', 'pitt:31735055723393-0650', 'pitt:31735055723393-0651', 'pitt:31735055723393-0652', 'pitt:31735055723393-0653', 'pitt:31735055723393-0654', 'pitt:31735055723393-0655', 'pitt:31735055723393-0656', 'pitt:31735055723393-0657', 'pitt:31735055723393-0658', 'pitt:31735055723393-0659', 'pitt:31735055723393-0660', 'pitt:31735055723393-0661', 'pitt:31735055723393-0662', 'pitt:31735055723393-0663', 'pitt:31735055723393-0664', 'pitt:31735055723393-0665', 'pitt:31735055723393-0666', 'pitt:31735055723393-0667', 'pitt:31735055723393-0668', 'pitt:31735055723393-0669', 'pitt:31735055723393-0670', 'pitt:31735055723393-0671', 'pitt:31735055723393-0672', 'pitt:31735055723393-0673', 'pitt:31735055723393-0674', 'pitt:31735055723393-0675', 'pitt:31735055723393-0676', 'pitt:31735055723393-0677', 'pitt:31735055723393-0678', 'pitt:31735055723393-0679', 'pitt:31735055723393-0680', 'pitt:31735055723393-0681', 'pitt:31735055723393-0682', 'pitt:31735055723393-0683', 'pitt:31735055723393-0684', 'pitt:31735055723393-0685', 'pitt:31735055723393-0686', 'pitt:31735055723393-0687', 'pitt:31735055723393-0688', 'pitt:31735055723393-0689', 'pitt:31735055723393-0690', 'pitt:31735055723393-0691', 'pitt:31735055723393-0692', 'pitt:31735055723393-0693', 'pitt:31735055723393-0694', 'pitt:31735055723393-0695', 'pitt:31735055723393-0696', 'pitt:31735055723393-0697', 'pitt:31735055723393-0698', 'pitt:31735055723393-0699', 'pitt:31735055723393-0700', 'pitt:31735055723393-0701', 'pitt:31735055723393-0702', 'pitt:31735055723393-0703', 'pitt:31735055723393-0704', 'pitt:31735055723393-0705', 'pitt:31735055723393-0706', 'pitt:31735055723393-0707', 'pitt:31735055723393-0708', 'pitt:31735055723393-0709', 'pitt:31735055723393-0710', 'pitt:31735055723393-0711', 'pitt:31735055723393-0712', 'pitt:31735055723393-0713', 'pitt:31735055723393-1013', 'pitt:31735055723393-1014', 'pitt:31735055723393-1015', 'pitt:31735055723393-1016', 'pitt:31735055723393-1017', 'pitt:31735055723393-1018', 'pitt:31735055723393-1019', 'pitt:31735055723393-1020', 'pitt:31735055723393-1021', 'pitt:31735055723393-1022', 'pitt:31735055723393-1023', 'pitt:31735055723393-1024', 'pitt:31735055723393-1025', 'pitt:31735055723393-1026', 'pitt:31735055723393-1027', 'pitt:31735056286689-0009', 'pitt:31735056286689-0082', 'pitt:31735056286689-0083', 'pitt:31735056286689-0084', 'pitt:31735056286689-0085', 'pitt:31735056286689-0086', 'pitt:31735056286689-0087', 'pitt:31735056286689-0088', 'pitt:31735056286689-0089', 'pitt:31735056286689-0090', 'pitt:31735056286689-0091', 'pitt:31735056286689-0092', 'pitt:31735056286689-0093', 'pitt:31735056286689-0094', 'pitt:31735056286689-0181', 'pitt:31735056286689-0182', 'pitt:31735056286689-0183', 'pitt:31735056286689-0184', 'pitt:31735056286689-0185', 'pitt:31735056286689-0186', 'pitt:31735056286689-0187', 'pitt:31735056286689-0188', 'pitt:31735056286689-0189', 'pitt:31735056286689-0191', 'pitt:31735056286689-0192', 'pitt:31735056286689-0818', 'pitt:31735056286689-0819', 'pitt:31735056286689-0820', 'pitt:31735056286689-0821', 'pitt:31735056286689-0822', 'pitt:31735056286689-0823', 'pitt:31735056286689-0824', 'pitt:31735056286689-0825', 'pitt:31735056286689-0826', 'pitt:31735056286689-0827', 'pitt:31735056286689-0828', 'pitt:31735056286689-0829', 'pitt:31735056286689-0830', 'pitt:31735056286689-0831', 'pitt:31735056286689-0832', 'pitt:31735056286689-0833', 'pitt:31735056286689-0834', 'pitt:31735056286689-0835', 'pitt:31735056286689-0836', 'pitt:31735056286689-0837', 'pitt:31735056286689-0838', 'pitt:31735056286689-0839', 'pitt:31735056286689-0840', 'pitt:31735056286689-0841', 'pitt:31735056286689-0842', 'pitt:31735056286689-0843', 'pitt:31735056286689-0844', 'pitt:31735056286689-0845', 'pitt:31735056286689-0846', 'pitt:31735056286689-0847', 'pitt:31735056286689-0848', 'pitt:31735056286689-0849', 'pitt:31735056286689-0850', 'pitt:31735056286689-0851', 'pitt:31735056286689-0852', 'pitt:31735056286689-0853', 'pitt:31735056286689-0854', 'pitt:31735056286689-0855', 'pitt:31735056286689-0856', 'pitt:31735056286689-0857', 'pitt:31735056286689-0858', 'pitt:31735056286689-0859', 'pitt:31735056286689-0860', 'pitt:31735056286689-0861', 'pitt:31735056286689-0862', 'pitt:31735056286689-0863', 'pitt:31735056286689-0864', 'pitt:31735056286689-0865', 'pitt:31735056286689-0866', 'pitt:31735056286689-0867', 'pitt:31735056286689-0868', 'pitt:31735056286689-0869', 'pitt:31735056286689-0870', 'pitt:31735056286689-0871', 'pitt:31735056286689-0872', 'pitt:31735056286689-0873', 'pitt:31735056286689-0874', 'pitt:31735056286689-0875', 'pitt:31735056286689-0876', 'pitt:31735056286689-0877', 'pitt:31735056286689-0878', 'pitt:31735056286689-0879', 'pitt:31735056286689-0880', 'pitt:31735056286689-0881', 'pitt:31735056286689-0882', 'pitt:31735056286689-0883', 
  'pitt:31735056286689-0884', 'pitt:31735056286689-0885', 'pitt:31735056286689-0886', 'pitt:31735056286689-0887', 'pitt:31735056286689-0888', 'pitt:31735056286689-0889', 'pitt:31735056286689-0890', 'pitt:31735056286689-0891', 'pitt:31735056286689-0892', 'pitt:31735056286689-0893', 'pitt:31735056286689-0894', 'pitt:31735056286689-0895', 'pitt:31735056286689-0896', 'pitt:31735056286689-0897', 'pitt:31735056286689-0898', 'pitt:31735056286689-0899', 'pitt:31735056286689-0900', 'pitt:31735056286689-0901', 'pitt:31735056286689-0902', 'pitt:31735056286689-0903', 'pitt:31735056286689-0904', 'pitt:31735056286689-0905', 'pitt:31735056286689-0906', 'pitt:31735056286689-0907', 'pitt:31735056286689-0908', 'pitt:31735056286689-0909', 'pitt:31735056286689-0910', 'pitt:31735056286689-0911', 'pitt:31735056286689-0912', 'pitt:31735056286689-0913', 'pitt:31735056286689-0914', 'pitt:31735056286689-0915', 'pitt:31735056286689-0916', 'pitt:31735056286689-0917', 'pitt:31735056286689-0918', 'pitt:31735056286689-0919', 'pitt:31735056286689-0920', 'pitt:31735056286689-0921', 'pitt:31735056286689-0922', 'pitt:31735056286689-0923', 'pitt:31735056286689-0924', 'pitt:31735056286689-0925', 'pitt:31735056286689-0926', 'pitt:31735056286689-0927', 'pitt:31735056286689-0928', 'pitt:31735056286689-0929', 'pitt:31735056286689-0930', 'pitt:31735056286689-0931', 'pitt:31735056286689-0932', 'pitt:31735056286689-0933', 'pitt:31735056286689-0934', 'pitt:31735056286689-0935', 'pitt:31735056286689-0936', 'pitt:31735056286689-0937', 'pitt:31735056286689-0938', 'pitt:31735056286689-0939', 'pitt:31735056286689-0940', 'pitt:31735056286689-0941', 'pitt:31735056286689-0942', 'pitt:31735056286689-0943', 'pitt:31735056286689-0944', 'pitt:31735056286689-0945', 'pitt:31735056286689-0946', 'pitt:31735056286689-0947', 'pitt:31735056286689-0948', 'pitt:31735056286689-0949', 'pitt:31735056286689-1807', 'pitt:31735056286689-1808', 'pitt:31735056286689-1809', 'pitt:31735056286689-1810', 'pitt:31735056286689-1811', 'pitt:31735056286689-1812', 'pitt:31735056286689-1813', 'pitt:31735056286689-1814', 'pitt:31735056286689-1815', 'pitt:31735056286689-1816', 'pitt:31735056286689-1817', 'pitt:31735056286689-1818', 'pitt:31735056286689-1819', 'pitt:31735056286689-1820', 'pitt:31735056286689-1821', 'pitt:31735056286689-1822', 'pitt:31735056286689-1823', 'pitt:31735056286689-1824', 'pitt:31735056286689-1825', 'pitt:31735056286689-1826', 'pitt:31735056286689-1827', 'pitt:31735056286689-1828', 'pitt:31735056286689-1829', 'pitt:31735056286689-1830', 'pitt:31735056286689-1831', 'pitt:31735056286689-1832', 'pitt:31735056286689-1833', 'pitt:31735056286689-1834', 'pitt:31735056286689-1835', 'pitt:31735056286689-1836', 'pitt:31735056286689-1837', 'pitt:31735056286689-1838', 'pitt:31735056286689-1839', 'pitt:31735056286689-1840', 'pitt:31735056286689-1841', 'pitt:31735056286689-1842', 'pitt:31735056286689-1843', 'pitt:31735056286689-1844', 'pitt:31735056286689-1845', 'pitt:31735056286689-1846', 'pitt:31735056286689-1847', 'pitt:31735056286689-1848', 'pitt:31735056286689-1849', 'pitt:31735056286689-1850', 'pitt:31735056286689-1851', 'pitt:31735056286689-1852', 'pitt:31735056286689-1853', 'pitt:31735056286689-1854', 'pitt:31735056286689-1855', 'pitt:31735056286689-1856', 'pitt:31735056286689-1857', 'pitt:31735056286689-1858', 'pitt:31735056286689-1859', 'pitt:31735056286689-1860', 'pitt:31735056286689-1861', 'pitt:31735056286689-1862', 'pitt:31735056286689-1863', 'pitt:31735056286689-1864', 'pitt:31735056286689-1865', 'pitt:31735056286689-1866', 'pitt:31735056286689-1867', 'pitt:31735056286689-1868', 'pitt:31735056286689-1869', 'pitt:31735056286689-1870', 'pitt:31735056286689-1871', 'pitt:31735056286689-1872', 'pitt:31735056286689-1878', 'pitt:31735056286689-1879', 'pitt:31735056286689-1880', 'pitt:31735056286689-1881', 'pitt:31735056286689-1882', 'pitt:31735056286689-1883', 'pitt:31735056286689-1884', 'pitt:31735056286689-1885', 'pitt:31735056286689-1886', 'pitt:31735056286689-1887', 'pitt:31735056286689-1888', 'pitt:31735056286689-1889', 'pitt:31735056286689-1907', 'pitt:31735056286689-1908', 'pitt:31735056286689-1909', 'pitt:31735056286689-1910', 'pitt:31735056286689-1911', 'pitt:31735056286689-1912', 'pitt:31735056286689-1913', 'pitt:31735056286689-1914', 'pitt:31735056286689-1915', 'pitt:31735056286689-1916', 'pitt:31735056286689-1917', 'pitt:31735056286689-1918', 'pitt:31735056286689-1919', 'pitt:31735056286689-1920', 'pitt:31735056286689-1921', 'pitt:31735056287075-0093', 'pitt:31735056287182-0030', 'pitt:31735056287182-0031', 'pitt:31735056287422-0325', 'pitt:31735056287422-0326', 'pitt:31735056290350-0176', 'pitt:31735057893202-0015', 'pitt:31735057893202-0018', 'pitt:31735057893202-0019', 'pitt:31735057893202-0025', 'pitt:31735057893202-0026', 'pitt:31735057893202-0027', 'pitt:31735057893202-0028', 'pitt:31735057893202-0141', 'pitt:31735057893202-0142', 'pitt:31735057893202-0143', 'pitt:31735057893202-0144', 'pitt:31735057893202-0145', 'pitt:31735057893202-0146', 'pitt:31735057893202-0147', 'pitt:31735057893202-0148', 'pitt:31735057893202-0149', 'pitt:31735057893202-0150', 'pitt:31735057893202-0151', 'pitt:31735057893202-0152', 'pitt:31735057893202-0153', 'pitt:31735057893202-0154', 'pitt:31735057893202-0155', 'pitt:31735057893202-0156', 'pitt:31735057893202-0157', 'pitt:31735057893202-0175', 'pitt:31735057893202-0176', 'pitt:31735057893202-0177', 'pitt:31735057893202-0178', 'pitt:31735057893202-0179', 'pitt:31735057893202-0180', 'pitt:31735057893202-0181', 'pitt:31735057893202-0182', 'pitt:31735057893202-0183', 'pitt:31735057893202-0184', 'pitt:31735057893202-0185', 'pitt:31735057893202-0186', 'pitt:31735057893202-0187', 'pitt:31735057893202-0188', 'pitt:31735057893202-0189', 'pitt:31735057893202-0190', 'pitt:31735057893202-0204', 'pitt:31735057893202-0205', 'pitt:31735057893202-0206', 'pitt:31735057893202-0207', 'pitt:31735057893202-0242', 'pitt:31735057893202-0243', 'pitt:31735057893202-0244', 'pitt:31735057893202-0245', 'pitt:31735057893202-0246', 'pitt:31735057893202-0247', 'pitt:31735057893202-0248', 'pitt:31735057893202-0249', 'pitt:31735057893202-0250', 'pitt:31735057893202-0251', 'pitt:31735057893202-0252', 'pitt:31735057893202-0253', 'pitt:31735057893202-0254', 'pitt:31735057893202-0255', 'pitt:31735057893202-0256', 'pitt:31735057893202-0257', 'pitt:31735057893202-0258', 'pitt:31735057893202-0259', 'pitt:31735057893202-0260', 'pitt:31735057893202-0261', 'pitt:31735057893202-0262', 'pitt:31735057893202-0263', 'pitt:31735057893202-0264', 'pitt:31735057893202-0265', 'pitt:31735057893202-0266', 'pitt:31735057893202-0267', 'pitt:31735057893202-0268', 'pitt:31735057893202-0269', 'pitt:31735057893202-0270', 'pitt:31735057893202-0271', 'pitt:31735057893202-0272', 'pitt:31735057893202-0273', 'pitt:31735057893202-0274', 'pitt:31735057893202-0275', 'pitt:31735057893202-0276', 'pitt:31735057893202-0277', 'pitt:31735057893202-0278', 'pitt:31735057893202-0279', 'pitt:31735057893202-0280', 'pitt:31735057893202-0281', 'pitt:31735057893202-0282', 'pitt:31735057893202-0283', 'pitt:31735057893202-0284', 'pitt:31735057893202-0285', 'pitt:31735057893202-0286', 'pitt:31735057893202-0287', 'pitt:31735057893202-0288', 'pitt:31735057893202-0289', 'pitt:31735057895074-0027', 'pitt:31735057895074-0267', 'pitt:31735057895074-0268', 'pitt:31735057895074-0269', 'pitt:31735057895074-0313', 'pitt:31735057895074-0314', 'pitt:31735057895074-0315', 'pitt:31735057895793-0034', 'pitt:31735057895793-0035', 'pitt:31735057895793-0337', 'pitt:31735057895793-0338', 'pitt:31735057895793-0339', 'pitt:31735057895793-0340', 'pitt:31735057895793-0341', 'pitt:31735057895793-0342', 'pitt:31735057895793-0343', 'pitt:31735057895793-0344', 'pitt:31735057895793-0345', 'pitt:31735057895793-0346', 'pitt:31735057895793-0347', 'pitt:31735057895793-0348', 'pitt:31735057895793-0349', 'pitt:31735057895793-0350', 'pitt:31735057895793-0351', 'pitt:31735057895793-0352', 'pitt:31735057895876-0014', 'pitt:31735057895876-0018', 'pitt:31735057895876-0019', 'pitt:31735057895876-0022', 'pitt:31735057895876-0023', 'pitt:31735057895876-0025', 'pitt:31735057895876-0026', 'pitt:31735057895876-0131', 'pitt:31735057895876-0132', 'pitt:31735057895876-0133', 'pitt:31735057895876-0134', 'pitt:31735057895876-0135', 'pitt:31735057895876-0136', 'pitt:31735057895876-0137', 'pitt:31735057895876-0138', 'pitt:31735057895876-0139', 'pitt:31735057895876-0140', 'pitt:31735057895876-0141', 'pitt:31735057895876-0142', 'pitt:31735057895876-0143', 'pitt:31735057895876-0144', 'pitt:31735057895876-0145', 'pitt:31735057895876-0146', 'pitt:31735057895876-0147', 'pitt:31735057895876-0148', 'pitt:31735057895876-0180', 'pitt:31735057895876-0181', 'pitt:31735057895876-0182', 'pitt:31735057895876-0183', 'pitt:31735057895876-0184', 'pitt:31735057895876-0185', 'pitt:31735057895876-0186', 'pitt:31735057895876-0187', 'pitt:31735057895876-0188', 'pitt:31735057895876-0189', 'pitt:31735057895876-0190', 'pitt:31735057895876-0191', 'pitt:31735057895876-0192', 'pitt:31735057895876-0193', 'pitt:31735057895876-0194', 'pitt:31735057895876-0195', 'pitt:31735057895876-0196', 'pitt:31735057895876-0212', 'pitt:31735057895876-0213', 'pitt:31735057895876-0214', 'pitt:31735057895876-0215', 'pitt:31735057895876-0216', 'pitt:31735057895876-0217', 'pitt:31735057895876-0218', 'pitt:31735057895876-0219', 'pitt:31735057895876-0220', 'pitt:31735057895876-0221', 'pitt:31735057895876-0222', 'pitt:31735057895876-0223', 'pitt:31735057895876-0224', 'pitt:31735057895876-0225', 'pitt:31735057895876-0226', 'pitt:31735057895876-0227', 'pitt:31735057895876-0228', 'pitt:31735057895876-0229', 'pitt:31735057895876-0230', 'pitt:31735057895876-0231', 'pitt:31735057895876-0232', 'pitt:31735057895876-0233', 'pitt:31735057895876-0234', 'pitt:31735057895876-0235', 'pitt:31735057895876-0247', 'pitt:31735057895876-0248', 'pitt:31735057895876-0249', 'pitt:31735057895876-0250', 'pitt:31735057895876-0251', 'pitt:31735057895876-0252', 'pitt:31735057895876-0253', 'pitt:31735057895876-0254', 'pitt:31735057895876-0255', 'pitt:31735057895876-0256', 'pitt:31735057895876-0257', 'pitt:31735057895876-0258', 'pitt:31735057895876-0259', 'pitt:31735057895876-0260', 'pitt:31735057895876-0261', 'pitt:31735057895876-0262', 'pitt:31735057895876-0263', 'pitt:31735057895876-0264', 'pitt:31735057896460-0002', 'pitt:31735057896460-0004', 'pitt:31735057896460-0005', 'pitt:31735057896460-0007', 'pitt:31735057896460-0008', 'pitt:31735057896460-0010', 
  'pitt:31735057896460-0011', 'pitt:31735057896460-0012', 'pitt:31735057896460-0013', 'pitt:31735057896460-0014', 'pitt:31735057896460-0015', 'pitt:31735057896460-0016', 'pitt:31735057896460-0017', 'pitt:31735057896460-0018', 'pitt:31735057896460-0019', 'pitt:31735057896460-0020', 'pitt:31735057896460-0021', 'pitt:31735057896460-0022', 'pitt:31735057896460-0023', 'pitt:31735057896460-0024', 'pitt:31735057896460-0025', 'pitt:31735057896460-0034', 'pitt:31735057896460-0035', 'pitt:31735057896460-0036', 'pitt:31735057896460-0037', 'pitt:31735057896460-0038', 'pitt:31735057896460-0039', 'pitt:31735057896460-0040', 'pitt:31735057896460-0041', 'pitt:31735057896460-0042', 'pitt:31735057896460-0043', 'pitt:31735057896460-0044', 'pitt:31735057896460-0045', 'pitt:31735057896460-0046', 'pitt:31735057896460-0047', 'pitt:31735057896460-0048', 'pitt:31735057896460-0049', 'pitt:31735057896460-0050', 'pitt:31735057896460-0068', 'pitt:31735057896460-0069', 'pitt:31735057896460-0070', 'pitt:31735057896460-0071', 'pitt:31735057896460-0072', 'pitt:31735057896460-0073', 'pitt:31735057896460-0074', 'pitt:31735057896460-0075', 'pitt:31735057896460-0076', 'pitt:31735057896460-0077', 'pitt:31735057896460-0078', 'pitt:31735057896460-0079', 'pitt:31735057896460-0080', 'pitt:31735057896460-0081', 'pitt:31735057896460-0082', 'pitt:31735057896460-0083', 'pitt:31735057896460-0084', 'pitt:31735057896460-0100', 'pitt:31735057896460-0101', 'pitt:31735057896460-0102', 'pitt:31735057896460-0103', 'pitt:31735057896460-0104', 'pitt:31735057896460-0105', 'pitt:31735057896460-0106', 'pitt:31735057896460-0107', 'pitt:31735057896460-0108', 'pitt:31735057896460-0109', 'pitt:31735057896460-0110', 'pitt:31735057896460-0111', 'pitt:31735057896460-0112', 'pitt:31735057896460-0113', 'pitt:31735057896460-0114', 'pitt:31735057896460-0115', 'pitt:31735057896460-0116', 'pitt:31735057896460-0117', 'pitt:31735057896460-0118', 'pitt:31735057896460-0119', 'pitt:31735057896460-0120', 'pitt:31735057896460-0121', 'pitt:31735057896460-0122', 'pitt:31735057896460-0123', 'pitt:31735057896460-0124', 'pitt:31735057896460-0125', 'pitt:31735057896460-0126', 'pitt:31735057896460-0127', 'pitt:31735057896460-0128', 'pitt:31735057896460-0129', 'pitt:31735057896460-0130', 'pitt:31735057896460-0131', 'pitt:31735057896460-0132', 'pitt:31735057896460-0133', 'pitt:31735057896460-0134', 'pitt:31735057896460-0135', 'pitt:31735057896460-0136', 'pitt:31735057896460-0137', 'pitt:31735057896460-0138', 'pitt:31735057896460-0139', 'pitt:31735057896460-0140', 'pitt:31735057896460-0141', 'pitt:31735057896460-0142', 'pitt:31735057896460-0143', 'pitt:31735057896460-0144', 'pitt:31735057896460-0145', 'pitt:31735057896460-0146', 'pitt:31735057896460-0147', 'pitt:31735057896460-0148', 'pitt:31735057896460-0149', 'pitt:31735057896460-0150', 'pitt:31735057896460-0151', 'pitt:31735057896460-0152', 'pitt:31735057896460-0153', 'pitt:31735057896460-0154', 'pitt:31735057896460-0155', 'pitt:31735057896460-0156', 'pitt:31735057896460-0157', 'pitt:31735057896460-0158', 'pitt:31735057896460-0159', 'pitt:31735057896460-0160', 'pitt:31735057896460-0161', 'pitt:31735057896460-0162', 'pitt:31735057896460-0163', 'pitt:31735057896460-0164', 'pitt:31735057896460-0165', 'pitt:31735057896460-0166', 'pitt:31735057896460-0167', 'pitt:31735057896460-0168', 'pitt:31735057896460-0169', 'pitt:31735057896460-0170', 'pitt:31735057896460-0171', 'pitt:31735057896460-0172', 'pitt:31735057896460-0173', 'pitt:31735057896460-0174', 'pitt:31735057896460-0175', 'pitt:31735057896460-0176', 'pitt:31735057896460-0177', 'pitt:31735057896460-0178', 'pitt:31735057896460-0179', 'pitt:31735057896460-0180', 'pitt:31735057896460-0181', 'pitt:31735057896460-0182', 'pitt:31735057896460-0183', 'pitt:31735057896460-0184', 'pitt:31735057896460-0185', 'pitt:31735057896460-0186', 'pitt:31735057896460-0187', 'pitt:31735057896460-0188', 'pitt:31735057896460-0189', 'pitt:31735057896460-0190', 'pitt:31735057896460-0191', 'pitt:31735057896460-0192', 'pitt:31735057896460-0193', 'pitt:31735057896460-0194', 'pitt:31735057896460-0195', 'pitt:31735057896460-0196', 'pitt:31735057896460-0197', 'pitt:31735057896460-0198', 'pitt:31735057896460-0199', 'pitt:31735057896460-0200', 'pitt:31735057896460-0201', 'pitt:31735057896460-0202', 'pitt:31735057896460-0203', 'pitt:31735057896460-0204', 'pitt:31735057896460-0205', 'pitt:31735057896460-0206', 'pitt:31735057896460-0207', 'pitt:31735057896460-0208', 'pitt:31735057896460-0209', 'pitt:31735057896460-0210', 'pitt:31735057896460-0211', 'pitt:31735057896460-0212', 'pitt:31735057896460-0213', 'pitt:31735057896460-0214', 'pitt:31735057896460-0215', 'pitt:31735057896460-0216', 'pitt:31735057896460-0217', 'pitt:31735057896460-0218', 'pitt:31735057896460-0219', 'pitt:31735057896460-0220', 'pitt:31735057896460-0221', 'pitt:31735057896460-0222', 'pitt:31735057896460-0223', 'pitt:31735057896460-0224', 'pitt:31735057896460-0225', 'pitt:31735057896460-0226', 'pitt:31735057896460-0227', 'pitt:31735057896460-0228', 'pitt:31735057896460-0229', 'pitt:31735057896460-0230', 'pitt:31735057896460-0231', 'pitt:31735057896460-0232', 'pitt:31735057896460-0233', 'pitt:31735057896460-0234', 'pitt:31735057896460-0235', 'pitt:31735057896460-0236', 'pitt:31735057896460-0237', 'pitt:31735057896460-0238', 'pitt:31735057896460-0239', 'pitt:31735057896460-0240', 'pitt:31735057896460-0241', 'pitt:31735057896460-0242', 'pitt:31735057896460-0243', 'pitt:31735057896460-0244', 'pitt:31735057896460-0245', 'pitt:31735057896460-0246', 'pitt:31735057896460-0247', 'pitt:31735057896460-0248', 'pitt:31735057896460-0249', 'pitt:31735057896460-0250', 'pitt:31735057896460-0251', 'pitt:31735057896460-0252', 'pitt:31735057896460-0253', 'pitt:31735057896460-0254', 'pitt:31735057896460-0255', 'pitt:31735057896460-0256', 'pitt:31735057896460-0257', 'pitt:31735057896460-0258', 'pitt:31735057896460-0259', 'pitt:31735058194055-0002', 'pitt:31735058194063-0007', 'pitt:31735059397905-0003', 'pitt:31735059398036-0028', 'pitt:31735060225871-0015', 'pitt:31735060225871-0046', 'pitt:31735060226168-0001', 'pitt:31735060226598-0001', 'pitt:31735060227570-0002', 'pitt:31735060246893-0004', 'pitt:31735060247412-0001', 'pitt:31735060247834-0003', 'pitt:31735060248121-0004', 'pitt:31735060249053-0002', 'pitt:31735060249137-0001', 'pitt:31735060249897-0002', 'pitt:31735060250101-0022', 'pitt:31735060250101-0029', 'pitt:31735060250283-0001', 'pitt:31735060250564-0008', 'pitt:31735060251257-0001', 'pitt:31735060251877-0003', 'pitt:31735060251877-0007', 'pitt:31735060474362-0001', 'pitt:31735060474388-0061', 'pitt:31735060474420-0034', 'pitt:31735060474842-0037', 'pitt:31735060474917-0002', 'pitt:31735060474925-0012', 'pitt:31735060474974-0011', 'pitt:31735060475104-0003', 'pitt:31735060475310-0008', 'pitt:31735060475310-0014', 'pitt:31735060475336-0024', 'pitt:31735060475468-0137', 'pitt:31735060475468-0194', 'pitt:31735060475468-0211', 'pitt:31735060475468-0230', 'pitt:31735060475468-0261', 'pitt:31735060475617-0050', 'pitt:31735060475617-0056', 'pitt:31735060476011-0002', 'pitt:31735060476698-0017', 'pitt:31735060476995-0004', 'pitt:31735060477043-0009', 'pitt:31735060477191-0013', 'pitt:31735060477282-0074', 'pitt:31735060477282-0076', 'pitt:31735060477332-0003', 'pitt:31735060477415-0009', 'pitt:31735060477415-0036', 'pitt:31735060477803-0009', 'pitt:31735060478058-0008', 'pitt:31735060478694-0003', 'pitt:31735060479130-0024', 'pitt:31735060479239-0008', 'pitt:31735060480146-0011', 'pitt:31735060480245-0014', 'pitt:31735060480435-0001', 'pitt:31735060480583-0027', 'pitt:31735060480583-0089', 'pitt:31735060480583-0100', 'pitt:31735060480583-0149', 'pitt:31735060480617-0042', 'pitt:31735060480617-0149', 'pitt:31735060480658-0082', 'pitt:31735060480658-0123', 'pitt:31735060480732-0030', 'pitt:31735060480773-0105', 'pitt:31735060480773-0180', 'pitt:31735060480773-0210', 'pitt:31735060480971-0010', 'pitt:31735060481011-0003', 'pitt:31735060481052-0065', 'pitt:31735060481102-0056', 'pitt:31735060481102-0145', 'pitt:31735060481102-0150', 'pitt:31735060481144-0007', 'pitt:31735060481144-0069', 'pitt:31735060481144-0102', 'pitt:31735060481177-0112', 'pitt:31735060481177-0136', 'pitt:31735060481250-0039', 'pitt:31735060481250-0150', 'pitt:31735060481268-0031', 'pitt:31735060481300-0056', 'pitt:31735060481300-0097', 'pitt:31735060481300-0143', 'pitt:31735060481300-0239', 'pitt:31735060481375-0117', 'pitt:31735060481375-0149', 'pitt:31735060481375-0154', 'pitt:31735060481375-0184', 'pitt:31735060481417-0024', 'pitt:31735060481417-0137', 'pitt:31735060481417-0169', 'pitt:31735060481698-0002', 'pitt:31735060482027-0036', 'pitt:31735060482027-0051', 'pitt:31735060482027-0065', 'pitt:31735060482027-0091', 'pitt:31735060482134-0189', 'pitt:31735060482134-0191', 'pitt:31735060482134-0328', 'pitt:31735060482134-0386', 'pitt:31735060482134-0450', 'pitt:31735060482134-0490', 'pitt:31735060482134-0592', 'pitt:31735060482688-0015', 'pitt:31735060483686-0017', 'pitt:31735060483686-0037', 'pitt:31735060483769-0007', 'pitt:31735060483777-0013', 'pitt:31735060483777-0053', 'pitt:31735060483777-0062', 'pitt:31735060483777-0109', 'pitt:31735060483777-0120', 'pitt:31735060483835-0026', 'pitt:31735060483835-0037', 'pitt:31735060484064-0006', 'pitt:31735060484064-0009', 'pitt:31735060484106-0005', 'pitt:31735060484148-0015', 'pitt:31735060484155-0041', 'pitt:31735060484155-0058', 'pitt:31735060484163-0014', 'pitt:31735060484189-0013', 'pitt:31735060484205-0082', 'pitt:31735060484205-0167', 'pitt:31735060484221-0056', 'pitt:31735060484254-0081', 'pitt:31735060484692-0016', 'pitt:31735060484742-0001', 'pitt:31735060484791-0001', 'pitt:31735060485194-0007', 'pitt:31735060485236-0046', 'pitt:31735060485236-0050', 'pitt:31735060485236-0080', 'pitt:31735060485236-0104', 'pitt:31735060485582-0123', 'pitt:31735060485806-0015', 'pitt:31735060485806-0075', 'pitt:31735060485897-0037', 'pitt:31735060485897-0043', 'pitt:31735060485913-0008', 'pitt:31735060485947-0005', 'pitt:31735060486689-0002', 'pitt:31735060487018-0002', 'pitt:31735060487018-0035', 'pitt:31735060487117-0012', 'pitt:31735060487216-0001', 'pitt:31735060487216-0050', 'pitt:31735060487216-0081', 'pitt:31735060487463-0025', 'pitt:31735060487505-0005', 'pitt:31735060487539-0006', 'pitt:31735060487596-0060', 'pitt:31735060487596-0082', 'pitt:31735060487646-0015', 'pitt:31735060487653-0008', 'pitt:31735060487703-0004', 'pitt:31735060487703-0019', 'pitt:31735060487901-0018', 
  'pitt:31735060488040-0003', 'pitt:31735060488040-0004', 'pitt:31735060488115-0005', 'pitt:31735060490665-0309', 'pitt:31735061276188-0001', 'pitt:31735061277749-0005', 'pitt:31735061278077-0005', 'pitt:31735061278077-0029', 'pitt:31735061278424-0080', 'pitt:31735061278424-0104', 'pitt:31735061278507-0015', 'pitt:31735061537258-0059', 'pitt:31735061537258-0067', 'pitt:31735061538066-0002', 'pitt:31735061539437-0028', 'pitt:31735061539825-0003', 'pitt:31735061540351-0019', 'pitt:31735061540807-0007', 'pitt:31735061541037-0003', 'pitt:31735061541227-0005', 'pitt:31735061541227-0009', 'pitt:31735061541227-0022', 'pitt:31735061541599-0060', 'pitt:31735061541607-0017', 'pitt:31735061541615-0024', 'pitt:31735061545020-0001', 'pitt:31735061545046-0002', 'pitt:31735061546200-0004', 'pitt:31735061546408-0002', 'pitt:31735061546796-0001', 'pitt:31735061547422-0046', 'pitt:31735061547422-0092', 'pitt:31735061547711-0001', 'pitt:31735061548412-0003', 'pitt:31735061549014-0003', 'pitt:31735061549071-0001', 'pitt:31735061564971-0002', 'pitt:31735061565267-0001', 'pitt:31735061565424-0002', 'pitt:31735061565754-0001', 'pitt:31735061565754-0005', 'pitt:31735061568980-0006', 'pitt:31735061570432-0001', 'pitt:31735061571257-0002', 'pitt:31735061571935-0003', 'pitt:31735061573832-0001', 'pitt:31735061575381-0007', 'pitt:31735061575688-0006', 'pitt:31735061576694-0002', 'pitt:31735061578690-0001', 'pitt:31735061586701-0017', 'pitt:31735061586743-0001', 'pitt:31735061586750-0006', 'pitt:31735061586784-0049', 'pitt:31735061587071-0051', 'pitt:31735061587071-0179', 'pitt:31735061587071-0193', 'pitt:31735061587089-0030', 'pitt:31735061587089-0058', 'pitt:31735061587089-0190', 'pitt:31735061587089-0319', 'pitt:31735061655787-0007', 'pitt:31735061655787-0026', 'pitt:31735061655787-0063', 'pitt:31735061656892-0024', 'pitt:31735061657742-0084', 'pitt:31735061657890-0028', 'pitt:31735061658377-0009', 'pitt:31735061659169-0032', 'pitt:31735061659359-0002', 'pitt:31735061659359-0006', 'pitt:31735061659359-0037', 'pitt:31735061659359-0060', 'pitt:31735061659755-0041', 'pitt:31735061812859-0008', 'pitt:31735061813600-0115', 'pitt:31735061813600-0151', 'pitt:31735061813600-0230', 'pitt:31735061815720-0049', 'pitt:31735061815951-0040', 'pitt:31735061816819-0028', 'pitt:31735061816819-0071', 'pitt:31735061816850-0128', 'pitt:31735061816884-0200', 'pitt:31735061816884-0270', 'pitt:31735061816884-0294', 'pitt:31735061817643-0013', 'pitt:31735061817817-0040', 'pitt:31735061817841-0134', 'pitt:31735061817858-0033', 'pitt:31735061817858-0058', 'pitt:31735061817858-0081', 'pitt:31735061817866-0088', 'pitt:31735061817866-0105', 'pitt:31735061817999-0070', 'pitt:31735061818013-0006', 'pitt:31735061818013-0053', 'pitt:31735061818013-0081', 'pitt:31735061818013-0129', 'pitt:31735061818047-0117', 'pitt:31735061818054-0058', 'pitt:31735061818062-0052', 'pitt:31735061818062-0124', 'pitt:31735061818062-0130', 'pitt:31735061818096-0039', 'pitt:31735061821603-0009', 'pitt:31735061822635-0003', 'pitt:31735061822684-0031', 'pitt:31735061832857-0002', 'pitt:31735061834044-0005', 'pitt:31735061835546-0003', 'pitt:31735061845172-0018', 'pitt:31735061845198-0001', 'pitt:31735061845198-0026', 'pitt:31735061845198-0053', 'pitt:31735061845198-0054', 'pitt:31735061845214-0013', 'pitt:31735061845446-0028', 'pitt:31735061845446-0052', 'pitt:31735061845529-0063', 'pitt:31735061845586-0029', 'pitt:31735061846816-0008', 'pitt:31735061847368-0015', 'pitt:31735061848275-0020', 'pitt:31735061848861-0007', 'pitt:31735061849117-0007', 'pitt:31735062135276-0037', 'pitt:31735062135276-0182', 'pitt:31735062214238-0145', 'pitt:31735062214337-0006', 'pitt:31735062214758-0003', 'pitt:31735062214758-0006', 'pitt:31735062214758-0031', 'pitt:31735062214808-0038', 'pitt:31735062214857-0027', 'pitt:31735062215169-0002', 'pitt:31735062215342-0015', 'pitt:31735062215375-0025', 'pitt:31735062215466-0015', 'pitt:31735062215714-0023', 'pitt:31735062215714-0028', 'pitt:31735062215979-0009', 'pitt:31735062216118-0002', 'pitt:31735062216142-0004', 'pitt:31735062216191-0011', 'pitt:31735062216365-0004', 'pitt:31735062216423-0004', 'pitt:31735062216548-0005', 'pitt:31735062216548-0006', 'pitt:31735062216712-0001', 'pitt:31735062216712-0158', 'pitt:31735062216712-0229', 'pitt:31735062216712-0254', 'pitt:31735062216738-0013', 'pitt:31735062216746-0140', 'pitt:31735062216811-0036', 'pitt:31735062217520-0039', 'pitt:31735062217520-0056', 'pitt:31735062217603-0042', 'pitt:31735062217603-0088', 'pitt:31735062217629-0039', 'pitt:31735062217629-0063', 'pitt:31735062217751-0001', 'pitt:31735062217751-0045', 'pitt:31735062217751-0117', 'pitt:31735062217835-0021', 'pitt:31735062218148-0024', 'pitt:31735062218148-0049', 'pitt:31735062218247-0031', 'pitt:31735062218650-0178', 'pitt:31735062218718-0014', 'pitt:31735062218718-0052', 'pitt:31735062218718-0113', 'pitt:31735062218718-0134', 'pitt:31735062218726-0007', 'pitt:31735062218726-0063', 'pitt:31735062218726-0074', 'pitt:31735062218726-0087', 'pitt:31735062218726-0094', 'pitt:31735062218726-0127', 'pitt:31735062218726-0140', 'pitt:31735062218726-0172', 'pitt:31735062218726-0178', 'pitt:31735062218858-0032', 'pitt:31735062218858-0060', 'pitt:31735062218866-0021', 'pitt:31735062218866-0046', 'pitt:31735062218874-0031', 'pitt:31735062218874-0044', 'pitt:31735062218874-0165', 'pitt:31735062219039-0026', 'pitt:31735062219054-0025', 'pitt:31735062219146-0002', 'pitt:31735062219146-0015', 'pitt:31735062219286-0031', 'pitt:31735062219344-0050', 'pitt:31735062219534-0018', 'pitt:31735062219542-0004', 'pitt:31735062219591-0018', 'pitt:31735062219732-0057', 'pitt:31735062219831-0012', 'pitt:31735062220037-0013', 'pitt:31735062220300-0005', 'pitt:31735062220342-0005', 'pitt:31735062220441-0003', 'pitt:31735062220524-0013', 'pitt:31735062220870-0069', 'pitt:31735062220870-0105', 'pitt:31735062220870-0120', 'pitt:31735062220870-0142', 'pitt:31735062220870-0169', 'pitt:31735062220870-0222', 'pitt:31735062220870-0226', 'pitt:31735062220870-0239', 'pitt:31735062221027-0034', 'pitt:31735062221050-0019', 'pitt:31735062221068-0013', 'pitt:31735062221084-0002', 'pitt:31735062221084-0016', 'pitt:31735062221191-0070', 'pitt:31735062221308-0022', 'pitt:31735062221373-0010', 'pitt:31735062221373-0041', 'pitt:31735062221449-0023', 'pitt:31735062221449-0038', 'pitt:31735062221498-0033', 'pitt:31735062221597-0008', 'pitt:31735062221670-0010', 'pitt:31735062221753-0006', 'pitt:31735062221779', 'pitt:31735062221803-0017', 'pitt:31735062221829-0008', 'pitt:31735062221845-0005', 'pitt:31735062221878-0004', 'pitt:31735062221936-0030', 'pitt:31735062222017-0035', 'pitt:31735062222025-0005', 'pitt:31735062222041-0024', 'pitt:31735062222074-0041', 'pitt:31735062222223-0027', 'pitt:31735062222223-0082', 'pitt:31735062222280-0038', 'pitt:31735062222397-0020', 'pitt:31735062222413-0037', 'pitt:31735062222496-0106', 'pitt:31735062222645-0016', 'pitt:31735062222660-0025', 'pitt:31735062222702-0005', 'pitt:31735062222751-0011', 'pitt:31735062222827-0009', 'pitt:31735062222868-0014', 'pitt:31735062222975-0002', 'pitt:31735062223056-0003', 'pitt:31735062223239-0002', 'pitt:31735062223239-0003', 'pitt:31735062223239-0014', 'pitt:31735062223254-0015', 'pitt:31735062223304-0028', 'pitt:31735062223304-0102', 'pitt:31735062223387-0007', 'pitt:31735062223429-0010', 'pitt:31735062223494-0008', 'pitt:31735062223635-0067', 'pitt:31735062223635-0070', 'pitt:31735062223635-0073', 'pitt:31735062223635-0101', 'pitt:31735062223999-0019', 'pitt:31735062224112-0018', 'pitt:31735062224179-0006', 'pitt:31735062224179-0025', 'pitt:31735062224179-0039', 'pitt:31735062224195-0019', 'pitt:31735062224195-0033', 'pitt:31735062224229-0088', 'pitt:31735062224302-0035', 'pitt:31735062224401-0011', 'pitt:31735062224526-0002', 'pitt:31735062224609-0048', 'pitt:31735062224609-0073', 'pitt:31735062224674-0037', 'pitt:31735062224674-0044', 'pitt:31735062224799-0008', 'pitt:31735062224799-0032', 'pitt:31735062224849-0011', 'pitt:31735062224963-0070', 'pitt:31735062224971-0003', 'pitt:31735062225093-0001', 'pitt:31735062225093-0043', 'pitt:31735062225440-0048', 'pitt:31735062225465-0001', 'pitt:31735062225473-0007', 'pitt:31735062225499-0045', 'pitt:31735062225499-0060', 'pitt:31735062225507-0133', 'pitt:31735062225507-0208', 'pitt:31735062225507-0209', 'pitt:31735062225507-0220', 'pitt:31735062225614-0062', 'pitt:31735062225648-0011', 'pitt:31735062225648-0026', 'pitt:31735062225655-0024', 'pitt:31735062225663-0139', 'pitt:31735062227156-0014', 'pitt:31735064527918-0023', 'pitt:31735064527942-0002', 'pitt:31735064527942-0011', 'pitt:31735064527967-0001', 'pitt:31735064527975-0003', 'pitt:31735064528163-0042', 'pitt:31735064528189-0009', 'pitt:31735064528189-0060', 'pitt:31735064528189-0063', 'pitt:31735064528221-0004', 'pitt:31735064528254-0030', 'pitt:31735064528254-0031', 'pitt:31735064528270-0006', 'pitt:31735064528288-0012', 'pitt:31735064528304-0092', 'pitt:31735064528304-0109', 'pitt:31735064528338-0002', 'pitt:31735064528346-0031', 'pitt:31735064528361-0127', 'pitt:31735064528437-0008', 'pitt:31735064528437-0039', 'pitt:31735064528452-0105', 'pitt:31735064528460-0009', 'pitt:31735064528486-0016', 'pitt:31735064528494-0001', 'pitt:31735064528502-0011', 'pitt:31735064528510-0084', 'pitt:31735064528536-0082', 'pitt:31735064528536-0083', 'pitt:31735064528536-0084', 'pitt:31735064528536-0085', 'pitt:31735064528536-0086', 'pitt:31735064528536-0087', 'pitt:31735064528536-0088', 'pitt:31735064528536-0089', 'pitt:31735064528536-0090', 'pitt:31735064528536-0091', 'pitt:31735064528536-0092', 'pitt:31735064528536-0093', 'pitt:31735064528536-0107', 'pitt:31735064528536-0108', 'pitt:31735064528536-0109', 'pitt:31735064528536-0110', 'pitt:31735064528536-0111', 'pitt:31735064528536-0112', 'pitt:31735064528536-0113', 'pitt:31735064528536-0114', 'pitt:31735064528536-0115', 'pitt:31735064528536-0116', 'pitt:31735064528536-0117', 'pitt:31735064528536-0118', 'pitt:31735064528536-0119', 'pitt:31735064528536-0120', 'pitt:31735064528536-0121', 'pitt:31735064528536-0122', 'pitt:31735064528536-0123', 'pitt:31735064528536-0124', 'pitt:31735064528536-0125', 'pitt:31735064528536-0126', 'pitt:31735064528536-0127', 'pitt:31735064528536-0128', 'pitt:31735064528536-0129', 'pitt:31735064528536-0130', 'pitt:31735064528536-0131', 'pitt:31735064528536-0132', 'pitt:31735064528536-0133', 'pitt:31735064528536-0134', 'pitt:31735064528536-0135', 'pitt:31735064528536-0136', 
  'pitt:31735064528536-0137', 'pitt:31735064528536-0138', 'pitt:31735064528536-0139', 'pitt:31735064528536-0140', 'pitt:31735064528536-0141', 'pitt:31735064528536-0142', 'pitt:31735064528536-0143', 'pitt:31735064528536-0144', 'pitt:31735064528536-0145', 'pitt:31735064528536-0146', 'pitt:31735064528536-0147', 'pitt:31735064528536-0148', 'pitt:31735064528536-0149', 'pitt:31735064528536-0150', 'pitt:31735064528536-0151', 'pitt:31735064528536-0152', 'pitt:31735064528536-0153', 'pitt:31735064528536-0154', 'pitt:31735064528536-0155', 'pitt:31735064528536-0156', 'pitt:31735064528536-0157', 'pitt:31735064528536-0161', 'pitt:31735064528536-0162', 'pitt:31735064528536-0163', 'pitt:31735064528536-0164', 'pitt:31735064528536-0165', 'pitt:31735064528536-0166', 'pitt:31735064528536-0167', 'pitt:31735064528536-0168', 'pitt:31735064528536-0169', 'pitt:31735064528536-0170', 'pitt:31735064528536-0171', 'pitt:31735064528536-0172', 'pitt:31735064528536-0173', 'pitt:31735064528536-0174', 'pitt:31735064528536-0175', 'pitt:31735064528536-0176', 'pitt:31735064528536-0177', 'pitt:31735064528536-0178', 'pitt:31735064528536-0179', 'pitt:31735064528536-0180', 'pitt:31735064528536-0181', 'pitt:31735064528536-0182', 'pitt:31735064528536-0183', 'pitt:31735064528536-0184', 'pitt:31735064528536-0185', 'pitt:31735064528536-0186', 'pitt:31735064528536-0187', 'pitt:31735064528536-0188', 'pitt:31735064528536-0189', 'pitt:31735064528536-0190', 'pitt:31735064528536-0191', 'pitt:31735064528536-0192', 'pitt:31735064528536-0193', 'pitt:31735064528536-0194', 'pitt:31735064528536-0195', 'pitt:31735064528536-0196', 'pitt:31735064528544-0048', 'pitt:31735064528627-0001', 'pitt:31735064529930-0188', 'pitt:31735064529930-0189', 'pitt:31735064529930-0190', 'pitt:31735064529930-0191', 'pitt:31735064661329-0001', 'pitt:31735064661436-0001', 'pitt:31735064662095-0003', 'pitt:31735064662210', 'pitt:31735064662392-0003', 'pitt:31735065150504-0017', 'pitt:31735065150520-0046', 'pitt:31735065150538-0051', 'pitt:31735065150538-0052', 'pitt:31735065150538-0066', 'pitt:31735065150579-0068', 'pitt:31735065790895-0004', 'pitt:31735066058177-0019', 'pitt:31735066058177-0136', 'pitt:31735066058433-0008', 'pitt:31735066058433-0029', 'pitt:31735066059019-0021', 'pitt:31735066204128-0001', 'pitt:31735066205216-0040', 'pitt:31735066205216-0320', 'pitt:31735066205216-0373', 'pitt:31735066205224-0095', 'pitt:31735066205224-0226', 'pitt:31735066205224-0298', 'pitt:31735066205224-0350', 'pitt:31735066205265-0062', 'pitt:31735066205265-0127', 'pitt:31735066205265-0170', 'pitt:31735066205265-0245', 'pitt:31735066205265-0284', 'pitt:31735066205299-0003', 'pitt:31735066205299-0040', 'pitt:31735066205976-0003', 'pitt:31735066206032-0019', 'pitt:31735066206032-0022', 'pitt:31735066206032-0031', 'pitt:31735066206032-0035', 'pitt:31735066223722-0002', 'pitt:31735066224050-0020', 'pitt:31735066224217-0076', 'pitt:31735066224217-0110', 'pitt:31735066224217-0173', 'pitt:31735066224316-0001', 'pitt:31735066224357-0003', 'pitt:31735066224563-0001', 'pitt:31735066224886-0004', 'pitt:31735066225131-0001', 'pitt:31735066225636-0001', 'pitt:31735066225776-0004', 'pitt:31735066225826-0002', 'pitt:31735066225834-0002', 'pitt:31735066225859-0009', 'pitt:31735066226014-0007', 'pitt:31735066226089-0005', 'pitt:31735066226550-0001', 'pitt:31735066226626-0024', 'pitt:31735066226857-0004', 'pitt:31735066227301-0166', 'pitt:31735066227301-0170', 'pitt:31735066227301-0177', 'pitt:31735066227327-0070', 'pitt:31735066227327-0112', 'pitt:31735066227343-0013', 'pitt:31735066227343-0104', 'pitt:31735066227368-0126', 'pitt:31735066227780-0042', 'pitt:31735066228481-0061', 'pitt:31735066229109-0011', 'pitt:31735066229133-0010', 'pitt:31735066229372-0004', 'pitt:31735066229653-0060', 'pitt:31735066229653-0121', 'pitt:31735066229653-0176', 'pitt:31735066229653-0230', 'pitt:31735066229653-0332', 'pitt:31735066229653-0333', 'pitt:31735066229653-0337', 'pitt:31735066229653-0434', 'pitt:31735066232814-0051', 'pitt:31735066258355-0050', 'pitt:31735066258371-0001', 'pitt:31735066258371-0009', 'pitt:31735066258371-0027', 'pitt:31735066258405-0009', 'pitt:31735066258421-0007', 'pitt:31735066258447-0030', 'pitt:31735066261433-0017', 'pitt:31735066261433-0028', 'pitt:31735066261441', 'pitt:31735066261441-0002', 'pitt:31735066261441-0072', 'pitt:31735066261441-0083', 'pitt:31735066261441-0084', 'pitt:31735066261441-0088', 'pitt:31735066261441-0089', 'pitt:31735066261466-0027', 'pitt:31735066261466-0031', 'pitt:31735066261466-0040', 'pitt:31735066261466-0044', 'pitt:31735066261466-0046', 'pitt:31735066261474-0002', 'pitt:31735066261482-0004', 'pitt:31735066261490', 'pitt:31735066261490-0003', 'pitt:31735066261490-0005', 'pitt:31735066261490-0007', 'pitt:31735066261516-0001', 'pitt:31735066261516-0003', 'pitt:31735066261516-0008', 'pitt:31735066261516-0009', 'pitt:31735066261524-0001', 'pitt:31735066261524-0003', 'pitt:31735066261524-0007', 'pitt:31735066261524-0008', 'pitt:31735066261524-0009', 'pitt:31735066261524-0013', 'pitt:31735066261524-0014', 'pitt:31735066261524-0021', 'pitt:31735066261524-0022', 'pitt:31735066261524-0024', 'pitt:31735066261524-0027', 'pitt:31735066261524-0030', 'pitt:31735066261524-0031', 'pitt:31735066261524-0033', 'pitt:31735066261524-0039', 'pitt:31735066261524-0044', 'pitt:31735066261524-0045', 'pitt:31735066261524-0050', 'pitt:31735066261524-0052', 'pitt:31735066261524-0053', 'pitt:31735066261524-0055', 'pitt:31735066261524-0061', 'pitt:31735066261524-0063', 'pitt:31735066261524-0076', 'pitt:31735066261524-0078', 'pitt:31735066261524-0088', 'pitt:31735066261524-0092', 'pitt:31735066261524-0096', 'pitt:31735066261524-0111', 'pitt:31735066261524-0112', 'pitt:31735066261524-0114', 'pitt:31735066261524-0115', 'pitt:31735066261524-0117', 'pitt:31735066261524-0118', 'pitt:31735066261524-0120', 'pitt:31735066261524-0123', 'pitt:31735066261524-0124', 'pitt:31735066261524-0125', 'pitt:31735066261524-0126', 'pitt:31735066261524-0127', 'pitt:31735066261524-0131', 'pitt:31735066261524-0133', 'pitt:31735066261524-0136', 'pitt:31735066261524-0139', 'pitt:31735066261524-0140', 'pitt:31735066261524-0143', 'pitt:31735066261524-0145', 'pitt:31735066261524-0146', 'pitt:31735066261524-0148', 'pitt:31735066261524-0149', 'pitt:31735066261524-0153', 'pitt:31735066261524-0154', 'pitt:31735066261524-0155', 'pitt:31735066261524-0156', 'pitt:31735066261524-0159', 'pitt:31735066261524-0160', 'pitt:31735066261524-0161', 'pitt:31735066261524-0164', 'pitt:31735066261524-0165', 'pitt:31735066261524-0167', 'pitt:31735066261524-0169', 'pitt:31735066261524-0170', 'pitt:31735066261532', 'pitt:31735066261532-0001', 'pitt:31735066261532-0002', 'pitt:31735066261532-0003', 'pitt:31735066261532-0004', 'pitt:31735066261532-0006', 'pitt:31735066261532-0009', 'pitt:31735066261532-0011', 'pitt:31735066261532-0013', 'pitt:31735066261532-0014', 'pitt:31735066261532-0015', 'pitt:31735066261532-0016', 'pitt:31735066261532-0017', 'pitt:31735066261532-0018', 'pitt:31735066261532-0021', 'pitt:31735066261532-0022', 'pitt:31735066261532-0023', 'pitt:31735066261532-0024', 'pitt:31735066261532-0026', 'pitt:31735066261532-0027', 'pitt:31735066261532-0028', 'pitt:31735066261532-0029', 'pitt:31735066261532-0030', 'pitt:31735066261532-0031', 'pitt:31735066261532-0032', 'pitt:31735066261532-0033', 'pitt:31735066261532-0035', 'pitt:31735066261532-0036', 'pitt:31735066261532-0037', 'pitt:31735066261532-0038', 'pitt:31735066261532-0039', 'pitt:31735066261532-0040', 'pitt:31735066261532-0041', 'pitt:31735066261532-0042', 'pitt:31735066261532-0045', 'pitt:31735066261532-0046', 'pitt:31735066261532-0047', 'pitt:31735066261532-0050', 'pitt:31735066261532-0051', 'pitt:31735066261532-0052', 'pitt:31735066261532-0053', 'pitt:31735066261532-0054', 'pitt:31735066261532-0055', 'pitt:31735066261532-0056', 'pitt:31735066261532-0057', 'pitt:31735066261532-0058', 'pitt:31735066261532-0061', 'pitt:31735066261532-0062', 'pitt:31735066261532-0063', 'pitt:31735066261532-0064', 'pitt:31735066261532-0065', 'pitt:31735066261532-0068', 'pitt:31735066261532-0069', 'pitt:31735066261532-0071', 'pitt:31735066261532-0072', 'pitt:31735066261532-0073', 'pitt:31735066261532-0075', 'pitt:31735066261532-0078', 'pitt:31735066261532-0079', 'pitt:31735066261532-0080', 'pitt:31735066261532-0081', 'pitt:31735066261532-0082', 'pitt:31735066261532-0083', 'pitt:31735066261532-0084', 'pitt:31735066261532-0085', 'pitt:31735066261540-0001', 'pitt:31735066261540-0002', 'pitt:31735066261557', 'pitt:31735066261557-0001', 'pitt:31735066261557-0002', 'pitt:31735066261557-0003', 'pitt:31735066261557-0006', 'pitt:31735066261557-0007', 'pitt:31735066261557-0008', 'pitt:31735066261557-0010', 'pitt:31735066261565', 'pitt:31735066261565-0001', 'pitt:31735066261565-0002', 'pitt:31735066261573', 'pitt:31735066261573-0001', 'pitt:31735066261573-0002', 'pitt:31735066261581', 'pitt:31735066261581-0001', 'pitt:31735066261581-0002', 'pitt:31735066262514-0002', 'pitt:31735066263082-0002', 'pitt:31735066264445-0203', 'pitt:31735066264445-0297', 'pitt:31735066264643-0043', 'pitt:31735066264643-0061', 'pitt:31735066264643-0064', 'pitt:31735066264684-0022', 'pitt:31735066264684-0092', 'pitt:31735066264684-0107', 'pitt:31735066264908-0044', 'pitt:31735066264940-0073', 'pitt:31735066264940-0125', 'pitt:31735066264957-0065', 'pitt:31735066264973-0020', 'pitt:31735066264981-0102', 'pitt:31735066264981-0112', 'pitt:31735066265004-0026', 'pitt:31735066265004-0092', 'pitt:31735066265582-0016', 'pitt:31735066265608-0010', 'pitt:31735066265780-0105', 'pitt:31735066265830-0095', 'pitt:31735066265830-0109', 'pitt:31735066265830-0112', 'pitt:31735066266135-0020', 'pitt:31735066266135-0161', 'pitt:31735066266135-0170', 'pitt:31735066874532-0011', 'pitt:31735066875091-0016', 'pitt:516', 'pitt:524', 'pitt:53', 'pitt:606', 'pitt:619', 'pitt:676', 'pitt:69', 'pitt:715.144019.CP', 'pitt:715.275490.CP', 'pitt:738', 'pitt:778', 'pitt:895', 'pitt:92', 'pitt:941', 'pitt:FTBL03.UA', 'pitt:MSP285.B015.F06.I01', 'pitt:MSP285.B015.F06.I02', 'pitt:MSP285.B015.F06.I03', 'pitt:MSP285.B015.F06.I04', 'pitt:MSP285.B015.F06.I05', 'pitt:MSP285.B015.F10.I01', 'pitt:MSP285.B015.F11.I01', 'pitt:PSS027_B001_F001_I05', 'pitt:PSS027_B001_F002_I01', 'pitt:PSS027_B001_F002_I02', 'pitt:PSS027_B001_F002_I03', 'pitt:PSS027_B001_F002_I04', 'pitt:PSS027_B001_F003_I01', 'pitt:PSS027_B001_F003_I02', 'pitt:PSS027_B001_F003_I03', 'pitt:PSS027_B001_F003_I04', 'pitt:PSS027_B001_F003_I05', 
  'pitt:PSS027_B001_F003_I06', 'pitt:PSS027_B001_F003_I07', 'pitt:PSS027_B001_F003_I08', 'pitt:PSS027_B001_F004_I01', 'pitt:PSS027_B001_F004_I02', 'pitt:PSS027_B001_F004_I03', 'pitt:PSS027_B001_F004_I04', 'pitt:PSS027_B001_F004_I05', 'pitt:PSS027_B001_F004_I06', 'pitt:PSS027_B001_F004_I07', 'pitt:PSS027_B001_F005_I01', 'pitt:PSS027_B001_F005_I02', 'pitt:PSS027_B001_F005_I03', 'pitt:PSS027_B001_F005_I04', 'pitt:PSS027_B001_F006_I01', 'pitt:PSS027_B001_F006_I02', 'pitt:PSS027_B001_F006_I03', 'pitt:PSS027_B001_F007_I01', 'pitt:PSS027_B001_F007_I02', 'pitt:PSS027_B001_F008_I01', 'pitt:PSS027_B001_F008_I02', 'pitt:PSS027_B001_F008_I03', 'pitt:PSS027_B001_F008_I04', 'pitt:PSS027_B001_F009_I01', 'pitt:PSS027_B001_F009_I02', 'pitt:PSS027_B001_F009_I03', 'pitt:PSS027_B001_F009_I04', 'pitt:PSS027_B001_F009_I05', 'pitt:PSS027_B001_F009_I06', 'pitt:PSS027_B001_F010_I01', 'pitt:PSS027_B001_F010_I02', 'pitt:PSS027_B001_F010_I03', 'pitt:PSS027_B001_F010_I04', 'pitt:PSS027_B001_F018_I01', 'pitt:PSS027_B001_F018_I02', 'pitt:PSS027_B001_F018_I03', 'pitt:PSS027_B001_F018_I04', 'pitt:PSS027_B001_F019_I01', 'pitt:PSS027_B001_F019_I02', 'pitt:PSS027_B001_F019_I03', 'pitt:PSS027_B001_F019_I04', 'pitt:PSS027_B001_F019_I05', 'pitt:PSS027_B001_F020_I01', 'pitt:PSS027_B001_F020_I02', 'pitt:PSS027_B001_F020_I03', 'pitt:PSS027_B001_F024_I04', 'pitt:PSS027_B001_F024_I05', 'pitt:PSS027_B001_F025_I01', 'pitt:PSS027_B001_F025_I02', 'pitt:PSS027_B001_F025_I03', 'pitt:PSS027_B001_F025_I04', 'pitt:PSS027_B001_F025_I05', 'pitt:PSS027_B001_F026_I01', 'pitt:PSS027_B001_F026_I02', 'pitt:PSS027_B001_F026_I03', 'pitt:PSS027_B001_F026_I04', 'pitt:PSS027_B001_F027_I01', 'pitt:PSS027_B001_F027_I02', 'pitt:PSS027_B001_F027_I03', 'pitt:PSS027_B001_F027_I04', 'pitt:PSS027_B001_F027_I05', 'pitt:PSS027_B001_F028_I01', 'pitt:PSS027_B001_F028_I02', 'pitt:PSS027_B001_F028_I03', 'pitt:PSS027_B001_F028_I04', 'pitt:PSS027_B001_F028_I05', 'pitt:PSS027_B001_F028_I06', 'pitt:PSS027_B001_F028_I07', 'pitt:PSS027_B001_F028_I08', 'pitt:PSS027_B001_F029_I01', 'pitt:PSS027_B001_F029_I02', 'pitt:PSS027_B001_F029_I03', 'pitt:PSS027_B001_F029_I04', 'pitt:PSS027_B001_F029_I05', 'pitt:PSS027_B001_F030_I01', 'pitt:PSS027_B001_F030_I02', 'pitt:PSS027_B001_F030_I03', 'pitt:PSS027_B001_F030_I04', 'pitt:PSS6.B007.I04', 'pitt:PSS6.B007.I07', 'pitt:PSS_028_B001_F017_I004', 'pitt:PSS_028_B001_F017_I005', 'pitt:PSS_028_B001_F017_I006', 'pitt:PSS_028_B001_F017_I007', 'pitt:PSS_028_B001_F018_I001', 'pitt:PSS_028_B001_F018_I002', 'pitt:PSS_028_B001_F018_I003', 'pitt:PSS_028_B001_F018_I004', 'pitt:PSS_028_B001_F018_I005', 'pitt:PSS_028_B001_F018_I006', 'pitt:PSS_028_B001_F018_I007', 'pitt:PSS_028_B001_F018_I008', 'pitt:US-PPiU-ais196443', 'pitt:US-QQS-MSS42', 'pitt:ais9830.11.02.1719-0004', 'pitt:collection.11', 'pitt:collection.136', 'pitt:collection.297', 'pitt:collection.hidden.workaround', 'pitt:e39398v22n04-0036', 'pitt:fa.digital', 'pitt:fa.documenting', 'pitt:fa.heinz', 'pitt:fa.histpitt', 'pitt:finding-aids', 'pitt:pittpressreleases19540085-0001', 'pitt:pittpressreleases19560245-0001', 'pitt:pittpressreleases19640306-0001', 'pitt:pittpressreleases19640306-0002', 'pitt:pittpressreleases19730017-0001', 'pitt:pittpressreleases19790363-0001', 'pitt:pittpressreleases19790363-0002', 'pitt:pittpressreleases19790363-0003', 'pitt:pittpressreleases19790363-0004', 'pitt:pittpressreleases19790363-0005', 'pitt:pittpressreleases19820230-0001', 'pitt:pittpressreleases19820230-0002', 'pitt:pittpressreleases19880204-0001', 'pitt:pittpressreleases19880204-0002', 'pitt:pittpressreleases19920290-0001', 'pitt:pittpressreleases19930005-0002');
}
