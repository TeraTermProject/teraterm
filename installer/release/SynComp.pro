# ##########################################################################################
# This is Syntax Completion Proposal definition file for TeraTerm Macro language.
# File name: SynComp.pro
# Created: February 11, 2007
# Author:  Boris Maisuradze
# Last updated: February 15, 2008
# The file is used by applications LogMeTT and TTLEdit.
# The file has to be stored in TeraTerm installation directory.
# Press <CTRL-Space> while in macro editor to activate Syntax Completion Proposal.
# For questions and support visit TeraTerm support forum at http://www.neocom.ca/forum
# You are free to modify this file, however please keep above notice unchanged.
# ##########################################################################################


# Communication commands

BplusRecv::command \column{}\style{+B}BplusRecv \style{-B}
BplusSend::command \column{}\style{+B}BplusSend \style{-B}<filename>
CallMenu::command \column{}\style{+B}CallMenu \style{-B}<menu ID>
ChangeDir::command \column{}\style{+B}ChangeDir \style{-B}<path>
ClearScreen::command \column{}\style{+B}ClearScreen \style{-B}<int>
CloseTT::command \column{}\style{+B}CloseTT \style{-B}
Connect 'myserver:23 /nossh'::command \column{}\style{+B}Connect \style{-B} 'myserver:23 /nossh'
Connect 'myserver:22 /ssh /1'::command \column{}\style{+B}Connect \style{-B} 'myserver:22 /ssh /1'
Connect 'myserver:22 /ssh /1 /auth=password /user=username /passwd=password'::command \column{}\style{+B}Connect \style{-B} 'myserver:22 /ssh /1 /auth=password /user=username /passwd=password'
Connect 'myserver:22 /ssh /1 /auth=publickey/user=username /passwd=password /keyfile=private-key-file'::command \column{}\style{+B}Connect \style{-B} 'myserver:22 /ssh /1 /auth=publickey/user=username /passwd=password /keyfile=private-key-file'
Connect 'myserver:22 /ssh /2'::command \column{}\style{+B}Connect \style{-B} 'myserver:22 /ssh /2'
Connect 'myserver:22 /ssh /2 /auth=password /user=username /passwd=password'::command \column{}\style{+B}Connect \style{-B} 'myserver:22 /ssh /2 /auth=password /user=username /passwd=password'
Connect 'myserver:22 /ssh /2 /auth=publickey/user=username /passwd=password /keyfile=private-key-file'::command \column{}\style{+B}Connect \style{-B} 'myserver:22 /ssh /2 /auth=publickey/user=username /passwd=password /keyfile=private-key-file'
Connect 'myserver /timeout=<value>'::command \column{}\style{+B}Connect \style{-B} '/timeout=<value>'
Connect '/C=x'::command \column{}\style{+B}Connect \style{-B} '/C=x'
CygConnect::command \column{}\style{+B}CygConnect \style{-B}
CygConnect::command \column{}\style{+B}CygConnect \style{-B}<command line parameters>
Disconnect::command \column{}\style{+B}Disconnect \style{-B}
EnableKeyb <flag>::command \column{}\style{+B}EnableKeyb \style{-B} <flag>
FlushRecv::command \column{}\style{+B}FlushRecv \style{-B}
GetTitle <strvar>::command \column{}\style{+B}GetTitle \style{-B} <strvar>
KmtFinish <filename>::command \column{}\style{+B}KmtFinish \style{-B} <filename>
KmtGet <filename>::command \column{}\style{+B}KmtGet \style{-B} <filename>
KmtRecv::command \column{}\style{+B}KmtRecv \style{-B}
KmtSend <filename>::command \column{}\style{+B}KmtSend \style{-B} <filename>
LoadKeymap <filename>::command \column{}\style{+B}LoadKeymap \style{-B} <filename>
LogClose::command \column{}\style{+B}LogClose \style{-B}
LogOpen <filename> <binary flag> <append flag>::command \column{}\style{+B}LogOpen \style{-B} <filename> <binary flag> <append flag>
LogPause::command \column{}\style{+B}LogPause \style{-B}
LogStart::command \column{}\style{+B}LogStart \style{-B}
LogWrite <string>::command \column{}\style{+B}LogWwrite \style{-B} <string>
QuickVanRecv::command \column{}\style{+B}QuickVanRecv \style{-B}
QuickVanSend <filename>::command \column{}\style{+B}QuickVanSend \style{-B} <filename>
Recvln::command \column{}\style{+B}Recvln \style{-B}
RestoreSetup <filename>::command \column{}\style{+B}RestoreSetup \style{-B} <filename>
ScpRecv::command \column{}\style{+B}ScpRecv \style{-B} <remote filename>
ScpRecv::command \column{}\style{+B}ScpRecv \style{-B} <remote filename> [<local filename>]
ScpSend::command \column{}\style{+B}ScpSend \style{-B} <filename>
ScpSend::command \column{}\style{+B}ScpSend \style{-B} <filename> [<destination filename>]
Send <data1> <data2> ...::command \column{}\style{+B}Send \style{-B} <data1> <data2> ...
SendBreak::command \column{}\style{+B}SendBreak \style{-B}
SendFile <filename> <binary flag>::command \column{}\style{+B}SendFile \style{-B} <filename> <binary flag>
SendKCode <key code> <repeat count>::command \column{}\style{+B}SendKCode \style{-B} <key code> <repeat count>
Sendln <data1> <data2> ...::command \column{}\style{+B}Sendln \style{-B} <data1> <data2> ...
SetBaud <value>::command \column{}\style{+B}SetBaud \style{-B} <value>
SetEcho <echo flag>::command \column{}\style{+B}SetEcho \style{-B} <echo flag>
SetSync <sync flag>::command \column{}\style{+B}SetSync \style{-B} <sync flag>
SetTitle <title>::command \column{}\style{+B}SetTitle \style{-B} <title>
ShowTT <show flag>::command \column{}\style{+B}ShowTT \style{-B} <show flag>
TestLink::command \column{}\style{+B}TestLink \style{-B}
Unlink::command \column{}\style{+B}Unlink \style{-B}
Wait <string1> <string2> ...::command \column{}\style{+B}Wait \style{-B} <string1> <string2> ...
WaitEvent <events>::command \column{}\style{+B}WaitEvent \style{-B} <events>
Waitln <string1> <string2> ...::command \column{}\style{+B}Waitln \style{-B} <string1> <string2> ...
WaitRecv <sub-string> <len> <pos>::command \column{}\style{+B}WaitRecv \style{-B} <sub-string> <len> <pos>
WaitRegex <string1 with regular expression> <string2 with regular expression> ...::command \column{}\style{+B}WaitRegex \style{-B} <string1 with regular expression> <string2 with regular expression> ...
XmodemRecv <filename> <binary flag> <option>::command \column{}\style{+B}XmodemRecv \style{-B} <filename> <binary flag> <option>
XmodemSend <filename> <option>::command \column{}\style{+B}XmodemSend \style{-B} <filename> <option>
ZmodemRecv::command \column{}\style{+B}ZmodemRecv \style{-B}
ZmodemSend <filename> <binary flag>::command \column{}\style{+B}ZmodemSend \style{-B} <filename> <binary flag>

