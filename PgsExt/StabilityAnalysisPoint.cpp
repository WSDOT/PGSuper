///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\StabilityAnalysisPoint.h>
#include <MFCTools\Format.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsStabilityAnalysisPoint::pgsStabilityAnalysisPoint()
{
   m_RefAttribute = POI_RELEASED_SEGMENT;
}

pgsStabilityAnalysisPoint::pgsStabilityAnalysisPoint(const pgsPointOfInterest& poi,PoiAttributeType refAttrib) : 
m_Poi(poi),
m_RefAttribute(refAttrib)
{
}

pgsStabilityAnalysisPoint::pgsStabilityAnalysisPoint(const pgsStabilityAnalysisPoint& other) :
m_Poi(other.m_Poi),
m_RefAttribute(other.m_RefAttribute)
{
}

pgsStabilityAnalysisPoint::~pgsStabilityAnalysisPoint()
{
}

void pgsStabilityAnalysisPoint::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

const pgsPointOfInterest& pgsStabilityAnalysisPoint::GetPointOfInterest() const
{
   return m_Poi;
}

void pgsStabilityAnalysisPoint::SetReferenceAttribute(PoiAttributeType refAttrib)
{
   m_RefAttribute = refAttrib;
}

PoiAttributeType pgsStabilityAnalysisPoint::GetReferenceAttribute() const
{
   return m_RefAttribute;
}

Float64 pgsStabilityAnalysisPoint::GetLocation() const
{
   // WBFL Stability doesn't have the concept of Left/Right faces for abrupt
   // section changes. When we have an abrupt section change, like at end block
   // boundaries, move the POI just a little to the left/right.
   if (m_Poi.HasAttribute(POI_SECTCHANGE_LEFTFACE) && !m_Poi.HasAttribute(POI_END_FACE))
   {
      return m_Poi.GetDistFromStart() - 0.0001;
   }
   else if (m_Poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE) && !m_Poi.HasAttribute(POI_START_FACE))
   {
      return m_Poi.GetDistFromStart() + 0.0001;
   }
   else
   {
      return m_Poi.GetDistFromStart();
   }
}

std::_tstring pgsStabilityAnalysisPoint::AsString(const WBFL::Units::LengthData& lengthUnit,Float64 offset,bool bShowUnit) const
{
   std::_tostringstream os;
   os << _T("(") << m_Poi.GetAttributes(m_RefAttribute,true) << _T(") ") << FormatDimension(m_Poi.GetDistFromStart()-offset,lengthUnit,bShowUnit).GetBuffer() << std::endl;
   return os.str();
}

std::unique_ptr<WBFL::Stability::IAnalysisPoint> pgsStabilityAnalysisPoint::Clone() const
{
   return std::make_unique<pgsStabilityAnalysisPoint>(*this);
}
