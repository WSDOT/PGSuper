cd \ARP\PGSuper\TxDOTAgent\Documentation

doxygen Doxygen.dox

\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap Toga
rmdir /S /Q \ARP\BridgeLink\Docs\TOGA\%1\
mkdir \ARP\BridgeLink\Docs\TOGA\%1\
copy \ARP\PGSuper\TxDOTAgent\Documentation\doc\html\* \ARP\BridgeLink\Docs\TOGA\%1\
copy \ARP\PGSuper\TxDOTAgent\Documentation\Toga.dm \ARP\BridgeLink\Docs\TOGA\%1\

