regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\Profile\TxDOTAgent.dll

cscript.exe runregression.js /VPROFILE %1 %2
