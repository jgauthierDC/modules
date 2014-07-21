<?php

/**
 * @file
 * Quiz Results Export file containing the forms & supporting code use for Quiz result exports.
 * Functions for creating the export are in the .module file
 *
 */

/**
 * Certificates form.
 */
function quiz_reporting_certificates_form(&$form_state) {
  // Add form Javascript.
  drupal_add_js(drupal_get_path('module', 'quiz_reporting') . '/quiz_reporting.form.js');

  // Build certificate options array.
  $options = array();
  foreach (quiz_reporting_certificates() as $certificate) {
    $options[$certificate['cert_nid']] = $certificate['cert_title'];
  }

  // Sort options alphabetically.
  natcasesort($options);

  // Certificates checkboxes, with "all" checkbox at top.
  $form['certificates'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Choose certificates'),
    '#options' => array('all' => '<strong>' . t('All certificates') . '</strong>') + $options,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Submit'),
  );
  return $form;
}

/**
 * Certificates form - submit callback.
 */
function quiz_reporting_certificates_form_submit($form, &$form_state) {
  // Remove "all" value.
  unset($form_state['values']['certificates']['all']);

  // Redirect to export form.
  drupal_goto('admin/quiz/export_quiz_results_form', array(
    'certificates' => array_values(array_filter($form_state['values']['certificates'])),
  ));
}

 /**
  * Export form.
  *
  * Builds the form used for selecting which users are to appear in the
  * exported file.
  */
