<?php

/**
 * Manages the batch process.
 */
function quiz_reporting_batch_process($form_values, $limit, &$context) {
  global $user;

  // Build query without a range(), so we can use its countQuery for the max.
  $query = quiz_reporting_user_query($form_values, $context);

  // $context holds values that persist across invocations.
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0; 
    $context['sandbox']['current_user'] = 0; // don't want users 0 or 1
    $context['sandbox']['max'] = $query->countQuery()->execute()->fetchField();
    $context['sandbox']['header_written'] = FALSE;
    $context['results']['report_id'] = $form_values['report_id'];
    $context['results']['start_time'] = time();
    $context['results']['count'] = 0;
    $context['results']['save_report'] = $form_values['save_report'];
    $context['results']['report_name'] = _quiz_reporting_create_report_name($form_values);
    $context['results']['stats'] = serialize(array());

    $export_path = file_create_path('quiz_reporting');
    if (!file_check_directory($export_path, FILE_CREATE_DIRECTORY)) {
      watchdog('Quiz Reporting', 'The directory %dir could not be created.', array('%dir' => $export_path), WATCHDOG_ERROR);
    }
    $export_path .= "/$user->uid";
    if (!file_check_directory($export_path, FILE_CREATE_DIRECTORY)) {
      watchdog('Quiz Reporting', 'The directory %dir could not be created.', array('%dir' => $export_path), WATCHDOG_ERROR);
    }

    $context['sandbox']['filepath'] = "$export_path/{$context['results']['report_name']}-Results.csv";
  }

  // Add the range to the query.
  $query->range($context['sandbox']['progress'], $limit);
  $result = $query->execute();

  // to store the stats
  $stats = unserialize($context['results']['stats']);

  // Create the file.
  if (!$context['sandbox']['header_written']) {
    $context['sandbox']['header_written'] = quiz_reporting_write_header($context['sandbox']['filepath'], $form_values['certificates']);
  }

  if ($context['sandbox']['max'] != 0) {
    $count = 0;
    foreach ($result as $row) {
      // progress is the status of the user
      $progress = quiz_reporting_write_row($row->uid, $context['sandbox']['filepath'], $form_values['certificates']);

      $stats[$progress]++;
      $context['results']['stats'] = serialize($stats);

      $context['results']['count']++;
      $count++;
      $context['sandbox']['current_user'] = $row->uid;
      $context['message'] = t('Now processing item %count of %max', array('%count' => $context['sandbox']['progress'], '%max' => $context['sandbox']['max']));
    }
    $context['sandbox']['progress'] = $context['sandbox']['progress'] + $count;
  } 
  else {
    // set this so we can skip right to finish if there are no records &
    // display a note to the user
    $context['results']['no_records'] = TRUE;
  }
  // Inform the batch engine that we are not finished,
  // and provide an estimation of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['results']['filepath'] = $context['sandbox']['filepath'];
    $context['results']['end_time'] = time();
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
  else {
    $context['results']['filepath'] = $context['sandbox']['filepath'];
  }
}

/**
 * Runs when the batch has finished.
 */
function quiz_reporting_batch_finished($success, $results, $operations) {
  // Show a link to the file.
  if ($success) {
    if ($results['no_records']) {
      $message = t('The search criteria produced no records. Please try again.');
    }
    else {
      drupal_set_message($results['count'] . ' items processed in ' . (time() - $results['start_time']) . ' seconds.');
      drupal_set_message("The file is ". basename($results['filepath']));
      $message = t('Click <a href="!url">here</a> to download it.', array(
        '!url' => url('admin/quiz/download/' . basename($results['filepath'])),
      ));
    }
    drupal_set_message($message);
  } 
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
}

/**
 * Modifies the user query.
 */
