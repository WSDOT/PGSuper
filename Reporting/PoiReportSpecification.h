#pragma once
#include <Reporting\BrokerReportSpecification.h>
#include <PsgLib\PointOfInterest.h>

class CPoiReportSpecification :
   public CBrokerReportSpecification
{
public:
   CPoiReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi);
   ~CPoiReportSpecification(void);

   virtual bool IsValid() const override;

   void SetPOI(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPOI() const;

protected:
   pgsPointOfInterest m_Poi;
};
