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
        '#title' => 'Bigfoot mapped path on this server',
        '#description' => 'Ask IT department if this value needs to be set.',
        '#default_value' => variable_get('upitt_islandora_inventory_audit_bigfoot_mapped_path', '/ingest/holding')
      )
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Return'),
    ),
    'populate' => array(
      '#type' => 'submit',
      '#value' => t('Populate audit table'),
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

    case 'edit-populate':
      module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');
      upitt_islandora_inventory_audit_populate();
      break;

    case 'edit-reset':
      variable_del('upitt_islandora_inventory_audit_bigfoot_mapped_path');
      break;
  }
}
