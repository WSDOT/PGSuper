REM - Script to publish templates and master library to WSDOT ftp server

SET PGSUPER_TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSuper\Version_3.0.0
SET PGSUPER_TEMPLATE_EXTENSION=PGT

del %PGSUPER_TARGET%\WSDOT.*
del %PGSUPER_TARGET%\AASHTO.*

\ARP\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\WSDOT.pgz  \ARP\PGSuper\Templates\WSDOT.lbr  \ARP\PGSuper\Templates\PGSuper\WSDOT   %PGSUPER_TEMPLATE_EXTENSION%
\ARP\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\AASHTO.pgz \ARP\PGSuper\Templates\AASHTO.lbr \ARP\PGSuper\Templates\PGSuper\AASHTO  %PGSUPER_TEMPLATE_EXTENSION%


SET PGSPLICE_TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSplice\Version_3.0.0
SET PGSPLICE_TEMPLATE_EXTENSION=SPT

del %PGSPLICE_TARGET%\WSDOT.*

\ARP\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSPLICE_TARGET%\WSDOT.pgz  \ARP\PGSuper\Templates\WSDOT.lbr  \ARP\PGSuper\Templates\PGSplice\WSDOT  %PGSPLICE_TEMPLATE_EXTENSION%
