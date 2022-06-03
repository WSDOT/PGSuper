#pragma once
#include <Reporting\BrokerReportSpecification.h>
#include <PgsExt\PointOfInterest.h>

class CPoiReportSpecification :
   public CBrokerReportSpecification
{
public:
   CPoiReportSpecification(LPCTSTR strReportName, IBroker* pBroker, const pgsPointOfInterest& poi);
   ~CPoiReportSpecification(void);

   virtual HRESULT Validate() const override;

   void SetPOI(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPOI() const;

protected:
   pgsPointOfInterest m_Poi;
};
