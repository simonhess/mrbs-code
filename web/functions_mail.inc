<?php
declare(strict_types=1);
namespace MRBS;

use PHPMailer\PHPMailer\PHPMailer;

// +---------------------------------------------------------------------------+
// | Meeting Room Booking System.                                              |
// +---------------------------------------------------------------------------+
// | Functions dedicated to emails handling.                                   |
// |---------------------------------------------------------------------------+
// | I keeped these functions in a separated file to avoid burden the main     |
// | function.inc files if emails are not used.                                |
// |                                                                           |
// | USE : This file should be included in all files where emails functions    |
// |        are likely to be used.                                             |
// +---------------------------------------------------------------------------+
//

define('MAIL_EOL', "\r\n");  // See RFC 5322 2.1

global $mail_settings;

if ($mail_settings['icalendar'])
{
  require_once "functions_ical.inc";
}


// Determines whether an email might need to be sent
function need_to_send_mail() : bool
{
  global $mail_settings;

  return ($mail_settings['admin_on_bookings'] or
          $mail_settings['area_admin_on_bookings'] or
          $mail_settings['room_admin_on_bookings'] or
          $mail_settings['booker'] or
          $mail_settings['book_admin_on_approval']);
}


function get_mail_locale() : string
{
  global $mail_settings;

  return $mail_settings['admin_lang'] ?? 'en';
}


function get_mail_os_locale()
{
  static $mail_os_locale;

  if (!isset($mail_os_locale))
  {
    $mail_os_locale = System::getOSlocale(get_mail_locale());
  }

  return $mail_os_locale;
}

// Get localized (for email) field name for a user defined table column
// Looks for a tag of the format tablename.columnname (where tablename is
// stripped of the table prefix) and if it can't find a string for that tag will
// return the column name
// TODO: It's actually returning tablename.columnname at the moment if it
// TODO: can't find a tag, rather than just the columnname.  Probably need
// TODO: to restructure the way get_vocab() etc work.
function get_mail_field_name(string $table, string $name) : string
{
  return get_mail_vocab(get_table_short_name($table) . ".$name");
}

// }}}
// {{{ getMailPeriodDateString()

/**
 * Format a timestamp in non-unicode output (for emails).
 *
 * @param   int         $t
 * @param   int         $mod_time
 * @return  array
 */
function getMailPeriodDateString(int $t, int $mod_time=0) : array
{
  global $periods, $datetime_formats;

  $time = getdate($t);
  $p_num = $time['minutes'] + $mod_time;

  // Make sure we have a valid period [should this be necessary??]
  if ($p_num < 0)
  {
    $p_num = 0;
  }

  if ($p_num > count($periods) - 1)
  {
    $p_num = count($periods) - 1;
  }

  $mailperiod = $periods[$p_num];

  // The separator is a ',' as a '-' leads to an ambiguous display in report.php when showing end times.
  return array($p_num, $mailperiod . ', ' . datetime_format($datetime_formats['date'], $t, get_mail_locale()));
}

// }}}
// {{{ getMailTimeDateString()

/**
 * Format a timestamp in non-unicode output (for emails).
 *
 * @param   int         $t         timestamp to format
 * @param   boolean     $inc_time  include time in return string
 * @return  string                 formatted string
 */
function getMailTimeDateString(int $t, bool $inc_time=true) : string
{
  global $datetime_formats;

  $format = ($inc_time) ? $datetime_formats['date_and_time'] : $datetime_formats['date'];
  return datetime_format($format, $t, get_mail_locale());
}


function getMailDateString($time)
{
  global $enable_periods;

  if ($enable_periods)
  {
    list($entry_period, $entry_date) = getMailPeriodDateString($time);
  }
  else
  {
    $entry_date = getMailTimeDateString($time);
  }
  return $entry_date;
}


// get_address_list($array)
//
// Takes an array of email addresses and returns a comma separated
// list of addresses with duplicates removed.
function get_address_list(array $address_strings) : string
{
  // Remove any leading and trailing whitespace and any empty strings
  $trimmed_array = array();
  foreach ($address_strings as $address_string)
  {
    $address_string = trim($address_string);
    if ($address_string !== '')
    {
      // Use parseAddresses() to validate the address because it could contain a display name
      if (count(PHPMailer::parseAddresses($address_string, true, get_mail_charset())) == 0)
      {
        $message = 'Invalid email address "' . $address_string . '"';
        mail_debug($message);
        trigger_error($message, E_USER_NOTICE);
      }
      $trimmed_array[] = $address_string;
    }
  }
  // remove duplicates
  $trimmed_array = array_unique($trimmed_array);
  // re-assemble the string
  return implode(',', $trimmed_array);
}


