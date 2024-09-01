/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Help constants */

/* help context IDs */
#define HlpFileNewConnection    1
#define HlpFileLog              2
#define HlpFileSend             3
#define HlpFileKmtGet           4
#define HlpFileKmtSend          5
#define HlpFileXmodemRecv       6
#define HlpFileXmodemSend       7
#define HlpFileZmodemSend       8
#define HlpFileBPlusSend        9
#define HlpFileQVSend           10
#define HlpFilePrint            12
#define HlpSetupTerminal        13
#define HlpSetupWindow          15
#define HlpSetupFont            16
#define HlpSetupKeyboard        18
#define HlpSetupSerialPort      20
#define HlpSetupTCPIP           21
#define HlpSetupGeneral         22
#define HlpSetupSave            23
#define HlpSetupRestore         24
#define HlpSetupLoadKeyMap      25
#define HlpWindowWindow         26
#define HlpTEKFilePrint         27
#define HlpTEKSetupWindow       28
#define HlpTEKSetupFont         29
#define HlpFileYmodemRecv       30
#define HlpFileYmodemSend       31


#define HlpIndex    10000

#define HlpAboutModule          20001
#define HlpAboutCopyright       20002
#define HlpAboutRequirements    20003
#define HlpAboutEmulations      20004
#define HlpAboutHistory         20005
#define HlpAboutQanda           20006
#define HlpAboutRequests        20007
#define HlpAboutContacts        20008
#define HlpAboutWriter          20010
#define HlpAboutCtrlseq         20011
#define HlpAboutDifference      20012
#define HlpAboutForeword        20013

#define HlpAboutQandaConnection	20101

#define HlpUsageKeyboard                30100
#define HlpUsageMouse                   30200
#define HlpUsageShortcut                30300
#define HlpUsageSsh                     30400
#define HlpUsageCygwin                  30500
#define HlpUsageProxy                   30600
#define HlpUsageLogmettLogMetttutorial  30700
#define HlpUsageTipsIndex               30800
#define HlpUsageTipsConfig              30801
#define HlpUsageTipsLog                 30802
#define HlpUsageTipsShortcut            30803
#define HlpUsageTipsNumeric             30804
#define HlpUsageTipsVt100               30805
#define HlpUsageTipsPc                  30806
#define HlpUsageTipsNifty               30807
#define HlpUsageTipsPcvan               30808
#define HlpUsageTipsXmodem              30809
#define HlpUsageTipsZmodem              30810
#define HlpUsageTipsBplus               30811
#define HlpUsageTipsQuickvan            30812
#define HlpUsageTipsForward             30813
#define HlpUsageTipsNotport23           30814
#define HlpUsageTipsDoubleclick         30815
#define HlpUsageTipsNotc1char           30816
#define HlpUsageTipsIncorrectkout       30817
#define HlpUsageTipsAppkeypad           30818
#define HlpUsageTipsIme                 30819
#define HlpUsageTipsNamedpipe           30820
#define HlpUsageTipsTelnetprotocol      30821
#define HlpUsageTipsVim                 30822
#define HlpUsageTipsVirtualstore        30823
#define HlpUsageUnicode                 30900
#define HlpUsageTransparent             31000
#define HlpUsageTtmenuTtmenu            31100
#define HlpUsageKcodechange             31200
#define HlpUsageRecurringcommand        31300
#define HlpUsageResizemenu              31400
#define HlpUsageTtyrec                  31500

