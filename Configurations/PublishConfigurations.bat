REM - Script to publish templates and master library to WSDOT ftp server

REM -------------------------------------------
REM - Publish PGSuper Configurations
REM -------------------------------------------

SET PGSUPER_TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSuper\Version_8.0.0
SET PGSUPER_TEMPLATE_EXTENSION=PGT

del %PGSUPER_TARGET%\WSDOT.*
del %PGSUPER_TARGET%\AASHTO.*
del %PGSUPER_TARGET%\PCI_UHPC.*
del %PGSUPER_TARGET%\FHWA_UHPC.*

%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\WSDOT.pgz  %ARPDIR%\PGSuper\Configurations\WSDOT.lbr  %ARPDIR%\PGSuper\Configurations\PGSuper\WSDOT   %PGSUPER_TEMPLATE_EXTENSION%
%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\AASHTO.pgz %ARPDIR%\PGSuper\Configurations\AASHTO.lbr %ARPDIR%\PGSuper\Configurations\PGSuper\AASHTO  %PGSUPER_TEMPLATE_EXTENSION%
%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\PCI_UHPC.pgz %ARPDIR%\PGSuper\Configurations\PCI_UHPC.lbr %ARPDIR%\PGSuper\Configurations\PGSuper\PCI_UHPC  %PGSUPER_TEMPLATE_EXTENSION%
%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSUPER_TARGET%\FHWA_UHPC.pgz %ARPDIR%\PGSuper\Configurations\FHWA_UHPC.lbr %ARPDIR%\PGSuper\Configurations\PGSuper\FHWA_UHPC  %PGSUPER_TEMPLATE_EXTENSION%

REM -------------------------------------------
REM - Publish PGSplice Configurations
REM -------------------------------------------


SET PGSPLICE_TARGET=\\wsdot\resources\Topics\Publish\FTP\Data\public\Bridge\Software\PGSplice\Version_8.0.0
SET PGSPLICE_TEMPLATE_EXTENSION=SPT

del %PGSPLICE_TARGET%\WSDOT.*

%ARPDIR%\BridgeLink\RegFreeCOM\x64\Release\makepgz.exe %PGSPLICE_TARGET%\WSDOT.pgz  %ARPDIR%\PGSuper\Configurations\WSDOT.lbr  %ARPDIR%\PGSuper\Configurations\PGSplice\WSDOT  %PGSPLICE_TEMPLATE_EXTENSION%

copy %ARPDIR%\PGSuper\Configurations\LibraryInfo.html \\wsdot\Resources\Topics\Publish\Web\ProdCF\EESC\Bridge\software\PGSuper\