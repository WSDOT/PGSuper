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

// AgeAdjustedMaterial.h : Declaration of the CAgeAdjustedMaterial

#pragma once

#include "resource.h"       // main symbols

#include <IFace\AgeAdjustedMaterial.h>

/////////////////////////////////////////////////////////////////////////////
// CAgeAdjustedMaterial
class ATL_NO_VTABLE CAgeAdjustedMaterial : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CAgeAdjustedMaterial, &CLSID_AgeAdjustedMaterial>,
   public IAgeAdjustedMaterial
{
public:
	CAgeAdjustedMaterial()
	{
      m_bIsDeck    = false;
      m_bIsClosure = false;
      m_bIsSegment = false;
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_AGE_ADJUSTED_MATERIAL)

BEGIN_COM_MAP(CAgeAdjustedMaterial)
   COM_INTERFACE_ENTRY(IAgeAdjustedMaterial)
   COM_INTERFACE_ENTRY(IMaterial)
END_COM_MAP()

private:
   CGirderKey m_GirderKey;
   CSegmentKey m_SegmentKey;
   CClosureKey m_ClosureKey;

   bool m_bIsDeck;
   bool m_bIsClosure;
   bool m_bIsSegment;

   // use weak refereces so we don't have circular dependencies
   IMaterials* m_pMaterials;

   bool IsDeck();
   bool IsClosureJoint();
   bool IsSegment();

// IAgeAdjustedMaterial
public:
   STDMETHOD(InitSegment)(const CSegmentKey& segmentKey,IMaterials* pMaterials);
   STDMETHOD(InitClosureJoint)(const CClosureKey& closureKey,IMaterials* pMaterials);
   STDMETHOD(InitDeck)(const CGirderKey& girderKey,IMaterials* pMaterials);

// IMaterial
public:
   STDMETHOD(get_E)(StageIndexType stageIdx,Float64* E);
	STDMETHOD(put_E)(StageIndexType stageIdx,Float64 E);
	STDMETHOD(get_Density)(StageIndexType stageIdx,Float64* w);
	STDMETHOD(put_Density)(StageIndexType stageIdx,Float64 w);
};
