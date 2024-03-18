#pragma once
#include <Reporting\BrokerReportSpecification.h>
#include "PoiReportSpecification.h"

interface IPointOfInterest;

class CCrackedSectionReportSpecification :
   public CPoiReportSpecification
{
public:
	CCrackedSectionReportSpecification(const std::_tstring& strReportName,IBroker* pBroker,const pgsPointOfInterest& poi,bool bPositiveMoment);
   ~CCrackedSectionReportSpecification(void);

   void SetOptions(const pgsPointOfInterest& poi,bool bPositiveMoment);
   bool IsPositiveMoment() const;

   // override to better check if poi is out of bounds
   virtual bool IsValid() const override;

   static PoiList GetCrackedSectionPois(IPointOfInterest* pPois, const CSegmentKey& segmentKey);

protected:
   bool m_bPositiveMoment;
};
