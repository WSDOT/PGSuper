#pragma once
#include <Reporting\BrokerReportSpecification.h>
#include "PoiReportSpecification.h"

class CCrackedSectionReportSpecification :
   public CPoiReportSpecification
{
public:
	CCrackedSectionReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const pgsPointOfInterest& poi,bool bPositiveMoment);
   ~CCrackedSectionReportSpecification(void);

   void SetOptions(const pgsPointOfInterest& poi,bool bPositiveMoment);
   bool IsPositiveMoment() const;

protected:
   bool m_bPositiveMoment;
};
