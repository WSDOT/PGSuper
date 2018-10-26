REM - Script to prepare for Release

SET BINTARGET=bin



REM - Library Editor
copy /Y RegFreeCOM\Release\LibraryEditor.exe	%BINTARGET%\LibraryEditor\

REM - COM DLLs
copy /Y Convert\Convert.dll			%BINTARGET%\AutomationDLLs\
copy /Y RegFreeCOM\Release\PGSuper*.dll		%BINTARGET%\AutomationDLLs\

REM - Extension Agents
copy /Y RegFreeCOM\Release\WSDOTAgent.dll	%BINTARGET%\Extensions\
copy /Y RegFreeCOM\Release\TxDOTAgent.dll	%BINTARGET%\Extensions\

REM - Image files
copy /Y images\*.gif				%BINTARGET%\images\
copy /Y images\*.jpg				%BINTARGET%\images\
copy /Y images\*.png				%BINTARGET%\images\

REM - Application files
copy /Y RegFreeCOM\Release\PGSuper.exe	%BINTARGET%\App\
copy /Y RegFreeCOM\Release\md5deep.exe  %BINTARGET%\App\
copy /Y RegFreeCOM\Release\MakePgz.exe  %BINTARGET%\App\
copy /Y PGSuper.tip			%BINTARGET%\App\
copy /Y License.txt			%BINTARGET%\App\
copy /Y PGSuper.chm			%BINTARGET%\App\
copy /Y Trucks.pgs			%BINTARGET%\App\

REM - Template Files
copy /Y WSDOT.lbr				%BINTARGET%\Templates\
copy /Y Templates\WSDOT\W-Girders\*.pgt		%BINTARGET%\Templates\W-Girders\
copy /Y Templates\WSDOT\U-Girders\*.pgt		%BINTARGET%\Templates\U-Girders\
copy /Y Templates\WSDOT\WF-Girders\*.pgt	%BINTARGET%\Templates\WF-Girders\
copy /Y Templates\WSDOT\WBT-Girders\*.pgt	%BINTARGET%\Templates\WBT-Girders\
copy /Y Templates\WSDOT\Deck_Bulb_Tees\*.pgt	%BINTARGET%\Templates\Deck_Bulb_Tees\
copy /Y Templates\WSDOT\MultiWeb\*.pgt		%BINTARGET%\Templates\MultiWeb\
copy /Y Templates\WSDOT\Slabs\*.pgt		%BINTARGET%\Templates\Slabs\
