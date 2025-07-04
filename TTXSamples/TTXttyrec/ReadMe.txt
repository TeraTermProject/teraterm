TTXttyrec, TTXttyplay -- Terminal Screen Recorder/Player

Features:
  This feature are recording a terminal display data, and writing to a file.
  If a user selects Record menu under Control menu, the saving dialog is shown.
  TTY recording will start after selecting a file.
  If a use reselects the menu, TTY recording will finish.

  In this version, recording is supported for both TCP connections and serial connections.
  However, to record serial connections, Tera Term version 4.60 or later is required.
  This plugin can be used with versions prior to 4.60, but only TCP connections will be supported.
  In this case, if you attempt to record during a serial connection, nothing will be recorded.

  TTXttyplay plays back the data saved by TTXttyrec.
  By selecting "TTY Replay" from the file menu, a file selection dialog will appear,
  allowing you to choose the recorded data to start playback.
  TTXttyplay requires Tera Term version 4.60 or later.

  The data format is the same as ttyrec (http://0xcc.net/ttyrec/),
  so data recorded with TTXttyrec can be played back with ttyplay,
  and data recorded with ttyrec can be played back with TTXttyplay.

Description:
  This feature is a sample plugin to hook the TCP communication data.
  This plugin hooks Precv handler, writes the current timestamp and the receiving data to a file with ttyrec format.
  The recording of serial connections utilizes the serial connection hooks added in Tera Term 4.60.

  TTXttyplay uses the log playback hooks added in Tera Term 4.60.
  During log playback, instead of sequentially reading data, it introduces a wait by returning ERROR_IO_PENDING.

  The gettimeofday API is built with the full scratch because Windows API does not have this function.

Bugs:
  * The time accuracy is not very good.
