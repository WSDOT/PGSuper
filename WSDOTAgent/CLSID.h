///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////
#pragma once

#include <initguid.h>

DEFINE_GUID(CLSID_WSDOTAgent, 0xB1A19633, 0x8880, 0x40bc, 0xA3, 0xC9, 0xDD, 0xF4, 0x7F, 0x7F, 0x18, 0x44);
struct __declspec(uuid("{B1A19633-8880-40bc-A3C9-DDF47F7F1844}")) WSDOTAgent;

DEFINE_GUID(CLSID_WSDOTPGSuperComponentInfo, 0x5656F52E, 0x4DC8, 0x4299, 0x8A, 0xD1, 0x23, 0xEB, 0x3A, 0xE4, 0x63, 0x53);

DEFINE_GUID(CLSID_WSDOTPGSpliceComponentInfo, 0xE389A200, 0xD722, 0x4977, 0xAE, 0x9F, 0x93, 0x9F, 0x9C, 0x12, 0x1A, 0x1C);
