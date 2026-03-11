///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include "StdAfx.h"
#include "PoiReportSpecification.h"

#include <IFace/Tools.h>
#include <IFace\Bridge.h>


CPoiReportSpecification::CPoiReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi) :
   CBrokerReportSpecification(strReportName, pBroker)
{
   m_Poi = poi;
}

CPoiReportSpecification::~CPoiReportSpecification(void)
{
}

bool CPoiReportSpecification::IsValid() const
{
   GET_IFACE2(GetBroker(),IBridge, pBridge);

   const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if (nGroups <= segmentKey.groupIndex)
   {
      // the group index is out of range (group probably got deleted)
      return false;
   }

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   if (nGirders <= segmentKey.girderIndex)
   {
      return false;
   }

   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   if (nSegments <= segmentKey.segmentIndex)
   {
      return false;
   }

   return CBrokerReportSpecification::IsValid();
}

void CPoiReportSpecification::SetPOI(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

const pgsPointOfInterest& CPoiReportSpecification::GetPOI() const
{
   return m_Poi;
}
