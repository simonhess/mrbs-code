<?php
declare(strict_types=1);
namespace MRBS;

// Returns a string containing a single line of data details, or if $as_html is set
// a table row of details.   The first column is the $label and the second column
// the $value.   $class is an optional class name which can be applied to the
// second column.  If $email is set then the $value will be turned into a mailto: link.
// $value can be of type null|int|float|string (union declarations are not supported until PHP 8.0).
function create_details_row(string $label, $value, bool $as_html=false, string $class='', ?string $email=null) : string
{
  $value = $value ?? '';
  $result = '';

  if ($as_html)
  {
    $escaped_value = escape_html($value);
    if (is_string($escaped_value))
    {
      $escaped_value = mrbs_nl2br($escaped_value);
    }
    $result .= "<tr>\n";
    $result .= "<td>$label</td>\n";
    $result .= "<td" . ((!empty($class)) ? " class=\"$class\"" : '') . '>';
    if (isset($email))
    {
      // Obfuscate the email address
      $html = '<a href="mailto:' . rawurlencode($email) . '">' . $escaped_value . '</a>';
      $result .= '<span class="contact" data-html="' . base64_encode($html) . '">';
    }
    $result .= $escaped_value;
    if (isset($email))
    {
      $result .= '</span>';
    }
    $result .= "</td>\n";
    $result .= "</tr>\n";
  }
  else
  {
    // Some of the vocab strings contain &nbsp;
    $result .= str_replace('&nbsp;', ' ', $label) . ": $value\n";
  }
  return $result;
}


