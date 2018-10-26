REM - Build PGSuper Documentation
cd \ARP\PGSuper\Documentation\PGSuper
doxygen Doxygen.dox

REM - Build PGSplice Documentation
cd \ARP\PGSuper\Documentation\PGSplice
doxygen Doxygen.dox

REM - Build PGS Library Documentation
cd \ARP\PGSuper\Documentation\PGSLibrary
doxygen Doxygen.dox

REM - Build the Documentation Map File
cd \ARP\PGSuper\Documentation
\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap PGSuper

REM - Make a copy of the DM file for PGSplice and PGS Library
copy PGSuper.dm PGSplice.dm
copy PGSuper.dm PGSLibrary.dm

REM - Copy the documentation to BridgeLink
copy \ARP\PGSuper\Documentation\PGSuper\doc\html\* \ARP\BridgeLink\Docs\PGSuper
copy \ARP\PGSuper\Documentation\PGSuper.dm \ARP\BridgeLink\Docs\PGSuper

copy \ARP\PGSuper\Documentation\PGSplice\doc\html\* \ARP\BridgeLink\Docs\PGSplice
copy \ARP\PGSuper\Documentation\PGSplice.dm \ARP\BridgeLink\Docs\PGSplice

copy \ARP\PGSuper\Documentation\PGSLibrary\doc\html\* \ARP\BridgeLink\Docs\PGSLibrary
copy \ARP\PGSuper\Documentation\PGSLibrary.dm \ARP\BridgeLink\Docs\PGSLibrary

