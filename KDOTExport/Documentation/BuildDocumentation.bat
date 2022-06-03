cd %ARPDIR%\PGSuper\KDOTExport\Documentation

doxygen Doxygen.dox

%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap KDOT
rmdir /S /Q %ARPDIR%\BridgeLink\Docs\KDOT\%1\
mkdir %ARPDIR%\BridgeLink\Docs\KDOT\%1\
copy %ARPDIR%\PGSuper\KDOTExport\Documentation\doc\html\* %ARPDIR%\BridgeLink\Docs\KDOT\%1\
copy %ARPDIR%\PGSuper\KDOTExport\Documentation\KDOT.dm %ARPDIR%\BridgeLink\Docs\KDOT\%1\

