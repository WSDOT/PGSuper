///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright c 1999-2012  Washington State Department of Transportation
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

/////////////////////////////////////////////////////////////////////////////
// Beam family CLSIDs (PGSuper)
/////////////////////////////////////////////////////////////////////////////

// {2AAD714F-6974-4fee-A573-4DE9430A742C}
DEFINE_GUID(CLSID_WFBeamFamily, 
0x2aad714f, 0x6974, 0x4fee, 0xa5, 0x73, 0x4d, 0xe9, 0x43, 0xa, 0x74, 0x2c);

// {34F855F9-08CF-45b0-B2F5-B74F84A74A43}
DEFINE_GUID(CLSID_UBeamFamily, 
0x34f855f9, 0x8cf, 0x45b0, 0xb2, 0xf5, 0xb7, 0x4f, 0x84, 0xa7, 0x4a, 0x43);

// {69938984-AAD1-426f-89E2-6154B0A79E9A}
DEFINE_GUID(CLSID_BoxBeamFamily, 
0x69938984, 0xaad1, 0x426f, 0x89, 0xe2, 0x61, 0x54, 0xb0, 0xa7, 0x9e, 0x9a);

// {F35DAB20-CD13-4529-A0AC-A65BA6D03DA0}
DEFINE_GUID(CLSID_DeckBulbTeeBeamFamily, 
0xf35dab20, 0xcd13, 0x4529, 0xa0, 0xac, 0xa6, 0x5b, 0xa6, 0xd0, 0x3d, 0xa0);

// {82B858CE-58B4-44ff-95A3-6D9C399C0600}
DEFINE_GUID(CLSID_DoubleTeeBeamFamily, 
0x82b858ce, 0x58b4, 0x44ff, 0x95, 0xa3, 0x6d, 0x9c, 0x39, 0x9c, 0x6, 0x0);

// {ABBD3DAF-884D-4072-ACCF-6C3CE836C6F2}
DEFINE_GUID(CLSID_RibbedBeamFamily, 
0xabbd3daf, 0x884d, 0x4072, 0xac, 0xcf, 0x6c, 0x3c, 0xe8, 0x36, 0xc6, 0xf2);

// {B6BA3A08-82BA-43d5-8196-A3DBC3E98689}
DEFINE_GUID(CLSID_SlabBeamFamily, 
0xb6ba3a08, 0x82ba, 0x43d5, 0x81, 0x96, 0xa3, 0xdb, 0xc3, 0xe9, 0x86, 0x89);

// {58D7AA2D-057A-4ec0-ADF3-81F1CAD84288}
DEFINE_GUID(CLSID_DeckedSlabBeamFamily, 
0x58d7aa2d, 0x57a, 0x4ec0, 0xad, 0xf3, 0x81, 0xf1, 0xca, 0xd8, 0x42, 0x88);

/////////////////////////////////////////////////////////////////////////////
// Beam family CLSIDs (PGSplice)
/////////////////////////////////////////////////////////////////////////////
// {137DE1EA-B3A8-4cfb-8CAC-762991ACBCCE}
DEFINE_GUID(CLSID_SplicedIBeamFamily, 
0x137de1ea, 0xb3a8, 0x4cfb, 0x8c, 0xac, 0x76, 0x29, 0x91, 0xac, 0xbc, 0xce);

// {4229DFFE-F5D7-4c16-9D11-F296ED06161C}
DEFINE_GUID(CLSID_SplicedUBeamFamily, 
0x4229dffe, 0xf5d7, 0x4c16, 0x9d, 0x11, 0xf2, 0x96, 0xed, 0x6, 0x16, 0x1c);
