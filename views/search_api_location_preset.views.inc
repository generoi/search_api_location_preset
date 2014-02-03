<?php
/**
 * @file
 * Provide views data for Search API location
 */

/**
 * Implements hook_views_data_alter().
 */
function search_api_location_preset_views_data_alter(&$data) {
  foreach (search_api_index_load_multiple(FALSE) as $id => $index) {
    $table = &$data['search_api_index_' . $id];
    foreach (search_api_location_get_location_fields($index) as $key => $field) {
      $key = _entity_views_field_identifier($key, $table);
      if (isset($table[$key])) {
        // Add additional pseudo field to set point separately by an argument.
        $table[$key . '_location_preset'] = $table[$key];
        $table[$key . '_location_preset']['title'] = $table[$key]['title'] . ' exposed filter';
        $table[$key . '_location_preset']['help'] = t('Location filter with predefined locations.');
        $table[$key . '_location_preset']['filter']['handler'] = 'SearchApiLocationPresetViewsHandlerFilter';
        $table[$key . '_location_preset']['filter']['type'] = $field['real_type'];
      }
    }
  }
}
