cd \ARP\PGSuper\KDOTExport\Documentation

doxygen Doxygen.dox

\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap KDOT
copy \ARP\PGSuper\KDOTExport\Documentation\doc\html\* \ARP\BridgeLink\Docs\KDOT
copy \ARP\PGSuper\KDOTExport\Documentation\KDOT.dm \ARP\BridgeLink\Docs\KDOT