// Get the admin email address(es), provided that the config settings allow it.
function get_admin_email() : string
{
  global $mail_settings;

  return ($mail_settings['admin_on_bookings'] && isset($mail_settings['recipients'])) ? $mail_settings['recipients'] : '';
}


// get the list of email addresses that are allowed to approve bookings
// for the room with id $room_id
// (At the moment this is just the admin email address, but this could
// be extended.)
function get_approvers_email($room_id) : string
{
  return get_admin_email();
}


// Get the area_admin_email for an entry $id
// If $series is set this is an entry in the repeat table, otherwise the entry table
// Returns an empty string in the case of an error
function get_area_admin_email($id, bool $series=false) : string
{
  $id_table = ($series) ? "rep" : "e";

  $sql = "SELECT a.area_admin_email ";
  $sql .= "FROM " . _tbl('room') . " r, " . _tbl('area') . " a, " . _tbl('entry') . " e ";
  // If this is a repeating entry...
  if ($id_table == 'rep')
  {
    // ...use the repeat table
    $sql .= ", " . _tbl('repeat') . " rep ";
  }
  $sql .= "WHERE {$id_table}.id=?
             AND r.id={$id_table}.room_id
             AND a.id=r.area_id
           LIMIT 1";
  $email = db()->query_scalar_non_bool($sql, array($id));

  return (!isset($email) || ($email === false)) ? '' : $email;
}


// Get the room_admin_email for an entry $id
// If $series is set this is an entry in the repeat table, otherwise the entry table
// Returns an empty string in the case of an error
function get_room_admin_email($id, bool $series=false) : string
{
  $id_table = ($series) ? "rep" : "e";

  $sql = "SELECT r.room_admin_email ";
  $sql .= "FROM " . _tbl('room') . " r, " . _tbl('entry') . " e ";
  // If this is a repeating entry...
  if ($id_table == 'rep')
  {
    // ...use the repeat table
    $sql .= ", " . _tbl('repeat') . " rep ";
  }
  $sql .= "WHERE {$id_table}.id=?
             AND r.id={$id_table}.room_id
           LIMIT 1";
  $email = db()->query_scalar_non_bool($sql, array($id));

  return (!isset($email) || ($email === false)) ? '' : $email;
}


// Create a row of a table in either plain text or HTML format.
// Plain text:  returns "$label: $new" . MAIL_EOL
// HTML:        returns "<tr><td>$label: </td><td>$new</td></tr>" . MAIL_EOL
// $new and $old can be of type null|int|float|string (union declarations are not supported until PHP 8.0).
// If $compare is TRUE then a third column is output with $old in parentheses
function create_body_table_row(string $label, $new, $old, bool $compare=false, bool $as_html=false) : string
{
  $new = $new ?? '';
  $old = $old ?? '';

  $result  = ($as_html) ? '<tr>' . MAIL_EOL : '';

  // The label
  $result .= ($as_html) ? '<td>' : '';
  $result .= ($as_html) ? escape_html("$label: ") : "$label: ";
  $result .= ($as_html) ? '</td>' . MAIL_EOL : '';
  // The new value
  $result .= ($as_html) ? '<td>' : '';
  $result .= ($as_html) ? escape_html($new) : "$new";
  $result .= ($as_html) ? '</td>' . MAIL_EOL : '';
  // The old value (if we're doing a comparison)
  if ($compare)
  {
    $result .= ($as_html) ? '<td>' : '';
    if ($new == $old)
    {
      $result .= ($as_html) ? "&nbsp;" : '';
    }
    else
    {
      // Put parentheses around the HTML version as well as the plain text
      // version in case the table is not rendered properly in HTML.  The
      // parentheses will make the old value stand out.
      $result .= ($as_html) ? escape_html(" ($old)") : " ($old)";
    }
    $result .= ($as_html) ? '</td>' . MAIL_EOL : '';
  }

  $result .= ($as_html) ? '</tr>' : '';
  $result .= MAIL_EOL;
  return $result;
}


