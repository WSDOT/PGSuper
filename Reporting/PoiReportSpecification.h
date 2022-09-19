#pragma once
#include <Reporting\BrokerReportSpecification.h>
#include <PgsExt\PointOfInterest.h>

class CPoiReportSpecification :
   public CBrokerReportSpecification
{
public:
   CPoiReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, const pgsPointOfInterest& poi);
   ~CPoiReportSpecification(void);

   virtual bool IsValid() const override;

   void SetPOI(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPOI() const;

protected:
   pgsPointOfInterest m_Poi;
};
