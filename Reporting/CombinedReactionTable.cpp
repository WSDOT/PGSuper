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
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>

#include <PgsExt\PierData2.h>
#include <PgsExt\GirderGroupData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CCombinedReactionTable
****************************************************************************/

// Function to report rows for combined pedestrian result
inline void CombineReportPedResult(ILiveLoads::PedestrianLoadApplicationType appType,
                              rptRcTable* pTable, rptForceSectionValue& reaction,
                              RowIndexType row, ColumnIndexType& pedCol,
                              Float64 llMin, Float64 llMax, Float64 pedMin, Float64 pedMax)
{
   if (appType==ILiveLoads::PedDontApply)
   {
      // no combo
     (*pTable)(row,pedCol++) << reaction.SetValue( llMax );
     (*pTable)(row,pedCol++) << reaction.SetValue( llMin );
   }
   else if (appType==ILiveLoads::PedConcurrentWithVehicular)
   {
      // sum
     (*pTable)(row,pedCol++) << reaction.SetValue( llMax + pedMax );
     (*pTable)(row,pedCol++) << reaction.SetValue( llMin + pedMin );
   }
   else if (appType==ILiveLoads::PedEnvelopeWithVehicular)
   {
     // envelope
     (*pTable)(row,pedCol++) << reaction.SetValue( Max(llMax, pedMax) );
     (*pTable)(row,pedCol++) << reaction.SetValue( Min(llMin, pedMin) );
   }
   else
   {
      ATLASSERT(0);
   }
}

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedReactionTable::CCombinedReactionTable()
{
}

CCombinedReactionTable::CCombinedReactionTable(const CCombinedReactionTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedReactionTable::~CCombinedReactionTable()
{
}

//======================== OPERATORS  =======================================
CCombinedReactionTable& CCombinedReactionTable::operator= (const CCombinedReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedReactionTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                          const CGirderKey& girderKey, 
                                          IEAFDisplayUnits* pDisplayUnits,
                                          IntervalIndexType intervalIdx, pgsTypes::AnalysisType analysisType,ReactionTableType tableType,
                                          bool bDesign,bool bRating) const
{
   BuildCombinedDeadTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, tableType, bDesign, bRating);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      if (bDesign)
         BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, tableType, true, true, false);

      if (bRating)
         BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, tableType, true, false, true);

      if (bDesign)
         BuildLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, tableType, true, false);

      if (bRating)
         BuildLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, tableType, false, true);
   }
}

void CCombinedReactionTable::BuildForBearingDesign(IBroker* pBroker, rptChapter* pChapter,
                                          const CGirderKey& girderKey, 
                                          IEAFDisplayUnits* pDisplayUnits,
                                          IntervalIndexType intervalIdx, pgsTypes::AnalysisType analysisType) const
{
   ReactionTableType tableType = BearingReactionsTable;

   BuildCombinedDeadTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, tableType, true, false);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if (liveLoadIntervalIdx <= intervalIdx)
   {
      // first no impact
      BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, tableType, false, true, false);
      BuildLimitStateTable(pBroker, pChapter, girderKey, false, pDisplayUnits, intervalIdx, analysisType, tableType, true, false);

      // with impact
      BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, tableType, true, true, false);
      BuildLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, tableType, true, false);
   }
}

