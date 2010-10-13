<?php

/**
 * @file confwizard.admin.inc
 * Features Config Wizard - admin interface
 */

/**
 * Form API callback: admin interface
 */
function confwizard_form_admin_settings(&$form_state) {
  $form = array(
    '#tree' => TRUE,
  );

  $conf_urls = confwizard_db_get_urls();
  if(user_access("administer confwizard")) {
    $conf_urls[] = array();
  }

  foreach($conf_urls as $name => $conf_url) {
    $form[urlencode($name)] = array(
      '#type' => 'fieldset',
      '#title' => $conf_url['label'] ? $conf_url['label'] : "New config wizard hint",

      'name' => array(
        '#type' => 'textfield',
        '#title' => t('Unique semi-machine-readable name'),
        '#description' => t('This is the name that Features building will give you by the relevant checkbox.'),
        '#default_value' => $conf_url['name'],
      ),
      'label' => array(
        '#type' => 'textfield',
        '#title' => t('Human-friendly label'),
        '#default_value' => $conf_url['label'],
        '#description' => t('A clear, concise summary of the configuration task ahead.'),
      ),
      'path' => array(
        '#type' => 'textfield',
        '#title' => t('Drupal path for configuration'),
        '#default_value' => $conf_url['path'],
        '#description' => t('Internal Drupal URL to navigate to e.g. admin/settings/foo_api_keys'),
      ),
      'descr' => array(
        '#type' => 'textarea',
        '#title' => t('Longer description'),
        '#default_value' => $conf_url['descr'],
        '#description' => t('A longer description of what you want the site builder to configure when they click on your link.'),
      ),
    );
  }

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Form API submit callback: admin interface
 */
function confwizard_form_admin_settings_validate(&$form, &$form_state) {
  foreach($form_state['values'] as $likely_name => $val) {
    if(!(is_array($val) && array_key_exists('name',$val))) { continue; }

    // Ignore completely empty ones
    if(!strlen(implode("", $val))) { unset($form_state['values'][$likely_name]); }

    // Check name uniqueness
    $name = is_int($likely_name) ? $val['name'] : $likely_name;
    if ($names[$name]) { form_set_error("$likely_name][name", "Names of config wizard elements must be unique."); }
    $names[$name] = TRUE;

    // Check name, label and URL all exist for non-empty ones
    foreach(array('name', 'label', 'path') as $required) {
      $val[$required] || form_set_error("$likely_name][$required", "Each config wizard step need a $required.");
    }
  }
}

/**
 * Form API submit callback: admin interface
 */
function confwizard_form_admin_settings_submit(&$form, &$form_state) {
  foreach($form_state['values'] as $likely_name => $val) {
    if(!(is_array($val) && array_key_exists('name',$val))) { continue; }

    if(is_int($likely_name)) {
      drupal_write_record("confwizard_url", $val);
    }
    else {
      drupal_write_record("confwizard_url", $val, array('name'));
    }
  }
}

/**
 * Form API callback: follow config
 */
function confwizard_form_follow(&$form_state) {
  // Get all follows, ordered by unique name, and save into form
  $follows = confwizard_db_get_urls();
  $form['#confwizard'] = array(
    'follows' => $follows,
  );

  // Loop over them to produce a hierarchy of doneness and modules
  $all = array('to do' => array(), 'done' => array());
  foreach($follows as $n => $follow) {
    if ($follow['done']) { 
      $all['done']['confwizard'][$n] = $follow; 
    }
    elseif ($follow['feature']) {
      $all['to do'][$follow['feature']][$n] = $follow;
    }
    else {
      $all['to do']['confwizard'][$n] = $follow;
    }
  }

  $all_modules = module_rebuild_cache();
  // Nice hierarchical set of instructions based on doneness
  // and module that supports it
  foreach($all as $is_done => $modules) {
    $f = array(
      '#title' => t(ucfirst($is_done)),
      '#type' => 'fieldset',
    );
    foreach($modules as $mod => $follows) {
      $m = array(
        '#title' => $mod,
        '#type' => 'fieldset',
      );

      // Set friendly title if we can find one
      if ($all_modules[$mod]->info['name']) {
        $m['#title'] = $all_modules[$mod]->info['name'];
      }
      foreach($follows as $n => $follow) {
        $m[urlencode($n)] = array(
          '#type' => 'checkbox',
          '#title' => t($follow['label']) . ": " . l($follow['path'], $follow['path']),
          '#description' => $follow['descr'],
          '#confwizard_label' => $follow['label'],
          '#confwizard_path' => $follow['path'],
          '#default_value' => $follow['done'],
        );
      } 
      element_children($m) && ($f[$mod] = $m);
    }
    element_children($f) && ($form[$is_done] = $f);
  }

  $form['update'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  return $form;
}

/**
 * Form API submit callback: saving done statuses
 */
function confwizard_form_follow_submit(&$form, &$form_state) {
  $urls = confwizard_db_get_urls();

  // Save based on ID
  $new = array();
  foreach($form['#confwizard']['follows'] as $name => $old) {
    $new = array('done' => $form_state['values'][urlencode($name)], 'name' => $name);
    // If they're from a hook, then they won't be in the db to be written
    if ($urls[$name]["from_hook"]) {
      $pre_write = $urls[$name];
      drupal_write_record('confwizard_url', $pre_write);
    }
    drupal_write_record('confwizard_url', $new, array('name'));
  }
}

/**
 * Get URLs from the database
 */
function confwizard_db_get_urls() {
  $urls = array();

  // Add URLs from hook
  $files = drupal_system_listing('\.module$', 'modules', 'name', 0);
  foreach($files as $filename => $file) {
    $file->info = drupal_parse_info_file(dirname($file->filename) .'/'. $file->name .'.info');
    if ($file->info['features']['confwizard_url']) {
      module_load_include("inc", $file->name, $file->name.".default_confwizard_cw_url");
      foreach (module_invoke($file->name, 'default_confwizard_cw_url') as $name => $obj) {
        $urls[$name] = (array)$obj;
        $urls[$name]["from_hook"] = TRUE;
      }
    }
  }


  $result = db_query("SELECT * FROM {confwizard_url}");

  while($row = db_fetch_array($result)) {
    $urls[$row['name']] = $row;
  }
  return $urls;
}
