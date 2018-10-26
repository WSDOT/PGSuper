echo off

cscript.exe \ARP\PGSuper\RegressionTest\runregression.js /APGSuper /VRELEASE /Px64 %1 %2

if %errorlevel% NEQ 0 goto nopt2

IF "%1"=="/n" goto nopt2
IF "%1"=="/N" goto nopt2

call UpdateTestResults.bat

:nopt2
