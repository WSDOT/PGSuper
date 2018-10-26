REM - Build PGSuper Documentation

cd \ARP\PGSuper\Documentation\PGSuper
rmdir /S /Q doc
doxygen Doxygen.dox

REM - Build PGSplice Documentation
cd \ARP\PGSuper\Documentation\PGSplice
rmdir /S /Q doc
doxygen Doxygen.dox

REM - Build PGS Library Documentation
cd \ARP\PGSuper\Documentation\PGSLibrary
rmdir /S /Q doc
doxygen Doxygen.dox

REM - Build the Documentation Map File
cd \ARP\PGSuper\Documentation
\ARP\BridgeLink\RegFreeCOM\x64\Release\MakeDocMap PGSuper

REM - Make a copy of the DM file for PGSplice and PGS Library
copy PGSuper.dm PGSplice.dm
copy PGSuper.dm PGSLibrary.dm

REM - Copy the documentation to BridgeLink
rmdir /S /Q \ARP\BridgeLink\Docs\PGSuper\%1\
mkdir \ARP\BridgeLink\Docs\PGSuper\%1\
copy \ARP\PGSuper\Documentation\PGSuper\doc\html\* \ARP\BridgeLink\Docs\PGSuper\%1\
copy \ARP\PGSuper\Documentation\PGSuper.dm \ARP\BridgeLink\Docs\PGSuper\%1\

rmdir /S /Q \ARP\BridgeLink\Docs\PGSplice\%1\
mkdir \ARP\BridgeLink\Docs\PGSplice\%1\
copy \ARP\PGSuper\Documentation\PGSplice\doc\html\* \ARP\BridgeLink\Docs\PGSplice\%1\
copy \ARP\PGSuper\Documentation\PGSplice.dm \ARP\BridgeLink\Docs\PGSplice\%1\

rmdir /S /Q \ARP\BridgeLink\Docs\PGSLibrary\%1\
mkdir \ARP\BridgeLink\Docs\PGSLibrary\%1\
copy \ARP\PGSuper\Documentation\PGSLibrary\doc\html\* \ARP\BridgeLink\Docs\PGSLibrary\%1\
copy \ARP\PGSuper\Documentation\PGSLibrary.dm \ARP\BridgeLink\Docs\PGSLibrary\%1\

