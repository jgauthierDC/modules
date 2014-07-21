Drupal.behaviors.quizReportingForm = function(context) {

  // Certificate form: "all certificates" checkbox handling.
  $('input[id^=edit-certificates]').click(function() {
    var $all = $('input[id^=edit-certificates-all]');

    // If "all" was clicked, set checked state of remaining checkboxes to be
    // same as "all".
    if (this == $all.get(0)) {
      $('input[id^=edit-certificates]').not($all).attr('checked', $all.is(':checked'));
    }
    // If other checkbox was clicked, set checked state of "all" checkbox to be
    // true if they are all checked, or false if one is not checked.
    else {
      $all.attr('checked', !$('input[id^=edit-certificates]').not($all).not(':checked').length);
    }
  });

  // Export form: show Graduation Date only for Graduates report only.
  $('input[id^=edit-report]').click(function() {
    if ($('input[id^=edit-report]:checked').val() == 'graduates') {
      $('#graduation-date').show();
    }
    else {
      $('#graduation-date').hide();
    }
  }).triggerHandler('click');

};
