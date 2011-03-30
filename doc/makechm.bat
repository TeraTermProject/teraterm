CALL convtext.bat

perl htmlhelp_index_make.pl ja html > ja\Index.hhk
perl htmlhelp_index_make.pl en html > en\Index.hhk

set HELP_COMPILER=C:\progra~1\htmlhe~1\hhc.exe

%HELP_COMPILER% ja\teraterm.hhp
%HELP_COMPILER% en\teraterm.hhp