// Generate a list of dates from an array of start times
//
//   $dates      an array of start times
//   $as_html    (boolean) whether the list should be HTML or plain text
function create_date_list(array $dates, bool $as_html) : string
{
  $result = ($as_html) ? '<ul>' . MAIL_EOL : '';
  foreach ($dates as $date)
  {
    $result .= ($as_html) ? '<li>' : '';
    $date_string = getMailDateString($date);
    $result .= ($as_html) ? escape_html($date_string) : $date_string;
    $result .= ($as_html) ? '</li>' : '';
    // The newline is important to stop the line length exceeding 998 characters,
    // which will happen if there are a lot of dates.  See RFC 5322 2.1.1.
    $result .= MAIL_EOL;
  }
  $result .= ($as_html) ? '</ul>' . MAIL_EOL : '';
  return $result;
}


// Generate a list of repeat dates for a series
//
// $reps is an array of start_times that have been created/modified/deleted.
function create_repeat_list(array $data, $action, bool $as_html, array $reps) : string
{
  if (($data['repeat_rule']->getType() == RepeatRule::NONE) ||
       in_array($action, array('more_info', 'remind')))
  {
    return '';
  }

  // The introductory text
  $result = ($as_html) ? '<p>' : MAIL_EOL . MAIL_EOL;
  if (($action == "delete") || ($action == "reject"))
  {
    $result .= get_vocab("mail_body_repeats_deleted");
  }
  else
  {
    $result .= get_vocab("mail_body_repeats_booked");
  }
  $result .= ($as_html) ? '</p>' . MAIL_EOL : MAIL_EOL . MAIL_EOL;

  $result .= create_date_list($reps, $as_html);

  // Now add in the list of repeat bookings that could not be booked
  if (!empty($data['skip_list']))
  {
    // The introductory text
    $result .= ($as_html) ? '<p>' : MAIL_EOL . MAIL_EOL;
    $result .= get_vocab("mail_body_exceptions");
    $result .= ($as_html) ? '</p>' . MAIL_EOL : MAIL_EOL . MAIL_EOL;
    // Now the list of conflicts
    $result .= create_date_list($data['skip_list'], $as_html);
  }

  return $result;
}


