///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "resource.h"       // main symbols
#include "IBeamFactory.h" // CLSID

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
   m_HpTopElevationBoundary(0),m_HpBotElevationBoundary(0),
   m_EndTopElevationBoundary(0),m_EndBotElevationBoundary(0),
   m_EndIncrement(0),m_HpIncrement(0)
	{
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
	STDMETHOD(get_HpStrandElevationBoundaries)(/*[out]*/double* bottomMin,/*[out]*/double* topMax);
	STDMETHOD(get_EndStrandElevationBoundaries)(/*[out]*/double* bottomMin,/*[out]*/double* topMax);
	STDMETHOD(get_StrandIncrements)(/*[out]*/double* endIncrement,/*[out]*/double* hpIncrement);
	STDMETHOD(TestHpStrandLocation)(/*[in]*/double originalX, /*[in]*/double originalY, /*[in]*/double Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin );
	STDMETHOD(TestEndStrandLocation)(/*[in]*/double originalX, /*[in]*/double originalY, /*[in]*/double Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin );
	STDMETHOD(TranslateStrand)(/*[in]*/double originalX, /*[in]*/double originalY, /*[in]*/double Yoffset, /*[out]*/double* newX, /*[out]*/double* newY );

// IConfigureStrandMover
   STDMETHOD(get_TopElevation)(double* topElevation);
   // offset limits and default increments for harped strands
   STDMETHOD(SetHarpedStrandOffsetBounds)(double topElevation, 
                                          double topHpElevationBoundary,double botHpElevationBoundary,
                                          double topEndElevationBoundary,double botEndElevationBoundary,
                                          double endIncrement,double hpIncrement);
   // remove all regions
   STDMETHOD(ClearAll)();
   // Add a shape that is to be tested for PointInShape, and arcSlope (x/y) that a strand
   // point is to be moved along
   STDMETHOD(AddRegion)(IShape* shape, double arcSlope);
   // debuggin
   STDMETHOD(get_NumRegions)(IndexType* pNum);
   STDMETHOD(GetRegion)(IndexType index, IShape** shape, double* arcSlope);

private:
   double m_TopElevation;
   double m_HpTopElevationBoundary;
   double m_HpBotElevationBoundary;
   double m_EndTopElevationBoundary;
   double m_EndBotElevationBoundary;
   double m_EndIncrement;
   double m_HpIncrement;

   struct HarpRegion
   {
      CComPtr<IShape> pShape;
      double          dArcSlope;
   };

   typedef std::vector<HarpRegion>     RegionCollection;
   typedef RegionCollection::iterator  RegionIterator;
   RegionCollection m_Regions;

   CComPtr<IPoint2d> m_TestPoint;
};

#endif //__STRANDMOVERIMPL_H_
