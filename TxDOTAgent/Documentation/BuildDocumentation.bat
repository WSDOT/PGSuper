cd %ARPDIR%\PGSuper\TxDOTAgent\Documentation\TOGA

doxygen Doxygen.dox

%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap Toga
rmdir /S /Q %ARPDIR%\BridgeLink\Docs\TOGA\%1\
mkdir %ARPDIR%\BridgeLink\Docs\TOGA\%1\
copy %ARPDIR%\PGSuper\TxDOTAgent\Documentation\TOGA\doc\html\* %ARPDIR%\BridgeLink\Docs\TOGA\%1\
copy %ARPDIR%\PGSuper\TxDOTAgent\Documentation\TOGA\Toga.dm %ARPDIR%\BridgeLink\Docs\TOGA\%1\

cd %ARPDIR%\PGSuper\TxDOTAgent\Documentation\TxCADExport

doxygen Doxygen.dox

%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap TxCADExport
rmdir /S /Q %ARPDIR%\BridgeLink\Docs\TxCADExport\%1\
mkdir %ARPDIR%\BridgeLink\Docs\TxCADExport\%1\
copy %ARPDIR%\PGSuper\TxDOTAgent\Documentation\TxCADExport\doc\html\* %ARPDIR%\BridgeLink\Docs\TxCADExport\%1\
copy %ARPDIR%\PGSuper\TxDOTAgent\Documentation\TxCADExport\TxCADExport.dm %ARPDIR%\BridgeLink\Docs\TxCADExport\%1\

cd %ARPDIR%\PGSuper\TxDOTAgent\Documentation