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
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CCombinedMomentsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedMomentsTable::CCombinedMomentsTable()
{
}

CCombinedMomentsTable::CCombinedMomentsTable(const CCombinedMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedMomentsTable::~CCombinedMomentsTable()
{
}

//======================== OPERATORS  =======================================
CCombinedMomentsTable& CCombinedMomentsTable::operator= (const CCombinedMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedMomentsTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   BuildCombinedDeadTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, bDesign, bRating);

   if (liveLoadIntervalIdx <= intervalIdx)
   {
      if (bDesign)
         BuildCombinedLiveTable(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, true, false);

      if (bRating)
         BuildCombinedLiveTable(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, false, true);

      if (bDesign)
         BuildLimitStateTable(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, true, false);

      if (bRating)
         BuildLimitStateTable(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, false, true);
   }
}


void CCombinedMomentsTable::BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();


   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bExcludeNoncompositeMoments = !pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   GroupIndexType startGroupIdx = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
   GroupIndexType endGroupIdx   = girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : startGroupIdx;
   EventIndexType continuityEventIndex = MAX_INDEX;

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);
   PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(  endGroupIdx);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx != endPierIdx; pierIdx++ )
   {
      EventIndexType leftContinuityEventIdx, rightContinuityEventIdx;
      pBridge->GetContinuityEventIndex(pierIdx,&leftContinuityEventIdx,&rightContinuityEventIdx);
      continuityEventIndex = _cpp_min(continuityEventIndex,leftContinuityEventIdx);
      continuityEventIndex = _cpp_min(continuityEventIndex,rightContinuityEventIdx);
   }
   IntervalIndexType continunityIntervalIdx = pIntervals->GetInterval(continuityEventIndex);


   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptMomentUnitTag,unitmgtMomentData>(&p_table,pBroker,girderKey,_T("Moment"),false,bRating,intervalIdx,continunityIntervalIdx,
                                                                                    analysisType,pDisplayUnits,pDisplayUnits->GetMomentUnit());
   *p << p_table;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   RowIndexType row2 = 1;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,  pIPoi);
   GET_IFACE2(pBroker,ICombinedForces,   pForces);
   GET_IFACE2(pBroker,ICombinedForces2,  pForces2);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Fill up the table
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey,ALL_SEGMENTS)) );

      std::vector<Float64> dummy;
      std::vector<Float64> minServiceI, maxServiceI;
      std::vector<Float64> minDCinc, maxDCinc;
      std::vector<Float64> minDCcum, maxDCcum;
      std::vector<Float64> minDWinc, maxDWinc;
      std::vector<Float64> minDWcum, maxDWcum;

      if ( intervalIdx < castDeckIntervalIdx )
      {
         maxDCinc = pForces2->GetMoment( lcDC, intervalIdx, vPoi, ctIncremental, maxBAT );
         pLsForces2->GetMoment( pgsTypes::ServiceI, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceI );
      }
      else
      {
         maxDCinc = pForces2->GetMoment( lcDC, intervalIdx, vPoi, ctIncremental, maxBAT );
         minDCinc = pForces2->GetMoment( lcDC, intervalIdx, vPoi, ctIncremental, minBAT );
         maxDWinc = pForces2->GetMoment( bRating ? lcDWRating : lcDW, intervalIdx, vPoi, ctIncremental, maxBAT );
         minDWinc = pForces2->GetMoment( bRating ? lcDWRating : lcDW, intervalIdx, vPoi, ctIncremental, minBAT );
         maxDCcum = pForces2->GetMoment( lcDC, intervalIdx, vPoi, ctCummulative, maxBAT );
         minDCcum = pForces2->GetMoment( lcDC, intervalIdx, vPoi, ctCummulative, minBAT );
         maxDWcum = pForces2->GetMoment( bRating ? lcDWRating : lcDW, intervalIdx, vPoi, ctCummulative, maxBAT );
         minDWcum = pForces2->GetMoment( bRating ? lcDWRating : lcDW, intervalIdx, vPoi, ctCummulative, minBAT );

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces2->GetMoment( pgsTypes::ServiceI, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceI );
            pLsForces2->GetMoment( pgsTypes::ServiceI, intervalIdx, vPoi, minBAT, &minServiceI, &dummy );
         }
      }


      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
         const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

         IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
         IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(thisSegmentKey);
         IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(thisSegmentKey);

         ColumnIndexType col = 0;

         Float64 end_size = 0 ;
         if ( intervalIdx != releaseIntervalIdx )
            end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);
         
         (*p_table)(row,col++) << location.SetValue( intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT, poi, end_size );
         if ( intervalIdx < castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << moment.SetValue( maxDCinc[index] );
            (*p_table)(row,col++) << moment.SetValue( maxServiceI[index] );
         }
         else
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << moment.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << moment.SetValue( minDCinc[index] );
               (*p_table)(row,col++) << moment.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << moment.SetValue( minDWinc[index] );
               (*p_table)(row,col++) << moment.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << moment.SetValue( minDCcum[index] );
               (*p_table)(row,col++) << moment.SetValue( maxDWcum[index] );
               (*p_table)(row,col++) << moment.SetValue( minDWcum[index] );

               if ( intervalIdx < liveLoadIntervalIdx )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxServiceI[index] );
                  (*p_table)(row,col++) << moment.SetValue( minServiceI[index] );
               }
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxDCinc[index] );
               (*p_table)(row,col++) << moment.SetValue( maxDWinc[index] );
               (*p_table)(row,col++) << moment.SetValue( maxDCcum[index] );
               (*p_table)(row,col++) << moment.SetValue( maxDWcum[index] );

               if ( intervalIdx < liveLoadIntervalIdx )
               {
                  (*p_table)(row,col++) << moment.SetValue( maxServiceI[index] );
               }
            }
         }

         row++;
      }
   }
}