function quiz_reporting_user_query(&$form_values, &$context) {
  // Begin user query object.
  $user_query = db_select('users', 'u')
    ->fields('u', array('uid'))
    ->orderBy('uid', 'ASC')
    ->condition('u.uid', array(0, 1), 'NOT IN');
  $user_query->distinct();

  // Filter by state.
  if (!empty($form_values['state']) && !isset($form_values['state']['all'])) {
    $user_query->join('quiz_reporting_state_index', 'i', 'i.uid = u.uid');
    $user_query->condition('i.state', $form_values['state'], 'IN');
  }

  // Filter by business.
  if (!empty($form_values['primary_business'])) {
    $user_query->join('quiz_reporting_primary_business_index', 'bus_index', 'bus_index.uid = u.uid');
    $user_query->condition('bus_index.primary_business', $form_values['primary_business'], 'IN');
  }

  // Filter by registration date.
  if ($form_values['registration_date']['enabled']) {
    $start_date = mktime(0, 0, 0, $form_values['registration_date']['start_month'], 1, $form_values['registration_date']['start_year']);
    $end_date = mktime(0, 0, 0, $form_values['registration_date']['end_month'] + 1, 0, $form_values['registration_date']['end_year']);
    $user_query->condition('u.created', $start_date, '>');
    $user_query->condition('u.created', $end_date, '<');
  }

  // If report is unknown or All Registrants, skip the rest.
  if (empty($form_values['report']) || $form_values['report'] == 'all_registrants') {
    return $user_query;
  }

  // Assemble list of certificates.
  $certificates = $form_values['certificates'];
  if (empty($certificates)) {
    foreach (quiz_reporting_certificates() as $certificate) {
      $certificates[] = $certificate['cert_nid'];
    }
  }

  // Placeholder for filtered user IDs.
  $uids = array();

  // Assemble user IDs for each certificate.
  foreach ($certificates as $certicate_nid) {
    // Get quiz NIDs for certificate.
    $quizzes = array();
    $result = db_query("SELECT q.nid
      FROM {certify_conditions} cc
      LEFT JOIN {node} n ON cc.cid = n.nid
      LEFT JOIN {node} q ON cc.condnode = q.nid
      WHERE q.status = 1
        AND cc.cid = %d", $certicate_nid);
    while ($row = db_fetch_array($result)) {
      $quizzes[] = $row['nid'];
    }

    switch ($form_values['report']) {
      // Restrict to users who have not started any quizzes in this
      // certificate.
      case 'not_started':
        $placeholders = db_placeholders($quizzes);
        $result = db_query("SELECT DISTINCT uid
          FROM {quiz_node_results}
          WHERE nid IN ($placeholders)", $quizzes);
        while ($row = db_fetch_object($result)) {
          $uids[] = $row->uid;
        }
        break;

      // Restrict to users who have passed all quizzes in this certificate.
      case 'graduates':
        // Quizzes arguments.
        $args = $quizzes;
        $placeholders = db_placeholders($quizzes);

        // Filter by registration date.
        $date_condition = '';
        if ($form_values['graduation_date']['enabled']) {
          $args[] = mktime(0, 0, 0, $form_values['graduation_date']['start_month'], 1, $form_values['graduation_date']['start_year']);
          $args[] = mktime(0, 0, 0, $form_values['graduation_date']['end_month'] + 1, 0, $form_values['graduation_date']['end_year']);
          $date_condition = 'AND quiz_node_results.time_end BETWEEN %d AND %d';
        }

        // Total quizzes argument.
        $args[] = count($quizzes);

        $result = db_query("SELECT subquery2.uid
          FROM (
            SELECT quiz_node_results.uid, IF(MAX(quiz_node_results.score) >= quiz_node_properties.pass_rate, 1, 0) 'passed'
            FROM {quiz_node_results}
            INNER JOIN {quiz_node_properties} ON quiz_node_properties.nid = quiz_node_results.nid
            WHERE quiz_node_results.nid IN ($placeholders)
              AND quiz_node_results.score >= quiz_node_properties.pass_rate
              $date_condition
            GROUP BY quiz_node_results.uid, quiz_node_results.nid
          ) subquery2
          GROUP BY subquery2.uid
          HAVING COUNT(subquery2.passed) >= %d", $args);
        while ($row = db_fetch_object($result)) {
          $uids[] = $row->uid;
        }
        break;

      // Restrict to all users who have started quizzes in this certificate,
      // but not passed them.
      case 'not_finished':
        $placeholders = db_placeholders($quizzes);
        $result = db_query("SELECT subquery2.uid
          FROM (
            SELECT quiz_node_results.uid, IF(MAX(quiz_node_results.score) >= quiz_node_properties.pass_rate, 1, 0) 'passed'
            FROM {quiz_node_results}
            INNER JOIN {quiz_node_properties} ON quiz_node_properties.nid = quiz_node_results.nid
            WHERE quiz_node_results.nid IN ($placeholders)
            GROUP BY quiz_node_results.uid, quiz_node_results.nid
          ) subquery2
          GROUP BY subquery2.uid
          HAVING COUNT(subquery2.passed) < %d", array_merge($quizzes, array(count($quizzes))));
        while ($row = db_fetch_object($result)) {
          $uids[] = $row->uid;
        }
        break;
    }
  }

  // If no user IDs were found, add condition which returns 0 results.
  if (empty($uids)) {
    $user_query->condition(1, 0);
  }

  // Otherwise, filter by the user IDs we assembled above.
  else {
    $user_query->condition('u.uid', array_unique($uids), 'IN');
  }

  return $user_query;
}

/**
 * Gathers stats based on the form & saves them for later. Called by the batch
 * API after the csv is created.
 */
function quiz_reporting_save_stats($form_values, $report_id, $context){
  // If the user doesn't want to save the report, no need to run the stats.
  if (!$form_values['save_report']) {
    return;
  }

  // A state was selected.
  $state_join = '';
  $state_args = array();
  if (!empty($form_values['state']) && (!isset($form_values['state']['all']))) {
    $placeholders = db_placeholders($form_values['state'], 'text');
    $state_join = "INNER JOIN {quiz_reporting_state_index} ON quiz_reporting_state_index.uid = users.uid AND quiz_reporting_state_index.state IN ($placeholders)";
    $state_args = $form_values['state'];
  }

  // A business was selected.
  $business_join = '';
  $business_args = array();
  if (!empty($form_values['primary_business'])) {
    $placeholders = db_placeholders($form_values['primary_business']);
    $business_join = "INNER JOIN {quiz_reporting_primary_business_index} ON quiz_reporting_primary_business_index.uid = users.uid AND quiz_reporting_primary_business_index.primary_business IN ($placeholders)";
    $business_args = $form_values['state'];
  }

  // Filtered by date.
  $date_condition = '';
  if ($form_values['registration_date']['enabled']) {
    $start_date = strtotime($form_values['registration_date']['start_year'] . '-' . $form_values['registration_date']['start_month']);
    $end_date = strtotime($form_values['registration_date']['end_year'] . '-' . $form_values['registration_date']['end_month']);
    $date_condition = "AND users.created BETWEEN $start_date AND $end_date";
  }

  // Get total number of users.
  $stats['Total'] = db_result(db_query("SELECT COUNT(*)
    FROM {users}
    $state_join
    $business_join
    WHERE users.uid > 1
      $date_condition", array_merge($state_args, $business_args)));

  // Get NIDs of quizzes to pass for this certification.
  $quizzes_to_pass = quiz_reporting_quizzes_to_pass($form_values['certificate']);
  $quiz_placeholders = db_placeholders($quizzes_to_pass);

  // Get number of users who haven't started any of these quizzes.
  $stats['Not Started'] = db_result(db_query("SELECT COUNT(*)
    FROM {users}
    $state_join
    $business_join
    WHERE users.uid > 1
      $date_condition
      AND users.uid NOT IN (
        SELECT DISTINCT uid
        FROM {quiz_node_results}
        WHERE nid IN ($quiz_placeholders)
      )", array_merge($state_args, $business_args, $quizzes_to_pass)));

  // Get total number of users who have passed all of these quizzes.
  $stats['Certified'] = db_result(db_query("SELECT COUNT(*)
    FROM (
      SELECT subquery2.uid
      FROM (
        SELECT quiz_node_results.uid, IF(MAX(quiz_node_results.score) >= quiz_node_properties.pass_rate, 1, 0) 'passed'
        FROM {quiz_node_results}
        INNER JOIN {quiz_node_properties} ON quiz_node_properties.nid = quiz_node_results.nid
        INNER JOIN {users} ON quiz_node_results.uid = users.uid
        $state_join
        $business_join
        WHERE quiz_node_results.nid IN ($quiz_placeholders)
          $date_condition
        GROUP BY quiz_node_results.uid, quiz_node_results.nid
      ) subquery2
      GROUP BY subquery2.uid
      HAVING COUNT(subquery2.passed) >= %d
    ) subquery", array_merge($state_args, $business_args, $quizzes_to_pass, array(count($quizzes_to_pass)))));

  // A few more stats.
  $stats['Not Finished'] = $stats['Total'] - $stats['Certified'] - $stats['Not Started'];
  $stats['Not Certified'] = $stats['Total'] - $stats['Certified'];

  // Save the stats to the report.
  db_update('quiz_reporting_saved')
    ->fields(array(
      'user_stats_report' => serialize($stats),
      'cid' => $form_values['certificate'],
    ))
    ->condition('report_id', $report_id, '=')
    ->execute();
}
