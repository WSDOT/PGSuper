REM - Script to prepare for Release

SET BINTARGET=bin
SET REGFREECOM=\ARP\BridgeLink\RegFreeCOM


REM - COM DLLs
xcopy /Y /d Convert\Convert.dll				%BINTARGET%\AutomationDLLs\x64\
xcopy /Y /d %REGFREECOM%\x64\Release\PGSuper*.dll		%BINTARGET%\AutomationDLLs\x64\

REM - Extension Agents
REM - WSDOT
xcopy /Y /d %REGFREECOM%\x64\Release\WSDOTAgent.dll	%BINTARGET%\Extensions\WSDOT\x64\

REM - TXDOT
xcopy /Y /d %REGFREECOM%\x64\Release\TxDOTAgent.dll	%BINTARGET%\Extensions\TxDOT\x64\
xcopy /Y /d TxDOTAgent\TogaTemplates\*.pgs		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
xcopy /Y /d TxDOTAgent\TogaTemplates\*.togt		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
xcopy /Y /d TxDOTAgent\TogaTemplates\*.ico		%BINTARGET%\Extensions\TxDOT\TogaTemplates\
xcopy /Y /d TxDOTAgent\TogaTemplates\*.lbr		%BINTARGET%\Extensions\TxDOT\TogaTemplates\

REM - KDOT
xcopy /Y /d %REGFREECOM%\x64\Release\KDOTExport.dll	%BINTARGET%\Extensions\KDOT\x64\

REM - Image files
xcopy /Y /d images\*.gif				%BINTARGET%\images\
xcopy /Y /d images\*.jpg				%BINTARGET%\images\
xcopy /Y /d images\*.png				%BINTARGET%\images\

REM - Application files
xcopy /Y /d md5deep.exe			  	%BINTARGET%\App\
xcopy /Y /d %REGFREECOM%\x64\Release\MakePgz.exe  	%BINTARGET%\App\x64\
xcopy /Y /d PGSuper.tip				%BINTARGET%\App\
xcopy /Y /d License.txt				%BINTARGET%\App\
xcopy /Y /d Trucks.pgs				%BINTARGET%\App\

REM - Configuration Files
xcopy /Y /d Configurations\WSDOT.lbr				%BINTARGET%\Configurations\

xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\*.ico                   %BINTARGET%\Configurations\PGSuper\Legacy\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\Deck_Bulb_Tees\*.ico	%BINTARGET%\Configurations\PGSuper\Legacy\Deck_Bulb_Tees\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\Deck_Bulb_Tees\*.pgt	%BINTARGET%\Configurations\PGSuper\Legacy\Deck_Bulb_Tees\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\MultiWeb\*.ico		%BINTARGET%\Configurations\PGSuper\Legacy\MultiWeb\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\MultiWeb\*.pgt		%BINTARGET%\Configurations\PGSuper\Legacy\MultiWeb\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\Slabs\*.ico		%BINTARGET%\Configurations\PGSuper\Legacy\Slabs\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\Slabs\*.pgt		%BINTARGET%\Configurations\PGSuper\Legacy\Slabs\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\W-Girders\*.ico		%BINTARGET%\Configurations\PGSuper\Legacy\W-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\W-Girders\*.pgt		%BINTARGET%\Configurations\PGSuper\Legacy\W-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\WBT-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\Legacy\WBT-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\Legacy\WBT-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\Legacy\WBT-Girders\

xcopy /Y /d Configurations\PGSuper\WSDOT\Slabs\*.ico		%BINTARGET%\Configurations\PGSuper\Slabs\
xcopy /Y /d Configurations\PGSuper\WSDOT\Slabs\*.pgt		%BINTARGET%\Configurations\PGSuper\Slabs\
xcopy /Y /d Configurations\PGSuper\WSDOT\U-Girders\*.ico		%BINTARGET%\Configurations\PGSuper\U-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\U-Girders\*.pgt		%BINTARGET%\Configurations\PGSuper\U-Girders\
xcopy /Y /d "Configurations\PGSuper\WSDOT\WF_DG-Girders (Shear Key)\*.ico"	"%BINTARGET%\Configurations\PGSuper\WF_DG-Girders (Shear Key)\"
xcopy /Y /d "Configurations\PGSuper\WSDOT\WF_DG-Girders (Shear Key)\*.pgt"	"%BINTARGET%\Configurations\PGSuper\WF_DG-Girders (Shear Key)\"
xcopy /Y /d "Configurations\PGSuper\WSDOT\WF_DG-Girders (UHPC Joint)\*.ico"	"%BINTARGET%\Configurations\PGSuper\WF_DG-Girders (UHPC Joint)\"
xcopy /Y /d "Configurations\PGSuper\WSDOT\WF_DG-Girders (UHPC Joint)\*.pgt"	"%BINTARGET%\Configurations\PGSuper\WF_DG-Girders (UHPC Joint)\"
xcopy /Y /d Configurations\PGSuper\WSDOT\WF_TDG-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\WF_TDG-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\WF_TDG-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\WF_TDG-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\WF-Girders\*.ico	%BINTARGET%\Configurations\PGSuper\WF-Girders\
xcopy /Y /d Configurations\PGSuper\WSDOT\WF-Girders\*.pgt	%BINTARGET%\Configurations\PGSuper\WF-Girders\


xcopy /Y /d Configurations\PGSplice\WSDOT\I-Beams\*.ico          %BINTARGET%\Configurations\PGSplice\I-Beams\
xcopy /Y /d Configurations\PGSplice\WSDOT\I-Beams\*.spt          %BINTARGET%\Configurations\PGSplice\I-Beams\
xcopy /Y /d Configurations\PGSplice\WSDOT\U-Beams\*.ico          %BINTARGET%\Configurations\PGSplice\U-Beams\
xcopy /Y /d Configurations\PGSplice\WSDOT\U-Beams\*.spt          %BINTARGET%\Configurations\PGSplice\U-Beams\