void CCombinedMomentsTable::BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                                                  const CGirderKey& girderKey,
                                                  IEAFDisplayUnits* pDisplayUnits,
                                                  pgsTypes::AnalysisType analysisType,
                                                  bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval(); // always

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bExcludeNoncompositeMoments = !pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : startGroupIdx);
 
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   LPCTSTR strLabel = bDesign ? _T("Moment - Design Vehicles") : _T("Moment - Rating Vehicles");

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptMomentUnitTag,unitmgtMomentData>(&p_table,strLabel,false,bDesign,bPermit,bPedLoading,bRating,false,true,
                           intervalIdx,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetMomentUnit());

   RowIndexType row = Nhrows;

   *p << p_table;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   rptParagraph* pNote = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pNote;
   *pNote << LIVELOAD_PER_GIRDER << rptNewLine;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,  pIPoi);
   GET_IFACE2(pBroker,ICombinedForces,   pForces);
   GET_IFACE2(pBroker,ICombinedForces2,  pForces2);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Fill up the table
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey,ALL_SEGMENTS));

      std::vector<Float64> dummy;
      std::vector<Float64> minPedestrianLL, maxPedestrianLL;
      std::vector<Float64> minDesignLL, maxDesignLL;
      std::vector<Float64> minFatigueLL, maxFatigueLL;
      std::vector<Float64> minPermitLL, maxPermitLL;
      std::vector<Float64> minLegalRoutineLL, maxLegalRoutineLL;
      std::vector<Float64> minLegalSpecialLL, maxLegalSpecialLL;
      std::vector<Float64> minPermitRoutineLL, maxPermitRoutineLL;
      std::vector<Float64> minPermitSpecialLL, maxPermitSpecialLL;

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, intervalIdx, vPoi, maxBAT, &dummy, &maxPedestrianLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, intervalIdx, vPoi, minBAT, &minPedestrianLL, &dummy );
         }

         pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltDesign, intervalIdx, vPoi, maxBAT, &dummy, &maxDesignLL );
         pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltDesign, intervalIdx, vPoi, minBAT, &minDesignLL, &dummy );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltFatigue, intervalIdx, vPoi, maxBAT, &dummy, &maxFatigueLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltFatigue, intervalIdx, vPoi, minBAT, &minFatigueLL, &dummy );
         }

         if ( bPermit )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPermit, intervalIdx, vPoi, maxBAT, &dummy, &maxPermitLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPermit, intervalIdx, vPoi, minBAT, &minPermitLL, &dummy );
         }
      }

      if ( bRating )
      {
         if(pRatingSpec->IncludePedestrianLiveLoad())
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, intervalIdx, vPoi, maxBAT, &dummy, &maxPedestrianLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, intervalIdx, vPoi, minBAT, &minPedestrianLL, &dummy );
         }

         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltDesign, intervalIdx, vPoi, maxBAT, &dummy, &maxDesignLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltDesign, intervalIdx, vPoi, minBAT, &minDesignLL, &dummy );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltLegalRating_Routine, intervalIdx, vPoi, maxBAT, &dummy, &maxLegalRoutineLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltLegalRating_Routine, intervalIdx, vPoi, minBAT, &minLegalRoutineLL, &dummy );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltLegalRating_Special, intervalIdx, vPoi, maxBAT, &dummy, &maxLegalSpecialLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltLegalRating_Special, intervalIdx, vPoi, minBAT, &minLegalSpecialLL, &dummy );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPermitRating_Routine, intervalIdx, vPoi, maxBAT, &dummy, &maxPermitRoutineLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPermitRating_Routine, intervalIdx, vPoi, minBAT, &minPermitRoutineLL, &dummy );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPermitRating_Special, intervalIdx, vPoi, maxBAT, &dummy, &maxPermitSpecialLL );
            pForces2->GetCombinedLiveLoadMoment( pgsTypes::lltPermitRating_Special, intervalIdx, vPoi, minBAT, &minPermitSpecialLL, &dummy );
         }
      }

      // fill table (first half if design/ped load)
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      IndexType index = 0;
      ColumnIndexType col = 0;

      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
         col = 0;

         Float64 end_size = pBridge->GetSegmentStartEndDistance(poi.GetSegmentKey());
         
         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               (*p_table)(row,col++) << moment.SetValue( maxPedestrianLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minPedestrianLL[index] );
            }

            (*p_table)(row,col++) << moment.SetValue( maxDesignLL[index] );
            (*p_table)(row,col++) << moment.SetValue( minDesignLL[index] );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col++) << moment.SetValue( maxFatigueLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minFatigueLL[index] );
            }

            if ( bPermit )
            {
               (*p_table)(row,col++) << moment.SetValue( maxPermitLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minPermitLL[index] );
            }
         }

         if ( bRating )
         {
            if(bPedLoading && pRatingSpec->IncludePedestrianLiveLoad())
            {
               (*p_table)(row,col++) << moment.SetValue( maxPedestrianLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minPedestrianLL[index] );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)))
            {
               (*p_table)(row,col++) << moment.SetValue( maxDesignLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minDesignLL[index] );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col++) << moment.SetValue( maxLegalRoutineLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minLegalRoutineLL[index] );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col++) << moment.SetValue( maxLegalSpecialLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minLegalSpecialLL[index] );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col++) << moment.SetValue( maxPermitRoutineLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minPermitRoutineLL[index] );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col++) << moment.SetValue( maxPermitSpecialLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minPermitSpecialLL[index] );
            }
         }

         row++;
      }


      // fill second half of table if design & ped load
      if ( bDesign && bPedLoading )
      {
         // Sum or envelope pedestrian values with live loads to give final LL

         GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
         ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
         ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);
         ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);

         SumPedAndLiveLoad(DesignPedLoad, minDesignLL, maxDesignLL, minPedestrianLL, maxPedestrianLL);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            SumPedAndLiveLoad(FatiguePedLoad, minFatigueLL, maxFatigueLL, minPedestrianLL, maxPedestrianLL);
         }

         if ( bPermit )
         {
            SumPedAndLiveLoad(PermitPedLoad, minPermitLL, maxPermitLL, minPedestrianLL, maxPedestrianLL);
         }

         // Now we can fill table
         //RowIndexType    row = Nhrows;
         ColumnIndexType recCol = col;
         IndexType psiz = (IndexType)vPoi.size();
         for ( index=0; index<psiz; index++ )
         {
            col = recCol;

            (*p_table)(row,col++) << moment.SetValue( maxDesignLL[index] );
            (*p_table)(row,col++) << moment.SetValue( minDesignLL[index] );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col++) << moment.SetValue( maxFatigueLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minFatigueLL[index] );
            }

            if ( bPermit )
            {
               (*p_table)(row,col++) << moment.SetValue( maxPermitLL[index] );
               (*p_table)(row,col++) << moment.SetValue( minPermitLL[index] );
            }

            row++;
         }

         // footnotes for pedestrian loads
         int lnum=1;
         *pNote<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            *pNote << lnum++ << PedestrianFootnote(FatiguePedLoad) << rptNewLine;
         }

         if ( bPermit )
         {
            *pNote << lnum++ << PedestrianFootnote(PermitPedLoad) << rptNewLine;
         }
      }

      if ( bRating && pRatingSpec->IncludePedestrianLiveLoad())
      {
         // Note for rating and pedestrian load
         *pNote << _T("$ Pedestrian load results will be summed with vehicular load results at time of load combination.");
      }
   }
}

