///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <ReportManager\ReportManager.h>
#include <WBFLCore.h>

class REPORTINGCLASS CSpanReportSpecification :
   public CBrokerReportSpecification
{
public:
   CSpanReportSpecification(const char* strReportName,IBroker* pBroker,SpanIndexType spanIdx);
   ~CSpanReportSpecification(void);

   virtual std::string GetReportTitle() const;

   void SetSpan(SpanIndexType spanIdx);
   SpanIndexType GetSpan() const;

   virtual HRESULT Validate() const;

protected:
   SpanIndexType m_Span;
};

class REPORTINGCLASS CSpanGirderReportSpecification :
   public CSpanReportSpecification
{
public:
   CSpanGirderReportSpecification(const char* strReportName,IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx);
   ~CSpanGirderReportSpecification(void);
   
   virtual std::string GetReportTitle() const;

   void SetGirder(GirderIndexType gdrIdx);
   GirderIndexType GetGirder() const;

   virtual HRESULT Validate() const;

protected:
   GirderIndexType m_Girder;
};
