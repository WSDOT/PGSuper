///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
interface IIntervals;
interface IMaterials;


/*****************************************************************************
CLASS 
   CTimeStepDetailsChapterBuilder

DESCRIPTION
   Chapter builder for reporting details of time-step analysis calculations
   at a specified POI
*****************************************************************************/

class REPORTINGCLASS CTimeStepDetailsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CTimeStepDetailsChapterBuilder(bool bSelect = true);

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

protected:
   rptRcTable* BuildIntervalTable(const TIME_STEP_DETAILS& tsDetails,IIntervals* pIntervals,IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildConcreteTable(const TIME_STEP_DETAILS& tsDetails, const CSegmentKey& segmentKey, IMaterials* pMaterials, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildComponentPropertiesTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildSectionPropertiesTable(const TIME_STEP_DETAILS& tsDetails, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildFreeCreepDeformationTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildStrandRelaxationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildSegmentTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildGirderTendonRelaxationTable(const TIME_STEP_DETAILS& tsDetails, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildComponentRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildSectionRestrainingForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildSectionRestrainingDeformationTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildRestrainedSectionForceTable(const TIME_STEP_DETAILS& tsDetails,IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildRestrainedComponentForceTable(const TIME_STEP_DETAILS& tsDetails,bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildIncrementalForceTable(IBroker* pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildIncrementalStressTable(IBroker* pBroker,const std::vector<pgsTypes::ProductForceType>& vLoads,const TIME_STEP_DETAILS& tsDetails, bool bHasDeck, IEAFDisplayUnits* pDisplayUnits) const;
   rptRcTable* BuildConcreteStressSummaryTable(IBroker* pBroker,const pgsPointOfInterest& poi,ResultsType resultsType,bool bGirder,IEAFDisplayUnits* pDisplayUnits) const;

   void ReportCreepDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const;
   void ReportShrinkageDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const;
   void ReportStrandRelaxationDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const;
   void ReportSegmentTendonRelaxationDetails(rptChapter* pChapter, IBroker* pBroker, const pgsPointOfInterest& poi, IntervalIndexType firstIntervalIdx, IntervalIndexType lastIntervalIdx, IEAFDisplayUnits* pDisplayUnits) const;
   void ReportGirderTendonRelaxationDetails(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IEAFDisplayUnits* pDisplayUnits) const;

   // Prevent accidental copying and assignment
   CTimeStepDetailsChapterBuilder(const CTimeStepDetailsChapterBuilder&) = delete;
   CTimeStepDetailsChapterBuilder& operator=(const CTimeStepDetailsChapterBuilder&) = delete;
};
