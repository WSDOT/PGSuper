///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#pragma once

#include <Reporting\ReportingExp.h>
#include <Reporting\BrokerReportSpecification.h>
#include <PgsExt\ReportPointOfInterest.h>

class REPORTINGCLASS CPrincipalWebStressDetailsReportSpecification :
   public CBrokerReportSpecification
{
public:
	CPrincipalWebStressDetailsReportSpecification(LPCTSTR strReportName,IBroker* pBroker,bool bReportAtAllLocations,const pgsPointOfInterest& poi,
                                                 IntervalIndexType intervalIdx, bool bReportAxial, bool bReportShear);
   ~CPrincipalWebStressDetailsReportSpecification(void);

   void SetOptions(bool bReportAtAllLocations,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, bool bReportAxial, bool bReportShear);

   virtual HRESULT Validate() const;
   virtual std::_tstring GetReportContextString() const override;

   pgsPointOfInterest GetPointOfInterest() const;
   IntervalIndexType GetInterval() const;
   bool ReportAtAllLocations() const;
   bool ReportAxial() const;
   bool ReportShear() const;

protected:
   bool m_bReportAtAllLocations;
   pgsPointOfInterest m_Poi;
   IntervalIndexType m_IntervalIdx;
   bool m_bReportAxialDetails;
   bool m_bReportShearDetails;
};
