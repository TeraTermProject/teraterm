                      Introduction to LogMeTT
                      =======================

LogMeTT is the application that works together with popular freeware terminal
emulator TeraTerm.

LogMeTT helps to create, keep organized and easily accessible connection scripts
written on TeraTerm Macro language. Those who frequently connect to the same
remote sites using telnet, SSH1/SSH2 or modem connections can find LogMeTT as
the very useful tool saving them a lot of time.

LogMeTT keeps TeraTerm macro scripts in a hierarchical tree structure that at
certain degree can be considered as graphical representation of network topology.
Every script has corresponding item in LogMeTT popup menu that appears from the
icon in Windows system tray. Thus each predefined connection scenario is always
just 2 clicks away from the user. Tree type of script organization allows to
reuse "parent" scripts while connecting to the "child" nodes and saves additional
time while creating and keeping updated TeraTerm macros. Any connection or whole
tree of connections can be saved in a binary file and distributed among the group
of people that need to access the same remote nodes.

LogMeTT has built in TeraTerm Macro language command highlighter and context
sensitive Macro language help that assists in creating new scripts and improves
their readability. It also provides several typical connection templates that can
be studied and used as the examples while writing TeraTerm macros.

LogMeTT allows to associate specific color schema to each connection that reduces
the chance of human error while working with multiple TeraTerm windows on the
same desktop.

With LogMeTT user can select group of connection macros and execute them together.
Multiple TeraTerm windows will be opened and tiled automatically on the desktop.
This feature is especially convenient if TeraTerm Broadcast mode has to be used.

LogMeTT allows to run the scripts based on predefined schedules that can automate
lots of routine tasks like periodic data collection from remote sites, performing
configuration backups etc.

LogMeTT extends TeraTerm macro language with the set of keywords that can contain
user and connection specific variables.

LogMeTT is freeware application distributed without the source code.

More details can be found in LogMeTT.chm file.


Please visit LogMeTT support forum for more information:
http://www.neocom.ca/forum/index.php


                 LogMeTT Releases and features
                 =============================
                 (in reverse chronological order)

2008-06-07 (Ver 2.9 Release 4)
New features added:
- Adding new node activates node name editor.

Bugs fixed:
- New popup menu component introduced in version 9.2.3 was incorrectly
rendering menu structure and had possible memory leak issue. This is
fixed now.
- "Cut" operation applied to Tree nodes was updating icons incorrectly.
- Connection tree branches were not collapsing when "This computer" node
was selected.
- Opening LTT file could turn into Import when unsaved previous
LTT file was loaded and 'Cancel' opening button was pressed in Warning
dialog box.



2008-06-01 (Ver 2.9 Release 3)
New features added:
- Replaced Popup menu component to simplify menu structure. The
same menu can be used to start the macro and to open submenu. In
the previous versions such item was duplicated into it's own submenu.
- Added keyword $snippet$. This key word disables direct access to
the macro from LogMeTT popup menu. It is case-insensitive and can
be places anywhere within a Macro code. Example of use case: Node
"ABC" contains few starting lines of Macro code that are common for
all child node macros. The Macro linked to node "ABC" should never be
executed apart from child macros. Key word $snippet$ placed inside
the macro code attached to node "ABC" will restrict user from calling
this macro separately.
- Updated LogMeTT help file to reflect all the latest changes.

Bugs fixed:
- Restore TeraTerm configuration menu option was staying disabled
after saving configuration at first time.
- Fixed trailing spaces not being entered in Popup editor.
- Switching tabs was marking node content as "Modified"
- Node data was not showing up after certain sequence of clicks on
tree nodes and the Tabs in the right part of LogMeTTC window.
- Fixed the bug with Macro export into TTL file.