// Returns a string containing a set of details for $data consisting of a label/value
// pair for each data element in the array $data.   If $as_html is true then the string
// is the HTML for a table body; i.e. looks like "<tbody> ... </tbody>".
//    $keep_private       boolean if true, then any private fields will be given the class 'private';
//                                note that $data must already have had values substituted
//                                for private fields
//    $room_disabled      boolean if true, then a note will be added that the room is disabled
//    $major_details_only boolean if true, only show the major details
function create_details_body(array $data, bool $as_html=false, bool $keep_private=false, bool $room_disabled=false, bool $major_details_only=false) : string
{
  global $enable_periods, $confirmation_enabled, $approval_enabled;
  global $is_private_field, $standard_fields;
  global $datetime_formats;
  global $select_options, $booking_types;
  global $edit_entry_field_order;

  // Get the duration if we haven't got it already
  if (!isset($data['duration']))
  {
    // We will translate the units later
    $d = get_duration($data['start_time'], $data['end_time'], $enable_periods, $data['area_id'], false);
    $data['duration'] = $d['duration'];
    $data['dur_units'] = $d['dur_units'];
  }

  // Go through each of the columns and for each of them that can be made private
  // substitute the private text if the user is not allowed to see the data
  $private_text = get_vocab("unavailable");

  foreach ($data as $key => $value)
  {
    $hide_this_field = $keep_private && isset($is_private_field["entry.$key"]) && $is_private_field["entry.$key"];
    // We could just test each column against $is_private_field["entry.$key"]
    // but restricting the test to the columns below guards against the possibility
    // that somebody has accidentally configured a 'system' field to be private
    switch ($key)
    {
      case 'name':
      case 'description':
      case 'room_name':
      case 'area_name':
      case 'room_id':
      case 'entry_info_time':
      case 'entry_info_user':
      case 'entry_info_text':
      case 'repeat_info_time':
      case 'repeat_info_user':
      case 'repeat_info_text':
        $data[$key] = $hide_this_field ? $private_text : $data[$key];
        break;

      case 'type':
        $data[$key] = $hide_this_field ? $private_text : get_type_vocab($data[$key]);
        break;

      case 'create_by':
      case 'modified_by':
        if ($hide_this_field)
        {
          $data[$key] = $private_text;
        }
        else
        {
          $user = auth()->getUser($value);
          if (isset($user))
          {
            $data[$key] = $user->display_name;
          }
        }
        // Add in the email address if there is one and we are allowed to
        if (!isset($user) ||
            ($user->email === '') ||
            // The field must not be private
            $hide_this_field ||
            // And the user must be allowed to see others' email addresses
            !can_see_email_addresses())
        {
          $email[$key] = null;
        }
        else
        {
          $email[$key] = $user->email;
        }
        break;

      default:
        if (!in_array($key, $standard_fields['entry']))
        {
          if ($hide_this_field)
          {
            $data[$key] = $private_text;
          }
          else
          {
            $columns = Columns::getInstance(_tbl('entry'));
            $column = $columns->getColumnByName($key);
            // Convert DATE types into a locale friendly form
            if (isset($column) && ('date' == $column->getType()))
            {
              $date = DateTime::createFromFormat(DateTime::ISO8601_DATE, $data[$key]);
              if ($date !== false)
              {
                $data[$key] = datetime_format($datetime_formats['date'],  $date->getTimestamp());
              }
            }
          }
        }
        break;
    }
  }

  $fields = db()->field_info(_tbl('entry'));

  // Get the order in which to display fields
  $field_order = (isset($edit_entry_field_order)) ? $edit_entry_field_order : array();
  foreach ($fields as $field)
  {
    if (!in_array($field['name'], $field_order))
    {
      $field_order[] = $field['name'];
    }
  }

  // Add in the two special status fields
  foreach (array('approval_status', 'confirmation_status') as $key)
  {
    if (!in_array($key, $field_order))
    {
      array_push($field_order, $key);
    }
  }

  $tbody = '';
  $tbody .= ($as_html) ? "<tbody>\n" : "";

  foreach ($field_order as $key)
  {
    if ($major_details_only && !in_array($key, ['description', 'room_id', 'start_time', 'end_time']))
    {
      continue;
    }

    switch ($key)
    {
      // Ignore these
      case 'name':  // in the heading
      case 'id':
      case 'entry_type':
      case 'reminded':
      case 'repeat_id':
      case 'status':
      case 'info_time':
      case 'info_user':
      case 'info_text':
      case 'ical_uid':
      case 'ical_sequence':
      case 'ical_recur_id':
      case 'allow_registration':
      case 'registrant_limit':
      case 'registrant_limit_enabled':
      case 'registration_opens':
      case 'registration_opens_enabled':
      case 'registration_closes':
      case 'registration_closes_enabled':
        break;

      case 'description':
        // Description
        $class = ($keep_private & !empty($is_private_field['entry.description'])) ? "private" : "";
        $tbody .= create_details_row(get_vocab("description"), $data['description'], $as_html, $class);
        break;

      case 'confirmation_status':
        // Confirmation status
        if ($confirmation_enabled)
        {
          $value = ($data['tentative']) ? get_vocab("tentative") : get_vocab("confirmed");
          $tbody .= create_details_row(get_vocab("confirmation_status"), $value, $as_html);
        }
        break;

      case 'approval_status':
        // Approval status
        if ($approval_enabled)
        {
          $value = ($data['awaiting_approval']) ? get_vocab("awaiting_approval") : get_vocab("approved");
          $tbody .= create_details_row(get_vocab("approval_status"), $value, $as_html);
        }
        break;

      case 'room_id':
        // Room
        $value = $data['area_name'] . " - " . $data['room_name'];
        if ($room_disabled)
        {
          $value .= " (" . get_vocab("disabled") . ")";
        }
        $tbody .= create_details_row(get_vocab("room"), $value, $as_html);
        break;

      case 'start_time':
        // Start date
        if ($enable_periods)
        {
          list($start_period, $start_date) =  period_date_string($data['start_time'], $data['area_id']);
        }
        else
        {
          $start_date = time_date_string($data['start_time']);
        }
        $tbody .= create_details_row(get_vocab("start_date"), $start_date, $as_html);
        // Duration
        $tbody .= create_details_row(get_vocab("duration"), $data['duration'] . " " . get_vocab($data['dur_units']), $as_html);
        break;

      case 'end_time':
        // End date
        if ($enable_periods)
        {
          list( , $end_date) =  period_date_string($data['end_time'], $data['area_id'], true);
        }
        else
        {
          $end_date = time_date_string($data['end_time']);
        }
        $tbody .= create_details_row(get_vocab("end_date"), $end_date, $as_html);
        break;

      case 'type':
        // Type
        if (isset($booking_types) && (count($booking_types) > 1))
        {
          $class = ($keep_private && !empty($is_private_field['entry.type'])) ? "private" : "";
          $tbody .= create_details_row(get_vocab("type"), $data['type'], $as_html, $class);
        }
        break;

      case 'create_by':
        // Created by
        $class = ($keep_private && !empty($is_private_field['entry.create_by'])) ? "private" : "";
        $tbody .= create_details_row(get_vocab("createdby"), $data['create_by'], $as_html, $class, $email[$key]);
        break;

      case 'modified_by':
        // Modified by
        $class = ($keep_private && !empty($is_private_field['entry.modified_by'])) ? "private" : "";
        $tbody .= create_details_row(get_vocab("modifiedby"), $data['modified_by'], $as_html, $class, $email[$key]);
        break;

      case 'timestamp':
        // Last updated
        $tbody .= create_details_row(get_vocab("lastupdate"), time_date_string($data['last_updated']), $as_html);
        break;

      default:

        // The custom fields
        foreach ($fields as $field)
        {
          // Get this field
          if ($field['name'] == $key)
          {
            break;
          }
        }

        // The field doesn't exist.
        if ($field['name'] != $key)
        {
          break;
        }

        $label = get_loc_field_name(_tbl('entry'), $key);
        // Output a yes/no if it's a boolean or integer <= 2 bytes (which we will
        // assume are intended to be booleans)
        if (($field['nature'] == 'boolean') ||
            (($field['nature'] == 'integer') && isset($field['length']) && ($field['length'] <= 2)) )
        {
          if ($keep_private && isset($is_private_field["entry.$key"]) && $is_private_field["entry.$key"])
          {
            $value = $data[$key];  // Will have been set previously
          }
          else
          {
            $value = empty($data[$key]) ? get_vocab("no") : get_vocab("yes");
          }
        }
        // Otherwise output a string
        else
        {
          if (isset($data[$key]))
          {
            // If the custom field is an associative array then we want
            // the value rather than the array key
            if (isset($select_options["entry.$key"]) &&
                is_assoc($select_options["entry.$key"]) &&
                array_key_exists($data[$key], $select_options["entry.$key"]))
            {
              $value = $select_options["entry.$key"][$data[$key]];
            }
            else
            {
              $value = $data[$key];
            }
          }
          else
          {
            $value = "";
          }
        }
        $class = ($keep_private && isset($is_private_field["entry.$key"]) && $is_private_field["entry.$key"]) ? "private" : "";
        $tbody .= create_details_row($label, $value, $as_html, $class);
        break;
    }  // switch
  }  // foreach

  if (!$major_details_only)
  {
    // Repeat type
    $tbody .= create_details_row(get_vocab("rep_type"), get_vocab("rep_type_" . $data['rep_type']), $as_html);
    // Repeat details
    if ($data['repeat_rule']->getType() != RepeatRule::NONE)
    {
      switch ($data['repeat_rule']->getType())
      {
        case RepeatRule::WEEKLY:
          // Repeat days
          $tbody .= create_details_row(get_vocab("rep_rep_day"), $data['repeat_rule']->getDaysAsNames(), $as_html);
          break;
        case RepeatRule::MONTHLY:
          $tbody .= create_details_row(get_vocab("repeat_on"), get_monthly_repeat_day($data), $as_html);
          break;
        default:
          break;
      }

      // Repeat interval
      $value = $data['repeat_rule']->getInterval() . ' ' . $data['repeat_rule']->getIntervalUnits();
      $tbody .= create_details_row(get_vocab("rep_interval"), $value, $as_html);

      // Repeat end date
      $tbody .= create_details_row(
        get_vocab("rep_end_date"),
        datetime_format($datetime_formats['date'], $data['end_date']),
        $as_html
      );
    }
  }
  $tbody .= ($as_html) ? "</tbody>\n" : "";

  return $tbody;
}