void CCombinedReactionTable::BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                                          const CGirderKey& girderKey, 
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
                                         bool bDesign,bool bRating) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces = std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pLsForces,girderKey));
   }
   else
   {
      pForces = std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, intervalIdx, girderKey) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   EventIndexType continuityEventIndex = MAX_INDEX;
   for ( iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation( iter.CurrentItem() );
      if ( pBridge->IsBoundaryPier(reactionLocation.PierIdx) )
      {
         EventIndexType leftContinuityEventIdx, rightContinuityEventIdx;
         pBridge->GetContinuityEventIndex(reactionLocation.PierIdx,&leftContinuityEventIdx,&rightContinuityEventIdx);
         continuityEventIndex = Min(continuityEventIndex,leftContinuityEventIdx);
         continuityEventIndex = Min(continuityEventIndex,rightContinuityEventIdx);
      }
   }
   IntervalIndexType continuityIntervalIdx = pIntervals->GetInterval(continuityEventIndex);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table=0;

   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,pBroker,girderKey,(tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions")),
                                 true ,bRating,intervalIdx,continuityIntervalIdx,
                                 analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());

   *p << p_table;

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   // Fill up the table
   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation( iter.CurrentItem() );

     (*p_table)(row,0) << reactionLocation.PierLabel;

      ColumnIndexType col = 1;
      if ( analysisType == pgsTypes::Envelope /*&& continuityIntervalIdx == castDeckIntervalIdx*/ )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, intervalIdx, reactionLocation, ctIncremental, minBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, intervalIdx, reactionLocation, ctIncremental, minBAT ) );

         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, intervalIdx, reactionLocation, ctIncremental, minBAT ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcCR, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcCR, intervalIdx, reactionLocation, ctIncremental, minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcSH, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcSH, intervalIdx, reactionLocation, ctIncremental, minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcPS, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcPS, intervalIdx, reactionLocation, ctIncremental, minBAT ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, intervalIdx, reactionLocation, ctCummulative, minBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, intervalIdx, reactionLocation, ctCummulative, minBAT ) );

         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, intervalIdx, reactionLocation, ctCummulative, minBAT ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcCR, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcCR, intervalIdx, reactionLocation, ctCummulative, minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcSH, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcSH, intervalIdx, reactionLocation, ctCummulative, minBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcPS, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcPS, intervalIdx, reactionLocation, ctCummulative, minBAT ) );
         }

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces->GetReaction( pgsTypes::ServiceI, intervalIdx, reactionLocation.PierIdx, reactionLocation.GirderKey, maxBAT, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( pgsTypes::ServiceI, intervalIdx, reactionLocation.PierIdx, reactionLocation.GirderKey, minBAT,true,  &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
      }
      else
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
         
         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcCR, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcSH, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcPS, intervalIdx, reactionLocation, ctIncremental, maxBAT ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDC, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDW, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );

         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcDWRating, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcCR, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcSH, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lcPS, intervalIdx, reactionLocation, ctCummulative, maxBAT ) );
         }

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces->GetReaction( pgsTypes::ServiceI, intervalIdx, reactionLocation.PierIdx, reactionLocation.GirderKey, maxBAT, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
         }
      }

      row++;
   }
}

void CCombinedReactionTable::BuildLiveLoad(IBroker* pBroker, rptChapter* pChapter,
                                          const CGirderKey& girderKey, 
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
                                         bool includeImpact, bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
  std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType == PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pLsForces,girderKey));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, intervalIdx, girderKey) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   // Use first location to determine if ped load is applied
   iter.First();
   const ReactionLocation& first_locn = iter.CurrentItem();

   bool bPermit = pLsForces->IsStrengthIIApplicable(girderKey);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   // pedestrian live load combination types, if applicable
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
   ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);
   ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   std::_tstring strLabel (tableType == PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions"));
   strLabel += std::_tstring(bDesign ? _T(" - Design Vehicles") : _T(" - Rating Vehicles"));
   strLabel += std::_tstring(includeImpact ? _T(" (Including Impact)") : _T(" (Without Impact)"));

   rptRcTable* p_table=0;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table, strLabel.c_str(),
                                 true,bDesign,bPermit,bPedLoading,bRating,false,includeImpact,intervalIdx,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   // Compute start column for pedestrian-combined columns if needed
   ColumnIndexType startPedCol(INVALID_INDEX);
   if (bPedLoading)
   {
      ColumnIndexType nc = p_table->GetNumberOfColumns();
      startPedCol = 3 + (nc-3)/2; // first three rows loc, pedmn, pedmx
   }

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);


   // Fill up the table
   RowIndexType    row = Nhrows;
   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      PierIndexType pier = reactionLocation.PierIdx;
      CGirderKey girderKey(reactionLocation.GirderKey);

     (*p_table)(row,0) << reactionLocation.PierLabel;

      ColumnIndexType col = 1;
      ColumnIndexType pedCol = startPedCol;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, intervalIdx, reactionLocation, minBAT, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            // Design
            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );


            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            // Fatigue
            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(PermitPedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }
         }

         if ( bRating )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, intervalIdx, reactionLocation, minBAT, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         if ( bDesign )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, intervalIdx, reactionLocation, maxBAT, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltFatigue, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermit, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(PermitPedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }
         }

         if ( bRating )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPedestrian, intervalIdx, reactionLocation, maxBAT, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltDesign, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Routine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltLegalRating_Special, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Routine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( pgsTypes::lltPermitRating_Special, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }

      row++;
   }

   p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << p;
   *p << (includeImpact ? LIVELOAD_PER_GIRDER : LIVELOAD_PER_GIRDER_NO_IMPACT) << rptNewLine;

   if (bDesign && bPedLoading)
   {
      // footnotes for pedestrian loads
      int lnum=1;
      *p<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         *p << lnum++ << PedestrianFootnote(FatiguePedLoad) << rptNewLine;
      }

      if ( bPermit )
      {
         *p << lnum++ << PedestrianFootnote(PermitPedLoad) << rptNewLine;
      }
   }

   if ( bRating && pRatingSpec->IncludePedestrianLiveLoad())
   {
      // Note for rating and pedestrian load
      *p << _T("$ Pedestrian load results will be summed with vehicular load results at time of load combination.");
   }
}


