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
      )
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Return'),
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
      drupal_goto('/admin/islandora');
      break;

    case 'edit-reset':
      break;
  }
}
