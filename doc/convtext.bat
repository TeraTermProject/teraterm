set TOSJIS=perl ..\installer\2sjis.pl
set REF_E=en\html\reference
set REF_J=ja\html\reference
%TOSJIS% -i ..\libs\oniguruma\COPYING   -o %REF_E%\Oniguruma-LICENSE.txt -l unix
%TOSJIS% -i ..\libs\oniguruma\COPYING   -o %REF_J%\Oniguruma-LICENSE.txt -l unix
%TOSJIS% -i ..\libs\oniguruma\doc\RE    -o %REF_E%\RE.txt      -c euc-jp -l unix
%TOSJIS% -i ..\libs\oniguruma\doc\RE.ja -o %REF_J%\RE.txt      -c euc-jp -l unix
%TOSJIS% -i ..\libs\openssl\LICENSE     -o %REF_E%\OpenSSL-LICENSE.txt   -l unix
%TOSJIS% -i ..\libs\openssl\LICENSE     -o %REF_J%\OpenSSL-LICENSE.txt   -l unix
%TOSJIS% -i ..\libs\putty\LICENCE       -o %REF_E%\PuTTY-LICENSE.txt     -l crlf
%TOSJIS% -i ..\libs\putty\LICENCE       -o %REF_J%\PuTTY-LICENSE.txt     -l crlf
%TOSJIS% -i ..\libs\SFMT\LICENSE.txt    -o %REF_E%\SFMT-LICENSE.txt      -l unix
%TOSJIS% -i ..\libs\SFMT\LICENSE.txt    -o %REF_J%\SFMT-LICENSE.txt      -l unix
%TOSJIS% -i ..\cygterm\COPYING          -o %REF_E%\CygTerm+-LICENSE.txt  -l unix
%TOSJIS% -i ..\cygterm\COPYING          -o %REF_J%\CygTerm+-LICENSE.txt  -l unix
