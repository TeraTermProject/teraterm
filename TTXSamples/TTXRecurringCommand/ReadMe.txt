TTXRecurringCommand -- Sends a specified string periodically

Feature:  
This is a TTX to achieve the same functionality as the Recurring Command in the Ayera version of Tera Term Pro web.
When enabled, if there is no keyboard input for the specified number of seconds, the specified string (command) will be sent.
If the command contains any of the following strings, they will be replaced with the corresponding character.
\n -- Line feed (LF)
\t -- Tab
\r -- Carriage return (CR)
(with others)

Description:

Example:
  [TTXRecurringCommand]
  Enable=on
  Command=date\n
  Interval=300

Bug:
