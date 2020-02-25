REM - Script to publish templates and master library to WSDOT ftp server

REM -------------------------------------------
REM - Publish PGSuper Configurations
REM -------------------------------------------

SET PGSUPER_TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSuper\Version_5.0.0
REM SET PGSUPER_TARGET=\\wsdot\Resources\Topics\Publish\Web\ProdCF\EESC\Bridge\software\PGSuper\Version_5.0.0
SET PGSUPER_TEMPLATE_EXTENSION=PGT

del %PGSUPER_TARGET%\WSDOT.*
del %PGSUPER_TARGET%\AASHTO.*

\ARP\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\WSDOT.pgz  \ARP\PGSuper\Configurations\WSDOT.lbr  \ARP\PGSuper\Configurations\PGSuper\WSDOT   %PGSUPER_TEMPLATE_EXTENSION%
\ARP\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\AASHTO.pgz \ARP\PGSuper\Configurations\AASHTO.lbr \ARP\PGSuper\Configurations\PGSuper\AASHTO  %PGSUPER_TEMPLATE_EXTENSION%

REM -------------------------------------------
REM - Publish PGSplice Configurations
REM -------------------------------------------


SET PGSPLICE_TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSplice\Version_5.0.0
REM SET PGSPLICE_TARGET=\\wsdot\Resources\Topics\Publish\Web\ProdCF\EESC\Bridge\software\PGSuper\Version_5.0.0
SET PGSPLICE_TEMPLATE_EXTENSION=SPT

del %PGSPLICE_TARGET%\WSDOT.*

\ARP\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSPLICE_TARGET%\WSDOT.pgz  \ARP\PGSuper\Configurations\WSDOT.lbr  \ARP\PGSuper\Configurations\PGSplice\WSDOT  %PGSPLICE_TEMPLATE_EXTENSION%

copy \ARP\PGSuper\Configurations\LibraryInfo.html \\wsdot\Resources\Topics\Publish\Web\ProdCF\EESC\Bridge\software\PGSuper\