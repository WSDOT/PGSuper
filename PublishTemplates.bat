REM - Script to publish templates and master library to WSDOT ftp server

SET TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSuper\Version_2.7.3

del %TARGET%\WSDOT.*
del %TARGET%\AASHTO.*

\ARP\PGSuper\RegFreeCOM\x64\Release\makepgz.exe %TARGET%\WSDOT.pgz  \ARP\PGSuper\WSDOT.lbr  \ARP\PGSuper\Templates\WSDOT
\ARP\PGSuper\RegFreeCOM\x64\Release\makepgz.exe %TARGET%\AASHTO.pgz \ARP\PGSuper\AASHTO.lbr \ARP\PGSuper\Templates\AASHTO