#define HlpMenuFile                         41000
#define HlpMenuFileNew                      41100
#define HlpMenuFileLog                      41200
#define HlpMenuFileSendfile                 41300
#define HlpMenuFileTransfer                 41400
#define HlpMenuFileTransferKermit           41410
#define HlpMenuFileTransferKermitGet        41411
#define HlpMenuFileTransferKermitSend       41412
#define HlpMenuFileTransferXmodem           41420
#define HlpMenuFileTransferXmodemReceive    41421
#define HlpMenuFileTransferXmodemSend       41422
#define HlpMenuFileTransferYmodem           41460
#define HlpMenuFileTransferZmodem           41430
#define HlpMenuFileTransferZmodemSend       41431
#define HlpMenuFileTransferBplus            41440
#define HlpMenuFileTransferBplusSend        41441
#define HlpMenuFileTransferQuickvan         41450
#define HlpMenuFileTransferQuickvanSend     41451
#define HlpMenuFileChdir                    41500
#define HlpMenuFilePrint                    41600
#define HlpMenuEdit                         42000
#define HlpMenuSetup                        43000
#define HlpMenuSetupTerminal                43001
#define HlpMenuSetupWindow                  43002
#define HlpMenuSetupFont                    43003
#define HlpMenuSetupKeyboard                43004
#define HlpMenuSetupSerial                  43005
#define HlpMenuSetupProxy                   43006
#define HlpMenuSetupSsh                     43007
#define HlpMenuSetupSshauth                 43008
#define HlpMenuSetupSshforward              43009
#define HlpMenuSetupSshkeygen               43010
#define HlpMenuSetupTcpip                   43011
#define HlpMenuSetupAdditional              43012
#define HlpMenuSetupAdditionalCoding        43020
#define HlpMenuSetupAdditionalLog           43021
#define HlpMenuSetupAdditionalVisual        43022
#define HlpMenuSetupAdditionalFont          43023
#define HlpMenuSetupAdditionalTheme         43024
#define HlpMenuSetupThemeEditor             43025
#define HlpMenuSetupAdditionalKeyboard      43026
#define HlpMenuSetupAdditionalMouse         43027
#define HlpMenuSetupAdditionalGeneral       43028
#define HlpMenuSetupAdditionalTCPIP         43029
#define HlpMenuSetupAdditionalCopyAndPaste  43030
#define HlpMenuSetupSave                    43013
#define HlpMenuSetupRestore                 43014
#define HlpMenuSetupDir                     43016
#define HlpMenuSetupKeymap                  43015
#define HlpMenuControl                      44000
#define HlpMenuControlBroadcast             44001
#define HlpMenuWindow                       45000
#define HlpMenuWindowWindow                 45001
#define HlpMenuHelp                         46000
#define HlpMenuResize                       47000

#define HlpMenutekFile          50000
#define HlpMenutekFilePrint     50001
#define HlpMenutekEdit          51000
#define HlpMenutekSetup         52000
#define HlpMenutekSetupWindow   52001
#define HlpMenutekSetupFont     52002
#define HlpMenutekVtwin         53000
#define HlpMenutekWindow        54000
#define HlpMenutekWindowWindow  54001
#define HlpMenutekHelp          55000

#define HlpCmdlineTeraterm  60001
#define HlpCmdlineMacro     60002
#define HlpCmdlineTtssh     60003
#define HlpCmdlineCygterm   60004
#define HlpCmdlineTtproxy   60005
#define HlpCmdlineTtyplay   60006

#define HlpSetupTeraterm        71000
#define HlpSetupTeratermTerm    71001
#define HlpSetupTeratermWin     71002
#define HlpSetupTeratermCom     71003
#define HlpSetupTeratermTrans   71004
#define HlpSetupTeratermPrint   71005
#define HlpSetupTeratermMisc    71006
#define HlpSetupTeratermIni     71007
#define HlpSetupTeratermSSH     71008
#define HlpSetupKbd             71000
#define HlpSetupSshknownhosts   72000
#define HlpSetupCygterm         73000
#define HlpSetupLang            74000

#define HlpRefKeycode   80001
#define HlpRefRe        80002
#define HlpRefOpenssl   80003
#define HlpRefOpenssh   80004
#define HlpRefDev       80005
#define HlpRefPutty     80006
#define HlpRefSource    80007
#define HlpRefOniguruma 80008
#define HlpRefSFMT      80009
#define HlpRefCygterm   80010
#define HlpRefZlib      80011
#define HlpRefArgon2    80012
#define HlpRefCjson     80013

#define HlpMacro                        90000
#define HlpMacroExec                    90001
#define HlpMacroCommandline             90002
#define HlpMacroAssoc                   90003
#define HlpMacroSyntax                  91000
#define HlpMacroSyntaxType              91001
#define HlpMacroSyntaxFile              91007
#define HlpMacroSyntaxFormat            91002
#define HlpMacroSyntaxIdent             91003
#define HlpMacroSyntaxVar               91004
#define HlpMacroSyntaxExp               91005
#define HlpMacroSyntaxLine              91006
#define HlpMacroCommand                 92000
#define HlpMacroCommandBasename         92187
#define HlpMacroCommandBeep             92001
#define HlpMacroCommandBplusrecv        92002
#define HlpMacroCommandBplussend        92003
#define HlpMacroCommandBreak            92120
#define HlpMacroCommandCall             92004
#define HlpMacroCommandCallmenu         92125
#define HlpMacroCommandChangedir        92005
#define HlpMacroCommandChecksum8        92204
#define HlpMacroCommandChecksum8file    92205
#define HlpMacroCommandChecksum16       92206
#define HlpMacroCommandChecksum16file   92207
#define HlpMacroCommandChecksum32       92208
#define HlpMacroCommandChecksum32file   92209
#define HlpMacroCommandBringupbox       92210

