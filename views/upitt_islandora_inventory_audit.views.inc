<?php
/**
 * @file
 * Views hook implementations.
 */

/**
 * Implements hook_views_data().
 */
function upitt_islandora_inventory_audit_views_data() {
  $data = array();

  $data['upitt_islandora_audit_items_islandora'] = array(
    'table' => array(
      'group' => t('Islandora Inventory Audit'),
      'base' => array(
        'field' => 'id',
        'title' => t('Islandora Inventory Audit'),
        'help' => t('The queue of objects to batch ingest.'),
      ),
      'join' => array(
        'upitt_islandora_audit_items_bigfoot' => array(
          'left_field' => 'barcode',
          'field' => 'barcode',
        ),
        'upitt_islandora_audit_items_equinox' => array(
          'left_field' => 'barcode',
          'field' => 'barcode',
        ),
        'upitt_islandora_audit_items_workflow_record' => array(
          'left_field' => 'barcode',
          'field' => 'barcode',
        ),
      ),
    ),
    'id' => array(
      'title' => t('ID'),
      'help' => t('Autoincrement ID of an item.'),
      'field' => array(
        'handler' => 'views_handler_field',
        // This is use by the table display plugin.
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_string',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_string',
      ),
    ),
    'islandora_pid' => array(
      'title' => t('PID'),
      'help' => t('The Islandora PID value for an item.'),
      'field' => array(
        'handler' => 'views_handler_field',
        // This is use by the table display plugin.
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_string',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_string',
      ),
    ),
    'parent_pids' => array(
      'title' => t('Parent PIDs'),
      'help' => t('Identifiers of the parents for the current item.'),
      'field' => array(
        'handler' => 'views_handler_field',
        // This is use by the table display plugin.
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_string',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_string',
      ),
    ),

    'datastreams_latestsize' => array(
      'title' => t('Datastreams\' total size (latest only)'),
      'help' => t('Datastreams\' total size (latest only).'),
      'field' => array(
        'handler' => 'views_handler_field',
        // This is use by the table display plugin.
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_numeric',
      ),
    ),
    'datastreams_totalsize' => array(
      'title' => t('Datastreams\' total size (all versions)'),
      'help' => t('Datastreams\' total size (all versions).'),
      'field' => array(
        'handler' => 'views_handler_field',
        // This is use by the table display plugin.
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_numeric',
      ),
    ),
    'barcode' => array(
      'title' => t('Barcode'),
      'help' => t('Barcode value for an object.'),
      'field' => array(
        'handler' => 'views_handler_field_numeric',
        'click sortable' => TRUE,
      ),
      'filter' => array(
        'handler' => 'views_handler_field',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_string',
      ),
/*
      'relationship' => array(
        'base' => 'islandora_batch_set',
        'base field' => 'id',
        'handler' => 'views_handler_relationship',
        'label' => t('Associated Set'),
        'title' => t('Associated Set'),
        'help' => t('The set of objects with which the given object is associated.'),
      ),
 */
    ),
    'actions' => array(
      'title' => t('Item Actions'),
      'help' => t('A list of actions which may be performed on an item.'),
      'field' => array(
        'handler' => 'UpittIslandoraInventoryAuditViewsFieldHandlerQueueActions',
      ),
    ),
  );

  $data['islandora_batch_set'] = array(
    'table' => array(
      'group' => t('Islandora Batch'),
      'base' => array(
        'field' => 'id',
        'title' => t('Islandora Batch Set'),
        'help' => t('A set of items, to groups items from the queue.'),
      ),
    ),
    'id' => array(
      'title' => t('Set ID'),
      'help' => t('ID of a set of related items.'),
      'field' => array(
        'handler' => 'views_handler_field_numeric',
        'click sortable' => TRUE,
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
    ),
    'uid' => array(
      'title' => t('User ID'),
      'help' => t('ID of the user who created a set.'),
      'field' => array(
        'handler' => 'views_handler_field_user',
        'click sortable' => TRUE,
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_user_uid',
        'name field' => 'name',
      ),
      'filter' => array(
        'title' => t('Name'),
        'handler' => 'views_handler_filter_user_name',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'relationship' => array(
        'base' => 'users',
        'base field' => 'uid',
        'handler' => 'views_handler_relationship',
        'label' => t('Set creator'),
        'title' => t('Set Creator'),
        'help' => t('The user who created a given set.'),
      ),
    ),
    'created' => array(
      'title' => t('Created Date'),
      'help' => t('The date on which the given set was registered.'),
      'field' => array(
        'handler' => 'views_handler_field_date',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort_date',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_date',
      ),
    ),
    'actions' => array(
      'title' => t('Set Actions'),
      'help' => t('A list of actions which may be performed on a set of items.'),
      'field' => array(
        'handler' => 'UpittIslandoraInventoryAuditViewsFieldHandlerSetActions',
      ),
    ),
  );

  $data['islandora_batch_state'] = array(
    'table' => array(
      'group' => t('Islandora Batch'),
      'join' => array(
        'islandora_batch_queue' => array(
          'left_field' => 'id',
          'field' => 'id',
        ),
      ),
    ),
    'id' => array(
      'title' => t('ID'),
      'help' => t('Identifier of an item.'),
    ),
    'state' => array(
      'title' => t('Item State'),
      'help' => t('The state of an item.'),
      'field' => array(
        'handler' => 'UpittIslandoraInventoryAuditStateFieldHandler',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
    ),
  );

  $data['islandora_batch_queue_messages'] = array(
    'table' => array(
      'group' => t('Islandora Batch'),
      'join' => array(
        'islandora_batch_queue' => array(
          'left_field' => 'id',
          'field' => 'id',
        ),
      ),
    ),
    'id' => array(
      'title' => t('ID'),
      'help' => t('Identifier of an item.'),
    ),
    'message' => array(
      'title' => t('Item Message'),
      'help' => t('The message for an item.'),
      'field' => array(
        'handler' => 'views_handler_field',
      ),
    ),
  );
  return $data;
}