void CCombinedMomentsTable::BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                               const CGirderKey& girderKey,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               pgsTypes::AnalysisType analysisType,
                                               bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval(); // always

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bExcludeNoncompositeMoments = !pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : startGroupIdx);
 
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IEventMap,pEventMap);

   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   rptRcTable * p_table2;
   RowIndexType row2 = CreateLimitStateTableHeading<rptMomentUnitTag,unitmgtMomentData>(&p_table2,_T("Moment, Mu"),false,bDesign,bPermit,bRating,true,analysisType,pEventMap,pRatingSpec,pDisplayUnits,pDisplayUnits->GetMomentUnit());
   *p << p_table2;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table2->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table2->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,  pIPoi);
   GET_IFACE2(pBroker,ICombinedForces,   pForces);
   GET_IFACE2(pBroker,ICombinedForces2,  pForces2);
   GET_IFACE2(pBroker,ILimitStateForces, pLsForces);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Fill up the table
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey,ALL_SEGMENTS));

      std::vector<Float64> dummy;
      std::vector<Float64> minServiceI,   maxServiceI;
      std::vector<Float64> minServiceIA,  maxServiceIA;
      std::vector<Float64> minServiceIII, maxServiceIII;
      std::vector<Float64> minFatigueI,   maxFatigueI;
      std::vector<Float64> minStrengthI,  maxStrengthI;
      std::vector<Float64> minStrengthII, maxStrengthII;
      std::vector<Float64> minStrengthI_Inventory, maxStrengthI_Inventory;
      std::vector<Float64> minStrengthI_Operating, maxStrengthI_Operating;
      std::vector<Float64> minStrengthI_Legal_Routine, maxStrengthI_Legal_Routine;
      std::vector<Float64> minStrengthI_Legal_Special, maxStrengthI_Legal_Special;
      std::vector<Float64> minStrengthII_Permit_Routine, maxStrengthII_Permit_Routine;
      std::vector<Float64> minStrengthII_Permit_Special, maxStrengthII_Permit_Special;
      std::vector<Float64> minServiceI_Permit_Routine, maxServiceI_Permit_Routine;
      std::vector<Float64> minServiceI_Permit_Special, maxServiceI_Permit_Special;
      std::vector<Float64> slabStrengthI;
      std::vector<Float64> slabStrengthII;
      std::vector<Float64> slabStrengthI_Inventory;
      std::vector<Float64> slabStrengthI_Operating;
      std::vector<Float64> slabStrengthI_Legal_Routine;
      std::vector<Float64> slabStrengthI_Legal_Special;
      std::vector<Float64> slabStrengthII_Permit_Routine;
      std::vector<Float64> slabStrengthII_Permit_Special;

      // NOTE: need slab moments for other strength limit states

      if ( bDesign )
      {
         pLsForces2->GetMoment( pgsTypes::ServiceI, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceI );
         pLsForces2->GetMoment( pgsTypes::ServiceI, intervalIdx, vPoi, minBAT, &minServiceI, &dummy );

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pLsForces2->GetMoment( pgsTypes::ServiceIA, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceIA );
            pLsForces2->GetMoment( pgsTypes::ServiceIA, intervalIdx, vPoi, minBAT, &minServiceIA, &dummy );
         }
         else
         {
            pLsForces2->GetMoment( pgsTypes::FatigueI, intervalIdx, vPoi, maxBAT, &dummy, &maxFatigueI );
            pLsForces2->GetMoment( pgsTypes::FatigueI, intervalIdx, vPoi, minBAT, &minFatigueI, &dummy );
         }

         pLsForces2->GetMoment( pgsTypes::ServiceIII, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceIII );
         pLsForces2->GetMoment( pgsTypes::ServiceIII, intervalIdx, vPoi, minBAT, &minServiceIII, &dummy );

         pLsForces2->GetMoment( pgsTypes::StrengthI, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthI );
         pLsForces2->GetMoment( pgsTypes::StrengthI, intervalIdx, vPoi, minBAT, &minStrengthI, &dummy );
         slabStrengthI = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthI,vPoi,minBAT);

         if ( bPermit )
         {
            pLsForces2->GetMoment( pgsTypes::StrengthII, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthII );
            pLsForces2->GetMoment( pgsTypes::StrengthII, intervalIdx, vPoi, minBAT, &minStrengthII, &dummy );
           slabStrengthII = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthII,vPoi,minBAT);
         }
      }

      if ( bRating )
      {
         // Design - Inventory
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pLsForces2->GetMoment( pgsTypes::StrengthI_Inventory, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthI_Inventory );
            pLsForces2->GetMoment( pgsTypes::StrengthI_Inventory, intervalIdx, vPoi, maxBAT, &minStrengthI_Inventory, &dummy );
            slabStrengthI_Inventory = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthI_Inventory,vPoi,minBAT);
         }

         // Design - Operating
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
         {
            pLsForces2->GetMoment( pgsTypes::StrengthI_Operating, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthI_Operating );
            pLsForces2->GetMoment( pgsTypes::StrengthI_Operating, intervalIdx, vPoi, minBAT, &minStrengthI_Operating, &dummy );
            slabStrengthI_Operating = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthI_Operating,vPoi,minBAT);
         }

         // Legal - Routine
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pLsForces2->GetMoment( pgsTypes::StrengthI_LegalRoutine, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthI_Legal_Routine );
            pLsForces2->GetMoment( pgsTypes::StrengthI_LegalRoutine, intervalIdx, vPoi, maxBAT, &minStrengthI_Legal_Routine, &dummy );
            slabStrengthI_Legal_Routine = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthI_LegalRoutine,vPoi,minBAT);
         }

         // Legal - Special
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pLsForces2->GetMoment( pgsTypes::StrengthI_LegalSpecial, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthI_Legal_Special );
            pLsForces2->GetMoment( pgsTypes::StrengthI_LegalSpecial, intervalIdx, vPoi, minBAT, &minStrengthI_Legal_Special, &dummy );
            slabStrengthI_Legal_Special = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthI_LegalSpecial,vPoi,minBAT);
         }

         // Permit Rating - Routine
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pLsForces2->GetMoment( pgsTypes::ServiceI_PermitRoutine, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceI_Permit_Routine );
            pLsForces2->GetMoment( pgsTypes::ServiceI_PermitRoutine, intervalIdx, vPoi, minBAT, &minServiceI_Permit_Routine, &dummy );
            pLsForces2->GetMoment( pgsTypes::StrengthII_PermitRoutine, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthII_Permit_Routine );
            pLsForces2->GetMoment( pgsTypes::StrengthII_PermitRoutine, intervalIdx, vPoi, minBAT, &minStrengthII_Permit_Routine, &dummy );
            slabStrengthII_Permit_Routine = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthII_PermitRoutine,vPoi,minBAT);
         }

         // Permit Rating - Special
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pLsForces2->GetMoment( pgsTypes::ServiceI_PermitSpecial, intervalIdx, vPoi, maxBAT, &dummy, &maxServiceI_Permit_Special );
            pLsForces2->GetMoment( pgsTypes::ServiceI_PermitSpecial, intervalIdx, vPoi, minBAT, &minServiceI_Permit_Special, &dummy );
            pLsForces2->GetMoment( pgsTypes::StrengthII_PermitSpecial, intervalIdx, vPoi, maxBAT, &dummy, &maxStrengthII_Permit_Special );
            pLsForces2->GetMoment( pgsTypes::StrengthII_PermitSpecial, intervalIdx, vPoi, minBAT, &minStrengthII_Permit_Special, &dummy );
            slabStrengthII_Permit_Special = pLsForces2->GetSlabDesignMoment(pgsTypes::StrengthII_PermitSpecial,vPoi,minBAT);
         }
      }

      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         ColumnIndexType col = 0;

         const pgsPointOfInterest& poi = *i;

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());

         Float64 end_size = 0 ;
         if ( intervalIdx != releaseIntervalIdx )
            end_size = pBridge->GetSegmentStartEndDistance(poi.GetSegmentKey());

         (*p_table2)(row2,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bDesign )
            {
               (*p_table2)(row2,col++) << moment.SetValue( maxServiceI[index] );
               (*p_table2)(row2,col++) << moment.SetValue( minServiceI[index] );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxServiceIA[index] );
                  (*p_table2)(row2,col++) << moment.SetValue( minServiceIA[index] );
               }

               (*p_table2)(row2,col++) << moment.SetValue( maxServiceIII[index] );
               (*p_table2)(row2,col++) << moment.SetValue( minServiceIII[index] );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxFatigueI[index] );
                  (*p_table2)(row2,col++) << moment.SetValue( minFatigueI[index] );
               }

               (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI[index] );
               (*p_table2)(row2,col++) << moment.SetValue( minStrengthI[index] );
               (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI[index] );

               if ( bPermit )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthII[index] );
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthII[index] );
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthII[index] );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Inventory[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Inventory[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Inventory[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Operating[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Operating[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Operating[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Legal_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Legal_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Legal_Routine[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Legal_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Legal_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Legal_Special[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxServiceI_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minServiceI_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthII_Permit_Routine[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxServiceI_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minServiceI_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthII_Permit_Special[index]);
               }
            }
         }
         else
         {
            if ( bDesign )
            {
               (*p_table2)(row2,col++) << moment.SetValue( maxServiceI[index] );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
                  (*p_table2)(row2,col++) << moment.SetValue( maxServiceIA[index] );
               
               (*p_table2)(row2,col++) << moment.SetValue( maxServiceIII[index] );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
                  (*p_table2)(row2,col++) << moment.SetValue( maxFatigueI[index] );

               (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI[index] );
               (*p_table2)(row2,col++) << moment.SetValue( minStrengthI[index] );
               (*p_table2)(row2,col++) << moment.SetValue( slabStrengthI[index] );

               if ( bPermit )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthII[index] );
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthII[index] );
                  (*p_table2)(row2,col++) << moment.SetValue( slabStrengthII[index] );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Inventory[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Inventory[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Inventory[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Operating[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Operating[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Operating[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Legal_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Legal_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Legal_Routine[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue( maxStrengthI_Legal_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue( minStrengthI_Legal_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthI_Legal_Special[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue(maxServiceI_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(minServiceI_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(maxStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(minStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthII_Permit_Routine[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  (*p_table2)(row2,col++) << moment.SetValue(maxServiceI_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(minServiceI_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(maxStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(minStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << moment.SetValue(slabStrengthII_Permit_Special[index]);
               }
            }
         }

         row2++;
      }
   }

   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   if (bExcludeNoncompositeMoments)
   {
      (*p) << _T("* Deck moment (Mu) is for negative moment deck design, and is from superimposed dead load and live load only.") << rptNewLine;
   }
   else
   {
      (*p) << _T("* Deck moment (Mu) is for negative moment deck design.") << rptNewLine;
   }
}


//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCombinedMomentsTable::MakeCopy(const CCombinedMomentsTable& rOther)
{
   // Add copy code here...
}

void CCombinedMomentsTable::MakeAssignment(const CCombinedMomentsTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CCombinedMomentsTable::AssertValid() const
{
   return true;
}

void CCombinedMomentsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedMomentsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCombinedMomentsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCombinedMomentsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCombinedMomentsTable");

   TESTME_EPILOG("CCombinedMomentsTable");
}
#endif // _UNITTEST
