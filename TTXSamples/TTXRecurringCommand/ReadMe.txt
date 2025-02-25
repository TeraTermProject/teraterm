TTXRecurringCommand -- Sends a user specified string periodically

Feature:
This implements functionality equivalent to the Recurring Command found in the Ayera Technologies version of Tera Term Pro web.  When enabled, it sends a user specified string (command) if there is no keyboard input received for a specified number of seconds.  If the command includes the following strings, they will be replaced with the corresponding characters:
   \n -- line feed (LF)
   \r -- carriage return (CR)
   \t -- tab
(and others)

Description:
Users have encountered situations where it is desired to the same command over and over for such things as monitoring system health or running tasks that need to happen periodically and won't require user interaction.  This TTY module has been added to provide that functionality.  

Example:
  [TTXRecurringCommand]
  Enable=on
  Command=date\n
  Interval=300

Bug:
