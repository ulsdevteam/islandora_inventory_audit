  <?php 

/**
 * @file
 * Form to display results of any islandora object's audit details.
 */

/**
 * Islandora Audit form for a single islandora object -- display the status of this object in the other locations.
 */
function upitt_islandora_inventory_audit_form(array $form, array &$form_state, AbstractObject $object) {
  module_load_include('inc', 'upitt_workflow', 'includes/utilities');
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/utilities');
  module_load_include('inc', 'upitt_islandora_inventory_audit', 'includes/db');

  $id_no_namespace = '';
  $workflow_page_record_exists = FALSE;
  if (is_object($object)) {
    $is_paged = upitt_workflow_is_paged_object($object);
    @list($namespace, $id_no_namespace) = explode(":", $object->id);
    $audit_record = upitt_islandora_inventory_audit_get_record_by_pid($object->id);

    $parents = upitt_islandora_inventory_get_parent_values($object);
    // If the workflow_record value is set, get the status of that record from the database.
    // if (!empty($audit_record['workflow_record'])) {
    // }

    // check against the "all_ingests.txt" file
    $found = upitt_workflow_check_for_object_in_all_ingests($id_no_namespace);
    // For a paged object, just look for the pid value ($id_no_namespace) in the all_intests file, but for a
    // page or manuscript page the parent object should be on the all_ingests list.
    if (!$is_paged) {
      $workflow_page_record_exists = upitt_workflow_does_workflow_page_record_exist($id_no_namespace);
    }

    $solr_record = upitt_workflow_get_solr_record($object->id);
    $best_date = upitt_islandora_inventory_audit_best_date($solr_record);
  }

  $additional_markup = '<p><span class="format_label">Islandora Label</span> ' . $object->label . '</b></p>' .
    (($best_date) ? '<p><span class="format_label">Date</span>' . $best_date . '</p>' : '') .
    '<p><span class="format_label">Model</span>' . implode(', ', $object->models) . '</p><hr>' .

    '<h4>Legacy systems audit</h4>' .
    '<p><span class="format_label' . ((empty($audit_record['workflow_record'])) ? ' error' : '') .
          '">in django workflow?</span>' .
          ((empty($audit_record['workflow_record'])) ? 'No' : 'Yes | <b>' .
            l('Edit', 'workflow/object/' . $id_no_namespace . '/edit') . '</b>') .
    '</p>' .
    '<p><span class="format_label' . ((empty($audit_record['bigfoot_path'])) ? ' error' : '') .
          '">on bigfoot?</span>' . ((empty($audit_record['bigfoot_path'])) ? 'No' : 'Yes | ' . $audit_record['bigfoot_path']) .
    '<p><span class="format_label' . ((empty($audit_record['equinox_path'])) ? ' error' : '') .
          '">on equinox?</span>' . ((empty($audit_record['equinox_path'])) ? 'No' : 'Yes | ' . $audit_record['equinox_path']) .
    '</p>' .

    '<h4>Object "' . $id_no_namespace . '" ingested upstairs?</h4>' .
    '<p>' . l($id_no_namespace, '/workflow/object/' . $object->id) . ' is ' . ($found ? '' : 'not') . ' in the "all_ingests.txt" file. ' .
    (($is_paged) ? '' : 'This page object (' . $object->id . ') ' . (($workflow_page_record_exists) ? 'has' : 'DOES NOT HAVE') . ' a matching record in the workflow database') .
    '</p>';

  $form = array(  
    'object_audit' => array(
      '#type' => 'fieldset',
      '#title' => t('Inventory Audit for Object'),
      'markup' => array('#markup' => $additional_markup),
    ),
    'pid' => array(
      '#type' => 'hidden',
      '#default_value' => $object->id,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Close'),
    ),
  );  
  return $form;
}

function upitt_islandora_inventory_audit_form_submit(array $form, array &$form_state) {
  $form_state['redirect'] = '/islandora/object/' . $form_state['values']['pid'] . '/manage/audit_upitt_workflow';
}

