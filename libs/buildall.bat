CALL buildoniguruma6.bat
if ERRORLEVEL 1 (
    echo "buildall.bat を終了します"
    exit /b 1
)

CALL buildzlib.bat
if ERRORLEVEL 1 (
    echo "buildall.bat を終了します"
    exit /b 1
)

CALL buildopenssl11.bat
if ERRORLEVEL 1 (
    echo "buildall.bat を終了します"
    exit /b 1
)

CALL buildSFMT.bat
if ERRORLEVEL 1 (
    echo "buildall.bat を終了します"
    exit /b 1
)
