regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Profile\TxDOTAgent.dll

cscript.exe runregression.js /VPROFILE /PWIN32 %1 %2