function quiz_reporting_export_form(&$form_state) {
  // Add form Javascript.
  drupal_add_js(drupal_get_path('module', 'quiz_reporting') . '/quiz_reporting.form.js');

  // if the url ends with an integer, then we load a quiz_reporting_saved
  // settings from it
  $report_id = $_GET['report'];

  global $user;
  $result = db_query("SELECT report_name, formstate
    FROM {quiz_reporting_saved}
    WHERE report_id = %d
      AND uid = %d", $report_id, $user->uid);
  while ($record = db_fetch_array($result)) {
    // Do something with each $record
    $vals_s = $record['formstate'];
    $report_name = $record['report_name'];
  }
  $vals = unserialize($vals_s);

  $form = array();
  if (isset($report_name)) {
    $form['#prefix'] = 'Saved Report Name: ' . $report_name;
  }
  $form['report_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Enter a name for the report file. The .csv extension will be added automatically.'),
    '#size' => 30,
    '#maxlength' => 128,
    '#default_value' => $report_name ? $report_name : '',
  );
  $form['report_id'] = array(
    '#type' => 'hidden',
    '#description' => 'hidden report id for those loaded from db',
    '#default_value' => 0,
    '#default_value' => $report_id ? $report_id : '',
  );

  $form['save_report'] = array(
    '#type' => 'checkbox',
    '#title' => t('Save this report specification.'),
    '#default_value' => $vals['save_report'] ? $vals['save_report'] : '',
    '#description' => t("Check this box to save your report specification for use later."),
  );

  $form['batch_size'] = array(
    '#type' => 'hidden',
    '#title' => t('Batch Size'),
    '#size' => 5,
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
    //'#description' => t("This will govern the number of items processed in each batch run. It won't necessarily be a user-settable field"),
    '#default_value' => $vals['batch_size'] ? $vals['batch_size'] : 50,
  );



  // make sure there's a profile_state field in the database before showing the
  // select
  if (_quiz_reporting_check_field('profile_state')) {
    $form['state'] = array(
      '#title' => t('State'),
      '#type' => 'select',
      '#options' => _quiz_reporting_get_states(),
      '#multiple' => TRUE,
      '#description' => t("Select the states you want to have included in the report."),
      '#default_value' => $vals['state'] ? $vals['state'] : '',
    );
  }

  // make sure there's a profile_primary_business field in the database before
  // showing the select
  if (_quiz_reporting_check_field('profile_primary_business')) {
    $form['primary_business'] = array(
      '#title' => t('Primary Business'),
      '#type' => 'select',
      '#multiple' => TRUE,
      '#description' => t("Select the user's primary business."), 
      '#options' => _quiz_reporting_get_primary_businesses(),
      '#default_value' => $vals['primary_business'] ? $vals['primary_business'] : '',
    );
  }
  $form['select_report'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select Report'),
    '#collapsible' => FALSE,
  );

  // Store certificates from URL.
  $form['certificates'] = array(
    '#type' => 'value',
    '#value' => $_GET['certificates'],
  );

  $form['select_report']['report'] = array(
    '#type' => 'radios',
    '#title' => t('Which Users'),
    '#options' => array(
      'all_registrants' => ('All Registrants'),
      'not_finished' => t('Started - Not Finished'),
      'not_started' => ('Not Started'),
      'graduates' => ('Graduates'),
    ),
    '#default_value' => $vals['report']? $vals['report'] : 'all_registrants',
  );

  
 
  // Graduation date.
  $form['graduation_date'] = array(
    '#type' => 'fieldset',
    '#title' => t('Graduation Date'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
    '#prefix' => '<div id="graduation-date">',
    '#suffix' => '</div>',
  );
  $form['graduation_date']['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Filter by graduation date:'),
    '#default_value' => $vals['graduation_date']['enabled'] ? $vals : '',
  );
  $form['graduation_date']['start_month'] = array(
    '#type' => 'select',
    '#title' => 'Start Month',
    '#options' => drupal_map_assoc(range(1, 12)),
    '#default_value' => $vals['graduation_date']['start_month'] ? $vals['graduation_date']['start_month'] : array(date("m")),
    '#prefix' => '<div class="container-inline">',
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  $form['graduation_date']['start_year'] = array(
    '#type' => 'select',
    '#title' => t('Start Year'),
    '#suffix' => '</div>',
    '#default_value' => $vals['graduation_date']['start_year'] ? $vals['graduation_date']['start_year'] : array(date("Y")),
    '#options' => drupal_map_assoc(range(_quiz_reporting_min_year(), date('Y'))),
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  $form['graduation_date']['end_month'] = array(
    '#type' => 'select',
    '#title' => 'End Month',
    '#options' => drupal_map_assoc(range(1, 12)),
    '#default_value' =>  $vals['graduation_date']['end_month'] ? $vals['graduation_date']['end_month'] : array(date("m")),
    '#prefix' => '<div class="container-inline">',
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  $form['graduation_date']['end_year'] = array(
    '#type' => 'select',
    '#title' => t('End Year'),
    '#suffix' => '</div>',
    '#default_value' => $vals['graduation_date']['end_year'] ? $vals['graduation_date']['end_year'] : array(date("Y")),
    '#options' => drupal_map_assoc(range(_quiz_reporting_min_year(), date('Y') + 1)),
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );

  
  // Registration date.
  $form['registration_date'] = array(
    '#type' => 'fieldset',
    '#title' => t('Registration Date'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );
  $form['registration_date']['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Filter by registration date:'),
    '#default_value' => $vals['registration_date']['enabled'] ? $vals : '',
  );
  $form['registration_date']['start_month'] = array(
    '#type' => 'select',
    '#title' => 'Start Month',
    '#options' => drupal_map_assoc(range(1, 12)),
    '#default_value' => $vals['registration_date']['start_month'] ? $vals['registration_date']['start_month'] : array(date("m")),
    '#prefix' => '<div class="container-inline">',
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  $form['registration_date']['start_year'] = array(
    '#type' => 'select',
    '#title' => t('Start Year'),
    '#suffix' => '</div>',
    '#default_value' => $vals['registration_date']['start_year'] ? $vals['registration_date']['start_year'] : array(date("Y")),
    '#options' => drupal_map_assoc(range(_quiz_reporting_min_year(), date('Y'))),
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  $form['registration_date']['end_month'] = array(
    '#type' => 'select',
    '#title' => 'End Month',
    '#options' => drupal_map_assoc(range(1, 12)),
    '#default_value' =>  $vals['registration_date']['end_month'] ? $vals['registration_date']['end_month'] : array(date("m")),
    '#prefix' => '<div class="container-inline">',
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  $form['registration_date']['end_year'] = array(
    '#type' => 'select',
    '#title' => t('End Year'),
    '#suffix' => '</div>',
    '#default_value' => $vals['registration_date']['end_year'] ? $vals['registration_date']['end_year'] : array(date("Y")),
    '#options' => drupal_map_assoc(range(_quiz_reporting_min_year(), date('Y') + 1)),
    '#attributes' => array('onchange' => "$('form #edit-filter-by-date').attr('checked', true);"),
  );
  
  
  
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Run Report'),
  );
  return $form;
}

/**
 * Export form - validate callback.
 */
function quiz_reporting_export_form_validate($form, &$form_state) {
  // Registration date.
  $start_date = strtotime($form_state['values']['registration_date']['start_year'] . '-' . $form_state['values']['registration_date']['start_month']);
  $end_date = strtotime($form_state['values']['registration_date']['end_year'] . '-' . $form_state['values']['registration_date']['end_month']);
  if ($form_state['values']['registration_date']['enabled'] && $start_date > $end_date) {
    form_set_error('registration_date][start_year', t('Registration start month and year should not be greater than end month and year.'));
  }

  // Graduation date.
  $start_date = strtotime($form_state['values']['graduation_date']['start_year'] . '-' . $form_state['values']['graduation_date']['start_month']);
  $end_date = strtotime($form_state['values']['graduation_date']['end_year'] . '-' . $form_state['values']['graduation_date']['end_month']);
  if ($form_state['values']['graduation_date']['enabled'] && $start_date > $end_date) {
    form_set_error('graduation_date][start_year', t('Graduation start month and year should not be greater than end month and year.'));
  }
}

/**
 * Export form - submit callback.
 *
 * the call to batch_set at the end of the function kicks off the
 * bach process defined in the 'operations' item in the array
 *
 * passing the batch size from the form as the $limit to the batch function
 * but that can be changed, here, to hard code a limit.
 */
function quiz_reporting_export_form_submit($form, &$form_state) {
  $q = $_GET;
  unset($q['q']);
  global $user;

  if ($form_state['values']['save_report']) {
    if ($form_state['values']['report_id'] == 0) {
      $form_state['values']['report_id'] = db_insert('quiz_reporting_saved')
        ->fields(array(
            'uid' => $user->uid,
            'report_name' => _quiz_reporting_create_report_name($form_state['values']),
            'formstate' => serialize($form_state['values']),
            'timestamp' => time(),
        ))->execute();
      $query_options = array_merge($q, array('report' => $form_state['values']['report_id']));
      $options = array('query'=>$query_options);
      $uri = 'admin/quiz/export_quiz_results_form?report='.$form_state['values']['report_id'].'&cert='.$_GET['cert'];
      $return_uri = url('admin/quiz/export_quiz_results_form', $options);
    }
    else {
      // update the old one
      db_update('quiz_reporting_saved')
        ->fields(array(
            'uid' => $user->uid,
            'report_name' => _quiz_reporting_create_report_name($form_state['values']),
            'formstate' => serialize($form_state['values']),
            'timestamp' => time(),
        ))
        ->condition('report_id', $form_state['values']['report_id'], '=')
        ->execute();
        $query_options = array_merge($q, array('report' => $form_state['values']['report_id']));
        $options = array('query'=>$query_options);
        $return_uri = url('admin/quiz/export_quiz_results_form', $options);
    }
  }

  $operations = array(
    array('quiz_reporting_batch_process', array($form_state['values'], $form_state['values']['batch_size'])),
    array('quiz_reporting_save_stats', array($form_state['values'], $form_state['values']['report_id'])),
  );
  $batch = array(
    'title' => t('Gathering & Exporting user information.'),
    'operations' => $operations,
    'init_message' => t("Initializing"),
    // this is for cases where there are multiple processes
    'progress_message' => '',
    // 'error_message' =>  t('An error has occurred.').
    'finished' => 'quiz_reporting_batch_finished',
    'file' => drupal_get_path('module', 'quiz_reporting') . '/quiz_reporting.batch.inc',
  );
  batch_set($batch);
  // this line is what brings it back to the same uri, with the report id
  // appended. 5/23 commented out because of url encoding on the $return_uri
  // wasn't working anyway.
  // batch_process($return_uri);
  batch_process('admin/quiz/export_quiz_results');
}

//=============== Form helper functions =====================//

/**
 * Check the profile_fields table to make sure that there's a specific field implemented
 * @field is the field we want to test for
 */
function _quiz_reporting_check_field($field) {
  $result = db_query("SELECT COUNT(*) FROM {profile_fields} WHERE name = '%s'", $field);
  return db_result($result);
}

/**
 * Returns earliest year users were added to the users table.
 */
function _quiz_reporting_min_year() {
  return db_result(db_query("SELECT DATE_FORMAT(FROM_UNIXTIME(MIN(created)), '%Y') min_date
    FROM {users}
    WHERE created > 0"));
}

/**
* use the quiz_reporting_primary_business_index table 
* to get a list of primary businesses for the form
**/
function _quiz_reporting_get_primary_businesses(){
  $busineses = array();
  $query = 'SELECT DISTINCT primary_business
FROM {quiz_reporting_primary_business_index}
ORDER BY primary_business ASC';
  $result = db_query($query);
  while ($row = db_fetch_array($result)) {
    $busineses[$row['primary_business']] = $row['primary_business']; 
  }
  return $busineses;
}

/**
* Return an array suitable for use in an #options setting for a multiselect field
* based on states which are actually in the profile_state field
**/
function _quiz_reporting_get_states() {
  $state_options = array(
    'all' => 'All States',
    'AL' => 'Alabama',
    'AK' => 'Alaska',
    'AZ' => 'Arizona',
    'AR' => 'Arkansas',
    'CA' => 'California',
    'CO' => 'Colorado',
    'CT' => 'Connecticut',
    'DE' => 'Delaware',
    'DC' => 'District Of Columbia',
    'FL' => 'Florida',
    'GA' => 'Georgia',
    'HI' => 'Hawaii',
    'ID' => 'Idaho',
    'IL' => 'Illinois',
    'IN' => 'Indiana',
    'IA' => 'Iowa',
    'KS' => 'Kansas',
    'KY' => 'Kentucky',
    'LA' => 'Louisiana',
    'ME' => 'Maine',
    'MD' => 'Maryland',
    'MA' => 'Massachusetts',
    'MI' => 'Michigan',
    'MN' => 'Minnesota',
    'MS' => 'Mississippi',
    'MO' => 'Missouri',
    'MT' => 'Montana',
    'NE' => 'Nebraska',
    'NV' => 'Nevada',
    'NH' => 'New Hampshire',
    'NJ' => 'New Jersey',
    'NM' => 'New Mexico',
    'NY' => 'New York',
    'NC' => 'North Carolina',
    'ND' => 'North Dakota',
    'OH' => 'Ohio',
    'OK' => 'Oklahoma',
    'OR' => 'Oregon',
    'PA' => 'Pennsylvania',
    'RI' => 'Rhode Island',
    'SC' => 'South Carolina',
    'SD' => 'South Dakota',
    'TN' => 'Tennessee',
    'TX' => 'Texas',
    'UT' => 'Utah',
    'VT' => 'Vermont',
    'VA' => 'Virginia',
    'WA' => 'Washington',
    'WV' => 'West Virginia',
    'WI' => 'Wisconsin',
    'WY' => 'Wyoming',
  );
  return $state_options;
}
