///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <Reporting\BridgeAnalysisChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <Reporting\BridgeAnalysisReportSpecification.h>

#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>

#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>

#include <Reporting\LiveLoadDistributionFactorTable.h>

#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <Reporting\TSRemovalMomentsTable.h>
#include <Reporting\TSRemovalShearTable.h>
#include <Reporting\TSRemovalDisplacementsTable.h>
#include <Reporting\TSRemovalRotationTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBridgeAnalysisChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBridgeAnalysisChapterBuilder::CBridgeAnalysisChapterBuilder(LPCTSTR strTitle,pgsTypes::AnalysisType analysisType,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_strTitle = strTitle;
   m_AnalysisType = analysisType;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBridgeAnalysisChapterBuilder::GetName() const
{
   return m_strTitle.c_str();
}

rptChapter* CBridgeAnalysisChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBridgeAnalysisReportSpecification* pBridgeAnalysisRptSpec = dynamic_cast<CBridgeAnalysisReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBridgeAnalysisRptSpec->GetBroker(&pBroker);

   CGirderKey girderKey(pBridgeAnalysisRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ISpecification,pSpec);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* p = 0;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   if ( (m_AnalysisType == pgsTypes::Simple     && analysisType == pgsTypes::Continuous ) || 
        (m_AnalysisType == pgsTypes::Continuous && analysisType == pgsTypes::Simple )     ||
        (m_AnalysisType == pgsTypes::Envelope   && analysisType != pgsTypes::Envelope )
      )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << _T("The structural analysis method had changed since this report was created. Analysis results are no longer available. Close this report and re-create it to get analysis results.") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   bool bDesign = pBridgeAnalysisRptSpec->ReportDesignResults();
   bool bRating = pBridgeAnalysisRptSpec->ReportRatingResults();

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   bool bIndicateControllingLoad = true;

   // Product Moments
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << _T("Load Responses - Bridge Site")<<rptNewLine;
   p->SetName(_T("Bridge Site Results"));
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
 
   *p << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);
    

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool bAreThereUserLoads = pUDL->DoUserLoadsExist(girderKey);
   if (bAreThereUserLoads)
   {
      *p << CUserMomentsTable().Build(pBroker,girderKey,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   CTSRemovalMomentsTable().Build(pChapter,pBroker,girderKey,m_AnalysisType,pDisplayUnits);

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   if (bAreThereUserLoads)
   {
      *p << CUserShearTable().Build(pBroker,girderKey,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   CTSRemovalShearTable().Build(pChapter,pBroker,girderKey,m_AnalysisType,pDisplayUnits);

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,girderKey,m_AnalysisType,CProductReactionTable::PierReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   bool bDoBearingReaction, bDummy;
   bDoBearingReaction = pBearingDesign->AreBearingReactionsAvailable(girderKey,&bDummy,&bDummy);
   if ( bDoBearingReaction && girderKey.groupIndex != ALL_GROUPS )
   {
      *p << CProductReactionTable().Build(pBroker,girderKey,m_AnalysisType,CProductReactionTable::BearingReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);
   }

   if (bAreThereUserLoads)
   {
      *p << CUserReactionTable().Build(pBroker,girderKey,m_AnalysisType,CUserReactionTable::PierReactionsTable,pDisplayUnits) << rptNewLine;
      if(bDoBearingReaction)
      {
         *p << CUserReactionTable().Build(pBroker,girderKey,m_AnalysisType,CUserReactionTable::BearingReactionsTable,pDisplayUnits) << rptNewLine;
      }
   }

#pragma Reminder("Report reactions due to temporary support removal")
   //CTSRemovalReactionsTable().Build(pChapter,pBroker,girderKey,m_AnalysisType,pDisplayUnits);

   // Product Displacements
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductDisplacementsTable().Build(pBroker,girderKey,m_AnalysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   if (bAreThereUserLoads)
   {
      *p << CUserDisplacementsTable().Build(pBroker,girderKey,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   CTSRemovalDisplacementsTable().Build(pChapter,pBroker,girderKey,m_AnalysisType,pDisplayUnits);

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,girderKey,m_AnalysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girderKey,bDesign,bRating);

   if (bAreThereUserLoads)
   {
      *p << CUserRotationTable().Build(pBroker,girderKey,m_AnalysisType,pDisplayUnits) << rptNewLine;
   }

   CTSRemovalRotationTable().Build(pChapter,pBroker,girderKey,m_AnalysisType,pDisplayUnits);

   // Responses from individual live load vehicules
   std::vector<pgsTypes::LiveLoadType> live_load_types;
   if ( bDesign )
   {
      live_load_types.push_back(pgsTypes::lltDesign);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
        live_load_types.push_back(pgsTypes::lltFatigue);

      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
      bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
      if ( bPermit )
         live_load_types.push_back(pgsTypes::lltPermit);
   }

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   if ( bRating )
   {
      // if lltDesign isn't included because we aren't reporting design and if we are doing Design Inventory or Operating rating
      // then add the lltDesign
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         live_load_types.push_back(pgsTypes::lltDesign);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         live_load_types.push_back(pgsTypes::lltLegalRating_Routine);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         live_load_types.push_back(pgsTypes::lltLegalRating_Special);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         live_load_types.push_back(pgsTypes::lltPermitRating_Routine);

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         live_load_types.push_back(pgsTypes::lltPermitRating_Special);
   }

   std::vector<pgsTypes::LiveLoadType>::iterator iter;
   for ( iter = live_load_types.begin(); iter != live_load_types.end(); iter++ )
   {
      pgsTypes::LiveLoadType llType = *iter;

      GET_IFACE2( pBroker, IProductLoads, pProductLoads);
      std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

      // nothing to report if there are no loads
      if ( strLLNames.size() == 0 )
         continue;

      // if the only loading is the DUMMY load, then move on
      if ( strLLNames.size() == 1 && strLLNames[0] == std::_tstring(_T("No Live Load Defined")) )
         continue;

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;

      if ( llType == pgsTypes::lltDesign )
      {
         p->SetName(_T("Design Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermit )
      {
         p->SetName(_T("Permit Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltFatigue )
      {
         p->SetName(_T("Fatigue Live Load Individual Vehicle Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPedestrian )
      {
         p->SetName(_T("Pedestrian Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Routine )
      {
         p->SetName(_T("AASHTO Legal Rating Routine Commercial Vehicle Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltLegalRating_Special )
      {
         p->SetName(_T("AASHTO Legal Rating Specialized Hauling Vehicle Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Routine )
      {
         p->SetName(_T("Routine Permit Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else if ( llType == pgsTypes::lltPermitRating_Special )
      {
         p->SetName(_T("Special Permit Rating Individual Vehicle Live Load Response"));
         *p << p->GetName() << rptNewLine;
      }
      else
      {
         ATLASSERT(false); // is there a new live load type???
      }

      std::vector<std::_tstring>::iterator iter;
      for ( iter = strLLNames.begin(); iter != strLLNames.end(); iter++ )
      {
         std::_tstring strLLName = *iter;

         VehicleIndexType index = iter - strLLNames.begin();

         p = new rptParagraph;
         *pChapter << p;
         p->SetName( strLLName.c_str() );
         *p << CVehicularLoadResultsTable().Build(pBroker,girderKey,llType,strLLName,index,m_AnalysisType,true,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;

         *p << rptNewLine;

         *p << CVehicularLoadReactionTable().Build(pBroker,girderKey,llType,strLLName,index,m_AnalysisType,true,pDisplayUnits) << rptNewLine;
         *p << LIVELOAD_PER_LANE << rptNewLine;
      }
   }

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0)); // release interval is the same for all segments
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Load Combinations (DC, DW, etc) & Limit States
   for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      CString strName;
      strName.Format(_T("Combined Results - Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      p->SetName(strName);
      *p << p->GetName();

      if ( liveLoadIntervalIdx <= intervalIdx )
      {
         CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,girderKey,pDisplayUnits);
      }

      CCombinedMomentsTable().Build(pBroker,pChapter,girderKey,pDisplayUnits, intervalIdx, analysisType, bDesign, bRating);
      if ( castDeckIntervalIdx <= intervalIdx )
      {
         CCombinedShearTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
         CCombinedReactionTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx,analysisType,CCombinedReactionTable::PierReactionsTable, bDesign, bRating);
#pragma Reminder("UPDATE: this crashes for ALL_GROUPS")
         //if( bDoBearingReaction )
         //{
         //   CCombinedReactionTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx,analysisType,CCombinedReactionTable::BearingReactionsTable, bDesign, bRating);
         //}

         if ( liveLoadIntervalIdx <= intervalIdx )
         {
            p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << p;
            *p << _T("Live Load Reactions Without Impact") << rptNewLine;
            p->SetName(_T("Live Load Reactions Without Impact"));
            CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,girderKey,pDisplayUnits,analysisType,CCombinedReactionTable::PierReactionsTable, false, true, false);
#pragma Reminder("UPDATE: this crashes for ALL_GROUPS")
            //if(bDoBearingReaction)
            //{
            //   CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,girderKey,pDisplayUnits,analysisType,CCombinedReactionTable::BearingReactionsTable, false, true, false);
            //}
         }
      }
   } // next interval

#pragma Reminder("OBSOLETE: remove obsolete code")
//
//   GET_IFACE2(pBroker,IIntervals,pIntervals);
//   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
//   IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
//   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
//   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
//   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
//
//#pragma Reminder("UPDATE: need to be reporting on all intervals") // this is a hack to keep development moving along
//   IntervalIndexType erectSegmentIntervalIdx;
//   if ( segmentKey.groupIndex == ALL_GROUPS )
//      erectSegmentIntervalIdx = pIntervals->GetFirstErectedSegmentInterval();
//   else
//      erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
//
//   // Load Combinations (DC, DW, etc) & Limit States
//   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//   *pChapter << p;
//   *p << _T("Responses - Casting Yard Stage") << rptNewLine;
//   p->SetName(_T("Casting Yard"));
//   CCombinedMomentsTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,releaseIntervalIdx, m_AnalysisType);
//
//   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//   *pChapter << p;
//   *p << _T("Responses - Girder Placement") << rptNewLine;
//   p->SetName(_T("Girder Placement"));
//   CCombinedMomentsTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,erectSegmentIntervalIdx, m_AnalysisType);
//   CCombinedShearTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,erectSegmentIntervalIdx, m_AnalysisType);
//   CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,erectSegmentIntervalIdx, m_AnalysisType,CCombinedReactionTable::PierReactionsTable);
//   if(bDoBearingReaction && span != ALL_SPANS)
//   {
//      CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,erectSegmentIntervalIdx, m_AnalysisType,CCombinedReactionTable::BearingReactionsTable);
//   }
//
//   GET_IFACE2(pBroker,IBridge,pBridge);
//   SpanIndexType nSpans = pBridge->GetSpanCount();
//   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
//   bool bTempStrands = false;
//   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
//   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
//   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
//   {
//      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
//      GirderIndexType gdrIdx = (nGirders <= girder ? nGirders-1 : girder);
//      CSegmentKey segKey(spanIdx,gdrIdx,0);
//      if ( 0 < pStrandGeom->GetMaxStrands(segKey,pgsTypes::Temporary) )
//      {
//         bTempStrands = true;
//         break;
//      }
//   }
//
//   if ( bTempStrands )
//   {
//      // if there can be temporary strands, report the loads at the temporary strand removal stage
//      // because this is when the girder load is applied
//      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//      *pChapter << p;
//      *p << _T("Responses - Temporary Strand Removal Stage") << rptNewLine;
//      p->SetName(_T("Temporary Strand Removal"));
//      CCombinedMomentsTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,tsRemovalIntervalIdx, m_AnalysisType);
//      CCombinedShearTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,tsRemovalIntervalIdx, m_AnalysisType);
//      CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,tsRemovalIntervalIdx, m_AnalysisType,CCombinedReactionTable::PierReactionsTable);
//      if(bDoBearingReaction && span != ALL_SPANS)
//      {
//         CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,tsRemovalIntervalIdx, m_AnalysisType,CCombinedReactionTable::BearingReactionsTable);
//      }
//   }
//
//   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//   *pChapter << p;
//   *p << _T("Responses - Deck and Diaphragm Placement (Bridge Site 1)") << rptNewLine;
//   p->SetName(_T("Deck and Diaphragm Placement (Bridge Site 1)"));
//   CCombinedMomentsTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,castDeckIntervalIdx,m_AnalysisType);
//   CCombinedShearTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,castDeckIntervalIdx,m_AnalysisType);
//   CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,castDeckIntervalIdx,m_AnalysisType,CCombinedReactionTable::PierReactionsTable);
//   if(bDoBearingReaction && span != ALL_SPANS)
//   {
//      CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,castDeckIntervalIdx,m_AnalysisType,CCombinedReactionTable::BearingReactionsTable);
//   }
//
//   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//   *pChapter << p;
//   *p << _T("Responses - Superimposed Dead Loads (Bridge Site 2)") << rptNewLine;
//   p->SetName(_T("Superimposed Dead Loads (Bridge Site 2)"));
//   CCombinedMomentsTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,compositeDeckIntervalIdx,m_AnalysisType);
//   CCombinedShearTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,compositeDeckIntervalIdx,m_AnalysisType);
//   CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,compositeDeckIntervalIdx,m_AnalysisType,CCombinedReactionTable::PierReactionsTable);
//   if(bDoBearingReaction && span != ALL_SPANS)
//   {
//      CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,compositeDeckIntervalIdx,m_AnalysisType,CCombinedReactionTable::BearingReactionsTable);
//   }
//
//   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//   *pChapter << p;
//   *p << _T("Responses - Final with Live Load (Bridge Site 3)") << rptNewLine;
//   p->SetName(_T("Final with Live Load (Bridge Site 3)"));
//
//   CCombinedMomentsTable().Build( pBroker,pChapter,segmentKey,pDisplayUnits,liveLoadIntervalIdx,m_AnalysisType,bDesign,bRating);
//   CCombinedShearTable().Build(   pBroker,pChapter,segmentKey,pDisplayUnits,liveLoadIntervalIdx,m_AnalysisType,bDesign,bRating);
//   CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,liveLoadIntervalIdx,m_AnalysisType,CCombinedReactionTable::PierReactionsTable,bDesign,bRating);
//   if(bDoBearingReaction && span != ALL_SPANS)
//   {
//      CCombinedReactionTable().Build(pBroker,pChapter,segmentKey,pDisplayUnits,liveLoadIntervalIdx,m_AnalysisType,CCombinedReactionTable::BearingReactionsTable,bDesign,bRating);
//   }
//
//   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
//   *pChapter << p;
//   *p << _T("Live Load Reactions without Impact") << rptNewLine;
//   p->SetName(_T("Live Load Reactions without Impact"));
//   CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,segmentKey,pDisplayUnits, m_AnalysisType,CCombinedReactionTable::PierReactionsTable, false, true, false  );
//   CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,segmentKey,pDisplayUnits, m_AnalysisType,CCombinedReactionTable::PierReactionsTable, false, false, true  );
//
//   if(bDoBearingReaction && span!=ALL_SPANS)
//   {
//      CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,segmentKey,pDisplayUnits, m_AnalysisType,CCombinedReactionTable::BearingReactionsTable, false, true, false  );
//      CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,segmentKey,pDisplayUnits, m_AnalysisType,CCombinedReactionTable::BearingReactionsTable, false, false, true  );
//   }
   return pChapter;
}

CChapterBuilder* CBridgeAnalysisChapterBuilder::Clone() const
{
   return new CBridgeAnalysisChapterBuilder(m_strTitle.c_str(),m_AnalysisType);
}