# Control commands

Break::command \column{}\style{+B}Break \style{-B}
Call <label>::command \column{}\style{+B}Call \style{-B} <label>
Do::command \column{}\style{+B}Do \style{-B}
Do [{ while | until } <int> (option)]::command \column{}\style{+B}Do \style{-B} [{ while | until } <int> (option)]
End::command \column{}\style{+B}End \style{-B}
ExecCmnd <statement>::command \column{}\style{+B}ExecCmnd \style{-B} <statement>
Exit::command \column{}\style{+B}Exit \style{-B}
For <intvar> <first> <last>::command \column{}\style{+B}For \style{-B} <intvar> <first> <last>\style{+B}Next\style{-B}
Next::command \column{}\style{+B}For \style{-B} <intvar> <first> <last>\style{+B}Next\style{-B}
Goto <label>::command \column{}\style{+B}Goto \style{-B} <label>
If <int> Then::command \column{}\style{+B}If \style{-B} <int> \style{+B}Then\style{-B} <statement 1> \style{+B}[ ElseIf \style{-B}<int 2> \style{+B}Then \style{-B}<statement 2> \style{+B}] [ Else\style{-B} <statement 3> \style{+B}]  EndIf\style{-B}
Then::command \column{}\style{+B}Then\style{-B} <statement 1> \style{+B}[ ElseIf \style{-B}<int 2> \style{+B}Then \style{-B}<statement 2> \style{+B}] [ Else\style{-B} <statement 3> \style{+B}]  EndIf\style{-B}
ElseIf::command \column{}\style{+B}ElseIf \style{-B}<int 2> \style{+B}Then \style{-B}<statement 2> \style{+B} [ Else\style{-B} <statement 3> \style{+B}]  EndIf\style{-B}
Else::command \column{}\style{+B}Else\style{-B} <statement 3> \style{+B}  EndIf\style{-B}
EndIf::command \column{}\style{+B}EndIf\style{-B}
Include <include file name>::command \column{}\style{+B}Include \style{-B}<include file name>
Loop::command \column{}\style{+B}Loop \style{-B}
Loop [{ while | until } <int> (option)]::command \column{}\style{+B}Loop \style{-B} [{ while | until } <int> (option)]
Mpause <time>::command \column{}\style{+B}Mpause \style{-B}<time>
Pause <time>::command \column{}\style{+B}Pause \style{-B}<time>
Return::command \column{}\style{+B}Return \style{-B}
Until <int>::command \column{}\style{+B}Until \style{-B}<int>\style{+B}... EndUntil\style{-B}
EndUntil::command \column{}\style{+B}EndUntil \style{-B}
While <int>::command \column{}\style{+B}While \style{-B}<int>\style{+B}... EndWhile\style{-B}
EndWhile::command \column{}\style{+B}EndWhile \style{-B}

