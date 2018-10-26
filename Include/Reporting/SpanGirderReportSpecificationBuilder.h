///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <ReportManager\ReportManager.h>
#include <PgsExt\Keys.h>
#include <WBFLCore.h>

class REPORTINGCLASS CSpanReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CSpanReportSpecificationBuilder(IBroker* pBroker);
   ~CSpanReportSpecificationBuilder(void);

   virtual boost::shared_ptr<CReportSpecification> CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec);
   virtual boost::shared_ptr<CReportSpecification> CreateDefaultReportSpec(const CReportDescription& rptDesc);
};

class REPORTINGCLASS CGirderReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CGirderReportSpecificationBuilder(IBroker* pBroker,const CGirderKey& defaultGirderKey);
   ~CGirderReportSpecificationBuilder(void);

   virtual boost::shared_ptr<CReportSpecification> CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec);
   virtual boost::shared_ptr<CReportSpecification> CreateDefaultReportSpec(const CReportDescription& rptDesc);

private:
   CGirderKey m_GirderKey;
};

class REPORTINGCLASS CGirderLineReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CGirderLineReportSpecificationBuilder(IBroker* pBroker);
   ~CGirderLineReportSpecificationBuilder(void);

   virtual boost::shared_ptr<CReportSpecification> CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec);
   virtual boost::shared_ptr<CReportSpecification> CreateDefaultReportSpec(const CReportDescription& rptDesc);
};

class REPORTINGCLASS CMultiGirderReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CMultiGirderReportSpecificationBuilder(IBroker* pBroker);
   ~CMultiGirderReportSpecificationBuilder(void);

   virtual boost::shared_ptr<CReportSpecification> CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec);
   virtual boost::shared_ptr<CReportSpecification> CreateDefaultReportSpec(const CReportDescription& rptDesc);
};

// Allow opening of multiple view windows from a single dialog
class REPORTINGCLASS CMultiViewSpanGirderReportSpecificationBuilder :
   public CSpanReportSpecificationBuilder
{
public:
   CMultiViewSpanGirderReportSpecificationBuilder(IBroker* pBroker);
   ~CMultiViewSpanGirderReportSpecificationBuilder(void);

   virtual boost::shared_ptr<CReportSpecification> CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec);
   virtual boost::shared_ptr<CReportSpecification> CreateDefaultReportSpec(const CReportDescription& rptDesc);
};
