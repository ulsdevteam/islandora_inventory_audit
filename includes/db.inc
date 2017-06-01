<?php

define('BIGFOOT_UPDATE_CHUNK_SIZE', 1000);
define('UPITT_ISLANDORA_INVENTORY_SOLR_LIMIT', 2000000);
define('UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE', 50000);

/**
 * @file
 * Database functions for the Islandora Inventory Audit module.
 */

function upitt_islandora_inventory_audit_html_escape_string($html_escape) {
  $html_escape =  htmlspecialchars($html_escape, ENT_QUOTES | ENT_HTML5, 'UTF-8');
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

  $result = db_truncate('{upitt_islandora_audit_items_parents_islandora}')->execute();

  $result = db_truncate('{upitt_islandora_audit_items_islandora}')->execute();

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
function upitt_islandora_inventory_pagecounts() {
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

// Step 2 - clear the table for the new results.
  $truncate_result = mysqli_query($link, "TRUNCATE TABLE `upitt_islandora_audit_page_counts`");

  // Step 3 - populate the page_count table with the results of $booki_sql from Step 1.
  $result_i = mysqli_query($link, $booki_sql);
  if (!$result_i) {
    upitt_workflow_sql_error_die($link, $booki_sql);
  }
  dpm($booki_sql);
  while ($row = mysqli_fetch_assoc($result_i)) {
    $insert_sql = "INSERT INTO `upitt_islandora_audit_page_counts` (`barcode`, `islandora_page_count`) " .
                  "VALUES ('" . upitt_islandora_inventory_audit_html_escape_string($row['barcode']) . "', " .
                  $row['islandora_page_count'] . ")";
    $result = mysqli_query($link, $insert_sql);
  }

  // Step 4 - update the page_count table with the results of $bookb_sql from Step 1.
  $result_b = mysqli_query($link, $bookb_sql);
  if (!$result_b) {
    upitt_workflow_sql_error_die($link, $bookb_sql);
  }
  dpm($bookb_sql);
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
  $id_inserted = db_insert('upitt_islandora_audit_items_' . $table_suffix)
    ->fields($name_value_pairs)
    ->execute();
  return $id_inserted;
}

function upitt_islandora_inventory_audit_insert_islandora_parents($pid, $parents) {
  foreach ($parents as $parent) {
    if (is_object(islandora_object_load($parent))) {
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

      if (strstr($file, 'ulsmanuscripts/31735037971722') <> '') {
        $path_parts = explode("/", $path);
        $barcode = (count($path_parts) > 0) ? $path_parts[count($path_parts) - 1] : '';
        $pseudo_barcode = $barcode . '-' . $fname_only;

        echo "extension : [" . $path_info['extension'] . "]<br>" . 
             "file = " . $file . "<br>" .
             $fname_only . "<br>" .
             $barcode . "<br>" .
             $pseudo_barcode . "<hr>";
      }

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
        $pseudo_barcode = $barcode . '-' . $fname_only;

        // shift off the last element (the extension) -- and reimplode to get just the filename portion
        // Look at the filename -- if it looks like it is a page object (####.tif), then add that reference as
        // a barcode value like 31234567809-0041.
        $upcased_fname = strtoupper($fname_only);
        if (array_search($upcased_fname, $ignore_these_upcased_filenames) === FALSE) {
          if ((strstr($file, 'ulsmanuscripts/31735037971722') <> '') && ($path_info['extension'] == 'tif')) {
            echo "<b>ADDED</b><br>";
          }
          upitt_islandora_inventory_audit_insert('bigfoot', array('bigfoot_path' => $path, 'barcode' => $pseudo_barcode));
        }
      }
    }
    fclose($handle);
  } else {
    drupal_set_message('There was a problem opening the file "' . $files_list_path . '".  No inventory could be determined from the bigfoot location.', 'error');
  }
  // drupal_set_message('exiting upitt_islandora_inventory_audit_populate_bigfoot_path');
  die('done');
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

  $ret_arr = array();
  // run two queries --
  // 1) to get the datastreams available on this object
  $query_processor->solrQuery = 'PID:pitt\:*';
  $query_processor->solrStart = 0;
  $query_processor->solrLimit = 0;
  $query_processor->solrParams = array('fl' => 'PID');

  $url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
  $solr->setCreateDocuments(FALSE);
  try {
    $search_results = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams, 'GET');
    $tmp = json_decode($search_results->getRawResponse(), TRUE);

    $results = array();
    $numFound = $tmp['response']['numFound'];
    // repeatedly call this to process this many records each time:
    //   UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE
    $offset = 0;
    while ($offset < $numFound) {
      upitt_islandora_inventory_audit_populate_achunkof_islandora_pid($offset);
      $offset = $offset + UPITT_ISLANDORA_INVENTORY_SOLR_CHUNKSIZE;
    }
  }
  catch (Exception $e) {
    error_log('EXCEPTION in _save_solr_search_session : called from ' . $_SERVER['SERVER_NAME'] .
' - ' . $_SERVER['REQUEST_URI'] . '
' . print_r($e, true));
  }

/* CAN DO THIS ALL AT ONCE AFTER THE TWO TABLES ARE CALCULATED - since this is
 * more than 1.5 million records, the JOIN ~ UPDATE statement will run out of memory
 * making a temporary index... so a script was used to update to set the depositor
 * value for the page objects.
 *
 * set_time_limit(0);
 * while ($processed = do_some()) {
 *   echo ".";
 * }
 * function do_some() {
 *   $result = db_query("SELECT ai.depositor, ai.islandora_pid, p.parent_islandora_pid as parent_pid, ai2.depositor `set` " .
 *     "FROM upitt_islandora_audit_items_islandora ai " .
 *     "JOIN upitt_islandora_audit_items_parents_islandora p ON (p.islandora_pid = ai.islandora_pid) " .
 *     "JOIN upitt_islandora_audit_items_islandora ai2 ON (ai2.islandora_pid = p.parent_islandora_pid) " .
 *     "WHERE ai.depositor = '' AND ai.fedora_model IN ('pageCModel', 'newspaperPageCModel', 'manuscriptPageCModel') " .
 *     " AND ai2.depositor <> '' LIMIT 50000");
 *   $result ->execute();
 *   $processed = 0;
 *   while($record = $result->fetchAssoc()) {
 *     $r = db_query("UPDATE upitt_islandora_audit_items_islandora SET depositor = '" . addslashes($record['set']) . "' WHERE islandora_pid = '" . $record['islandora_pid'] . "'")->execute();
 *     $processed++;
 *   }
 *   return $processed;
 * }
 *
 */

  drupal_set_message('exiting upitt_islandora_inventory_audit_populate_islandora_pid');
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
  $query_processor->solrParams = array('fl' => 'PID');

  $url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
  $solr->setCreateDocuments(FALSE);
  try {
    $search_results = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams, 'GET');
    $tmp = json_decode($search_results->getRawResponse(), TRUE);

    $results = array();
    $numFound = $tmp['response']['numFound'];
    if ($tmp['response']['numFound'] > 0) {
      foreach ($tmp['response']['docs'] as $k=>$rec) {
        $pid = $rec['PID'];
        $datastream_sizes = upitt_workflow_get_depositor_model_and_datastreams_sizes($pid);
        $datastream_size_sums = upitt_workflow_sum_datastream_sizes($datastream_sizes);
        $barcode = substr($pid, strpos($pid, ':') + 1);
        $obj = islandora_object_load($pid);
        $parents = upitt_islandora_inventory_get_parent_values($obj);
        upitt_islandora_inventory_audit_insert('islandora', array('islandora_pid' => $pid, 
            'barcode' => $barcode,
            'datastreams_latestsize' => $datastream_size_sums['datastreams_latestsize'],
            'datastreams_totalsize' => $datastream_size_sums['datastreams_totalsize'],
            'fedora_model' => str_replace('info:fedora/islandora:', '', $datastream_sizes['RELS_EXT_hasModel_uri_s']),
            'depositor' => isset($datastream_sizes['mods_name_depositor_namePart_s']) ? $datastream_sizes['mods_name_depositor_namePart_s'] : '',
            ));
        // add records for the object to parent relationhips
        upitt_islandora_inventory_audit_insert_islandora_parents($pid, $parents);
      }
    }
  }
    
  catch (Exception $e) {
    error_log('EXCEPTION in _save_solr_search_session : called from ' . $_SERVER['SERVER_NAME'] .
' - ' . $_SERVER['REQUEST_URI'] . '
' . print_r($e, true));
  }
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
  $results = db_select('upitt_islandora_audit_items_photodb', 'ab')
            ->fields('ab')
            ->execute();
  $num_of_upitt_islandora_audit_items_photodb = $results->rowCount();
  return array('upitt_islandora_audit_items_islandora' => $num_of_upitt_islandora_audit_items_islandora,
      'upitt_islandora_audit_items_workflow_record' => $num_of_upitt_islandora_audit_items_workflow_record,
      'upitt_islandora_audit_items_bigfoot' => $num_of_upitt_islandora_audit_items_bigfoot,
      'upitt_islandora_audit_items_photodb' => $num_of_upitt_islandora_audit_items_photodb,
      );
}