2007-07-23 (Ver 2.9 Release 2)
New features added:
- Application was redesigned to support Microsoft Windows Vista and Windows XP themes.
- Replaced hardcoded macro Templates with the plain text file 'ttmacro.tpl' that can
be used as reference for building personal template library.
- Added configurable name of Templates file to avoid overwriting custom templates
during upgrades.
- Added tracking of admin rights when running under Windows Vista. Without admin rights
Vista does not allow to associate file extension with the application. Use ‘Run as
Administrator’ from Vista popup menu or change the properties of LogMeTTC.exe to be
always started as Administrator.
- 'About' window was combined with Disclaimer message. It also contains 'What's new' tab.
- 'Help' menu is now calling Vista compatible LogMeTT and TeraTerm help files in .chm
format instead of old file with extension .hlp. This applies also to Context sensitive
help in editor area. LogMeTT help file however does not reflect all changes introduced in
this release.
- Functionality of 'Print' speed button changed from showing Print Dialog to immediate
printing. Print Dialog is still accessible from File->Print… menu.
- Added 'Show Special Characters' option and speed button.
- All previously hardcoded keywords used by syntax highlighter were migrated to external
KeyFile.ini file. This will allow to add new key words without recompiling the application.
KeyFile.ini also contains Help ID-s that are used by context sensitive help.
- Upgraded Virtual Tree component to version 4.5.2
- Added shortcuts F11 - "Run all selected" and F12 - "Apply Changes".
- Added support of $[1]..$[8] key words that can be used as command line parameters for
RunLTT application.
- Added column selection mode. Use mouse while pressing <Alt> key to select columns in
Macro editor.
Bugs fixed:
- Fixed the error happening while duplicating expanded branch of the Connections tree.
- Fixed node path that was appearing in reverse order in Print Preview window and in the
header of printouts.
- Lots of other changes, code cleanup and bug fixing of 3rd party freeware components used
by LogMeTT and LogMeTTC.



2007-03-28 (Ver 2.9 Release 1)
New features added:
- Standalone TeraTerm Macro Editor - TTLEditor was created. It inherited all features
of LogMeTT Macro Editor of version from version 2.8 release 6 and also got few new ones.
TTLEditor supports drag and drop of plain text files and can be associated with file
extension TTL.
- Both LogMeTT Macro Editor and TTL Editor now support syntax completion proposal feature.
Press [Ctrl-Space] while typing macro command and resizeble hint window will appear.
- LogMeTT popup menu now has "IP Address" item that opens into sub menu allowing to
release/renew IP address for any or all network adapters or check adapter details.
- File extension association functions were rewritten.
- "Do not Prompt on disconnect" option was added to Settings window. It applies to
TeraTerm windows opened in "Run All Selected" mode.

Bugs fixed:
- Few minor fixes and code optimization.


2006-07-24 (Ver 2.8 Release 5)
New features added:
- None.

Bugs fixed:
- Fixed (finally) sporadic application error that was happening while closing Configuration
Utility. The error was triggered by the conflict of 2 freeware Delphi components (SynEdit
and XPMenu) that are used in LogMeTTc.


2006-07-03 (Ver 2.8 Release 4)
New features added:
- Added progress bar to LogMeTT Configuration Utility splash screen.

Bugs fixed:
- Reduced startup time of LogMeTT Configuration Utility.
- Fixed (hopefully) sporadic application error that was happening while closing Configuration Utility.


2006-03-19 (Ver 2.8 Release 3)
New features added:
- Added Information window under "File -> Information for" menu. It shows statistical
information about currently loaded Connections tree.
- Minor other changes and code optimization.

Bugs fixed:
- None.


2006-03-13 (Ver 2.8 Release 2)
New features added:
- Added mpause and random macro commands to syntax highlighter.
- Added Splash window at Configuration Utility startup.
- Updated Teraterm Macro help. Added recently introduced mpause and random macro commands
- Upgraded LogMeTT Installation Utility to CreateInstall Free v4.3 from www.createinstall.com

Bugs fixed:
- Fixed the problem starting Cygwin when the PATH environmental variable does not contain the path to cygwin1.dll


2006-01-17 (Ver 2.8 Release 1)
New features added:
- Added Operation Highlighting ( + - * / % and or xor not).
- Added text search and replace functionalities in currently open Macro. Menu items Find, Find Next, Find Previous, Replace under menu Search.
- Added Maximize, Hide and Restore options for Connection Tree.
- Added View menu.
- Added multi tab navigation among pages with Settings.
- Added Syntax highlighting setup page.
- Added ability to start macro from configuration window when other then Macro editor page is selected.
- Updated LogMeTT help file to reflect these changes.

