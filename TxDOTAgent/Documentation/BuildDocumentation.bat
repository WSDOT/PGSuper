cd \ARP\PGSuper\TxDOTAgent\Documentation\TOGA

doxygen Doxygen.dox

\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap Toga
rmdir /S /Q \ARP\BridgeLink\Docs\TOGA\%1\
mkdir \ARP\BridgeLink\Docs\TOGA\%1\
copy \ARP\PGSuper\TxDOTAgent\Documentation\TOGA\doc\html\* \ARP\BridgeLink\Docs\TOGA\%1\
copy \ARP\PGSuper\TxDOTAgent\Documentation\TOGA\Toga.dm \ARP\BridgeLink\Docs\TOGA\%1\

cd \ARP\PGSuper\TxDOTAgent\Documentation\TxCADExport

doxygen Doxygen.dox

\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap TxCADExport
rmdir /S /Q \ARP\BridgeLink\Docs\TxCADExport\%1\
mkdir \ARP\BridgeLink\Docs\TxCADExport\%1\
copy \ARP\PGSuper\TxDOTAgent\Documentation\TxCADExport\doc\html\* \ARP\BridgeLink\Docs\TxCADExport\%1\
copy \ARP\PGSuper\TxDOTAgent\Documentation\TxCADExport\TxCADExport.dm \ARP\BridgeLink\Docs\TxCADExport\%1\

cd \ARP\PGSuper\TxDOTAgent\Documentation