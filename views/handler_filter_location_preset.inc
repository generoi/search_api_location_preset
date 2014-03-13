<?php

/**
 * @file
 * Provides the views handler for location fields
 */

class SearchApiLocationPresetViewsHandlerFilter extends SearchApiViewsHandlerFilter {

  /**
   * String to disable the filter.
   * @var string
   */
  const FILTER_DISABLED = 'none';

  /**
   * String to mark custom set filter values.
   * @var string
   */
  const FILTER_CUSTOM = 'custom';

  /**
   * String to mark current set filter values.
   * @var string
   */
  const FILTER_CURRENT = 'current';

  /**
   * This filter is always exposed.
   *
   * @return bool|TRUE
   *   Always TRUE.
   */
  public function is_exposed() {
    return TRUE;
  }

  /**
   * Defines the options the filter has.
   *
   * @return array
   *   Array with the options.
   */
  public function option_definition() {
    $options = parent::option_definition();

    // This handler makes only sense as exposed filter.
    $options['exposed'] = array('default' => TRUE);

    // Define exposed options.
    $options['expose']['contains'] += array(
      'allow_custom_values' => array('default' => FALSE),
      'add_current_position_preset' => array('default' => FALSE),
      'geocoder' => array(
        'contains' => array(
          'show' => array('default' => FALSE),
        ),
      ),
      'map' => array(
        'contains' => array(
          'show' => array('default' => FALSE),
          'settings' => array(
            'contains' => array(
              'allow_pick' => array('default' => FALSE),
              'openlayers_map' => array('default' => NULL),
            ),
          ),
        ),
      ),
      'radius' => array(
        'contains' => array(
          'show' => array('default' => FALSE),
          'settings' => array(
            'contains' => array(
              'radius' => array('default' => 5),
              'unit' => array('default' => 1),
              'min' => array('default' => 1),
              'max' => array('default' => 10),
              'step' => array('default' => 1),
            ),
          ),
        ),
      ),
      'use_operator' => array('default' => FALSE),
      'description' => array('default' => NULL),
      'remember' => array('default' => FALSE),
      'multiple' => array('default' => FALSE),
      'required' => array('default' => FALSE),
    );

    // Define exposed locations array.
    $options['expose']['contains']['locations'] = array(
      'contains' => array(),
      'export' => 'handleExport',
      'unpack_translatable' => 'unpackTranslations',
    );
    return $options;
  }

  /**
   * Special export handler for exposed locations.
   */
  public function handleExport($indent, $prefix, $storage, $option, $definition, $parents) {
    $output = '';
    $parents[] = 'locations';

    // Define sub-element structure for export.
    $definition = array(
      'contains' => array(
        'label' => array('default' => '', 'translatable' => TRUE),
        'lat' => array('default' => '51.03826129999999'),
        'long' => array('default' => '3.7125661000000036'),
        'radius' => array('default' => 5),
        'radius_min' => array('default' => 5),
        'radius_max' => array('default' => 100),
        'radius_step' => array('default' => 5),
        'radius_unit' => array('default' => 1),
      ),
    );

    // Iterate over locations and build the options.
    foreach ($storage['locations'] as $index => $location) {
      $output .= $this->export_option($indent, $prefix, array($index => $location), $index, $definition, $parents);
    }
    return $output;
  }

  /**
   * Special translatable handler for exposed locations.
   */
  public function unpackTranslations(&$translatable, $storage, $option, $definition, $parents, $keys) {
    // Define options structure for export.
    $definition = array(
      'contains' => array(
        'label' => array('default' => '', 'translatable' => TRUE),
      ),
    );
    $parents[] = 'locations';

    // Iterate over locations and build the options.
    foreach ($storage['locations'] as $index => $location) {
      $this->unpack_translatable($translatable, array($index => $location), $index, $definition, $parents, $parents);
    }
  }