# String operation commands

Code2Str <strvar> <ASCII code>::command \column{}\style{+B}Code2Str \style{-B}<strvar> <ASCII code>
Int2Str <strvar> <integer value>::command \column{}\style{+B}Int2Str \style{-B}<strvar> <integer value>
Sprintf FORMAT [argument]>::command \column{}\style{+B}Sprintf \style{-B}FORMAT [argument]
Str2Code <intvar> <string>::command \column{}\style{+B}Str2Code \style{-B}<intvar> <string>
Str2Int <intvar> <string>::command \column{}\style{+B}Str2Int \style{-B}<intvar> <string>
StrCompare <string1> <string2>::command \column{}\style{+B}StrCompare \style{-B} <string1> <string2>
StrConcat <strvar> <string>::command \column{}\style{+B}StrConcat \style{-B}<strvar> <string>
StrCopy <string> <pos> <len> <strvar>::command \column{}\style{+B}StrCopy \style{-B} <string> <pos> <len> <strvar>
StrLen <string>::command \column{}\style{+B}StrLen \style{-B}<string>
StrScan <string> <substring>::command \column{}\style{+B}StrScan \style{-B}<string> <substring>
ToLower <strvar> <string>::command \column{}\style{+B}ToLower \style{-B}<strvar> <string>
ToUpper <strvar> <string>::command \column{}\style{+B}ToUpper \style{-B}<strvar> <string>

# File operation commands

FileClose <file handle>::command \column{}\style{+B}FileClose \style{-B}<file handle>
FileConcat <file1> <file2>::command \column{}\style{+B}FileConcat \style{-B}<file1> <file2>
FileCopy <file1> <file2>::command \column{}\style{+B}FileCopy \style{-B}<file1> <file2>
FileCreate <file handle> <filename>::command \column{}\style{+B}FileCreate \style{-B}<file handle> <filename>
FileDelete <filename>::command \column{}\style{+B}FileDelete \style{-B}<filename>
FileMarkPtr <file handle>::command \column{}\style{+B}FileMarkPtr \style{-B}<file handle>
FileOpen <file handle> <file name> <append flag>::command \column{}\style{+B}FileOpen \style{-B}<file handle> <file name> <append flag>
FileReadln <file handle> <strvar>::command \column{}\style{+B}FileReadln \style{-B}<file handle> <strvar>
FileRename <file1> <file2>::command \column{}\style{+B}FileRename \style{-B}<file1> <file2>
FileSearch <filename>::command \column{}\style{+B}FileSearch \style{-B}<filename>
FileSeek <file handle> <offset> <origin>::command \column{}\style{+B}FileSeek \style{-B}<file handle> <offset> <origin>
FileSeekBack <file handle>::command \column{}\style{+B}FileSeekBack \style{-B}<file handle>
FileStrSeek <file handle> <string>::command \column{}\style{+B}FileStrSeek \style{-B}<file handle> <string>
FileStrSeek2 <file handle> <string>::command \column{}\style{+B}FileStrSeek2 \style{-B}<file handle> <string>
FileWrite <file handle> <string>::command \column{}\style{+B}FileWrite \style{-B}<file handle> <string>
FileWriteln <file handle> <string>::command \column{}\style{+B}FileWriteln \style{-B}<file handle> <string>
FindFirst <dir handle> <file name> <strvar>::command \column{}\style{+B}FindFirst \style{-B}<dir handle> <file name> <strvar>
FindNext <dir handle> <strvar>::command \column{}\style{+B}FindNext \style{-B}<dir handle> <strvar>
FindClose <dir handle>::command \column{}\style{+B}FindClose \style{-B} <dir handle>
GetDir <strvar>::command \column{}\style{+B}GetDir \style{-B}<strvar>
MakePath <strvar> <dir> <name>::command \column{}\style{+B}MakePath \style{-B}<strvar> <dir> <name>
SetDir <dir>::command \column{}\style{+B}SetDir \style{-B}<dir>

