///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PsgLib\Keys.h>


class REPORTINGCLASS CSpanReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CSpanReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   ~CSpanReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

class REPORTINGCLASS CGirderReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CGirderReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker,const CGirderKey& defaultGirderKey);
   ~CGirderReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;

private:
   mutable CGirderKey m_GirderKey;
};

class REPORTINGCLASS CSegmentReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CSegmentReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& defaultSegmentKey);
   ~CSegmentReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;

private:
   CSegmentKey m_SegmentKey;
};

class REPORTINGCLASS CGirderLineReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CGirderLineReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   ~CGirderLineReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

class REPORTINGCLASS CMultiGirderReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CMultiGirderReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   ~CMultiGirderReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

// Allow opening of multiple view windows from a single dialog
class REPORTINGCLASS CMultiViewSpanGirderReportSpecificationBuilder :
   public CSpanReportSpecificationBuilder
{
public:
   CMultiViewSpanGirderReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   ~CMultiViewSpanGirderReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

class REPORTINGCLASS CPointOfInterestReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CPointOfInterestReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   ~CPointOfInterestReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};
