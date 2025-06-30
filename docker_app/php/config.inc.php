<?php // -*-mode: PHP; coding:utf-8;-*-
namespace MRBS;

$timezone = "Europe/Berlin";
$dbsys = "mysql";
$db_host = "db";
$db_database = "mrbs";
$db_login = "mrbs";
$db_password = "mrbs";
$db_tbl_prefix = "mrbs_";
$db_persist = FALSE;

/* Style */

$theme = "modern";


/**********
 * Language
 **********/

// Set this to true to disable the automatic language changing MRBS performs
// based on the user's browser language settings. It will ensure that
// the language displayed is always the value of $default_language_tokens,
// as specified below
$disable_automatic_language_changing = true;

// Set this to a different language specifier to default to different
// language tokens. This must equate to a lang.* file in MRBS.
// e.g. use "fr" to use the translations in "lang.fr" as the default
// translations.  [NOTE: it is only necessary to change this if you
// have disabled automatic language changing above]
$default_language_tokens = "de";

// Website infos

$mrbs_company = "";

$vocab_override['de']['mrbs'] = "TC Nüsttal Platzbuchungssystem";

/* 

Calendar format 

*/


// Set this to true to add styling to weekend days
//$style_weekends = true;

// Start of week: 0 for Sunday, 1 for Monday, etc.
$weekstarts = 1;


$default_view_all = false;

// Define default starting view (month, week or day)
// Default is day
$default_view = "week";

// The default format for day names
$datetime_formats['day_name'] = array(
  'pattern' => 'cccc'
);

// The day and month as used in the header row of the week view
$datetime_formats['view_week_day_month'] = array(
  'skeleton' => 'dMMMM',
  'pattern' => 'MMMM d'
);

// To display the row labels (times, rooms or days) on the right hand side as well as the
// left hand side in the day and week views, set to true;
// (was called $times_right_side in earlier versions of MRBS)
//$row_labels_both_sides = true;

/*

Entry settings

*/

// Default brief description for new bookings
$default_name = "Buchung";


$edit_entry_field_order = array('name', 'booker_name', 'booker_email', 'description');

$vocab_override['de']['entry.booker_name'] = "Dein Name";
$vocab_override['de']['entry.booker_email'] = "Deine E-Mail-Adresse";
$vocab_override['de']['entry.description'] = "Kommentar";

$vocab_override['de']['fulldescription'] = "Kommentar";
$vocab_override['de']['fulldescription_short'] = "Kommentar";

$vocab_override['de']['rooms'] = "Platz";

$vocab_override['de']['username_or_email'] = "gast oder mitglied";


$vocab_override["de"]["type.I"] =     "Buchung";

$vocab_override["de"]["type.E"] =     "Training";



// The maximum length of a database field for which a text input can be used on a form
// (eg when editing a user or room).  If longer than this a text area will be used.
$text_input_max = 80;  // characters

// SETTINGS FOR BOOKING CONFIRMATION

// Allows bookings to be marked as "tentative", ie not yet 100% certain,
// and confirmed later.   Useful if you want to reserve a slot but at the same
// time let other people know that there's a possibility it may not be needed.
$confirmation_enabled = FALSE;

// Resolution - what blocks can be booked, in seconds.
// Default is half an hour: 1800 seconds.
$resolution = (60 * 60);  // DEFAULT VALUE FOR NEW AREAS

// The beginning of the first slot of the day (DEFAULT VALUES FOR NEW AREAS)
$morningstarts         = 7;   // must be integer in range 0-23
$morningstarts_minutes = 0;   // must be integer in range 0-59

// The beginning of the last slot of the day (DEFAULT VALUES FOR NEW AREAS)
$eveningends           = 23;  // must be integer in range 0-23
$eveningends_minutes   = 0;   // must be integer in range 0-59

// Set this to false if you do not want to have the ability to create events for which
// other people can register.
$enable_registration = false;


// DEFAULT VALUES FOR NEW AREAS

// Creating new bookings
$min_create_ahead_enabled = true;    // set to TRUE to enforce a minimum advance booking time
$max_create_ahead_enabled = true;    // set to TRUE to enforce a maximum advance booking time

// DEFAULT VALUES FOR NEW AREAS

// Creating new bookings
$min_create_ahead_secs = 0;           // (seconds) cannot book in the past
$max_create_ahead_secs = 60*60*24*7;  // (seconds) no more than one week ahead


/*

Admin settings

*/

// If you want only administrators to be able to make repeat bookings,
// set this variable to true
$auth['only_admin_can_book_repeat'] = true;

// If you want only administrators to be able to make bookings spanning
// more than one day, set this variable to true.
$auth['only_admin_can_book_multiday'] = true;

// If you want only administrators to be able to select multiple rooms
// on the booking form then set this to true.  (It doesn't stop ordinary users
// making separate bookings for the same time slot, but it does slow them down).
$auth['only_admin_can_select_multiroom'] = true;

// Set this to true if you want to restrict the ability to use the "Copy" button on
// the view_entry page to ordinary users viewing their own entries and to admins.
$auth['only_admin_can_copy_others_entries'] = true;

// If you don't want ordinary users to be able to see the other users'
// details then set this to true.  Used by the 'db' authentication scheme to determine
// whether to show other users to non-admins, and also generally to determine whether
// to create mailto: links, eg when viewing booking details.
$auth['only_admin_can_see_other_users'] = true;

// Or else you can allow them to see that there is a booking, but the
// details will be shown as private if you set this to true.
//$auth['force_private_for_guests'] = true;

// Global variable to determine whether the password can be reset
$allowPasswordReset=false;

// Global variable to determine whether users or only the admin can click on past timeslots with no entry
$auth['only_admin_can_click_past_slots_no_entry'] = true;

// Global variable to determine whether users or only the admin can click on timeslots with existing entries 
$auth['only_admin_can_click_slots_existing_entry'] = true;

// Determine whether users or only the admin can resize existing entries 
$auth['only_admin_can_resize_entries'] = true;