Bugs fixed:
- None.


2006-01-12 (Ver 2.7 Release 5)
New features added:
- In addition to TeraTerm.ini file backup and restore, LogMeTT now also performs backup
and restore of cygterm.cfg file.

Bugs fixed:
- The bug was fixed when after importing .LTT file with the connection having non-unique
name the existing connection was being renamed as "Copy of" instead of newly imported connection.


2005-11-16 (Ver 2.7 Release 4)
New features added:
- None.

Bugs fixed:
- The bug was fixed when after dragging and dropping the node with subnodes in
Connection tree subnode level related data was becoming corrupted.
- Virtual Tree component from Soft-Gems used in Configuration window was upgraded
to version 4.4.2. For list of improvements and bug fixes visit
http://www.soft-gems.net/VirtualTreeview/VTHistory.php


2005-11-14 (Ver 2.7 Release 3)
New features added:
- Two new node related key words were added. They are $parent$ and $PARENT$.
These key words are similar to key words $connection$ and $CONNECTION$ with only
difference that parent node's captions are being substituted instead. Keyword
highlighter and LogMeTT help file were updated to reflect this new feature.

Bugs fixed:
- Few minor fixes and code optimization not affecting the functionality.

Other issues:
- Starting November 7th, 2005 Neocom Solutions carries new name "Neocom Ltd." From
this date Neocom Ltd. appears as copyright owner of all software products
distributed from NeoCom.ca web site unless stated otherwise. It also owns
NeoCom.ca web site itself.


2005-10-15 (Ver 2.7 Release 2)
New features added:
- 9 new TeraTerm Macro system variables from 'groupmatchstr1' to 'groupmatchstr9'
that are introduced in TeraTerm 4.22 were added to syntax highlighter.

Bugs fixed:
- Job scheduler was starting to work from the second minute after LogMeTT start
instead of starting from the end of the current minute.
- The problem with check for updates algorithm was fixed when tray icon was
keeping to flash if upgrade to the latest version was done from "outside" LogMeTT.


2005-10-11 (Ver 2.7 Release 1)
New features added:
- The 3rd page of settings was added to LogMeTT Configuration Window.
Press 'Next' button 2 times to reach it. The page contains default settings
for Colors and Logging for newly created connections.
- Teraterm.ini color settings were added to the set of predefined color
schemas on Colors tab.
- The whole algorithm of checking for updated versions via Internet was
replaced. Manual check option was added to 'Help' menu. Also the interval
to check for updates can be set on the first 'Settings' page. Available
options are:  every day, every 3 days, every 7 days, every 14 days, every
30 days, never. At maiden installation the option 'Never' is selected by
default (as per suggestion of PaVel). Before performing actual upgrade user
will get the list of new features and bugs fixed in newly released version.
Upgrade procedure will automatically shut down LogMeTT and start downloading
new version of TeraTerm or LogMeTT. While upgrading TeraTerm user gets option
to perform teraterm.ini backup.
- 'waitregex' macro command and variable 'matchstr' were added to syntax
ighlighter.
- Both LogMeTT and Macro help files were updated.

Bugs fixed:
- The major problem with checking for newer releases introduced in the previous
release was fixed.
- If no default printer installed, application interface was crashing and error
message was popping up all the time.
- The problem with scheduler was fixed when day of the month setting was
overwriting day of the week setting. Thanks to Victor for reporting.
- Lots of other minor fixes and code optimisation.


2005-08-08 (Ver 2.6 Release 1)
New features added:
- Print, Print Preview and Printer Setup functions were added. They are
reachable from File menu of LogMeTT Configuration window and allow to print
the macro associated to currently selected node. Functions are available only
when Macro page is visible.
- As per request of LogMeTT users the functionality of checking for updated
versions of TeraTerm and LogMeTT was changed. LogMeTT now check for updates
on startup and then once every 24 hours of running. This functionality can be
disabled by selecting newly added checkbox "Do not connect the Internet to
check for newer versions" on Settings page of Configuration window.
- Help file was updated to reflect these new changes.
Bugs fixed:
- None.


