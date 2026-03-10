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
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include <IFace\AnalysisResults.h>

struct TIME_STEP_DETAILS;
class IIntervals;
class IMaterials;


class REPORTINGCLASS CTimeStepDetailsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CTimeStepDetailsChapterBuilder(bool bSelect = true);

   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

protected:
   rptRcTable* BuildIntervalTable(const TIME_STEP_DETAILS& tsDetails,std::shared_ptr<IIntervals> pIntervals,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildConcreteTable(const TIME_STEP_DETAILS& tsDetails, const CSegmentKey& segmentKey, std::shared_ptr<IMaterials> pMaterials, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildComponentPropertiesTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildSectionPropertiesTable(const TIME_STEP_DETAILS& tsDetails, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildFreeCreepDeformationTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildStrandRelaxationTable(const TIME_STEP_DETAILS& tsDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildSegmentTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildGirderTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildComponentRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildSectionRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildSectionRestrainingDeformationTable(const TIME_STEP_DETAILS& tsDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildRestrainedSectionForceTable(const TIME_STEP_DETAILS& tsDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildRestrainedComponentForceTable(const TIME_STEP_DETAILS& tsDetails,bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildIncrementalForceTable(std::shared_ptr<WBFL::EAF::Broker> pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildIncrementalStressTable(std::shared_ptr<WBFL::EAF::Broker> pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildIncrementalStrainTable(std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::vector<pgsTypes::ProductForceType>& vLoads, const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* BuildConcreteStressSummaryTable(std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,ResultsType resultsType,bool bGirder,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   void ReportCreepDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   void ReportShrinkageDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   void ReportStrandRelaxationDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   void ReportSegmentTendonRelaxationDetails(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi, IntervalIndexType firstIntervalIdx, IntervalIndexType lastIntervalIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   void ReportGirderTendonRelaxationDetails(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
};
