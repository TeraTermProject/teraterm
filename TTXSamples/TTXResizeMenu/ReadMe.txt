TTXResizeMenu -- Resizing VT window

Feature:
  This feature adds Resize menu.
  If a user selects the Resize menu item, Tera Term VT window is changed to
  the specified size.

Description:
  TTXResizeMenu is based on TTXResizeWin plugin. 
  Please refer to TTXResizeWin plugin description about the detail
  implementation of TTXResizeMenu plugin.

  Basically, the [Resize] menu does not display after the plugin is installed.
  Next, a user must register the menu entry in the teraterm.ini file. 

  Add a [Resize Menu] section in the teraterm.ini file, and describe the
  "ResizeMenuN = X, Y" entry. N is an 1-origin number.
  When X or Y equals zero, the value is not changed. For example,
  (X, Y)=(0, 37) means that the width is not changed and the height is changed
  into 37 lines.
  These value maximum is 20.

ToDo:
  To configure with TERATERM.INI file againt Resize menu item.
