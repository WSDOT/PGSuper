///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

STDMETHODIMP CStrandMoverImpl::get_TopElevation(double* topElevation)
{
   *topElevation = m_TopElevation;
   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::get_HpStrandElevationBoundaries(/*[out]*/double* bottomMin,/*[out]*/double* topMax)
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

STDMETHODIMP CStrandMoverImpl::get_EndStrandElevationBoundaries(/*[out]*/double* bottomMin,/*[out]*/double* topMax)
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


STDMETHODIMP CStrandMoverImpl::get_StrandIncrements(/*[out]*/double* endIncrement,/*[out]*/double* hpIncrement)
{
   CHECK_RETVAL(endIncrement);
   CHECK_RETVAL(hpIncrement);

   *endIncrement = m_EndIncrement;
   *hpIncrement  = m_HpIncrement;

   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::TestHpStrandLocation(/*[in]*/double originalX, /*[in]*/double originalY, /*[in]*/double Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin )
{
   CHECK_RETVAL(isWithin);

   double newx, newy;

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

STDMETHODIMP CStrandMoverImpl::TestEndStrandLocation(/*[in]*/double originalX, /*[in]*/double originalY, /*[in]*/double Yoffset, /*[out,retval]*/VARIANT_BOOL* isWithin )
{
   CHECK_RETVAL(isWithin);

   double newx, newy;

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


STDMETHODIMP CStrandMoverImpl::TranslateStrand(/*[in]*/double originalX, /*[in]*/double originalY, /*[in]*/double Yoffset,
                                               /*[out]*/double* newX, /*[out]*/double* newY )
{
   CHECK_RETVAL(newX);
   CHECK_RETVAL(newY);
   
   bool found=false;
   m_TestPoint->Move(originalX,originalY);

   double arc_slope=0.0;

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
STDMETHODIMP CStrandMoverImpl::SetHarpedStrandOffsetBounds(double topElevation, 
                                          double topHpElevationBoundary,double botHpElevationBoundary,
                                          double topEndElevationBoundary,double botEndElevationBoundary,
                                          double endIncrement,double hpIncrement)
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

STDMETHODIMP CStrandMoverImpl::AddRegion(IShape* shape, double arcSlope)
{
   CHECK_IN(shape);

   HarpRegion region;
   region.pShape = shape;
   region.dArcSlope = arcSlope;

   m_Regions.push_back(region);
   
   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::get_NumRegions(long* pNum)
{
   *pNum = m_Regions.size();
   return S_OK;
}

STDMETHODIMP CStrandMoverImpl::GetRegion(long index, IShape** shape, double* arcSlope)
{
   HarpRegion& region = m_Regions[index];
   
   *arcSlope = region.dArcSlope;
   return region.pShape.CopyTo(shape);
}

