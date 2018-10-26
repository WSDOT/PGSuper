///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
// Beam factory category - CATIDs

// {431AE25F-0D66-4436-A9B9-9B1DF0CC223C}
DEFINE_GUID(CATID_WFBeamFactory, 
0x431ae25f, 0xd66, 0x4436, 0xa9, 0xb9, 0x9b, 0x1d, 0xf0, 0xcc, 0x22, 0x3c);

// {1ECF2C84-3442-422b-B7C0-A68F9D14F2B6}
DEFINE_GUID(CATID_UBeamFactory, 
0x1ecf2c84, 0x3442, 0x422b, 0xb7, 0xc0, 0xa6, 0x8f, 0x9d, 0x14, 0xf2, 0xb6);

// {FABF69B6-C816-455f-B417-C8ED7448FC5F}
DEFINE_GUID(CATID_BoxBeamFactory, 
0xfabf69b6, 0xc816, 0x455f, 0xb4, 0x17, 0xc8, 0xed, 0x74, 0x48, 0xfc, 0x5f);

// {E63C9BEF-3239-4093-A9B3-E93BB9F2C084}
DEFINE_GUID(CATID_DeckBulbTeeBeamFactory, 
0xe63c9bef, 0x3239, 0x4093, 0xa9, 0xb3, 0xe9, 0x3b, 0xb9, 0xf2, 0xc0, 0x84);

// {891B0FE5-DDE6-470a-B7A8-F3906E403DD3}
DEFINE_GUID(CATID_DoubleTeeBeamFactory, 
0x891b0fe5, 0xdde6, 0x470a, 0xb7, 0xa8, 0xf3, 0x90, 0x6e, 0x40, 0x3d, 0xd3);

// {E797CCCB-62EE-4c48-AC2F-E4D559E268AC}
DEFINE_GUID(CATID_RibbedBeamFactory, 
0xe797cccb, 0x62ee, 0x4c48, 0xac, 0x2f, 0xe4, 0xd5, 0x59, 0xe2, 0x68, 0xac);

// {E1CB1BD3-9F0E-4eb9-839A-E7F0CA689BF0}
DEFINE_GUID(CATID_SlabBeamFactory, 
0xe1cb1bd3, 0x9f0e, 0x4eb9, 0x83, 0x9a, 0xe7, 0xf0, 0xca, 0x68, 0x9b, 0xf0);

// {9F841D9B-EEBC-4987-A1A5-B14458C81B31}
DEFINE_GUID(CATID_DeckedSlabBeamFactory, 
0x9f841d9b, 0xeebc, 0x4987, 0xa1, 0xa5, 0xb1, 0x44, 0x58, 0xc8, 0x1b, 0x31);
