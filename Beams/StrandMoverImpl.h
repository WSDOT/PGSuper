///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// StrandMoverImpl.h : Declaration of the CStrandMoverImpl

#ifndef __STRANDMOVERIMPL_H_
#define __STRANDMOVERIMPL_H_

#include <Plugins\ConfigureStrandMover.h>
#include "resource.h"       // main symbols
#include "IBeamFactory.h" // CLSID

#include <WBFLGenericBridgeTools.h>

/////////////////////////////////////////////////////////////////////////////
// CStrandMoverImpl
class ATL_NO_VTABLE CStrandMoverImpl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CStrandMoverImpl, &CLSID_StrandMoverImpl>,
	public ISupportErrorInfo,
	public IStrandMover,
   public IConfigureStrandMover
{
public:
   CStrandMoverImpl():
   m_TopElevation(0),
   m_Height(0),
   m_EndIncrement(0),m_HpIncrement(0)
	{
      m_HpTopElevationBoundary[etStart] = 0;
      m_HpBotElevationBoundary[etStart] = 0;
      m_HpTopElevationBoundary[etEnd]   = 0;
      m_HpBotElevationBoundary[etEnd]   = 0;
      m_EndTopElevationBoundary[etStart] = 0;
      m_EndBotElevationBoundary[etStart] = 0;
      m_EndTopElevationBoundary[etEnd] = 0;
      m_EndBotElevationBoundary[etEnd] = 0;
	}

   virtual ~CStrandMoverImpl() {;}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_STRANDMOVERIMPL)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStrandMoverImpl)
	COM_INTERFACE_ENTRY(IStrandMover)
	COM_INTERFACE_ENTRY(IConfigureStrandMover)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IStrandMover
public:
	STDMETHOD(get_HpStrandElevationBoundaries)(/*[in]*/EndType endType,/*[out]*/Float64* bottomMin,/*[out]*/Float64* topMax);
	STDMETHOD(get_EndStrandElevationBoundaries)(/*[in]*/EndType endType,/*[out]*/Float64* bottomMin,/*[out]*/Float64* topMax);
	STDMETHOD(get_StrandIncrements)(/*[out]*/Float64* endIncrement,/*[out]*/Float64* hpIncrement);
	STDMETHOD(TestHpStrandLocation)(/*[in]*/EndType endType,/*[in]*/Float64 Hg,/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin );
	STDMETHOD(TestEndStrandLocation)(/*[in]*/EndType endType,/*[in]*/Float64 Hg,/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin );
	STDMETHOD(TranslateHpStrand)(/*[in]*/EndType endType,/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset, /*[out]*/Float64* newX, /*[out]*/Float64* newY );
	STDMETHOD(TranslateEndStrand)(/*[in]*/EndType endType,/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset, /*[out]*/Float64* newX, /*[out]*/Float64* newY );

   STDMETHOD(get_TopElevation)(Float64* topElevation);
   STDMETHOD(get_SectionHeight)(Float64* pHeight);

// IConfigureStrandMover
public:
   // offset limits and default increments for harped strands
   STDMETHOD(SetHarpedStrandOffsetBounds)(Float64 topElevation, Float64 Hg,
                                          Float64 topStartElevationBoundary,Float64 botStartElevationBoundary,
                                          Float64 topHp1ElevationBoundary,Float64 botHp1ElevationBoundary,
                                          Float64 topHp2ElevationBoundary,Float64 botHp2ElevationBoundary,
                                          Float64 topEndElevationBoundary,Float64 botEndElevationBoundary,
                                          Float64 endIncrement,Float64 hpIncrement) override;
   // remove all regions
   STDMETHOD(ClearAll)() override;
   // Add a shape that is to be tested for PointInShape, and arcSlope (x/y) that a strand
   // point is to be moved along
   STDMETHOD(AddRegion)(IShape* shape, Float64 arcSlope) override;
   // debuggin
   STDMETHOD(GetRegionCount)(ZoneIndexType* pNum) const override;
   STDMETHOD(GetRegion)(ZoneIndexType index, IShape** shape, Float64* arcSlope) const override;

private:
   Float64 m_TopElevation;
   Float64 m_Height;
   Float64 m_HpTopElevationBoundary[2]; // use EndType to access array
   Float64 m_HpBotElevationBoundary[2];
   Float64 m_EndTopElevationBoundary[2];
   Float64 m_EndBotElevationBoundary[2];
   Float64 m_EndIncrement;
   Float64 m_HpIncrement;

   struct HarpRegion
   {
      CComPtr<IShape> pShape;
      Float64         dArcSlope;
   };

   typedef std::vector<HarpRegion>     RegionCollection;
   typedef RegionCollection::iterator  RegionIterator;
   RegionCollection m_Regions;

   CComPtr<IPoint2d> m_TestPoint;
};

#endif //__STRANDMOVERIMPL_H_
