TTXttyrec -- a tty recorder

Feature:
  This feature are recording a terminal display data, and writing to a file.
  If a user selects Record menu under Control menu, the saving dialog is shown. TTY recording will start after selecting a file.
  If a use reselects the menu, TTY recording will finish.

  The saving file data can be replayed with ttyplay(http://0xcc.net/ttyrec/index.html.en).

Description:
  This feature is a sample plugin to hook the TCP communication data.
  This plugin hooks Precv handler, writes the current timestamp and the receiving data to a file with ttyrec format.
  
  The gettimeofday API is built with the full scratch because Windows API does not have this function.

Bug:
  * A user can only record. Can not replay the recording data by using Tera Term.

ToDo:
  * Support for replaying the recording data.
