regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperAnalysisAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperBeams.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperBridgeAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperEngAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperProjectAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperReportAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperSpecAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\PGSuperTestAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\WSDOTAgent.dll
regsvr32 /s \ARP\PGSuper\RegFreeCOM\WIN32\Debug\TxDOTAgent.dll

cscript.exe runregression.js /VDEBUG /PWIN32 %1 %2