// $start_times is an array of start_times that have been created/modified/deleted.
// If not specified the function works them out for itself from the repeat data
function create_body(array $data, ?array $mail_previous, bool $compare, bool $series, string $action, array $start_times, bool $as_html=false, ?string $note=null) : string
{
  global $mrbs_company;
  global $enable_periods, $approval_enabled, $confirmation_enabled;
  global $mail_settings, $standard_fields;
  global $select_options, $booking_types;

  // If we haven't got a previous entry just give it one.   It won't get used,
  // but will prevent a series if undefined index notices.
  if (empty($mail_previous))
  {
    $mail_previous = $data;
  }

  // set up the body
  $body = "";

  if ($as_html)
  {
    $body .= DOCTYPE . MAIL_EOL;
    $body .= '<html lang="' . convert_to_BCP47(get_mail_lang()) . '">' . MAIL_EOL;
    $body .= '<head>' . MAIL_EOL;
    $body .= '<meta http-equiv="Content-Type" content="text/html; charset=' . get_mail_charset() . '">' . MAIL_EOL;
    $body .= '<title>' . escape_html($mrbs_company) . '</title>' . MAIL_EOL;
    $body .= '<style type="text/css">' . MAIL_EOL;
    $css_file = 'css/mrbs-mail.css.php';
    if (is_file($css_file) && is_readable($css_file))
    {
      ob_start();
      include $css_file;
      $css = ob_get_clean();
      //  Remove any whitespace from the beginning
      $css = preg_replace('/^\s+/', '', $css);
      // Remove comments
      $css = preg_replace('!/\*.*?\*/!s', '', $css);
      // Remove blank lines and also replace all new line sequences with the preferred
      // EOL sequence - hence the '+'.  Note that the CSS file will probably have Unix LF
      // endings, so these will need to be converted.
      $css = preg_replace("/(?:\R\h*)+/", MAIL_EOL, $css);
      $body .= $css;
    }
    $body .= '</style>' . MAIL_EOL;
    $body .= '</head>' . MAIL_EOL;
    $body .= '<body id="mrbs">' . MAIL_EOL;
    $body .= '<div id="header">' . escape_html($mrbs_company . ' - ' . get_mail_vocab('mrbs')) . '</div>' . MAIL_EOL;
    $body .= '<div id="contents">' . MAIL_EOL;
  }

  $body .= ($as_html) ? "<p>" : "";

  $mrbs_user = session()->getCurrentUser();
  if (isset($mrbs_user))
  {
    $user_escaped = ($as_html) ? escape_html($mrbs_user->display_name) : $mrbs_user->display_name;
  }
  else
  {
    $user_escaped = '';
  }

  switch ($action)
  {
    case "approve":
      $body .= get_mail_vocab("mail_body_approved", $user_escaped);
      break;
    case "more_info":
      $body .= get_mail_vocab("mail_body_more_info", $user_escaped);
      $body .= ($as_html) ? '</p><p>' : MAIL_EOL . MAIL_EOL;
      $body .= get_mail_vocab("info_requested") . ": ";
      $body .= $note ?? '';
      break;
    case "remind":
      $body .= get_mail_vocab("mail_body_reminder");
      break;
    case "reject":
      $body .= get_mail_vocab("mail_body_rej_entry", $user_escaped);
      $body .= ($as_html) ? '</p><p>' : MAIL_EOL . MAIL_EOL;
      $body .= get_mail_vocab("reason") . ': ';
      $body .= $note ?? '';
      break;
    case "delete":
      $body .= get_mail_vocab("mail_body_del_entry", $user_escaped);
      break;
    default:
      if ($compare)
      {
        $body .= get_mail_vocab("mail_body_changed_entry", $user_escaped);
      }
      else
      {
        $body .= get_mail_vocab("mail_body_new_entry", $user_escaped);
      }
      break;
  }

  // Create a link to the entry, unless we're deleting it of course,
  // because then there won't be one.
  if (($action != "delete") && ($action != "reject"))
  {
    $body .= ($as_html) ? '</p><p>' : MAIL_EOL . MAIL_EOL;
    $body .= ($as_html) ? '<a target="_blank" href="' : '';
    // Set the link to view entry page
    $body .= url_base() . multisite('view_entry.php?id=' . $data['id']);
    if ($series)
    {
      $body .= '&series=1';
    }
    $body .= ($as_html) ? '">' . escape_html($data['name']) . '</a>' : '';
  }
  $body .= ($as_html) ? '</p>' . MAIL_EOL : MAIL_EOL . MAIL_EOL;

  $body .= ($as_html) ? '<table>' : '';
  $body .= MAIL_EOL;

  if ($compare && $as_html)
  {
    $body .= '<thead>' . MAIL_EOL;
    $body .= '<tr>' . MAIL_EOL;
    $body .= '<th>&nbsp;</th>' . MAIL_EOL;
    $body .= '<th>' . get_vocab("new_value") . '</th>' . MAIL_EOL;
    $body .= '<th>(' . get_vocab("old_value") . ')</th>' . MAIL_EOL;
    $body .= '</tr>' . MAIL_EOL;
    $body .= '</thead>' . MAIL_EOL;
  }

  $body .= ($as_html) ? '<tbody>' . MAIL_EOL : '';


  // Always display the brief description
  $body .= create_body_table_row (get_mail_vocab("namebooker"),
                                  $data['name'],
                                  $mail_previous['name'],
                                  $compare, $as_html);

  // Displays/don't displays entry details
  if ($mail_settings['details'])
  {
    // Description:
    $body .= create_body_table_row (get_mail_vocab("description"),
                                    $data['description'],
                                    $mail_previous['description'],
                                    $compare, $as_html);

    if ($confirmation_enabled)
    {
      // Confirmation status:
      $new_status = ($data['tentative']) ? get_mail_vocab("tentative") : get_mail_vocab("confirmed");
      $old_status = ($mail_previous['tentative']) ? get_mail_vocab("tentative") : get_mail_vocab("confirmed");
      $body .= create_body_table_row (get_mail_vocab("confirmation_status"),
                                      $new_status,
                                      $old_status,
                                      $compare, $as_html);
    }

    if ($approval_enabled)
    {
      // Approval status:
      $new_status = ($data['awaiting_approval']) ? get_mail_vocab("awaiting_approval") : get_mail_vocab("approved");
      $old_status = ($mail_previous['awaiting_approval']) ? get_mail_vocab("awaiting_approval") : get_mail_vocab("approved");
      $body .= create_body_table_row (get_mail_vocab("approval_status"),
                                      $new_status,
                                      $old_status,
                                      $compare, $as_html);
    }

    // Room:
    $new_room = $data['area_name'] . " - " . $data['room_name'];
    $old_room = $mail_previous['area_name'] . " - " . $mail_previous['room_name'];
    $body .= create_body_table_row (get_mail_vocab("room"),
                                    $new_room,
                                    $old_room,
                                    $compare, $as_html);

    // Start time
    $body .= create_body_table_row (get_mail_vocab("start_date"),
                                    getMailDateString($data['start_time']),
                                    getMailDateString($mail_previous['start_time']),
                                    $compare, $as_html);

    // Duration
    $new_duration = $data['duration'] . " " . get_mail_vocab($data['dur_units']);
    $old_duration = $mail_previous['duration'] . " " . get_mail_vocab($mail_previous['dur_units']);
    $body .= create_body_table_row (get_mail_vocab("duration"),
                                    $new_duration,
                                    $old_duration,
                                    $compare, $as_html);

    // End time
    $this_endtime = $data['end_time'];
    $previous_endtime = ($compare) ? $mail_previous['end_time'] : 0;
    if ($enable_periods)
    {
      // If we are using periods then the end_time is the end of the last
      // period.   We therefore need to subtract 60 seconds from it so that
      // we get the name of that period, rather than the name of the next one.
      $this_endtime = $this_endtime - 60;
      $previous_endtime = $previous_endtime - 60;
    }
    $body .= create_body_table_row (get_mail_vocab("end_date"),
                                    getMailDateString($this_endtime),
                                    getMailDateString($previous_endtime),
                                    $compare, $as_html);

    // Type of booking
    if (isset($booking_types) && (count($booking_types) > 1))
    {
      $body .= create_body_table_row (get_mail_vocab("type"),
                                      get_type_vocab($data['type']),
                                      get_type_vocab($mail_previous['type']),
                                      $compare, $as_html);
    }

    // Created by
    $body .= create_body_table_row (get_mail_vocab("createdby"),
                                    auth()->getDisplayName($data['create_by']),
                                    auth()->getDisplayName($mail_previous['create_by']),
                                    $compare, $as_html);

    // Custom fields
    $columns = Columns::getInstance(_tbl('entry'));
    foreach ($columns as $column)
    {
      if (!in_array($column->name, $standard_fields['entry']))
      {
        $key = $column->name;
        $value = $data[$key];
        // Convert any booleans or pseudo-booleans to text strings (in the mail language)
        if ($column->isBooleanLike())
        {
          $value = ($value) ? get_mail_vocab("yes") : get_mail_vocab("no");
          if ($compare)
          {
            $mail_previous[$key] = ($mail_previous[$key]) ? get_mail_vocab("yes") : get_mail_vocab("no");
          }
        }
        // For any associative arrays we want the value rather than the key
        if (isset($select_options["entry.$key"]) &&
            is_assoc($select_options["entry.$key"]) &&
            array_key_exists($value, $select_options["entry.$key"]))
        {
          $value = $select_options["entry.$key"][$value];
          if ($compare &&
              array_key_exists($mail_previous[$key], $select_options["entry.$key"]))
          {
            $mail_previous[$key] = $select_options["entry.$key"][$mail_previous[$key]];
          }
        }
        $body .= create_body_table_row (get_mail_field_name(_tbl('entry'), $key),
                                        $value,
                                        ($compare) ? $mail_previous[$key] : '',
                                        $compare, $as_html);
      }
    }

    // Last updated
    $body .= create_body_table_row (get_mail_vocab("lastupdate"),
                                    getMailTimeDateString(time()),
                                    ($compare) ? getMailTimeDateString($mail_previous['last_updated']) : '',
                                    $compare, $as_html);

    // Repeat Type
    $body .= create_body_table_row (get_mail_vocab("rep_type"),
                                    get_mail_vocab("rep_type_" . $data['repeat_rule']->getType()),
                                    get_mail_vocab("rep_type_" . $mail_previous['repeat_rule']->getType()),
                                    $compare, $as_html);

    // Details if a series
    if ($data['repeat_rule']->getType() != RepeatRule::NONE)
    {

      if ($data['repeat_rule']->getType() == RepeatRule::WEEKLY)
      {
        // Repeat days
        // Display day names according to language and preferred weekday start.
        $opt = $data['repeat_rule']->getDaysAsNames(true);
        $opt_previous = ($compare) ? $mail_previous['repeat_rule']->getDaysAsNames(true) : '';
        $body .= create_body_table_row (get_mail_vocab("rep_rep_day"),
                                        $opt,
                                        $opt_previous,
                                        $compare, $as_html);
      }

      if ($data['repeat_rule']->getType() == RepeatRule::MONTHLY)
      {
        $previous_repeat_day = ($mail_previous['repeat_rule']->getType() == RepeatRule::MONTHLY) ? get_monthly_repeat_day($mail_previous) : '';
        $body .= create_body_table_row (get_mail_vocab("repeat_on"),
                                        get_monthly_repeat_day($data),
                                        $previous_repeat_day,
                                        $compare, $as_html);
      }

      // Repeat interval
      $repeat_interval = $data['repeat_rule']->getInterval();
      $new = $repeat_interval . ' ' . $data['repeat_rule']->getIntervalUnits(true);

      $previous_repeat_interval = $mail_previous['repeat_rule']->getInterval();
      if (isset($previous_repeat_interval))
      {
        $old = $previous_repeat_interval . ' ' . $mail_previous['repeat_rule']->getIntervalUnits(true);
      }
      else
      {
        $old = '';
      }

      $body .= create_body_table_row (get_mail_vocab("rep_interval"),
                                      $new,
                                      $old,
                                      $compare, $as_html);

      // Repeat end date
      $end_previous = ($mail_previous['repeat_rule']->getType() == RepeatRule::NONE) ? '' : getMailTimeDateString($mail_previous['repeat_rule']->getEndDate()->getTimestamp(), false);
      $body .= create_body_table_row (get_mail_vocab("rep_end_date"),
                                      getMailTimeDateString($data['repeat_rule']->getEndDate()->getTimestamp(), false),
                                      $end_previous,
                                      $compare, $as_html);

    }
  }

  if ($as_html)
  {
    $body .= '</tbody>' . MAIL_EOL;
    $body .= '</table>' . MAIL_EOL;
  }

  // Add in a list of repeat dates.  Although we've given them the repeat characteristics
  // above, it's often helpful to have this expanded out into a list of actual dates to
  // avoid any confusion.    The repeat list also gives a list of dates that could not
  // be booked due to conflicts.
  if ($data['repeat_rule']->getType() != RepeatRule::NONE)
  {
    $body .= create_repeat_list($data, $action, $as_html, $start_times);
  }

  if ($as_html)
  {
    $body .= '</div>' . MAIL_EOL;
    $body .= '</body>' . MAIL_EOL;
    $body .= '</html>' . MAIL_EOL;
  }

  return $body;
}


