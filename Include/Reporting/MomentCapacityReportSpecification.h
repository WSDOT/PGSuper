///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#pragma once
#include <Reporting\BrokerReportSpecification.h>
#include "PoiReportSpecification.h"

interface IPointOfInterest;

class REPORTINGCLASS CMomentCapacityReportSpecification :
   public CPoiReportSpecification
{
public:
	CMomentCapacityReportSpecification(const std::_tstring& strReportName,IBroker* pBroker,const pgsPointOfInterest& poi,bool bPositiveMoment);
   ~CMomentCapacityReportSpecification(void);

   void SetOptions(const pgsPointOfInterest& poi,bool bPositiveMoment);

   bool IsPositiveMoment() const;

   // override to better check if poi is out of bounds
   virtual bool IsValid() const override;

   static PoiList GetMomentCapacityDetailsPois(IPointOfInterest* pPois, const CSegmentKey& segmentKey);


protected:
   bool m_bPositiveMoment;
};
