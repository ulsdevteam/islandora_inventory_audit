<?php

/**
 * @file
 * Administration forms and submit handling for Islandora Inventory Audit.
 */
   
/**
 * Administration and configuration form for upitt_islandora_inventory_audit.
 *
 * @ingroup forms
 *
 * @todo break out non-settings elements into their own form.
 *
 * @return array
 *   The Drupal form definition.
 */
function upitt_islandora_inventory_audit_admin_form() {
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
  $inventory_record_counts = upitt_islandora_inventory_all_record_counts();
  $form = array(
    'reports' => array(
      '#type' => 'item',
      '#markup' => '<h3>' . l('View Islandora Audit Reports', 'admin/islandora/inventory_audit/reports') . '</h3>',
    ),
    'frame' => array(
      '#type' => 'fieldset',
      '#title' => 'About Islandora Inventory Audit',
      'bigfoot_mapped_path' => array(
        '#type' => 'textfield',
        '#title' => 'Bigfoot files list file on this server',
        '#description' => 'Ask IT department if this value needs to be set.',
        '#default_value' => variable_get('upitt_islandora_inventory_audit_bigfoot_files_mapped_path')
      ),
      'photodb_export' => array(
        '#type' => 'textfield',
        '#title' => 'PhotoDB export file on this server',
        '#description' => 'Ask IT department if this value needs to be set.',
        '#default_value' => variable_get('upitt_islandora_inventory_audit_photodb_export')
      ),
    ),
    'frame2' => array(
      '#type' => 'fieldset',
      '#title' => t('Islandora inventory options'),
      'islandora_offset' => array(
        '#type' => 'textfield',
        '#title' => t('Starting offset'),
        '#description' => t('This will cause the inventory process to resume ' .
            'at a specific index.  This can be any unsigned integer value.'),
        '#default_value' => variable_get('upitt_islandora_inventory_audit_islandora_offset', '0'),
      ),
      'islandora_start_date' => array(
        '#type' => 'textfield',
        '#title' => t('Starting date for inventory process'),
        '#description' => t('This date value must be in ISO 8601 format and is ' .
            'used in the Solr query to limit results of the inventory process ' .
            'to all items created after that specific date - the resulting ' .
            'format of the Solr query would have <code>&fq=fgs_createdDate_dt' .
            ':[<b>2013-07-17T00:00:00Z</b> TO NOW]</code>.'),
        '#attributes' => array(
            'placeholder' => '2013-07-17T00:00:00Z',
        ),
        '#default_value' => variable_get('upitt_islandora_inventory_audit_islandora_start_date', ''),
      ),
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save Configuration'),
    ),
    'populate' => array(
      '#type' => 'submit',
      '#value' => t('Populate ALL inventory tables'),
    ),
    'pagecounts' => array(
      '#type' => 'fieldset',
      '#title' => 'Recalc Page Counts table',
      '#description' => 'This process will update a new table that tracks the page counts for objects in islandora and bigfoot',
      'update_pagecounts' => array(
        '#type' => 'submit',
        '#value' => t('Update Page counts'),
      ),
    ),
    'islandora' => array(
      '#type' => 'fieldset',
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_islandora']) . ' Islandora objects',
      'sync_islandora' => array(
        '#type' => 'submit',
        '#value' => t('Inventory Islandora'),
      ),
      'update_depositors_islandora' => array(
        '#type' => 'submit',
        '#prefix' => '<br><i style="color:red">This must be done after the "Inventory Islandora" process is completed : </i> ',
        '#suffix' => 'Updates the depositor value for each islandora inventory record object - including page objects.',
        '#value' => t('Update Depositors for all Islandora objects'),
      ),
    ),
    'workflow' => array(
      '#type' => 'fieldset',
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_workflow_record']) . ' legacy django workflow records',
      'sync_workflow' => array(
        '#type' => 'submit',
        '#value' => t('Inventory Workflow'),
      ),
    ),
    'bigfoot' => array(
      '#type' => 'fieldset',
      '#title' => number_format($inventory_record_counts['upitt_islandora_audit_items_bigfoot']) . ' bigfoot object files',
      '#suffix' => '<b>' . l('Populate Records with batch name', '/admin/islandora/inventory_audit/islandora_inventory_populate_batch_name') . '</b>',
      'sync_bigfoot' => array(
        '#type' => 'submit',
        '#value' => t('Inventory bigfoot'),
      ),
      'updatebigfoottable' => array(
        '#type' => 'submit',
        '#prefix' => '<br><i style="color:red">This must be done after the "Inventory bigfoot" process is completed : </i>',
        '#suffix' => 'Updates a new field "in all_ingests" for each object.',
        '#value' => t('Update bigfoot Table'),
        '#description' => 'This process will update a new field "in all_ingests" for each object',
      ),
    ),
  );
  return $form;
}

/**
 * Function that sets the Drupal variables with user's input.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 */
function upitt_islandora_inventory_audit_admin_form_submit(array $form, array &$form_state) {
  $id = $form_state['triggering_element']['#id'];
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
  switch ($id) {
    case 'edit-submit':
      variable_set('upitt_islandora_inventory_audit_bigfoot_files_mapped_path', $form_state['values']['bigfoot_mapped_path']);
      variable_set('upitt_islandora_inventory_audit_photodb_export', $form_state['values']['photodb_export']);
      variable_set('upitt_islandora_inventory_audit_islandora_start_date', $form_state['values']['islandora_start_date']);
      variable_set('upitt_islandora_inventory_audit_islandora_offset', intval($form_state['values']['islandora_offset'] + 0));
      drupal_set_message('Settings updated');
      drupal_goto('/admin/islandora/inventory_audit');
      break;

    case 'edit-sync-islandora':
      upitt_islandora_inventory_audit_sync_islandora();
      break;

    case 'edit-update-depositors-islandora':
      upitt_islandora_inventory_audit_update_depositors();
      break;
    
    case 'edit-sync-workflow':
      upitt_islandora_inventory_audit_sync_workflow();
      break;

    case 'edit-sync-bigfoot':
      upitt_islandora_inventory_audit_sync_bigfoot();
      break;

    case 'edit-sync-workflow':
      upitt_islandora_inventory_audit_sync_photodb();
      break;

    case 'edit-populate':
      upitt_islandora_inventory_audit_populate();
      break;

    case 'edit-updatebigfoottable':
      upitt_islandora_inventory_updatebigfoottable();
      break;

    case 'edit-update-pagecounts':
      upitt_islandora_inventory_pagecounts();
      break;

    case 'edit-reset':
      variable_del('upitt_islandora_inventory_audit_bigfoot_files_mapped_path');
      variable_del('upitt_islandora_inventory_audit_photodb_export');
      variable_del('upitt_islandora_inventory_audit_islandora_start_date');
      variable_del('upitt_islandora_inventory_audit_islandora_offset');
      break;
  }
}
