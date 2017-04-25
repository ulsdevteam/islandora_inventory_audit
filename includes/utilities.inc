<?php

/**
 * @file
 * Utility functions for the Islandora Inventory Audit module.
 */

/**
 * Simple function to replicate PHP 5 behaviour
 */
function upitt_islandora_inventory_audit_microtime_float() {
  list($usec, $sec) = explode(" ", microtime());
  return ((float)$usec + (float)$sec);
}

/**
 * Given a $solr_record of an object, this will return the preferred date value.
 *
 * @param type $solr_record
 * @return string
 */
function upitt_islandora_inventory_audit_best_date($solr_record) {
  $best_date = '';

  $solr_date_fieldnames = array('mods_originInfo_type_display_dateOther_s',
                                'dc.date',
                                'mods_originInfo_dateOther_ms',
                                'mods_originInfo_encoding_iso8601_keyDate_yes_dateIssued_s');
  foreach ($solr_date_fieldnames as $solr_date_fieldname) {
    if (!$best_date && array_key_exists($solr_date_fieldname, $solr_record)) {
      $best_date = $solr_record[$solr_date_fieldname];
    }
  }
  return $best_date;
}

/**
 * Returns the PID values of the parents to the given object -- uses isMemberOf
 * as well as isMemberOfCollection to determine parents.
 *
 * @param AbstractObject $object
 * @return array
 */
function upitt_islandora_inventory_get_parent_values($object) {
  $parents = array_merge(
    $object->relationships->get(FEDORA_RELS_EXT_URI, 'isMemberOfCollection'),
    $object->relationships->get(FEDORA_RELS_EXT_URI, 'isMemberOf'));
  $parent_pids = array();
  foreach ($parents as $parent) {
    if (isset($parent['object']) && isset($parent['object']['value'])) {
      $parent_pids[] = $parent['object']['value'];
    }
  }
  return $parent_pids;
}