// Merges a comma separated string of addresses (could be a single address) into
// an array of addresses
// TODO: simplify all address handling
function address_merge(array $addresses, string $address_string) : array
{
  $result = $addresses;

  $new_addresses = PHPMailer::parseAddresses($address_string, true, get_mail_charset());
  $mailer = new PHPMailer();
  $mailer->CharSet = get_mail_charset();

  foreach ($new_addresses as $new_address)
  {
    $result[] = $mailer->addrFormat(array($new_address['address'], $new_address['name']));
  }

  return $result;
}


// create_addresses($data, $action)
//
// Returns an array indexed by 'from', 'reply_to', 'to' and 'cc' with each element
// consisting of a comma separated list of email addresses.
//
// Parameters:
//   $data     an array containing all the data concerning this booking
//   $action   the action that has caused this email to be sent
//
function create_addresses(array $data, string $action) : array
{
  global $approval_enabled, $mail_settings;

  $reply_to = array();
  $to = array();
  $cc = array();

  if (!empty($mail_settings['cc']))
  {
    $cc = address_merge($cc, $mail_settings['cc']);
  }

  $mrbs_user = session()->getCurrentUser();
  if (isset($mrbs_user))
  {
    // Set the Reply-To address
    if ($mail_settings['use_reply_to'] && !empty($mrbs_user->email))
    {
      $reply_to[] = $mrbs_user->mailbox();
    }
    // Set the From address.  If this is a reminder email or a request for more info,
    // then set the From address to be the user's - unless we've configured MRBS not
    // to do this (to avoid the email being rejected as spam), in which case we'll
    // put the user's address on the Cc line, which will enable the recipient to
    // use it in a reply.
    if (in_array($action, array('more_info', 'remind')))
    {
      if ($mail_settings['use_from_for_all_mail'])
      {
        $cc[] = $mrbs_user->mailbox();
      }
      else
      {
        $from = $mrbs_user->mailbox();
      }
    }
  }

  if (empty($from))
  {
    $from = (isset($mail_settings['from'])) ? $mail_settings['from'] : null;
  }

  // if we're requiring bookings to be approved and this user needs approval
  // for this room, then get the email addresses of the approvers
  if (!in_array($action, array('delete', 'reject')) &&
      $approval_enabled &&
      !is_book_admin($data['room_id']))
  {
    $email = get_approvers_email($data['room_id']);
    if (!empty($email))
    {
      $to = address_merge($to, $email);
    }
  }

  $to = address_merge($to, get_admin_email());

  if ($mail_settings['area_admin_on_bookings'])
  {
    // Look for list of area admins email addresses
    if (empty($data['area_admin_email']))
    {
      $email = get_area_admin_email($data['id'], ($data['repeat_rule']->getType() != RepeatRule::NONE));
    }
    else
    {
      $email = $data['area_admin_email'];
    }
    if (!empty($email))
    {
      $to = address_merge($to, $email);
    }
  }

  if ($mail_settings['room_admin_on_bookings'])
  {
    // Look for list of room admins email addresses
    if (empty($data['room_admin_email']))
    {
      $email = get_room_admin_email($data['id'], ($data['repeat_rule']->getType() != RepeatRule::NONE));
    }
    else
    {
      $email = $data['room_admin_email'];
    }
    if (!empty($email))
    {
      $to = address_merge($to, $email);
    }
  }

  if ($mail_settings['booker'])
  {
    if (in_array($action, array("approve", "more_info", "reject")))
    {
      // Put the addresses on the cc line and the booker will go
      // on the to line
      $cc = array_merge($cc, $to);
      $to = array();
    }
    $booker = auth()->getUser($data['create_by']);
    if (!empty($booker->email))
    {
      // $to[] = $booker->mailbox();
      $to[] = $data['booker_email'];
    }
  }

  $addresses = array();
  $addresses['from'] = $from;
  $addresses['reply_to'] = (empty($reply_to)) ? '' : get_address_list($reply_to);
  $addresses['to'] = (empty($to)) ? '' : get_address_list($to);
  $addresses['cc'] = (empty($cc)) ? '' : get_address_list($cc);

  return $addresses;
}


