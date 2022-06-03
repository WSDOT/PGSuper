#include "StdAfx.h"
#include "PoiReportSpecification.h"
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPoiReportSpecification::CPoiReportSpecification(LPCTSTR strReportName, IBroker* pBroker, const pgsPointOfInterest& poi) :
   CBrokerReportSpecification(strReportName, pBroker)
{
   m_Poi = poi;
}

CPoiReportSpecification::~CPoiReportSpecification(void)
{
}

HRESULT CPoiReportSpecification::Validate() const
{
   GET_IFACE(IBridge, pBridge);

   const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if (nGroups <= segmentKey.groupIndex)
   {
      // the group index is out of range (group probably got deleted)
      return RPT_E_INVALID_SEGMENT;
   }

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   if (nGirders <= segmentKey.girderIndex)
   {
      return RPT_E_INVALID_SEGMENT;
   }

   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   if (nSegments <= segmentKey.segmentIndex)
   {
      return RPT_E_INVALID_SEGMENT;
   }

   return CBrokerReportSpecification::Validate();
}

void CPoiReportSpecification::SetPOI(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

const pgsPointOfInterest& CPoiReportSpecification::GetPOI() const
{
   return m_Poi;
}
