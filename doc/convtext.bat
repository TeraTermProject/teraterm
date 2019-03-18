set TOSJIS=perl ..\installer\2sjis.pl
set ZLIBCP=perl ..\installer\copy-zliblicense.pl
set REF_E=en\html\reference
set REF_J=ja\html\reference
%TOSJIS% -i ..\libs\oniguruma\COPYING   -o %REF_E%\Oniguruma-LICENSE.txt -l unix
%TOSJIS% -i ..\libs\oniguruma\COPYING   -o %REF_J%\Oniguruma-LICENSE.txt -l unix
%TOSJIS% -i ..\libs\oniguruma\doc\RE    -o %REF_E%\RE.txt                -l unix
%TOSJIS% -i ..\libs\oniguruma\doc\RE.ja -o %REF_J%\RE.txt      -c utf8   -l unix
%TOSJIS% -i ..\libs\openssl\LICENSE     -o %REF_E%\OpenSSL-LICENSE.txt   -l unix
%TOSJIS% -i ..\libs\openssl\LICENSE     -o %REF_J%\OpenSSL-LICENSE.txt   -l unix
%TOSJIS% -i ..\libs\putty\LICENCE       -o %REF_E%\PuTTY-LICENSE.txt     -l crlf
%TOSJIS% -i ..\libs\putty\LICENCE       -o %REF_J%\PuTTY-LICENSE.txt     -l crlf
%TOSJIS% -i ..\libs\SFMT\LICENSE.txt    -o %REF_E%\SFMT-LICENSE.txt      -l unix
%TOSJIS% -i ..\libs\SFMT\LICENSE.txt    -o %REF_J%\SFMT-LICENSE.txt      -l unix
%TOSJIS% -i ..\cygterm\COPYING          -o %REF_E%\CygTerm+-LICENSE.txt  -l unix
%TOSJIS% -i ..\cygterm\COPYING          -o %REF_J%\CygTerm+-LICENSE.txt  -l unix
%ZLIBCP% -i ..\libs\zlib\README         -o %REF_E%\zlib-LICENSE.txt      -l unix
%ZLIBCP% -i ..\libs\zlib\README         -o %REF_J%\zlib-LICENSE.txt      -l unix

perl -pe 'tr/\x{feff}//d' ja/html/reference/build_with_cmake.md | perl Markdown_1.0.1/Markdown.pl > ja/html/reference/build_with_cmake_utf8.html
%TOSJIS% -i ja/html/reference/build_with_cmake_utf8.html -o ja/html/reference/build_with_cmake.html  -c utf8