// }}}
// {{{ notifyAdminOnBooking()

/**
 * Send email to administrator to notify a new/changed entry.
 *
 * @param array   $data          contains the data for this entry
 * @param array   $mail_previous contains the data for the previous entry, or is an empty array
 * @param bool    $new_entry     whether this is a new entry or not
 * @param bool    $series        whether this is a series or not
 * @param array   $start_times   an array of start times that have been made
 * @param string  $action        the booking action (eg "delete", "more_info", etc.)
 * @param string  $note          a note that is used with "more_info"
 */
function notifyAdminOnBooking(array $data, array $mail_previous, bool $new_entry, bool $series, array $start_times, string $action='book', ?string $note=null) : void
{
  global $mail_settings, $enable_periods;
  global $mrbs_company;

  mail_debug('Preparing email for new or changed booking ...');

  // Add some values to the $data array before we go and create the addresses
  if (!$new_entry)
  {
    $data['area_admin_email'] = (!empty($mail_previous['area_admin_email'])) ? $mail_previous['area_admin_email'] : NULL;
    $data['room_admin_email'] = (!empty($mail_previous['room_admin_email'])) ? $mail_previous['room_admin_email'] : NULL;
  }

  // Set up the addresses (from, to and cc)
  $addresses = create_addresses($data, $action);
  if (empty($addresses['to']) && empty($addresses['cc']))
  {
    mail_debug('Email abandoned: no addresses.');
    return;
  }

  // Set up the subject
  //
  // If we're sending iCalendar notifications, then it seems that some calendar
  // applications use the email subject as the booking title instead of the iCal
  // SUMMARY field.   As far as I can see this is wrong, but as a circumvention
  // we'll put the booking title in the email subject line.   (See also
  // SF Tracker id 3297799)
  if ($mail_settings['icalendar'] && !$enable_periods)
  {
    $subject = $data['name'];
  }
  else
  {
    switch ($action)
    {
      case "approve":
        $subject = get_mail_vocab("mail_subject_approved", $mrbs_company);
        break;
      case "more_info":
        $subject = get_mail_vocab("mail_subject_more_info", $mrbs_company);
        break;
      case "remind":
        $subject = get_mail_vocab("mail_subject_reminder", $mrbs_company);
        break;
      default:
        if ($new_entry)
        {
          $subject = get_mail_vocab("mail_subject_new_entry", $mrbs_company);
        }
        else
        {
          $subject = get_mail_vocab("mail_subject_changed_entry", $mrbs_company);
        }
        break;
    }
  }

  // Create the text body
  $compare = !$new_entry;
  $text_body = create_body($data, $mail_previous, $compare, $series, $action, $start_times, false, $note);

  // Create the HTML body
  if ($mail_settings['html'])
  {
    $html_body = create_body($data, $mail_previous, $compare, $series, $action, $start_times, true, $note);
  }

  // Create the iCalendar if required
  // Don't add an iCalendar if we're using periods, because MRBS doesn't know when periods
  // occur in the day, so can't construct a VEVENT.
  // Don't add an iCalendar if this is a reminder or a request for more info because then
  // the recipient probably won't be able to reply to the email: we just want an ordinary
  // email and not a calendar notification.
  $attachment = array();
  if ($mail_settings['icalendar'] &&
      !$enable_periods &&
      !in_array($action, array('more_info', 'remind')))
  {
    $attachment['method']   = "REQUEST";
    $ical_components = array();
    // TODO: the addresses have by this stage been MIME-encoded.  It would probably be better to
    // TODO: pass unencoded addresses to create_ical_event() so that we don't have to decode them,
    // TODO: as mb_decode_mimeheader() isn't guaranteed to exist.
    $ical_components[] = create_ical_event($attachment['method'], $data, $addresses, $series);
    $attachment['content']  = create_icalendar($attachment['method'], $ical_components);
    $attachment['name']     = $mail_settings['ics_filename'] . ".ics";
  }

  MailQueue::add(
      $addresses,
      $subject,
      $text_body,
      $html_body ?? null,
      $attachment,
      get_mail_charset()
    );
}

