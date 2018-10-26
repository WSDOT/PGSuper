cd \ARP\PGSuper\KDOTExport\Documentation

doxygen Doxygen.dox

\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap KDOT
rmdir /S /Q \ARP\BridgeLink\Docs\KDOT\%1\
mkdir \ARP\BridgeLink\Docs\KDOT\%1\
copy \ARP\PGSuper\KDOTExport\Documentation\doc\html\* \ARP\BridgeLink\Docs\KDOT\%1\
copy \ARP\PGSuper\KDOTExport\Documentation\KDOT.dm \ARP\BridgeLink\Docs\KDOT\%1\

