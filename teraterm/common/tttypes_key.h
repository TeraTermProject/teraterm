/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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

#include "tttypes.h"

#pragma once

/* Tera Term internal key codes */
#define IdUp               1
#define IdDown             2
#define IdRight            3
#define IdLeft             4
#define Id0                5
#define Id1                6
#define Id2                7
#define Id3                8
#define Id4                9
#define Id5               10
#define Id6               11
#define Id7               12
#define Id8               13
#define Id9               14
#define IdMinus           15
#define IdComma           16
#define IdPeriod          17
#define IdSlash           18
#define IdAsterisk        19
#define IdPlus            20
#define IdEnter           21
#define IdPF1             22
#define IdPF2             23
#define IdPF3             24
#define IdPF4             25
#define IdFind            26
#define IdInsert          27
#define IdRemove          28
#define IdSelect          29
#define IdPrev            30
#define IdNext            31
#define IdF6              32
#define IdF7              33
#define IdF8              34
#define IdF9              35
#define IdF10             36
#define IdF11             37
#define IdF12             38
#define IdF13             39
#define IdF14             40
#define IdHelp            41
#define IdDo              42
#define IdF17             43
#define IdF18             44
#define IdF19             45
#define IdF20             46
#define IdXF1             47
#define IdXF2             48
#define IdXF3             49
#define IdXF4             50
#define IdXF5             51
#define IdUDK6            52
#define IdUDK7            53
#define IdUDK8            54
#define IdUDK9            55
#define IdUDK10           56
#define IdUDK11           57
#define IdUDK12           58
#define IdUDK13           59
#define IdUDK14           60
#define IdUDK15           61
#define IdUDK16           62
#define IdUDK17           63
#define IdUDK18           64
#define IdUDK19           65
#define IdUDK20           66
#define IdHold            67
#define IdPrint           68
#define IdBreak           69
#define IdXBackTab        70
#define IdCmdEditCopy     71
#define IdCmdEditPaste    72
#define IdCmdEditPasteCR  73
#define IdCmdEditCLS      74
#define IdCmdEditCLB      75
#define IdCmdCtrlOpenTEK  76
#define IdCmdCtrlCloseTEK 77
#define IdCmdLineUp       78
#define IdCmdLineDown     79
#define IdCmdPageUp       80
#define IdCmdPageDown     81
#define IdCmdBuffTop      82
#define IdCmdBuffBottom   83
#define IdCmdNextWin      84
#define IdCmdPrevWin      85
#define IdCmdNextSWin     86
#define IdCmdPrevSWin     87
#define IdCmdLocalEcho    88
#define IdCmdScrollLock   89
#define IdUser1           90
#define NumOfUDK          IdUDK20-IdUDK6+1
#define NumOfUserKey      99
#define IdKeyMax          IdUser1+NumOfUserKey-1

// key code for macro commands
#define IdCmdDisconnect   1000
#define IdCmdLoadKeyMap   1001
#define IdCmdRestoreSetup 1002
