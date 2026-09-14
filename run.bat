cls
pushd client
call npm.cmd exec tsc
if %errorlevel% neq 0 (
    echo TypeScript build failed
    popd
    pause
    cls
    exit /b 1
)
popd
pause

cls
cmake --build .\server\build
if %errorlevel% neq 0 (
    echo CMake build failed
    pause
    cls
    exit /b 1
)

cls
echo Opening in browser...
start http://localhost:8080
timeout /t 1 /nobreak >nul
echo Starting server...
.\server\build\tecmessage.exe
pause
cls
