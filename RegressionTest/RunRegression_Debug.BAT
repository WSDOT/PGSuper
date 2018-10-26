regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Debug\TxDOTAgent.dll

cscript.exe runregression.js /DC: /VDEBUG
