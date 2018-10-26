REM - Script to prepare for Release

SET BINTARGET=bin


REM - COM DLLs
copy /Y Convert\Convert.dll				%BINTARGET%\AutomationDLLs\Win32\
copy /Y Convert\Convert.dll				%BINTARGET%\AutomationDLLs\x64\
copy /Y RegFreeCOM\Win32\Release\PGSuper*.dll		%BINTARGET%\AutomationDLLs\Win32\
copy /Y RegFreeCOM\x64\Release\PGSuper*.dll		%BINTARGET%\AutomationDLLs\x64\

REM - Extension Agents
REM - WSDOT
copy /Y RegFreeCOM\Win32\Release\WSDOTAgent.dll	%BINTARGET%\Extensions\WSDOT\Win32\
copy /Y RegFreeCOM\x64\Release\WSDOTAgent.dll	%BINTARGET%\Extensions\WSDOT\x64\

REM - TXDOT
copy /Y RegFreeCOM\Win32\Release\TxDOTAgent.dll	%BINTARGET%\Extensions\TxDOT\Win32\
copy /Y RegFreeCOM\x64\Release\TxDOTAgent.dll	%BINTARGET%\Extensions\TxDOT\x64\
copy /Y TxDOTAgent\TOGA.chm			%BINTARGET%\Extensions\TxDOT\
copy /Y TxDOTAgent\TogaTemplates\*.pgs		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
copy /Y TxDOTAgent\TogaTemplates\*.togt		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
copy /Y TxDOTAgent\TogaTemplates\*.ico		%BINTARGET%\Extensions\TxDOT\TogaTemplates\

REM - Image files
copy /Y images\*.gif				%BINTARGET%\images\
copy /Y images\*.jpg				%BINTARGET%\images\
copy /Y images\*.png				%BINTARGET%\images\

REM - Application files
copy /Y RegFreeCOM\Win32\Release\PGSuper.exe	%BINTARGET%\App\Win32\
copy /Y RegFreeCOM\x64\Release\PGSuper.exe	%BINTARGET%\App\x64\
copy /Y RegFreeCOM\Win32\Release\md5deep.exe  	%BINTARGET%\App\Win32\
copy /Y RegFreeCOM\x64\Release\md5deep.exe  	%BINTARGET%\App\x64\
copy /Y RegFreeCOM\Win32\Release\MakePgz.exe  	%BINTARGET%\App\Win32\
copy /Y RegFreeCOM\x64\Release\MakePgz.exe  	%BINTARGET%\App\x64\
copy /Y PGSuper.tip				%BINTARGET%\App\
copy /Y License.txt				%BINTARGET%\App\
copy /Y PGSuper.chm				%BINTARGET%\App\
copy /Y Trucks.pgs				%BINTARGET%\App\

REM - Template Files
copy /Y WSDOT.lbr				%BINTARGET%\Templates\
copy /Y Templates\WSDOT\W-Girders\*.pgt		%BINTARGET%\Templates\W-Girders\
copy /Y Templates\WSDOT\U-Girders\*.pgt		%BINTARGET%\Templates\U-Girders\
copy /Y Templates\WSDOT\WF-Girders\*.pgt	%BINTARGET%\Templates\WF-Girders\
copy /Y Templates\WSDOT\WBT-Girders\*.pgt	%BINTARGET%\Templates\WBT-Girders\
copy /Y Templates\WSDOT\Deck_Bulb_Tees\*.pgt	%BINTARGET%\Templates\Deck_Bulb_Tees\
copy /Y Templates\WSDOT\MultiWeb\*.pgt		%BINTARGET%\Templates\MultiWeb\
copy /Y Templates\WSDOT\Slabs\*.pgt		%BINTARGET%\Templates\Slabs\