void CCombinedReactionTable::BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey, bool includeImpact,
                                         IEAFDisplayUnits* pDisplayUnits,IntervalIndexType intervalIdx,
                                         pgsTypes::AnalysisType analysisType, ReactionTableType tableType,
                                         bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,ICombinedForces,pCmbForces);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker,IEventMap,pEventMap);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pCmbForces,pLsForces,girderKey));
   }

   else
   {
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, intervalIdx, girderKey) );
   }


   bool bPermit = pLsForces->IsStrengthIIApplicable(girderKey);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   std::_tstring strLabel (tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions"));
   strLabel += std::_tstring(includeImpact ? _T(" (Including Impact)") : _T(" (Without Impact)"));

   rptRcTable * p_table;
   RowIndexType row = CreateLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table, strLabel.c_str(),
                             true,bDesign,bPermit,bRating,false,analysisType,pEventMap,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;


   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   Float64 min, max;
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;
      const ReactionLocation& reactionLocation( iter.CurrentItem() );

     (*p_table)(row,col++) << reactionLocation.PierLabel;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            pForces->GetReaction( pgsTypes::ServiceI, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceI, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( pgsTypes::ServiceIA, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceIA, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::ServiceIII, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::ServiceIII, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims  <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( pgsTypes::FatigueI, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::FatigueI, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( pgsTypes::StrengthI, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( pgsTypes::StrengthI, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( pgsTypes::StrengthII, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Inventory, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_Inventory, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Operating, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_Operating, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, intervalIdx, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         if ( bDesign )
         {
            pForces->GetReaction( pgsTypes::ServiceI, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( pgsTypes::ServiceIA, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pForces->GetReaction( pgsTypes::ServiceIII, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( pgsTypes::FatigueI, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pForces->GetReaction( pgsTypes::StrengthI, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( pgsTypes::StrengthII, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Inventory, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_Operating, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalRoutine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( pgsTypes::StrengthI_LegalSpecial, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitRoutine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitRoutine, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( pgsTypes::ServiceI_PermitSpecial, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( pgsTypes::StrengthII_PermitSpecial, intervalIdx, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }

      row++;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCombinedReactionTable::MakeCopy(const CCombinedReactionTable& rOther)
{
   // Add copy code here...
}

void CCombinedReactionTable::MakeAssignment(const CCombinedReactionTable& rOther)
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
bool CCombinedReactionTable::AssertValid() const
{
   return true;
}

void CCombinedReactionTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedReactionTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCombinedReactionTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCombinedReactionTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCombinedReactionTable");

   TESTME_EPILOG("CCombinedReactionTable");
}
#endif // _UNITTEST
