///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include <WBFLGenericBridge.h>

interface IStages;
interface IMaterials;

/*****************************************************************************
INTERFACE
   IAgeAdjustedMaterial

DESCRIPTION
   Extends the WBFL Generic Bridge IMaterial interface so that it works
   with PGSuper/PGSplice materials with an age adjusted modulus. The
   age adjusted modulus is a "chicken-and-egg" problem. We need to set
   the material properties on the generic bridge model during its creation
   however we need to have a fully created generic bridge model to get
   the age adjusted elastic modulus. The age adjusted elastic modulus
   needs the creep coefficient which depends on the V/S ratio which depends
   on the generic bridge model. This object enables us to define the
   concrete material as age adjusted but delays the need to get V/S until
   long after the generic bridge model is valided.
*****************************************************************************/
// {6E0E1094-075A-4cad-80C1-0D423B6162BD}
DEFINE_GUID(CLSID_AgeAdjustedMaterial, 
0x6e0e1094, 0x75a, 0x4cad, 0x80, 0xc1, 0xd, 0x42, 0x3b, 0x61, 0x62, 0xbd);

// {4D1CA7C0-1991-421c-91FA-0BA7E22E1563}
DEFINE_GUID(IID_IAgeAdjustedMaterial, 
0x4d1ca7c0, 0x1991, 0x421c, 0x91, 0xfa, 0xb, 0xa7, 0xe2, 0x2e, 0x15, 0x63);
struct __declspec(uuid("{4D1CA7C0-1991-421c-91FA-0BA7E22E1563}")) IAgeAdjustedMaterial;
interface IAgeAdjustedMaterial : IMaterial
{
   STDMETHOD(InitSegment)(const CSegmentKey& segmentKey,IMaterials* pMaterials) = 0;
   STDMETHOD(InitClosureJoint)(const CClosureKey& closureKey,IMaterials* pMaterials) = 0;
   STDMETHOD(InitDeck)(IndexType deckCastingRegionIdx,IMaterials* pMaterials) = 0;
   STDMETHOD(InitLongitudinalJoint)(const CSegmentKey& segmentKey, IMaterials* pMaterials) = 0;
};

