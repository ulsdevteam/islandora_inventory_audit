<?php

/**
 * @file
 * Report forms for the Islandora Inventory Audit module.
 */

/*
 * -- objects on bigfoot that are not in Islandora
 * SELECT ab.barcode, ab.bigfoot_path
 * FROM upitt_islandora_audit_items_bigfoot ab
 * WHERE ab.barcode NOT IN (
 *   SELECT ai.barcode FROM upitt_islandora_audit_items_islandora ai);
 *
 * -- objects in workflow system that are in not Islandora
 * SELECT aw.barcode
 * FROM upitt_islandora_audit_items_workflow_record aw
 * WHERE aw.barcode NOT IN (
 *   SELECT ai.barcode FROM upitt_islandora_audit_items_islandora ai);
 *
 *
 */