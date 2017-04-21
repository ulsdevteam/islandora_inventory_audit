<?php

/**
 * @file
 * Utility functions for the Islandora Inventory Audit module.
 */

function upitt_islandora_inventory_audit_foo() {
  return '';
}

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