  /**
   * Provide a form for setting the filter value.
   */
  public function value_form(&$form, &$form_state) {
    // Process only when viewed in the frontend.
    if (!empty($this->view->editing)) {
      return;
    }

    $identifier = $this->options['expose']['identifier'];
    $field = $this->options['id'];
    $html_id = drupal_html_id($identifier);

    $form['value'] = array('#tree' => TRUE);

    // Get preset.
    $input = (isset($form_state['input'][$identifier])) ? $form_state['input'][$identifier] : array();
    $presets = array();
    $preset = $this->getCurrentPreset($input);
    $preset_index = key($preset);
    $preset_location = reset($preset);
    $preset_changed = (!empty($input['last_preset']) && $input['last_preset'] != $preset_index);
    if (!$preset_location) {
      $preset_location = array();
    }

    if (!empty($this->options['expose']['locations'])) {
      $presets[self::FILTER_DISABLED] = t('None');
      foreach ($this->options['expose']['locations'] as $preset_item) {
        $presets[] = t($preset_item['label']);
      }
      if ($preset_index === self::FILTER_CUSTOM || (!empty($this->options['expose']['allow_custom_values']) && (!empty($this->options['expose']['geocoder']['show']) || !empty($this->options['expose']['map']['settings']['allow_pick'])))) {
        $presets[self::FILTER_CUSTOM] = t('Custom');
      }
      $form['value']['preset'] = array(
        '#type' => 'select',
        '#options' => $presets,
        '#default_value' => $preset_index,
        '#value' => $preset_index,
        '#attributes' => array(
          'id' => $html_id . '-preset',
        ),
      );
      $form['value']['last_preset'] = array(
        '#type' => 'hidden',
        '#default_value' => $preset_index,
        '#value' => $preset_index,
      );
      // Ensure an empty radius or a changed preset gets the default.
      if (empty($form_state['input'][$identifier]['radius']) || $preset_changed) {
        unset(
        $form_state['input'][$identifier]['radius'],
        $form_state['values'][$identifier]['radius']
        );
      }
    }

    if (!empty($this->options['expose']['geocoder']['show'])) {
      $form['address'] = array(
        '#type' => 'textfield',
        '#maxlength' => 120,
        '#attributes' => array(
          'id' => $html_id . '-address',
        ),
        '#field_suffix' => '<a id="geocode">' . t('Get location') . '</a>',
      );
      $form['help'] = array(
        '#attributes' => array(
          'id' => $html_id . '-help',
        ),
        '#markup' => t('Enter an address / location in the textfield or click on the map to set the marker'),
      );
    }

    // Replace with openlayers integration.
    if (!empty($this->options['expose']['map']['show'])) {
      $form['value']['gmap'] = array(
        '#type' => 'container',
        '#attributes' => array(
          'id' => $html_id . '-gmap',
          'style' => array('width:100%; height:400px;'),
        ),
      );
    }

    $form['value']['radius'] = array(
      '#type' => 'hidden',
      '#default_value' => $this->options['expose']['radius']['settings']['radius'],
    );
    if ($preset_index !== self::FILTER_DISABLED) {
      // Set the preset specific default radius.
      $form['value']['radius']['#default_value'] = $preset_location['radius'];
      if (!empty($this->options['expose']['radius']['show']) && !empty($preset_location['radius']) && $preset_location['radius_min'] != $preset_location['radius_max']) {
        $radius_units = search_api_location_preset_radius_options();
        $form['value']['radius'] = array(
          '#type' => 'textfield',
          '#title' => t('Radius'),
          '#size' => 3,
          '#default_value' => $preset_location['radius'],
          '#field_suffix' => $radius_units[$preset_location['radius_unit']],
          '#attributes' => array(
            'id' => $html_id . '-radius',
          ),
        );
        $form['value']['slider'] = array(
          '#type' => 'container',
          '#attributes' => array(
            'id' => $html_id . '-slider',
          ),
          '#attached' => array(
            'library' => array(array('system', 'ui.slider')),
          ),
        );
      }
    }
    $form['value']['lat'] = array(
      '#type' => 'hidden',
      '#default_value' => (isset($preset_location['lat'])) ? $preset_location['lat'] : NULL,
      '#attributes' => array(
        'id' => $html_id . '-lat',
      ),
    );
    $form['value']['long'] = array(
      '#type' => 'hidden',
      '#default_value' => (isset($preset_location['long'])) ? $preset_location['long'] : NULL,
      '#attributes' => array(
        'id' => $html_id . '-long',
      ),
      '#attached' => array(
        'js' => array(
          drupal_get_path('module', 'search_api_location_preset') . '/search_api_location_preset.pick.js' => array(
            'type' => 'file',
            'scope' => 'footer',
          ),
          0 => array(
            'data' => array(
              'searchApiLocationPreset' => array(
                $html_id => $preset_location + array('add_current_position' => !empty($this->options['expose']['add_current_position_preset']), 'preset' => $preset_index),
              ),
            ),
            'type' => 'setting',
          ),
        ),
      ),
    );

    // Attach google maps scripts if necessary.
    if (!empty($this->options['expose']['map_show']) || !empty($this->options['expose']['show_geocoder'])) {
      $form['value']['long']['#attached']['js'] += array(
        '//www.google.com/jsapi' => array('type' => 'external'),
        '//maps.google.com/maps/api/js?sensor=false' => array('type' => 'external'),
      );
    }

  }

