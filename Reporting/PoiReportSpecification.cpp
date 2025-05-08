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
