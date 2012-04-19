@echo off

rem
rem OpenSSL + signtool を使って、オレオレコードサイニング証明書を付与する。
rem

if "%1"=="/?" goto help
if "%1"=="" goto help

SET exe="%1"
SET openssl=..\libs\openssl\out32\openssl.exe
SET sslconf=..\libs\openssl\apps\openssl.cnf
SET pass="teraterm"

rem SSL証明書と秘密鍵を作成する。

del /q cakey.pem cacert.pem certificate.pfx
echo パスフレーズは %pass% を入れてください

%openssl% req -new -x509 -keyout cakey.pem -out cacert.pem -days 365 -config %sslconf%
%openssl% pkcs12 -export -out certificate.pfx -inkey cakey.pem -in cacert.pem
signtool sign /f certificate.pfx /a /t http://timestamp.verisign.com/scripts/timstamp.dll /p teraterm %exe%

exit /b

:help
echo OpenSSL + signtool を使って、オレオレコードサイニング証明書を付与する。
echo.
echo   %0 Output\teraterm-4.72-RC1.exe
echo.
exit /b

