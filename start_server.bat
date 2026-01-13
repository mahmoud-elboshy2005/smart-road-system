@echo off
echo Installing required NPM packages...

call npm install

echo.
echo Starting Smart Road System Server...
call npm run server

pause
