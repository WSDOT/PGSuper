regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Release\TxDOTAgent.dll

cscript.exe runregression.js /DC: /VRELEASE
call UpdateTestResults.bat