2005-07-31 (Ver 2.5 Release 7)
New features added:
- Additional option "Do not connect the Internet to check for newer versions"
was added to Settings page of Configuration window. Thanks to PaVel for
suggesting this feature. By default this option is not selected and LogMeTT
connects to neocom.ca web site to check for updated versions of TeraTerm and
LogMeTT. The first check is performed on application startut and the every
3 hours.
Bugs fixed:
- None.

2005-07-22 (Ver 2.5 Release 6)
New features added:
- None.
Bugs fixed:
- Memory utilisation issues were addressed. In some cases allocated memory was
reduced up to 20 times.
- Hourglass icon was mistakenly appearing in tray area when new releases of
LogMeTT or TeraTerm are published on the web.


2005-07-19 (Ver 2.5 Release 5)
New features added:
- LogMeTT icon has the shape of hourglass while reloading data.
- Further improvement of LogMeTT popup menu redrawing function.
- Added filter on $phone$, $mobile$ and $pager$ variables to pass into macro only
the following characters: 0..9 # * + , P p. Other characters can still appear on
Settings page but will be stripped from the macro code.
Example: "(123) 456-7890" will be converted to "1234567890". Help file was updated.
- LogMeTT_README.txt file updated.
Bugs fixed:
- Interaction between LogMeTT.exe and LogMeTTc.exe was optimized.
- Double click on tray icon under certain conditions was causing application to crash.

2005-08-08 (Ver 2.6 Release 1)
New features added:
- Print, Print Preview and Printer Setup functions were added. They are reachable from
File menu of LogMeTT Configuration window and allow to print the macro associated to
currently selected node. Functions are available only when Macro page is visible.
- As per request of LogMeTT users the functionality of checking for updated versions
of TeraTerm and LogMeTT was changed. LogMeTT now check for updates on startup and then
once every 24 hours of running. This functionality can be disabled by selecting newly
added checkbox "Do not connect the Internet to check for newer versions" on Settings
page of Configuration window.
- Help file was updated to reflect these new changes.
Bugs fixed:
- None.

2005-07-13 (Ver 2.5 Release 4)
New features added:
- None.
Bugs fixed:
- It was not possible to drag and drop connection between 2 separator lines.
- Additional restrictions were added on drag and drop operations inside the
  connection tree.
- Fixed the bug in 'Reverse Colors' function.
- The function checking for duplicate connection names was not working correctly
under certain conditions.

2005-07-04 (Ver 2.5 Release 3)
New features added:
- LogMeTT popup menu redrawing function was improved.
- LogMeTT installer created.

Bugs fixed:
- Parent level macro was being executed twice when started from LogMeTT popup menu.
Thanks to suibian for reporting this bug.

2005-06-28 (Ver 2.5 Release 2)
New features added:
- None.
Bugs fixed:
- 'Run All Selected' function from 'Actions' menu was not working correctly.

2005-06-21 (Ver 2.5 Release 1)
New features added:
- LogMeTT.hlp file was created. LogMeTT Configuration window is supporting now
context sensitive help that can be called by pressing <F1>.
- Context sensitive help for LogMeTT key words in Macro editor was added. It can
be called by placing cursor on the word and pressing <Ctrl-F1> (the same applies
to TeraTerm macro commands).
- Current value of LogMeTT key words appears on <Ctrl-Click> on these key words
in Macro editor.
- LogMeTT key words are being highlighted in Macro editor with red color.
- Current line is being highlighted in Macro editor.
- Further improvement of 'Run to cursor' function. From now it does not depend
on selection within Macro editor but runs from the first line of the macro down
to the line containing the cursor (inclusive).
- TeraTerm.ini file backup/restore functions were added into
'File' -> 'TeraTerm Configuration' menu. They help to preserve user's settings
during TeraTerm upgrades.
- $CONNECTION$, $ttdir$, $windir$, $logfilename$ and $LOGFILENAME$ key words were
added. See LogMeTT help for their descriptions.
- URL color was added. This feature is still not fully supported by TeraTerm.
- 'CygWin' menu was added to LogMeTT popup menu and to 'Actions' menu of
Configuration Screen. It is only visible if Cygwin is installed.
- Additional option 'Start CygWin' was added to the list of functions called on
double click on tray icon.
- Logging when checkbox 'Start logging when macro ends' is selected is now disabled
when 'Run to cursor' or 'Run branch to cursor' function is executed from
Configuration window.
- The second page of Settings was added.
- Most recently used files can appear with or without full path in 'File' menu based
on configuration setting.
- Fonts of menus and Macro editor are configurable. The font from Macro editor will
also be used in TeraTerm main window. Preferable character set can be also selected.
- The behaviour of <Tab> key in macro editor is now configurable.
- The format of LTT file changed. The files created by earlier versions of LogMeTT
will be backed up and converted automatically when first opened in Configuration
window.
Bugs fixed:
- Temporary file were not clean up properly.
- Fixed bug with scheduler starting 3 copies of the macro in some cases.
- Lots of other minor fixes and code optimisation.


