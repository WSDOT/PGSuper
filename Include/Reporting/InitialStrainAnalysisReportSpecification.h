///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

class REPORTINGCLASS CInitialStrainAnalysisReportSpecification :
   public CBrokerReportSpecification
{
public:
	CInitialStrainAnalysisReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const CGirderKey& girderKey,IntervalIndexType intervalIdx);
   CInitialStrainAnalysisReportSpecification(const CInitialStrainAnalysisReportSpecification& rOther);
   ~CInitialStrainAnalysisReportSpecification(void);

   void SetOptions(const CGirderKey& girderKey,IntervalIndexType intervalIdx);

   virtual HRESULT Validate() const;

   const CGirderKey& GetGirderKey() const;
   IntervalIndexType GetInterval() const;

protected:
   CGirderKey m_GirderKey;
   IntervalIndexType m_IntervalIdx;
};