// }}}
// {{{ notifyAdminOnDelete()

/**
 * Send email to administrator to notify a new/changed entry.
 *
 * @param   array   $data      contains deleted entry data for email body
 * @param   array   $start_times an array of start times that have been deleted
 * @param   bool    $series    whether this is a series or not
 * @param   string  $action    the booking action (eg 'delete', 'more_info', etc.)
 * @param   string  $note      a note that is used with 'reject'
 */
function notifyAdminOnDelete($data, $start_times, bool $series=false, string $action='delete', ?string $note=null)
{
  global $mail_settings, $enable_periods;
  global $mrbs_company;

  if ($mail_settings['debug'])
  {
    mail_debug('Preparing email for deleted booking');
  }

  // As we are going to cancel this booking we need to increment the iCalendar
  // sequence number
  $data['ical_sequence']++;

  // Set up the addresses (from, to and cc)
  $addresses = create_addresses($data, $action);
  if (empty($addresses['to']) && empty($addresses['cc']))
  {
    mail_debug('Email abandoned: no addresses.');
    return;
  }

  // Set the subject
  //
  // If we're sending iCalendar notifications, then it seems that some calendar
  // applications use the email subject as the booking title instead of the iCal
  // SUMMARY field.   As far as I can see this is wrong, but as a circumvention
  // we'll put the booking title in the email subject line.   (See also
  // SF Tracker id 3297799)
  if ($mail_settings['icalendar'] && !$enable_periods)
  {
    $subject = $data['name'];
  }
  elseif ($action == "reject")
  {
    $subject = get_mail_vocab("mail_subject_rejected", $mrbs_company);
  }
  else
  {
    $subject = get_mail_vocab("mail_subject_delete", $mrbs_company);
  }

  // Create the text body
  $text_body = create_body($data, null, false, $series, $action, $start_times, false, $note);

  // Create the HTML body
  if ($mail_settings['html'])
  {
    $html_body = create_body($data, null, false, $series, $action, $start_times, true, $note);
  }

  // Set up the attachment
  $attachment = array();
  if ($mail_settings['icalendar'] && !$enable_periods)
  {
    $attachment['method']   = "CANCEL";
    $ical_components = array();
    $ical_components[] = create_ical_event($attachment['method'], $data, $addresses, $series);
    $attachment['content']  = create_icalendar($attachment['method'], $ical_components);
    $attachment['name']     = $mail_settings['ics_filename'] . ".ics";
  }

  MailQueue::add(
      $addresses,
      $subject,
      $text_body,
      $html_body ?? null,
      $attachment,
      get_mail_charset()
    );
}


function debug_output(string $message) : void
{
  global $mail_settings;

  if (isset($mail_settings['debug_output']) &&
      ($mail_settings['debug_output'] == 'browser'))
  {
    echo escape_html($message) . "<br>\n";
    // flush in case they have output_buffering configured on
    if (ob_get_length() !== FALSE)
    {
      ob_flush();
    }
    flush();
  }
  else  // anything else goes to the error log
  {
    error_log($message);
  }
}


function mail_debug(string $message) : void
{
  global $mail_settings;

  if ($mail_settings['debug'])
  {
    debug_output('[DEBUG] ' . $message);
  }
}




