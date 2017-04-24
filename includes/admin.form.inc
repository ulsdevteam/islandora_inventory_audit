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
  $form = array(
    'frame' => array(
      '#type' => 'fieldset',
      '#title' => 'About Islandora Inventory Audit',
      'about' => array(
        '#type' => 'item',
        '#markup' => 'Inventory Audit',
      ),
      'bigfoot_mapped_path' => array(
        '#type' => 'textfield',
        '#title' => 'Bigfoot files list file on this server',
        '#description' => 'Ask IT department if this value needs to be set.',
        '#default_value' => variable_get('upitt_islandora_inventory_audit_bigfoot_files_mapped_path')
      ),
      'equinox_mapped_path' => array(
        '#type' => 'textfield',
        '#title' => 'Equinox files list file on this server',
        '#description' => 'Ask IT department if this value needs to be set.',
        '#default_value' => variable_get('upitt_islandora_inventory_audit_equinox_files_mapped_path')
      )
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save Configuration'),
    ),
    'populate' => array(
      '#type' => 'submit',
      '#value' => t('Populate audit table'),
    ),
    'sync_islandora' => array(
      '#type' => 'submit',
      '#value' => t('Sync Islandora'),
    ),
    'sync_bigfoot' => array(
      '#type' => 'submit',
      '#value' => t('Sync bigfoot'),
    ),
    'sync_equinox' => array(
      '#type' => 'submit',
      '#value' => t('Sync equinox'),
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
  switch ($id) {
    case 'edit-submit':
      variable_set('upitt_islandora_inventory_audit_bigfoot_mapped_path', $form_state['values']['bigfoot_mapped_path']);
      drupal_set_message('Settings updated');
      drupal_goto('/admin/islandora');
      break;

    case 'edit-sync-islandora':
      module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
      upitt_islandora_inventory_audit_sync_islandora();
      break;

    case 'edit-sync-workflow':
      module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
      upitt_islandora_inventory_audit_sync_workflow();
      break;

    case 'edit-sync-bigfoot':
      module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
      upitt_islandora_inventory_audit_sync_bigfoot();
      break;

    case 'edit-sync-equinox':
      module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
      upitt_islandora_inventory_audit_sync_equinox();
      break;

    case 'edit-populate':
      module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
      upitt_islandora_inventory_audit_populate();
      break;

    case 'edit-reset':
      variable_del('upitt_islandora_inventory_audit_bigfoot_files_mapped_path');
      variable_del('upitt_islandora_inventory_audit_equinox_files_mapped_path');
      break;
  }
}