2005-04-13 (Ver 2.4 Release 4)
New features added:
- Context sensitive help was added to macro editor. When macro command is
selected in macro editor, pressing <Ctrl-F1> will open corresponding page in
macro.hlp file.
- Shortcuts to 'TeraTerm Help' and 'TeraTerm Macro Language Help' in 'Help' menu
of Configuration window changed to <Shift-Ctrl-F1> and <Shift-F1>.
- 'Run to Cursor' functionality was improved and now there is no need to select
part of the macro to run it. 'Run to Cursor' will itself execute part of the
macro from the first line to the end of the line containing cursor. The same
applies to 'Run branch to cursor' function.
- Help file macro.hlp was modified and contains now updated information about
the syntax of command connect.
- LogMeTT_README.txt file was updated.
Bugs fixed:
- Function 'Run to cursor' in certain cases was not properly expanding the
selection to the end of the last selected line.
- Several macro language commands were not highlighted.
- Double click was not selecting the numbers in Macro editor.


2005-04-08 (Ver 2.4 Release 3)
New features added:
- Added $mobile$ and $pager$ key words. Their values can be assigned in the
Settings tab of Configuration window.
Bugs fixed:
- None.

2005-03-29 (Ver 2.4 Release 2)
New features added:
- Added $logdir$ key word variable. It is being substituted with the full path
to 'Logs' folder that is under TeraTerm home directory.
Sample: 'C:\Program Files\teraterm\Logs\'
Bugs fixed:
- When connection marked in status bar as 'Modified' and focus is in
connections tree, pressing Ctrl-A was not updating the right part of
configuration window.
- When connection marked in status bar as 'Modified'

2005-03-27 (Ver 2.4 Release 1)
New features added:
- Added automatic macro execution based on the schedule.
- Added 8 macro templates for different types of connection under
'File' -> 'Import macro template' menu of Configuration window.
Bugs fixed:
- LogMeTT was showing up in Windows Task Bar and staying there in some cases
after closing Popup message. - The function stripping macro command 'END' from
the parent macros was removed since it could affect functionality of certain
complex macro scripts.
- Line and Column numbers in status bar were shown vice versa.
- Automatic expansion of selected part of the macro in 'Run to cursor' mode was
not working after adding syntax highlight feature.

2005-03-22 (Ver 2.3 Release 3)
New features added:
- LogMeTT tray icon is now flashing when the new versions of either LogMeTT or
TeraTerm are available for downloading (only when PC is connected to the
Internet).
- Upgrade related 2 menu items were moved from Help menu of Configuration window
into LogMeTT popup menu. There are visible only when the newer versions of
software are available for downloading.
Bugs fixed:
- Key word $CONNECTION$ was not correctly substituted for parent macros.

2005-03-20 (Ver 2.3 Release 2)
Bugs fixed:
- Macros started from Coniguration window were always including all parent macros.
- Fixed Internal error happenning when user was clicking in macro editor area
while being in connection name editing mode.
- Modifyed the way default log file name is generated.

2005-03-18 (Ver 2.3 Release 1)
New features added:
- Macro language syntax highliting feature was added to macro editor. It is
active when 'Connecton colors in Macro editor' option is switched off.
- New panel was added to status bar showing line and column number of caret
location inside macro and popup editor boxes.
- The information about freeware Delphi components used in LogMeTT was added to
About window.
- Added dynamic font size change based on screen resolution in Configuration
window.
- Import/Export menu items were migrated under File menu of Configuration
window.
- Menu 'Macro' was renamed to 'Actions'.
Bugs fixed:
- When modifyed macro was executed from Configuration window it was
automatically applied to current connection even without pressing Apply button.
- Optimized speed buttons refresh procedure.
- NumLock, CapsLock and ScrollLock were not showing up on Configuration window
start.

