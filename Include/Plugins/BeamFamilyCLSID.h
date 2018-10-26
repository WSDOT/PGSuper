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

// {AB6A4CD6-0F10-4d2f-BB98-4425146B133E}
DEFINE_GUID(CLSID_WFBeamFamily, 
0xab6a4cd6, 0xf10, 0x4d2f, 0xbb, 0x98, 0x44, 0x25, 0x14, 0x6b, 0x13, 0x3e);

// {917D405F-8752-4bfc-BF6B-9CE7BC2E4C80}
DEFINE_GUID(CLSID_UBeamFamily, 
0x917d405f, 0x8752, 0x4bfc, 0xbf, 0x6b, 0x9c, 0xe7, 0xbc, 0x2e, 0x4c, 0x80);

// {CE4BA8B3-9AD3-4fa0-8EF0-F0E58D3CF432}
DEFINE_GUID(CLSID_BoxBeamFamily, 
0xce4ba8b3, 0x9ad3, 0x4fa0, 0x8e, 0xf0, 0xf0, 0xe5, 0x8d, 0x3c, 0xf4, 0x32);

// {F89E7939-C179-4337-8CC8-BAD7F910B9A6}
DEFINE_GUID(CLSID_DeckBulbTeeBeamFamily, 
0xf89e7939, 0xc179, 0x4337, 0x8c, 0xc8, 0xba, 0xd7, 0xf9, 0x10, 0xb9, 0xa6);

// {8167A722-A9CD-404f-BEDE-CF57D5305871}
DEFINE_GUID(CLSID_DoubleTeeBeamFamily, 
0x8167a722, 0xa9cd, 0x404f, 0xbe, 0xde, 0xcf, 0x57, 0xd5, 0x30, 0x58, 0x71);

// {FACA64B5-C2E6-4335-89E2-F488D1B271BD}
DEFINE_GUID(CLSID_RibbedBeamFamily, 
0xfaca64b5, 0xc2e6, 0x4335, 0x89, 0xe2, 0xf4, 0x88, 0xd1, 0xb2, 0x71, 0xbd);

// {07BDEB00-D913-4c67-8393-61263E065DC6}
DEFINE_GUID(CLSID_SlabBeamFamily, 
0x7bdeb00, 0xd913, 0x4c67, 0x83, 0x93, 0x61, 0x26, 0x3e, 0x6, 0x5d, 0xc6);

// {BFC403E9-AAF5-4e49-86AB-B537F1C50066}
DEFINE_GUID(CLSID_DeckedSlabBeamFamily, 
0xbfc403e9, 0xaaf5, 0x4e49, 0x86, 0xab, 0xb5, 0x37, 0xf1, 0xc5, 0x0, 0x66);

/////////////////////////////////////////////////////////////////////////////
// Beam family CLSIDs (PGSplice)
/////////////////////////////////////////////////////////////////////////////
// {137DE1EA-B3A8-4cfb-8CAC-762991ACBCCE}
DEFINE_GUID(CLSID_SplicedIBeamFamily, 
0x137de1ea, 0xb3a8, 0x4cfb, 0x8c, 0xac, 0x76, 0x29, 0x91, 0xac, 0xbc, 0xce);

// {4229DFFE-F5D7-4c16-9D11-F296ED06161C}
DEFINE_GUID(CLSID_SplicedUBeamFamily, 
0x4229dffe, 0xf5d7, 0x4c16, 0x9d, 0x11, 0xf2, 0x96, 0xed, 0x6, 0x16, 0x1c);
