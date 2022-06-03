REM - Build PGSuper Documentation

cd %ARPDIR%\PGSuper\Documentation\PGSuper
rmdir /S /Q doc
doxygen Doxygen.dox

REM - Build PGSplice Documentation
cd %ARPDIR%\PGSuper\Documentation\PGSplice
rmdir /S /Q doc
doxygen Doxygen.dox

REM - Build PGS Library Documentation
cd %ARPDIR%\PGSuper\Documentation\PGSLibrary
rmdir /S /Q doc
doxygen Doxygen.dox

REM - Build the Documentation Map File
cd %ARPDIR%\PGSuper\Documentation
%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap PGSuper

REM - Make a copy of the DM file for PGSplice and PGS Library
copy PGSuper.dm PGSplice.dm
copy PGSuper.dm PGSLibrary.dm

REM - Copy the documentation to BridgeLink
rmdir /S /Q %ARPDIR%\BridgeLink\Docs\PGSuper\%1\
mkdir %ARPDIR%\BridgeLink\Docs\PGSuper\%1\
copy %ARPDIR%\PGSuper\Documentation\PGSuper\doc\html\* %ARPDIR%\BridgeLink\Docs\PGSuper\%1\
copy %ARPDIR%\PGSuper\Documentation\PGSuper.dm %ARPDIR%\BridgeLink\Docs\PGSuper\%1\

rmdir /S /Q %ARPDIR%\BridgeLink\Docs\PGSplice\%1\
mkdir %ARPDIR%\BridgeLink\Docs\PGSplice\%1\
copy %ARPDIR%\PGSuper\Documentation\PGSplice\doc\html\* %ARPDIR%\BridgeLink\Docs\PGSplice\%1\
copy %ARPDIR%\PGSuper\Documentation\PGSplice.dm %ARPDIR%\BridgeLink\Docs\PGSplice\%1\

rmdir /S /Q %ARPDIR%\BridgeLink\Docs\PGSLibrary\%1\
mkdir %ARPDIR%\BridgeLink\Docs\PGSLibrary\%1\
copy %ARPDIR%\PGSuper\Documentation\PGSLibrary\doc\html\* %ARPDIR%\BridgeLink\Docs\PGSLibrary\%1\
copy %ARPDIR%\PGSuper\Documentation\PGSLibrary.dm %ARPDIR%\BridgeLink\Docs\PGSLibrary\%1\

