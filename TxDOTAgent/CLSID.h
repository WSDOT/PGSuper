///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

DEFINE_GUID(CLSID_TxDOTCadExporter,0x9274354C,0xD0B7,0x437c,0xA5,0xB3,0x3F,0xFB,0xFB,0x17,0xAD,0xE3);
struct __declspec(uuid("{9274354C-D0B7-437c-A5B3-3FFBFB17ADE3}")) TxDOTCadExporter;

DEFINE_GUID(CLSID_TxDOTAgent, 0x3700B253, 0x8489, 0x457c, 0x8A, 0x6D, 0xD1, 0x74, 0xF9, 0x5C, 0x45, 0x7C);
struct __declspec(uuid("{3700B253-8489-457c-8A6D-D174F95C457C}")) TxDOTAgent;

DEFINE_GUID(CLSID_TxDOTAppPlugin, 0xF4629B75, 0x7EF8, 0x4159, 0xA0, 0x9A, 0x9F, 0x4F, 0x30, 0xB6, 0x05, 0x01);
struct __declspec(uuid("{F4629B75-7EF8-4159-A09A-9F4F30B60501}")) TxDOTAppPlugin;

DEFINE_GUID(CLSID_TxDOTComponentInfo, 0x785F2ACE, 0x127B, 0x4647, 0x80, 0x62, 0xED, 0x49, 0x53, 0x7E, 0x96, 0x2C);
struct __declspec(uuid("{785F2ACE-127B-4647-8062-ED49537E962C}")) TxDOTComponentInfo;

DEFINE_GUID(CLSID_TxDOTAppPluginComponentInfo, 0xAF010EB6, 0x4A8D, 0x4404, 0xAD, 0xAB, 0xD6, 0xCA, 0x62, 0xEC, 0x9A, 0xA1);
struct __declspec(uuid("{AF010EB6-4A8D-4404-ADAB-D6CA62EC9AA1}")) TxDOTAppPluginComponentInfo;