#define HlpMacroCommandClearscreen      92006
#define HlpMacroCommandClipb2var        92113
#define HlpMacroCommandClosesbox        92007
#define HlpMacroCommandClosett          92008
#define HlpMacroCommandCode2str         92009
#define HlpMacroCommandConnect          92010
#define HlpMacroCommandContinue         92155
#define HlpMacroCommandCrc16            92202
#define HlpMacroCommandCrc16File        92203
#define HlpMacroCommandCrc32            92138
#define HlpMacroCommandCrc32File        92139
#define HlpMacroCommandCygConnect       92130
#define HlpMacroCommandDelpassword      92011
#define HlpMacroCommandDelpassword2     92221
#define HlpMacroCommandDirname          92188
#define HlpMacroCommandDirnameBox       92214
#define HlpMacroCommandDisconnect       92012
#define HlpMacroCommandDispstr          92149
#define HlpMacroCommandDo               92126
#define HlpMacroCommandEnablekeyb       92015
#define HlpMacroCommandEnd              92016
#define HlpMacroCommandEndUntil         92129
#define HlpMacroCommandExec             92019
#define HlpMacroCommandExeccmnd         92020
#define HlpMacroCommandExit             92021
#define HlpMacroCommandExpandenv        92194
#define HlpMacroCommandFileclose        92022
#define HlpMacroCommandFileconcat       92023
#define HlpMacroCommandFilecopy         92024
#define HlpMacroCommandFilecreate       92025
#define HlpMacroCommandFiledelete       92026
#define HlpMacroCommandFileLock         92153
#define HlpMacroCommandFilemarkptr      92027
#define HlpMacroCommandFilenamebox      92124
#define HlpMacroCommandFileopen         92028
#define HlpMacroCommandFileread         92116
#define HlpMacroCommandFilereadln       92029
#define HlpMacroCommandFilerename       92030
#define HlpMacroCommandFilesearch       92031
#define HlpMacroCommandFileseek         92032
#define HlpMacroCommandFileseekback     92033
#define HlpMacroCommandFilestat         92178
#define HlpMacroCommandFilestrseek      92034
#define HlpMacroCommandFilestrseek2     92035
#define HlpMacroCommandFiletruncate     92179
#define HlpMacroCommandFileUnLock       92154
#define HlpMacroCommandFilewrite        92036
#define HlpMacroCommandFilewriteln      92037
#define HlpMacroCommandFindoperations   92039
#define HlpMacroCommandFlushrecv        92041
#define HlpMacroCommandFoldercreate     92191
#define HlpMacroCommandFolderdelete     92192
#define HlpMacroCommandFoldersearch     92193
#define HlpMacroCommandFornext          92042
#define HlpMacroCommandGetdate          92043
#define HlpMacroCommandGetdir           92044
#define HlpMacroCommandGetenv           92045
#define HlpMacroCommandGetfileattr      92189
#define HlpMacroCommandGethostname      92141
#define HlpMacroCommandGetipv4addr      92199
#define HlpMacroCommandGetipv6addr      92200
#define HlpMacroCommandGetmodemstatus   92213
#define HlpMacroCommandGetpassword      92046
#define HlpMacroCommandGetpassword2     92220
#define HlpMacroCommandGetspecialfolder 92195
#define HlpMacroCommandGettime          92047
#define HlpMacroCommandGettitle         92048
#define HlpMacroCommandGetttdir         92140
#define HlpMacroCommandGetttpos         92223
#define HlpMacroCommandGetver           92133
#define HlpMacroCommandGoto             92049
#define HlpMacroCommandIfdefined        92115
#define HlpMacroCommandIfthenelseif     92050
#define HlpMacroCommandInclude          92051
#define HlpMacroCommandInputbox         92052
#define HlpMacroCommandInt2str          92053
#define HlpMacroCommandIntdim           92150
#define HlpMacroCommandIsPassword       92197
#define HlpMacroCommandIsPassword2      92222
#define HlpMacroCommandKmtfinish        92054
#define HlpMacroCommandKmtget           92055
#define HlpMacroCommandKmtrecv          92056
#define HlpMacroCommandKmtsend          92057
#define HlpMacroCommandListBox          92198
#define HlpMacroCommandLoadkeymap       92058
#define HlpMacroCommandLogautoclose     92211
#define HlpMacroCommandLogclose         92059
#define HlpMacroCommandLoginfo          92152
#define HlpMacroCommandLogopen          92060
#define HlpMacroCommandLogpause         92061
#define HlpMacroCommandLogrotate        92201
#define HlpMacroCommandLogstart         92062
#define HlpMacroCommandLogwrite         92063
#define HlpMacroCommandLoop             92127
#define HlpMacroCommandMakepath         92064
#define HlpMacroCommandMessagebox       92065
#define HlpMacroCommandMpause           92111
#if defined(OUTPUTDEBUGSTRING_ENABLE)
#define HlpMacroCommandOutputdebugstring 92216
#endif
#define HlpMacroCommandPasswordbox      92067
#define HlpMacroCommandPause            92068
#define HlpMacroCommandQuickvanrecv     92069
#define HlpMacroCommandQuickvansend     92070
#define HlpMacroCommandRandom           92112
#define HlpMacroCommandRecvln           92071
#define HlpMacroCommandRegexoption      92156
#define HlpMacroCommandRestoresetup     92072
#define HlpMacroCommandReturn           92073
#define HlpMacroCommandRotateleft       92122
#define HlpMacroCommandRotateright      92121
#define HlpMacroCommandScprecv          92131
#define HlpMacroCommandScpsend          92132
#define HlpMacroCommandSend             92074
#define HlpMacroCommandSendbinary       92218
#define HlpMacroCommandSendbreak        92075
#define HlpMacroCommandSendbroadcast    92144
#define HlpMacroCommandSendlnbroadcast  92147
#define HlpMacroCommandSendfile         92076
#define HlpMacroCommandSendkcode        92077
#define HlpMacroCommandSendln           92078
#define HlpMacroCommandSendlnmulticast  92157
#define HlpMacroCommandSendmulticast    92145
#define HlpMacroCommandSendtext         92217
#define HlpMacroCommandSetbaud          92134
#define HlpMacroCommandSetdate          92079
#define HlpMacroCommandSetDebug         92175
#define HlpMacroCommandSetdir           92080
#define HlpMacroCommandSetdlgpos        92081
#define HlpMacroCommandSetdtr           92137
#define HlpMacroCommandSetenv           92123
#define HlpMacroCommandSetecho          92082
#define HlpMacroCommandSetexitcode      92083
#define HlpMacroCommandSetfileattr      92190
#define HlpMacroCommandSetflowctrl      92215
#define HlpMacroCommandSetMulticastName 92146
#define HlpMacroCommandSetPassword      92196
#define HlpMacroCommandSetPassword2     92219
#define HlpMacroCommandSetrts           92136
#define HlpMacroCommandSetspeed         92217
#define HlpMacroCommandSetsync          92084
#define HlpMacroCommandSettime          92085
#define HlpMacroCommandSettitle         92086
#define HlpMacroCommandShow             92087
#define HlpMacroCommandShowtt           92088
#define HlpMacroCommandSprintf          92117
#define HlpMacroCommandSprintf2         92142
#define HlpMacroCommandStatusbox        92089
#define HlpMacroCommandStr2code         92090
#define HlpMacroCommandStr2int          92091
#define HlpMacroCommandStrcompare       92092
#define HlpMacroCommandStrconcat        92093
#define HlpMacroCommandStrcopy          92094
#define HlpMacroCommandStrdim           92151
#define HlpMacroCommandStrinsert        92180
#define HlpMacroCommandStrjoin          92185
#define HlpMacroCommandStrlen           92095
#define HlpMacroCommandStrmatch         92135
#define HlpMacroCommandStrremove        92181
#define HlpMacroCommandStrreplace       92182
#define HlpMacroCommandStrscan          92096
#define HlpMacroCommandStrspecial       92186
#define HlpMacroCommandStrsplit         92184
#define HlpMacroCommandStrtrim          92183
#define HlpMacroCommandTestlink         92097
#define HlpMacroCommandTolower          92118
#define HlpMacroCommandToupper          92119
#define HlpMacroCommandUnlink           92099
#define HlpMacroCommandUntil            92128
#define HlpMacroCommandUptime           92212
#define HlpMacroCommandVar2clipb        92114
#define HlpMacroCommandWait             92100
#define HlpMacroCommandWait4all         92148
#define HlpMacroCommandWaitevent        92101
#define HlpMacroCommandWaitln           92102
#define HlpMacroCommandWaitn            92143
#define HlpMacroCommandWaitrecv         92103
#define HlpMacroCommandWaitregex        92110
#define HlpMacroCommandWhile            92104
#define HlpMacroCommandXmodemrecv       92105
#define HlpMacroCommandXmodemsend       92106
#define HlpMacroCommandYesnobox         92107
#define HlpMacroCommandYmodemrecv       92176
#define HlpMacroCommandYmodemsend       92177
#define HlpMacroCommandZmodemrecv       92108
#define HlpMacroCommandZmodemsend       92109

#define HlpMacroAppendixes              93000
#define HlpMacroAppendixesError         93001
#define HlpMacroAppendixesNewline       93002
#define HlpMacroAppendixesNegative      93003
#define HlpMacroAppendixesAscii         93004
#define HlpMacroAppendixesRegex         93005
#define HlpMacroAppendixesDisconnect    93006

#define HlpUninstall    100000