# Password commands

DelPassword <filename> <password name>::command \column{}\style{+B}DelPassword \style{-B}<filename> <password name>
GetPassword <filename> <password name> <strvar>::command \column{}\style{+B}GetPassword \style{-B}<filename> <password name> <strvar>
PasswordBox <message> <title>::command \column{}\style{+B}PasswordBox \style{-B}<message> <title>
PasswordBox <message> <title>::command \column{}\style{+B}PasswordBox \style{-B}<message> <title> [<special>]

# Miscellaneous commands

Beep::command \column{}\style{+B}Beep \style{-B}
ClosesBox::command \column{}\style{+B}ClosesBox \style{-B}
Clipb2Var <strvar>::command \column{}\style{+B}Clipb2Var \style{-B}<strvar>
Clipb2Var <strvar> [<offset>]::command \column{}\style{+B}Clipb2Var \style{-B}<strvar> [<offset>]
Exec <command line>::command \column{}\style{+B}Exec \style{-B}<command line>
FileNameBox <title>::command \column{}\style{+B}FileNameBox \style{-B} <title>
GetDate <strvar>::command \column{}\style{+B}GetDate \style{-B}<strvar>
GetEnv <envname> <strvar>::command \column{}\style{+B}GetEnv \style{-B}<envname> <strvar>
GetTime <strvar>::command \column{}\style{+B}GetTime \style{-B}<strvar>
GetVer <strvar>::command \column{}\style{+B}GetVer \style{-B}<strvar>
GetVer <strvar> [<version>]::command \column{}\style{+B}GetVer \style{-B}<strvar> [<version>]
IfDefined <var>::command \column{}\style{+B}IfDefined \style{-B}<var>
InputBox <message> <title>::command \column{}\style{+B}InputBox \style{-B}<message> <title>
InputBox <message> <title>::command \column{}\style{+B}InputBox \style{-B}<message> <title> [<default> [<special>]]
MessageBox <message> <title>::command \column{}\style{+B}MessageBox \style{-B}<message> <title>
MessageBox <message> <title>::command \column{}\style{+B}MessageBox \style{-B}<message> <title> [<special>]
Random <integer variable> <max value>::command \column{}\style{+B}Random \style{-B}<integer variable> <max value>
RotateLeft <intvar> <intval> <count>::command \column{}\style{+B}RotateLeft \style{-B}<intvar> <intval> <count>
RotateRight <intvar> <intval> <count>::command \column{}\style{+B}RotateRight \style{-B}<intvar> <intval> <count>
SetDate <date>::command \column{}\style{+B}SetDate \style{-B}<date>
SetDlgPos <x> <y>::command \column{}\style{+B}SetDlgPos \style{-B}<x> <y>
SetEnv <envname> <strvar>::command \column{}\style{+B}SetEnv \style{-B}<envname> <strvar>
SetExitCode <exit code>::command \column{}\style{+B}SetExitCode \style{-B}<exit code>
SetTime <time>::command \column{}\style{+B}SetTime \style{-B}<time>
Show <show flag>::command \column{}\style{+B}Show \style{-B}<show flag>
StatusBox <message> <title>::command \column{}\style{+B}StatusBox \style{-B}<message> <title>
StatusBox <message> <title>::command \column{}\style{+B}StatusBox \style{-B}<message> <title> [<special>]
Var2Clipb <string>::command \column{}\style{+B}Var2Clipb \style{-B}<string>
YesNoBox <message> <title>::command \column{}\style{+B}YesNoBox \style{-B}<message> <title>
YesNoBox <message> <title>::command \column{}\style{+B}YesNoBox \style{-B}<message> <title> [<special>]

# Variables

GroupMatchStr1::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr2::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr3::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr4::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr5::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr6::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr7::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
GroupMatchStr8::variable \column{}\style{+B}GroupMatchStr1 \style{-B}
InputStr::variable \column{}\style{+B}InputStr \style{-B}
MatchStr::variable \column{}\style{+B}MatchStr \style{-B}
Param2::variable \column{}\style{+B}Param2 \style{-B}
Param3::variable \column{}\style{+B}Param3 \style{-B}
Param4::variable \column{}\style{+B}Param4 \style{-B}
Param5::variable \column{}\style{+B}Param5 \style{-B}
Param6::variable \column{}\style{+B}Param6 \style{-B}
Param7::variable \column{}\style{+B}Param7 \style{-B}
Param8::variable \column{}\style{+B}Param8 \style{-B}
Param9::variable \column{}\style{+B}Param9 \style{-B}
Result::variable \column{}\style{+B}Result \style{-B}
Timeout::variable \column{}\style{+B}Timeout \style{-B}

