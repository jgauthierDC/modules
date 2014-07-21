<?php

// Bootstrap Drupal's database layer.
require_once './includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);

// Get certificates.
$certificates = array();
if (!empty($_REQUEST['cid'])) {
  $certificates[] = $_REQUEST['cid'];
}
else if (!empty($_REQUEST['cert'])) {
  $certificates[] = $_REQUEST['cert'];
}
else if (!empty($_REQUEST['certificates'])) {
  $certificates = $_REQUEST['certificates'];
}

// Get stats.
$placeholders = db_placeholders($certificates);
$result = db_query("SELECT *
  FROM {quiz_reporting_saved_stats}
  WHERE cert_nid IN ($placeholders)", $certificates);
while ($row = db_fetch_array($result)) {
  $vals = unserialize($row['vals']);
  $not_started = $vals['Not Started'];
  $not_finished = $vals['Not Finished'];
  $not_certified = $vals['Not Certified'];
  $certified = $vals['Certified'];
  $total = $vals['Total'];
}
?>
<script src="/sites/default/modules/quiz_reporting/jplot/jquery1.4.4.min.js" type="text/javascript"></script>
<script src="/sites/default/modules/quiz_reporting/jplot/jquery.jqplot.js?V" type="text/javascript"></script>
<script src="/sites/default/modules/quiz_reporting/jplot/plugins/jqplot.pieRenderer.min.js?V" type="text/javascript"></script>
<link type="text/css" rel="stylesheet" media="all" href="/sites/default/modules/quiz_reporting/jplot/jquery.jqplot.css" />    
<div style="height: 300px; width: 500px; position: relative;" id="ThisCert" class="jqplot-target">
<script type="text/javascript">
	$(document).ready(function(){
		var data = [
		['Not Started (<?php print $not_started; ?>)', <?php print $not_started; ?>],
		['Completed (<?php print $certified; ?>)', <?php print $certified; ?>],
		
		['Not Completed (<?php print $not_finished; ?>)', <?php print $not_finished; ?>],
		//['Users (<?php print $total; ?>)', <?php print $total; ?>],
		
		
		];
		var plot1 = jQuery.jqplot ('ThisCert', [data], 
		{ 
		  title:'All Time Registrants ' + '(<?php print $total; ?>)' ,
		  seriesDefaults: {
			// Make this a pie chart.
			renderer: jQuery.jqplot.PieRenderer, 
			rendererOptions: {
			  // Put data labels on the pie slices.
			  // By default, labels show the percentage of the slice.
			  showDataLabels: true
			}
		  }, 
		  legend: { show:true, location: 'e' }
		}
		);
	});
	</script>
</div>
