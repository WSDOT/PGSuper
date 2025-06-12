///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#include "StdAfx.h"
#include <Reporting\CrackedSectionReportSpecification.h>

#include <IFace/Tools.h>
#include <IFace\PointOfInterest.h>

CCrackedSectionReportSpecification::CCrackedSectionReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi, bool bPositiveMoment) :
   CPoiReportSpecification(strReportName,pBroker,poi)
{
   m_bPositiveMoment = bPositiveMoment;
}

CCrackedSectionReportSpecification::~CCrackedSectionReportSpecification(void)
{
}

void CCrackedSectionReportSpecification::SetOptions(const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   SetPOI(poi);
   m_bPositiveMoment = bPositiveMoment;
}

bool CCrackedSectionReportSpecification::IsPositiveMoment() const
{
   return m_bPositiveMoment;
}

bool CCrackedSectionReportSpecification::IsValid() const
{
   // parent checks if our poi is even on the bridge
   if (!CPoiReportSpecification::IsValid())
   {
      return false;
   }
   else
   {
      // next check if POI is in a valid range 
      // Note that this range needs to match the segments listed in CSelectCrackedSectionDlg::UpdatePOI
      GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
      const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();

      PoiList vPoi = GetCrackedSectionPois(pPoi, segmentKey);

      const pgsPointOfInterest& poiStart(vPoi.front());
      const pgsPointOfInterest& poiEnd(vPoi.back());

      Float64 loc = pPoi->ConvertPoiToGirderPathCoordinate(m_Poi);
      Float64 startLoc = pPoi->ConvertPoiToGirderPathCoordinate(poiStart);
      Float64 endLoc = pPoi->ConvertPoiToGirderPathCoordinate(poiEnd);

      return InRange(startLoc, loc, endLoc);
   }
}

PoiList CCrackedSectionReportSpecification::GetCrackedSectionPois(std::shared_ptr<IPointOfInterest> pPois, const CSegmentKey& segmentKey)
{
   PoiList vPoi;
   CSegmentKey segAsDialog(segmentKey.groupIndex, segmentKey.girderIndex, ALL_SEGMENTS);
   pPois->GetPointsOfInterest(segAsDialog, &vPoi);
   return vPoi;
}
