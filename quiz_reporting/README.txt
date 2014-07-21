Overview
--------
Quiz Reporting module provides some reporting services for the the Quiz module
for Drupal. It uses the batch api (a standard part of Drupal) to divide the task up into
chunks, so that the process will have minimal (but not nonexistent) impact on the server,
and so the report will not time out.

Requirements
------------
Drupal 6.x
PHP 5.1 (I believe, may be higher, for the DBTNG module)
Quiz module
DBTNG module (http://drupal.org/project/dbtng)

Installation
------------
As with any Drupal module, install in modules directory, install DBTNG in modules folder.

Access
------
All pages rely on the "Export Quiz Results" permission.

Enabling
--------
Enable as any other module.

When the module is enabled, it will (as of time of writing), create three tables:

quiz_reporting_primary_business_index
quiz_reporting_state_index

These two tables are used to speed queries across fields which are in the profile_values
and profile_fields tables. When the tables are installed, any data from profile_state and
profile_primary_business are copied into these tables. This may take a few seconds; it hasn't
been tested against sites with a large number of profile fields and user records.

quiz_reporting_saved

This table holds users saved searches.

Generating report files
-----------------------

Reports can be run at admin/quiz/export_quiz_results.

The form is fairly self-explanatory. 

* Name. If you enter a name for the report when you generate it,
the specs for that report will be saved in quiz_reporting_saved. 

* batch size: a hidden field, batch_size determines how many records to process 
fore each batch. The field is set to 50, but testing may show that a larger or smaller
batch works better. The field can easily be made visible for testing.

* Date Range. Searches for users by their created date. The user needs to click the Filter by 
date box to enable date searching. The Start Year popup shows only the earliest year users 
were registered.

* State: If there is no profile_state field listed in profile_fields, this will not be 
displayed. It only displays states actually present in the database, using the 
quiz_reporting_state_index fields. It's a multi-select field, so the control or
command keys let the user specify multiple states. Selecting All States or not selecting 
any states causes the state field not to be searched.

* Primary Business. A free form field searches the profile_field profile_primary_business, 
and uses the quiz_reporting_primary_business table.

* Select Report. The reports are:

** All Registrants: a row in the file is created for each user which meets the other 
selection criteria.

** Certified: The file contains only records of users who have passed ALL the certifications,
as deterimined in the certify_conditions table. This is a complex query, and I am not totally
convinced it's done the right way, or the best way, and it may need revisiting. The 
query is in the function quiz_reporting_user_query() in the quiz_reporting_bath.inc file.

** Started - Not Finished: This report uses much of the same code as the Certified query,
but searches for users who have NOT passed all the tests.

** Not Started: this is done with a join from the users table to the quiz_node_results table
and shows users who have no entries in that table.

* Running the report

The report/export is triggered by the form. It uses the batch api to present a progress
bar to the user, and show how many lines have been and will be wirtten to the CSV file.
If there are no records to write, the module creates no file and notifies the user.
Once the file has been saved, the user is provided with a link to the file in the message
area, clicking this link downloads the file.

Reports are saved with a name with this PHP code:

str_replace(' ', '-', variable_get('site_name', '')) . '-' . $form_values['report'] . '-Results.csv';

If a file exists with a given report name, it is overwritten by the new file.

Saved Report Files
------------------

Reports are saved in the standard files directry, in a subdirectory named quiz_reporting.
Within that directory separate directories are created for each user_id who has run a 
report. This prevents one user from overwriting a report created by another user.

Users can view their saved reports at admin/quiz/export_quiz_files. File are listed
and can be downloaded by clicking on them. 

Saved Report queries
--------------------

Saved report specs or queries are saved in the tale quiz_reporting_saved. Users can view 
their saved specs at admin/quiz/export_saved_queries. The list shows the date the spec
was created and its name; clicking on it brings up the form, with the fields already filled
in. If the user changes anything on the form, and then runs the report again, the spec
is saved with the new settings. A hidden report_id field governs this.

Statistics
----------
A statistics page is at admin/quiz/monthly_stats. It shows, by month, the number of new
users, and the number of users who took a test that month. 

The stats is set to be run on a cron, and the cron is run every 86400 seconds (24 hours).
Results are stored in the variable quiz_reporting_stats, and the last run time is 
stored in quiz_reporting_last_run.

If cron hasn't be run, loading the stats page generates the query the first time, and
results are stored in the variables table. 

I've made a charting page, instead of a block, as the database I have doesn't show me
blocks I add! The page is at admin/quiz/user_stats.

NOTE: I added code to load jQuery 1.4.4 on this page, so that'll need to be done for
a block, too. Several files have been added to the quiz_reporting/jplot directory, 
including jquery1.4.4.min.js.

IMPORTANT: I had to change the jquery.jqplot.css file to add width: "auto !important;" to
on line 129. This is per http://stackoverflow.com/questions/6861210/jqplot-pie-chart-not-rendering-correctly
and https://bitbucket.org/cleonello/jqplot/issue/48/legends-doesnt-look-nice-when-using.
Otherwise, the legend did not show correctly.

Possible modifications
----------------------

* It may be that a better solution would be to use a separate table after all, to store
customized data about which quizzes the user has passed. I didn't find any obvious places
to hook into the module to do this, other than nodeapi, which might not be efficient. A 
way to do this may be to use Rules, Triggers, and Actions. Then we could Views-enable 
this new field.
* On the quiz reporting form, the user can enter a name, and the report spec will be saved 
into a table, allowing the spec to be recalled later. One modification might be to use that name
for the generated .csv file.
* Use Drupal tokens to help the user name the file.
* The Chosen widget (http://drupal.org/project/chosen) might be adaptable to be used 
with the state field on the report form to make it more obvious which states have 
been selected. I've used it with Drupal 7 fields, but not in a form like this.
* The Primary Business field could be made an ajax auto-complete field, or a 
restricted list with a popup.
* It would be fun, though probable overkill, to add a way for users to customize the cvs
file: choose which fields are on it, limit it to certain tests, etc. 