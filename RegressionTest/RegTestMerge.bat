echo off
REM This custom merge script copies the datum file from the repository over the local file.
REM After a git pull you will have the most current datum
REM 
REM To get this custom merge script to work, do the following
REM edit your .git/config file by adding the following
REM
REM [merge "RegTestMerge"]
REM    name = Custom merge for regression test files
REM    driver = ./RegressionTest/RegTestMerge.bat %O %A %B
REM
REM All other settings are already in the git repository and you should
REM get them automatically
echo on
echo Reg Test Custom Merge ... keeping datum from remote
copy %3 %2

