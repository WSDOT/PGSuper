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
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <PsgLib\Keys.h>
#include <IFace/AnalysisResults.h>



class REPORTINGCLASS CBearingReportSpecificationBuilder :
	public CBrokerReportSpecificationBuilder
{
public:
	CBearingReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker, const ReactionLocation& reactionLocation );
	~CBearingReportSpecificationBuilder(void);

	virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
	virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;

private:
	mutable ReactionLocation m_ReactionLocation;

};



class REPORTINGCLASS CMultiBearingReportSpecificationBuilder :
	public CBrokerReportSpecificationBuilder
{
public:
	CMultiBearingReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
	~CMultiBearingReportSpecificationBuilder(void);

	virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
	//virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

// Allow opening of multiple view windows from a single dialog
class REPORTINGCLASS CMultiViewSpanGirderBearingReportSpecificationBuilder :
	public CSpanReportSpecificationBuilder
{
public:
	CMultiViewSpanGirderBearingReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker);
	~CMultiViewSpanGirderBearingReportSpecificationBuilder(void);

	virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
	//virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;

};


