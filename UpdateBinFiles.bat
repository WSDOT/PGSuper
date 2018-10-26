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

REM - Template Files
copy /Y Templates\WSDOT.lbr				%BINTARGET%\Templates\
copy /Y Templates\PGSuper\WSDOT\W-Girders\*.ico		%BINTARGET%\Templates\PGSuper\W-Girders\
copy /Y Templates\PGSuper\WSDOT\W-Girders\*.pgt		%BINTARGET%\Templates\PGSuper\W-Girders\
copy /Y Templates\PGSuper\WSDOT\U-Girders\*.ico		%BINTARGET%\Templates\PGSuper\U-Girders\
copy /Y Templates\PGSuper\WSDOT\U-Girders\*.pgt		%BINTARGET%\Templates\PGSuper\U-Girders\
copy /Y Templates\PGSuper\WSDOT\WF-Girders\*.ico	%BINTARGET%\Templates\PGSuper\WF-Girders\
copy /Y Templates\PGSuper\WSDOT\WF-Girders\*.pgt	%BINTARGET%\Templates\PGSuper\WF-Girders\
copy /Y Templates\PGSuper\WSDOT\WBT-Girders\*.ico	%BINTARGET%\Templates\PGSuper\WBT-Girders\
copy /Y Templates\PGSuper\WSDOT\WBT-Girders\*.pgt	%BINTARGET%\Templates\PGSuper\WBT-Girders\
copy /Y Templates\PGSuper\WSDOT\Deck_Bulb_Tees\*.ico	%BINTARGET%\Templates\PGSuper\Deck_Bulb_Tees\
copy /Y Templates\PGSuper\WSDOT\Deck_Bulb_Tees\*.pgt	%BINTARGET%\Templates\PGSuper\Deck_Bulb_Tees\
copy /Y Templates\PGSuper\WSDOT\MultiWeb\*.ico		%BINTARGET%\Templates\PGSuper\MultiWeb\
copy /Y Templates\PGSuper\WSDOT\MultiWeb\*.pgt		%BINTARGET%\Templates\PGSuper\MultiWeb\
copy /Y Templates\PGSuper\WSDOT\Slabs\*.ico		%BINTARGET%\Templates\PGSuper\Slabs\
copy /Y Templates\PGSuper\WSDOT\Slabs\*.pgt		%BINTARGET%\Templates\PGSuper\Slabs\

copy /Y Templates\PGSplice\WSDOT\I-Beams\*.ico          %BINTARGET%\Templates\PGSplice\I-Beams\
copy /Y Templates\PGSplice\WSDOT\I-Beams\*.spt          %BINTARGET%\Templates\PGSplice\I-Beams\
copy /Y Templates\PGSplice\WSDOT\U-Beams\*.ico          %BINTARGET%\Templates\PGSplice\U-Beams\
copy /Y Templates\PGSplice\WSDOT\U-Beams\*.spt          %BINTARGET%\Templates\PGSplice\U-Beams\

