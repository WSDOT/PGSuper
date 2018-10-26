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

// StrandMoverImpl.cpp : Implementation of CStrandMoverImpl
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "StrandMoverImpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStrandMoverImpl

STDMETHODIMP CStrandMoverImpl::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IStrandMover
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT CStrandMoverImpl::FinalConstruct()
{
   HRESULT hr = m_TestPoint.CoCreateInstance(CLSID_Point2d);
   ATLASSERT(SUCCEEDED(hr));

   return hr;
}

STDMETHODIMP CStrandMoverImpl::get_TopElevation(Float64* topElevation)
{
   *topElevation = m_TopElevation;
   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::get_HpStrandElevationBoundaries(/*[out]*/Float64* bottomMin,/*[out]*/Float64* topMax)
{
   CHECK_RETVAL(bottomMin);
   CHECK_RETVAL(topMax);

   // if strands are not adjustable, this check is moot
   if (m_HpIncrement < 0.0)
   {
      *bottomMin = 0.0;
      *topMax    = m_TopElevation;
   }
   else
   {
      *bottomMin = m_HpBotElevationBoundary;
      *topMax    = m_HpTopElevationBoundary;
   }

   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::get_EndStrandElevationBoundaries(/*[out]*/Float64* bottomMin,/*[out]*/Float64* topMax)
{
   CHECK_RETVAL(bottomMin);
   CHECK_RETVAL(topMax);

   // if strands are not adjustable, this check is moot
   if (m_EndIncrement < 0.0)
   {
      *bottomMin = 0.0;
      *topMax    = m_TopElevation;
   }
   else
   {
      *bottomMin = m_EndBotElevationBoundary;
      *topMax    = m_EndTopElevationBoundary;
   }

   return S_OK;
}


STDMETHODIMP CStrandMoverImpl::get_StrandIncrements(/*[out]*/Float64* endIncrement,/*[out]*/Float64* hpIncrement)
{
   CHECK_RETVAL(endIncrement);
   CHECK_RETVAL(hpIncrement);

   *endIncrement = m_EndIncrement;
   *hpIncrement  = m_HpIncrement;

   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::TestHpStrandLocation(/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin )
{
   CHECK_RETVAL(isWithin);

   Float64 newx, newy;

   // use translate function to make test. Could be more stringent in future.
   HRESULT hr = TranslateStrand(originalX, originalY, Yoffset, &newx, &newy);
   *isWithin = SUCCEEDED(hr) ? VARIANT_TRUE:VARIANT_FALSE;

   // if strands are not adjustable, use entire height
   if (m_HpIncrement < 0.0)
   {
      if (newy > m_TopElevation+TOLERANCE || newy < 0.0)
      {
         *isWithin = VARIANT_FALSE;
      }
   }
   else
   {
      if (newy > m_HpTopElevationBoundary+TOLERANCE || newy < m_HpBotElevationBoundary-TOLERANCE)
      {
         *isWithin = VARIANT_FALSE;
      }
   }

   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::TestEndStrandLocation(/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin )
{
   CHECK_RETVAL(isWithin);

   Float64 newx, newy;

   // use translate function to make test. Could be more stringent in future.
   HRESULT hr = TranslateStrand(originalX, originalY, Yoffset, &newx, &newy);
   *isWithin = SUCCEEDED(hr) ? VARIANT_TRUE:VARIANT_FALSE;

   // if strands are not adjustable, use entire height
   if (m_EndIncrement < 0.0)
   {
      if (newy > m_TopElevation+TOLERANCE || newy < 0.0)
      {
         *isWithin = VARIANT_FALSE;
      }
   }
   else
   {
      if (newy > m_EndTopElevationBoundary+TOLERANCE || newy < m_EndBotElevationBoundary-TOLERANCE)
      {
         *isWithin = VARIANT_FALSE;
      }
   }
   return S_OK;
}


STDMETHODIMP CStrandMoverImpl::TranslateStrand(/*[in]*/Float64 originalX, /*[in]*/Float64 originalY, /*[in]*/Float64 Yoffset,
                                               /*[out]*/Float64* newX, /*[out]*/Float64* newY )
{
   CHECK_RETVAL(newX);
   CHECK_RETVAL(newY);
   
   bool found=false;
   m_TestPoint->Move(originalX,originalY);

   Float64 arc_slope=0.0;

   for (RegionIterator it=m_Regions.begin(); it!=m_Regions.end(); it++)
   {
      HarpRegion& region = *it;
      
      VARIANT_BOOL in_shape;
      region.pShape->PointInShape(m_TestPoint, &in_shape);

      if (in_shape!=VARIANT_FALSE)
      {
         arc_slope = region.dArcSlope;
         found = true;
         break;
      }
   }

   *newX = originalX + Yoffset*arc_slope;
   *newY = originalY + Yoffset;

   return found ? S_OK:E_INVALIDARG;
}


// IConfigureStrandMover
STDMETHODIMP CStrandMoverImpl::SetHarpedStrandOffsetBounds(Float64 topElevation, 
                                          Float64 topHpElevationBoundary,Float64 botHpElevationBoundary,
                                          Float64 topEndElevationBoundary,Float64 botEndElevationBoundary,
                                          Float64 endIncrement,Float64 hpIncrement)
{
   if (topElevation<topHpElevationBoundary || topHpElevationBoundary<0.0 || botHpElevationBoundary<0.0  ||
       topElevation<topEndElevationBoundary || topEndElevationBoundary<0.0 || botEndElevationBoundary<0.0)
   {
      ATLASSERT(0);
      return E_INVALIDARG;
   }

   m_TopElevation         = topElevation;
   m_HpTopElevationBoundary = topHpElevationBoundary;
   m_HpBotElevationBoundary = botHpElevationBoundary;
   m_EndTopElevationBoundary = topEndElevationBoundary;
   m_EndBotElevationBoundary = botEndElevationBoundary;
   m_EndIncrement = endIncrement;
   m_HpIncrement = hpIncrement;

   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::ClearAll()
{
   m_Regions.clear();

   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::AddRegion(IShape* shape, Float64 arcSlope)
{
   CHECK_IN(shape);

   HarpRegion region;
   region.pShape = shape;
   region.dArcSlope = arcSlope;

   m_Regions.push_back(region);
   
   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::get_NumRegions(IndexType* pNum)
{
   *pNum = m_Regions.size();
   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::GetRegion(IndexType index, IShape** shape, Float64* arcSlope)
{
   HarpRegion& region = m_Regions[index];
   
   *arcSlope = region.dArcSlope;
   return region.pShape.CopyTo(shape);
}