  /**
   * Analyzes the input and returns the appropriate preset.
   *
   * @param array $input
   *   The input values of the field.
   *
   * @return array
   *   An array with the index as key and the preset as value.
   */
  protected function getCurrentPreset($input) {
    // By default the first of the presets is used.
    $preset_index = self::FILTER_DISABLED;
    if (!empty($this->options['expose']['required'])) {
      $preset_index = 0;
    }
    $preset = FALSE;

    // Is a default preset requested? And was the preset changed?
    if (isset($input['preset'])) {
      $preset_index = check_plain($input['preset']);
    }

    // Check if custom coordinates are accepted and if so create the custom
    // preset.
    $custom_value_presets = array(
      self::FILTER_CUSTOM => self::FILTER_CUSTOM,
      self::FILTER_CURRENT => self::FILTER_CURRENT,
    );
    if (!empty($this->options['expose']['allow_custom_values']) && (isset($custom_value_presets[$preset_index])) && !empty($input)) {
      $location = array();
      if ($preset) {
        $location = $preset['location'];
      }
      if (!empty($this->options['expose']['allow_custom_values'])) {
        $location['lat'] = check_plain($input['lat']);
        $location['long'] = check_plain($input['long']);
      }

      // Ensure missing values are filled with the defaults.
      $location += array(
        'lat' => '51.03826129999999',
        'long' => '3.7125661000000036',
        'radius' => $this->options['expose']['radius']['settings']['radius'],
        'radius_unit' => $this->options['expose']['radius']['settings']['unit'],
        'radius_min' => $this->options['expose']['radius']['settings']['min'],
        'radius_max' => $this->options['expose']['radius']['settings']['max'],
        'radius_step' => $this->options['expose']['radius']['settings']['step'],
      );

      if ($preset_index == self::FILTER_CURRENT) {
        $preset_index = self::FILTER_CURRENT;
      }
      else {
        $preset_index = self::FILTER_CUSTOM;
      }
      $preset = $location;
    }
    elseif ($preset_index !== self::FILTER_DISABLED) {
      $preset = $this->options['expose']['locations'][$preset_index];
    }

    $preset_changed = (!empty($input['last_preset']) && $input['last_preset'] != $preset_index && $preset_index != self::FILTER_CUSTOM);

    // Check if radius information were posted. But only allow changes if the
    // preset wasn't changed on this request. This ensures that the preset
    // defaults get properly set initially.
    if ($preset !== FALSE && !empty($input) && !$preset_changed) {
      if (!empty($this->options['expose']['radius']['show']) || !empty($this->options['expose']['allow_custom_values'])) {
        if (isset($input['radius'])) {
          $preset['radius'] = (int) $input['radius'];
        }
        if (isset($input['radius_unit'])) {
          $preset['radius_unit'] = check_plain($input['radius_unit']);
        }
        if (isset($input['radius_min'])) {
          $preset['radius_min'] = (int) $input['radius_min'];
        }
        if (isset($input['radius_max'])) {
          $preset['radius_max'] = (int) $input['radius_max'];
        }
        if (isset($input['radius_step'])) {
          $preset['radius_step'] = (int) $input['radius_step'];
        }
      }
    }

    return array($preset_index => $preset);
  }

  /**
   * Provide custom options form.
   */
  public function options_form(&$form, &$form_state) {
    // Ensure the exposed stuff is set automatically.
    if (empty($this->options['expose']['identifier'])) {
      $this->options['expose']['operator'] = $this->options['id'] . '_op';
      $this->options['expose']['identifier'] = $this->options['id'];
      $this->options['expose']['label'] = $this->definition['title'];
    }

    // Fetch the defaults.
    parent::options_form($form, $form_state);
    // Remove not needed form elements.
    unset($form['operator']);
    unset($form['value']);
  }

