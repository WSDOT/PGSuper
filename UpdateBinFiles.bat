REM - Script to prepare for Release

SET BINTARGET=bin
SET REGFREECOM=\ARP\BridgeLink\RegFreeCOM


REM - COM DLLs
copy /Y Convert\Convert.dll				%BINTARGET%\AutomationDLLs\Win32\
copy /Y Convert\Convert.dll				%BINTARGET%\AutomationDLLs\x64\
copy /Y %REGFREECOM%\Win32\Release\PGSuper*.dll		%BINTARGET%\AutomationDLLs\Win32\
copy /Y %REGFREECOM%\x64\Release\PGSuper*.dll		%BINTARGET%\AutomationDLLs\x64\

REM - Extension Agents
REM - WSDOT
copy /Y %REGFREECOM%\Win32\Release\WSDOTAgent.dll	%BINTARGET%\Extensions\WSDOT\Win32\
copy /Y %REGFREECOM%\x64\Release\WSDOTAgent.dll	%BINTARGET%\Extensions\WSDOT\x64\

REM - TXDOT
copy /Y %REGFREECOM%\Win32\Release\TxDOTAgent.dll	%BINTARGET%\Extensions\TxDOT\Win32\
copy /Y %REGFREECOM%\x64\Release\TxDOTAgent.dll	%BINTARGET%\Extensions\TxDOT\x64\
copy /Y TOGA.chm				%BINTARGET%\Extensions\TxDOT\
copy /Y TxDOTAgent\TogaTemplates\*.pgs		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
copy /Y TxDOTAgent\TogaTemplates\*.togt		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
copy /Y TxDOTAgent\TogaTemplates\*.ico		%BINTARGET%\Extensions\TxDOT\TogaTemplates\

REM - KDOT
copy /Y %REGFREECOM%\Win32\Release\KDOTExport.dll   %BINTARGET%\Extensions\KDOT\Win32\
copy /Y %REGFREECOM%\x64\Release\KDOTExport.dll	    %BINTARGET%\Extensions\KDOT\x64\

REM - Image files
copy /Y images\*.gif				%BINTARGET%\images\
copy /Y images\*.jpg				%BINTARGET%\images\
copy /Y images\*.png				%BINTARGET%\images\

REM - Application files
copy /Y md5deep.exe			  	%BINTARGET%\App\
copy /Y %REGFREECOM%\Win32\Release\MakePgz.exe 	%BINTARGET%\App\Win32\
copy /Y %REGFREECOM%\x64\Release\MakePgz.exe  	%BINTARGET%\App\x64\
copy /Y PGSuper.tip				%BINTARGET%\App\
copy /Y License.txt				%BINTARGET%\App\
copy /Y \ARP\BridgeLink\PGSuper.chm		%BINTARGET%\App\
copy /Y Trucks.pgs				%BINTARGET%\App\

REM - Configuration Files
copy /Y Configurations\WSDOT.lbr				%BINTARGET%\Configurations\
copy /Y Configurations\PGSuper\WSDOT\WF_DG-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\WF_DG-Girders\
copy /Y Configurations\PGSuper\WSDOT\WF_DG-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\WF_DG-Girders\
copy /Y Configurations\PGSuper\WSDOT\WF_TDG-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\WF_TDG-Girders\
copy /Y Configurations\PGSuper\WSDOT\WF_TDG-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\WF_TDG-Girders\
copy /Y Configurations\PGSuper\WSDOT\W-Girders\*.ico		%BINTARGET%\Configurations\PGSuper\W-Girders\
copy /Y Configurations\PGSuper\WSDOT\W-Girders\*.pgt		%BINTARGET%\Configurations\PGSuper\W-Girders\
copy /Y Configurations\PGSuper\WSDOT\U-Girders\*.ico		%BINTARGET%\Configurations\PGSuper\U-Girders\
copy /Y Configurations\PGSuper\WSDOT\U-Girders\*.pgt		%BINTARGET%\Configurations\PGSuper\U-Girders\
copy /Y Configurations\PGSuper\WSDOT\WF-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\WF-Girders\
copy /Y Configurations\PGSuper\WSDOT\WF-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\WF-Girders\
copy /Y Configurations\PGSuper\WSDOT\WBT-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\WBT-Girders\
copy /Y Configurations\PGSuper\WSDOT\WBT-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\WBT-Girders\
copy /Y Configurations\PGSuper\WSDOT\Deck_Bulb_Tees\*.ico	%BINTARGET%\Configurations\PGSuper\Deck_Bulb_Tees\
copy /Y Configurations\PGSuper\WSDOT\Deck_Bulb_Tees\*.pgt	%BINTARGET%\Configurations\PGSuper\Deck_Bulb_Tees\
copy /Y Configurations\PGSuper\WSDOT\MultiWeb\*.ico		%BINTARGET%\Configurations\PGSuper\MultiWeb\
copy /Y Configurations\PGSuper\WSDOT\MultiWeb\*.pgt		%BINTARGET%\Configurations\PGSuper\MultiWeb\
copy /Y Configurations\PGSuper\WSDOT\Slabs\*.ico		%BINTARGET%\Configurations\PGSuper\Slabs\
copy /Y Configurations\PGSuper\WSDOT\Slabs\*.pgt		%BINTARGET%\Configurations\PGSuper\Slabs\

copy /Y Configurations\PGSplice\WSDOT\I-Beams\*.ico          %BINTARGET%\Configurations\PGSplice\I-Beams\
copy /Y Configurations\PGSplice\WSDOT\I-Beams\*.spt          %BINTARGET%\Configurations\PGSplice\I-Beams\
copy /Y Configurations\PGSplice\WSDOT\U-Beams\*.ico          %BINTARGET%\Configurations\PGSplice\U-Beams\
copy /Y Configurations\PGSplice\WSDOT\U-Beams\*.spt          %BINTARGET%\Configurations\PGSplice\U-Beams\
