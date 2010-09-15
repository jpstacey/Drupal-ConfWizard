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
    $form[$name] = array(
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
 * Get URLs from the database
 */
function confwizard_db_get_urls() {
  $result = db_query("SELECT * FROM {confwizard_url}");
  $urls = array();
  while($row = db_fetch_array($result)) {
    $urls[$row['name']] = $row;
  }
  return $urls;
}