  /**
   * Provide custom exposed form.
   */
  public function expose_form(&$form, &$form_state) {
    // Fetch and adjust the defaults.
    parent::expose_form($form, $form_state);
    $form['expose_button']['#access'] = FALSE;
    $form['expose']['multiple']['#access'] = FALSE;
    $form['expose']['use_operator']['#access'] = FALSE;
    $form['expose']['operator_id']['#access'] = FALSE;
    $form['expose']['required']['#description'] = t('Uses the first item in the preset list.');

    $form['expose']['allow_custom_values'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow custom values'),
      '#description' => t('If enabled you can configure which values can be changed by the users'),
      '#default_value' => !empty($this->options['expose']['allow_custom_values']),
    );
    $form['expose']['add_current_position_preset'] = array(
      '#type' => 'checkbox',
      '#title' => t('Provide a preset for the current position'),
      '#description' => t('If enabled and the browser supports it a preset for the current position is show.'),
      '#default_value' => !empty($this->options['expose']['add_current_position_preset']),
      '#states' => array(
        'visible' => array(
          ':input[name="options[expose][allow_custom_values]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['expose']['geocoder']['show'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show geocoder input field'),
      '#default_value' => !empty($this->options['expose']['geocoder']['show']),
      '#states' => array(
        'visible' => array(
          ':input[name="options[expose][allow_custom_values]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['expose']['map']['show'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show map'),
      '#default_value' => !empty($this->options['expose']['map']['show']),
    );
    $form['expose']['map']['settings'] = array(
      '#title' => t('Map settings'),
      '#type' => 'fieldset',
      '#states' => array(
        'visible' => array(
          ':input[name="options[expose][map][show]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['expose']['map']['settings']['allow_pick'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow picking new location on map'),
      '#default_value' => !empty($this->options['expose']['map']['settings']['allow_pick']),
      '#states' => array(
        'visible' => array(
          ':input[name="options[expose][allow_custom_values]"]' => array('checked' => TRUE),
        ),
      ),
    );
//    if (module_exists('openlayers')) {
//      $openlayer_maps = array('' => t('None')) + openlayers_map_options();
//      $form['expose']['map']['settings']['openlayers_map'] = array(
//        '#type' => 'select',
//        '#title' => t('Openlayers Map'),
//        '#options' => $openlayer_maps,
//        '#default_value' => !empty($this->options['expose']['openlayers_map']),
//      );
//    }

    // Radius.
    $form['expose']['radius']['show'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show radius input slider'),
      '#default_value' => !empty($this->options['expose']['radius']['show']),
    );
    $form['expose']['radius']['settings'] = array(
      '#title' => t('Radius settings'),
      '#type' => 'fieldset',
      '#states' => array(
        'visible' => array(
          ':input[name="options[expose][radius][show]"]' => array('checked' => TRUE),
        ),
      ),
      '#attributes' => array(
        'class' => array('container-inline'),
      ),
    );
    $form['expose']['radius']['settings']['radius'] = array(
      '#title' => t('Radius'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $this->options['expose']['radius']['settings']['radius'],
    );
    $form['expose']['radius']['settings']['unit'] = array(
      '#type' => 'select',
      '#options' => search_api_location_preset_radius_options(),
      '#default_value' => $this->options['expose']['radius']['settings']['unit'],
    );
    $form['expose']['radius']['settings']['min'] = array(
      '#title' => t('Min'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $this->options['expose']['radius']['settings']['min'],
    );
    $form['expose']['radius']['settings']['max'] = array(
      '#title' => t('Max'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $this->options['expose']['radius']['settings']['max'],
    );
    $form['expose']['radius']['settings']['step'] = array(
      '#title' => t('Step'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $this->options['expose']['radius']['settings']['step'],
    );

    // If there are no locations yet - add default one.
    if (empty($this->options['expose']['locations'])) {
      $this->options['expose']['locations'][0] = array(
        'label' => '',
        'location' => $this->options['location'],
      );
    }

    // Generate input for locations.
    $form['expose']['locations'] = array(
      '#theme' => 'search_api_location_preset_views_filter_table',
    );
    foreach ($this->options['expose']['locations'] as $weight => $location) {
      $form['expose']['locations'][$weight] = $this->createExposedLocationElement($weight, $location, $form, $form_state);
    }
    $form['expose']['save_order'] = array(
      '#type' => 'submit',
      '#value' => t('Save order'),
      '#name' => 'save_order',
      '#submit' => array('search_api_location_preset_views_ui_locations_submit'),
      '#limit_validation_errors' => array(
        array(
          'options',
          'expose',
          'locations',
        ),
      ),
      '#parents' => array(),
    );
    $form['expose']['add_item'] = array(
      '#type' => 'submit',
      '#value' => t('Add location'),
      '#name' => 'add_location',
      '#submit' => array('search_api_location_preset_views_ui_locations_submit'),
      '#limit_validation_errors' => array(),
      '#suffix' => t('If there are more than one item the user will see a dropdown to switch between the locations.'),
      '#parents' => array(),
    );

    // Create the part to edit a selected location.
    if (isset($form_state['#edit_location']) || isset($form_state['#add_location']) || isset($form_state['input']['save_location'])) {
      $location = array();
      $weight = 0;
      if (isset($this->options['expose']['locations'][$form_state['#edit_location']])) {
        $weight = $form_state['#edit_location'];
        $location = $this->options['expose']['locations'][$form_state['#edit_location']];
      }
      // Process form input if not a specific location was requested.
      if (isset($form_state['input']['edit_location']) && !isset($form_state['#edit_location'])) {
        $location = $form_state['input']['edit_location'];
        $weight = (int) $location['weight'];
        unset($location['weight']);
        // Since we're operating at a low level ensure the values are secure.
        foreach ($location as $k => $v) {
          $location[$k] = check_plain($v);
        }
      }
      if (isset($form_state['#add_location'])) {
        $weight = count($this->options['expose']['locations']);
        $location = array();
      }
      $form['expose']['edit_location'] = $this->locationPresetInputElement($weight, $location, $form_state);
      $form['expose']['edit_location']['#parents'] = array('edit_location');
      $form['expose']['edit_location']['save'] = array(
        '#type' => 'submit',
        '#value' => t('Save'),
        '#name' => 'save_location',
        '#submit' => array('search_api_location_preset_views_ui_locations_submit'),
        '#parents' => array(),
      );
    }
  }

  /**
   * Helper function to create a preset item.
   *
   * @param int $weight
   *   The weight of the location.
   * @param array $location
   *   The location to edit.
   * @param array $form_state
   *   The form state.
   *
   * @return array
   *   The form for the location.
   */
  protected function locationPresetInputElement($weight, array $location, array &$form_state) {
    // Ensure the location structure fits.
    $location += array(
      'label' => '',
      'lat' => '',
      'long' => '',
      'radius' => 5,
      'radius_unit' => 1,
      'radius_min' => 5,
      'radius_max' => 10,
      'radius_step' => 1,
    );
    $form = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#title' => t('Edit location'),
      '#attributes' => array(
        'class' => array('container-inline'),
      ),
    );
    $form['weight'] = array(
      '#type' => 'hidden',
      '#default_value' => $weight,
      '#value' => $weight,
    );
    $form['label'] = array(
      '#prefix' => '<br />',
      '#type' => 'textfield',
      '#title' => t('Label'),
      '#size' => 15,
      '#default_value' => $location['label'],
      '#value' => $location['label'],
    );
    $form['lat'] = array(
      '#title' => t('Lat'),
      '#type' => 'textfield',
      '#size' => 20,
      '#default_value' => $location['lat'],
      '#value' => $location['lat'],
    );
    $form['long'] = array(
      '#title' => t('Long'),
      '#type' => 'textfield',
      '#size' => 20,
      '#default_value' => $location['long'],
      '#value' => $location['long'],
      '#suffix' => '<br />',
    );
    $form['radius'] = array(
      '#title' => t('Radius'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $location['radius'],
      '#value' => $location['radius'],
    );
    $form['radius_unit'] = array(
      '#type' => 'select',
      '#options' => search_api_location_preset_radius_options(),
      '#default_value' => $location['radius_unit'],
      '#value' => $location['radius_unit'],
    );
    $form['radius_min'] = array(
      '#title' => t('Min'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $location['radius_min'],
      '#value' => $location['radius_min'],
    );
    $form['radius_max'] = array(
      '#title' => t('Max'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $location['radius_max'],
      '#value' => $location['radius_max'],
    );
    $form['radius_step'] = array(
      '#title' => t('Step'),
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $location['radius_step'],
      '#value' => $location['radius_step'],
      '#suffix' => '<br />',
    );
    return $form;
  }


  /**
   * Creates a form element for the exposed location selector.
   */
  protected function createExposedLocationElement($weight, $location, $form, &$form_state) {
    $element['list_item'] = array(
      '#type' => 'markup',
      '#prefix' => '<div>',
      '#suffix' => '</div>',
    );
    $element = array(
      '#type' => 'value',
      '#value' => $location,
    );
    $element['list_item']['weight'] = array(
      '#type' => 'weight',
      '#value' => $weight,
      '#attributes' => array('class' => array('weight')),
      '#parents' => array('location_preset', 'weights', $weight),
    );
    $element['list_item']['edit_' . $weight] = array(
      '#type' => 'submit',
      '#value' => t('Edit'),
      '#name' => 'edit_location_' . $weight,
      '#submit' => array('search_api_location_preset_views_ui_locations_submit'),
      '#parents' => array(),
    );
    if (count($this->options['expose']['locations']) > 1) {
      $element['list_item']['remove'] = array(
        '#type' => 'submit',
        '#value' => t('Remove'),
        '#name' => 'delete_location_' . $weight,
        '#submit' => array('search_api_location_preset_views_ui_locations_submit'),
        '#parents' => array(),
      );
    }
    return $element;
  }


  /**
   * Add this filter to the query.
   */
  public function query() {
    $field = $this->options['expose']['identifier'];
    $preset_location = FALSE;
    if (isset($this->view->exposed_input[$field])) {
      $preset = $this->getCurrentPreset($this->view->exposed_input[$field]);
      $preset_location = reset($preset);
    }
    if ($preset_location && isset($preset_location['lat']) && isset($preset_location['lat'])) {
      $search_api_location = $this->query->getOption('search_api_location', array());
      $search_api_location[$this->real_field] = array(
        'field' => $this->real_field,
        'lat' => (float) $preset_location['lat'],
        'lon' => (float) $preset_location['long'],
      );
      // Convert the radius value to kilometres based on the chosen unit.
      if ($preset_location['radius'] > 0) {
        $search_api_location[$this->real_field]['radius'] = $preset_location['radius'] * (float) $preset_location['radius_unit'];
      }
      $this->query->setOption('search_api_location', $search_api_location);
    }
  }

}

/**
 * Handles the special configuration form submits of the filter.
 */
function search_api_location_preset_views_ui_locations_submit(&$form, &$form_state) {
  // Ensure we've the necessary options array with the plugin options.
  $options = &$form_state['handler']->options;

  $values = $form_state['values']['options']['expose'];

  switch ($form_state['triggering_element']['#name']) {
    // Store the edited location.
    case 'save_location':
      $location = $form_state['values']['edit_location'];
      $weight = (int) $location['weight'];
      unset($location['weight']);
      $options['expose']['locations'][$weight] = $location;
      unset($form_state['input']['save_location']);
      break;

    // Add a new location.
    case 'add_location':
      $form_state['#add_location'] = TRUE;
      break;

    // Edit location.
    case strpos($form_state['triggering_element']['#name'], 'edit_location') === 0:
      // Get weight of edited location.
      $weight = str_replace('edit_location_', '', $form_state['triggering_element']['#name']);
      $form_state['#edit_location'] = $weight;
      break;

    // Remove location.
    case strpos($form_state['triggering_element']['#name'], 'delete_location') === 0:
      // Get weight of location to remove.
      $weight = str_replace('delete_location_', '', $form_state['triggering_element']['#name']);
      unset($options['expose']['locations'][$weight]);
      break;

    // Save order of locations.
    case 'save_order':
      $locations = array();
      foreach ($form_state['input']['location_preset']['weights'] as $weight => $weight_input) {
        $locations[(int) $weight_input] = $options['expose']['locations'][$weight];
      }
      ksort($locations);
      $locations = array_values($locations);
      $options['expose']['locations'] = $locations;
      break;
  }

  // Update the views / display handler object and clear the caches to ensure
  // its properly propagated.
  $form_state['view']->set_item($form_state['display_id'], $form_state['type'], $form_state['id'], $form_state['handler']->options);
  views_ui_cache_set($form_state['view']);

  // Request a re-render to build the form upon the new options.
  $form_state['rerender'] = TRUE;
  $form_state['rebuild'] = TRUE;
}