2005-03-15 (Ver 2.2 Release 4)
Bugs fixed:
- Another problem caused by double click on LogMeTT tray icon was fixed.
- The problem related to restoration of Configuration window placement was
fixed.

2005-03-14 (Ver 2.2 Release 3)
Bugs fixed:
- The error messages was showing on double click LogMeTT icon on the very first
run of application.

2005-03-14 (Ver 2.2 Release 2)
Bugs fixed:
- LockWorkStation function was causing LogMeTT to fail under Windows 98.
- New version related info was not displayed properly under Help menu of
Configuration window.

2005-03-13 (Ver 2.2 Release 1)
New features added:
- Modified the way LogMeTT Popup menu is called. It shows up only on Right click
from now.
- Added double click actions support to LogMeTT tray icon. The actions are
configurable from Settings tab of Configuration screen.
- Redesigned Color setting page.
- Added several predefined color schemas.
- Introduced pop up hint showing IP address on the PC when mouse is over LogMeTT
tray icon.
Bugs fixed:
- Text selection change when using mouse in connection name editor was not
updating the states of the speed buttons (Cut, Copy, Paste, etc.)
- The colors were not always updated after changing selected node.
- Double click of on LTT file was causing opening of the file instead of
Importing it.
- On moving the nodes Save button was not becoming active.
- Temp_TTL directory was not emptied on exit from LogMeTT.
- INS/OVR should not be showing when not in text editor mode.
- Updated setting were not passed to LogMeTT immediately but only after closing
Configuration window.
- On node change when in Modified state, answering "Cancel" on question whether
to save changes was causing lose of the changes.
- Last selection was lost if opening Configuration screen from LogMeTT.
- Added restriction of dropping the node on itself and on separator line.
- When clicking on speed buttons some of them were repainted incorrectly while
switching to disabled state.

2005-02-28 (Ver 2.1 Release 1)
New features added:
- Added auto scrolling on expand connection tree.
- Added auto scrolling of the tree while dragging the node.
- Added check for updates menu items for LogMeTT and TeraTerm under Help menu.
- Added Expand All and Collapse All menu items.
- Introduced new version numbering major.minor.build.
- On collapsing the branch where one of the child nodes was being edited prompt
was added to notifying user that the changes can be lost.
Bugs fixed:
- Cut/Copy/Paste buttons were enabled mistakenly when no node selected.
- Disabled 'Run selected' menu whenever not applicable.
- Fixed 'Last updated' message not showing in status bar is some cases.
- Moving the node using drag & drop between 2 separator lines was not working.
- Disabled TTermPro and LogMeTTc menu items in LogMeTT if they are not in
current directory.
- Temporarily disabled transparency when connection starts from LogMeTT since
this is affecting color settings. Transparency support will be available as an
option in one of the future LogMeTT releases.
- File Import was not working on double click on LTT file in Windows Explorer.
- On exit from LogMeTT Configuration window was not closing.


2005-02-22 (Ver 2.03 beta)
- Added support on shortcuts for Cut/Copy/Paste operations while editing
Connection name.
- Fixed the bug causing the last node in the tree to disappear or to show on the
wrong level.

2005-02-20 (Ver 2.02 beta)
- New feature added. The macro can now contain keyword $CONNECTION$ that will be
substituted during macro execution with the first word from connection name.
Thanks to Manfred for suggesting this feature. Check posting
http://www.neocom.ca/forum/viewtopic.php?t=47 for more details.
- Minor bug fixed related to update of right part of the window on tree node
selection change.

2005-02-19 (Ver 2.01 beta)
Beta version of LogMeTT that is completely redesigned LogMeIn application was
released. About 95% of the source code was rewritten.

=========================
LogMeTT is freeware application distributed without the source from
NeoCom Ltd. web site www.neocom.ca
NeoCom Ltd. is the solo Copyright owner of LogMeTT.