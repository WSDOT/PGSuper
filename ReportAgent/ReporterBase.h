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

#include <EAF\Agent.h>
#include <IReportManager.h>

class CReporterBase : public WBFL::EAF::Agent
{
public:
   HRESULT InitCommonReportBuilders(std::shared_ptr<WBFL::EAF::Broker> broker);

   HRESULT OnSpecificationChanged(std::shared_ptr<WBFL::EAF::Broker> broker);

protected:

   virtual WBFL::Reporting::TitlePageBuilder* CreateTitlePageBuilder(LPCTSTR strReportName,bool bFullVersion=true) = 0;
   void CreateBridgeGeometryReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateLoadRatingReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateLoadRatingSummaryReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateBearingDesignReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateBearingTimeStepDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);

   void CreateBridgeAnalysisReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateHaulingReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateLiftingReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateMultiGirderSpecCheckReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateSpecChecReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateDistributionFactorSummaryReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateMultiHaunchGeometryReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);

#if defined _DEBUG || defined _BETA_VERSION
   void CreateDistributionFactorsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
#endif

   void CreateStageByStageDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateTimeStepDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreatePrincipalWebStressDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreatePointOfInterestReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);

   void CreatePierReactionsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateTimelineReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateCopyGirderPropertiesReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateCopyPierPropertiesReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateCopyTempSupportPropertiesReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);

   void CreateMomentCapacityDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
   void CreateCrackedSectionDetailsReport(std::shared_ptr<WBFL::Reporting::IReportManager> pRptMgr);
};