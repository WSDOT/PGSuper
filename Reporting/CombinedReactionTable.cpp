///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
      ATLASSERT(false);
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
      {
         BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, tableType, true, true, false);
      }

      if (bRating)
      {
         BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, tableType, true, false, true);
      }

      if (bDesign)
      {
         BuildLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, tableType, true, false);
      }

      if (bRating)
      {
         BuildLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, tableType, false, true);
      }
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
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   GET_IFACE2_NOCHECK(pBroker,ILimitStateForces,pLsForces); // only used for certain reaction table types and intervals after live load is applied

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      GET_IFACE2(pBroker,IReactions,pReactions);
      pForces = std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pReactions,pLsForces,girderKey));
   }
   else
   {
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
      pForces = std::auto_ptr<ICmbLsReactionAdapter>(new CmbLsBearingDesignReactionAdapter(pBearingDesign, intervalIdx, girderKey) );
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table=0;

   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,pBroker,iter.CurrentItem().GirderKey,(tableType==PierReactionsTable ? _T("Total Girderline Reactions at Abutments and Piers"): _T("Girder Bearing Reactions")),
                                 true ,bRating,intervalIdx,
                                 analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());

   *p << p_table;

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // Fill up the table
   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

     (*p_table)(row,0) << reactionLocation.PierLabel;

      ColumnIndexType col = 1;
      if ( analysisType == pgsTypes::Envelope /*&& continuityIntervalIdx == castDeckIntervalIdx*/ )
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDC, reactionLocation, maxBAT, rtIncremental ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDC, reactionLocation, minBAT, rtIncremental ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDW, reactionLocation, maxBAT, rtIncremental ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDW, reactionLocation, minBAT, rtIncremental ) );

         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDWRating, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDWRating, reactionLocation, minBAT, rtIncremental ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcCR, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcCR, reactionLocation, minBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcSH, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcSH, reactionLocation, minBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcRE, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcRE, reactionLocation, minBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcPS, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcPS, reactionLocation, minBAT, rtIncremental ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDC, reactionLocation, maxBAT, rtCumulative ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDC, reactionLocation, minBAT, rtCumulative ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDW, reactionLocation, maxBAT, rtCumulative ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDW, reactionLocation, minBAT, rtCumulative ) );

         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDWRating, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDWRating, reactionLocation, minBAT, rtCumulative ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcCR, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcCR, reactionLocation, minBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcSH, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcSH, reactionLocation, minBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcRE, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcRE, reactionLocation, minBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcPS, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcPS, reactionLocation, minBAT, rtCumulative ) );
         }

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces->GetReaction( intervalIdx, pgsTypes::ServiceI, reactionLocation.PierIdx, reactionLocation.GirderKey, maxBAT, true, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pLsForces->GetReaction( intervalIdx, pgsTypes::ServiceI, reactionLocation.PierIdx, reactionLocation.GirderKey, minBAT,true,  &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );
         }
      }
      else
      {
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDC, reactionLocation, maxBAT, rtIncremental ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDW, reactionLocation, maxBAT, rtIncremental ) );
         
         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDWRating, reactionLocation, maxBAT, rtIncremental ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcCR, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcSH, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcRE, reactionLocation, maxBAT, rtIncremental ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcPS, reactionLocation, maxBAT, rtIncremental ) );
         }

         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDC, reactionLocation, maxBAT, rtCumulative ) );
         (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDW, reactionLocation, maxBAT, rtCumulative ) );

         if ( bRating )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcDWRating, reactionLocation, maxBAT, rtCumulative ) );
         }

         if ( bTimeStepMethod )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcCR, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcSH, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcRE, reactionLocation, maxBAT, rtCumulative ) );
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( intervalIdx, lcPS, reactionLocation, maxBAT, rtCumulative ) );
         }

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces->GetReaction( intervalIdx, pgsTypes::ServiceI, reactionLocation.PierIdx, reactionLocation.GirderKey, maxBAT, true, &min, &max );
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

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(), false );


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType == PierReactionsTable )
   {
      GET_IFACE2(pBroker,IReactions,pReactions);
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pReactions,pLsForces,girderKey));
   }
   else
   {
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
      IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();
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
                                 true,bDesign,bPermit,bPedLoading,bRating,false,includeImpact,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
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

      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

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
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, minBAT, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            // Design
            pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );


            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            // Fatigue
            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltFatigue, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltFatigue, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  
                                         row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPermit, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPermit, reactionLocation, minBAT, includeImpact, &min, &max );
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
            IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();

            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, includeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, minBAT, includeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltDesign, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, minBAT, includeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltFatigue, reactionLocation, maxBAT, includeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPermit, reactionLocation, maxBAT, includeImpact, &min, &max );
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
            IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();

            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, includeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, maxBAT, includeImpact, &min, &max );
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

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   std::auto_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      GET_IFACE2(pBroker,IReactions,pReactions);
      pForces =  std::auto_ptr<ICmbLsReactionAdapter>(new CombinedLsForcesReactionAdapter(pReactions,pLsForces,girderKey));
   }
   else
   {
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
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
                             true,bDesign,bPermit,bRating,false,analysisType,pProductLoads,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
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
            pForces->GetReaction( intervalIdx, pgsTypes::ServiceI, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( intervalIdx, pgsTypes::ServiceI, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::ServiceIA, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::ServiceIA, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( intervalIdx, pgsTypes::ServiceIII, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( intervalIdx, pgsTypes::ServiceIII, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims  <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::FatigueI, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::FatigueI, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pForces->GetReaction( intervalIdx, pgsTypes::StrengthI, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetReaction( intervalIdx, pgsTypes::StrengthI, reactionLocation, minBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_Inventory, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_Inventory, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_Operating, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_Operating, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_LegalRoutine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_LegalRoutine, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_LegalSpecial, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_LegalSpecial, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::ServiceI_PermitRoutine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::ServiceI_PermitRoutine, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII_PermitRoutine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII_PermitRoutine, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::ServiceI_PermitSpecial, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::ServiceI_PermitSpecial, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII_PermitSpecial, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII_PermitSpecial, reactionLocation, minBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         if ( bDesign )
         {
            pForces->GetReaction( intervalIdx, pgsTypes::ServiceI, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::ServiceIA, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pForces->GetReaction( intervalIdx, pgsTypes::ServiceIII, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::FatigueI, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pForces->GetReaction( intervalIdx, pgsTypes::StrengthI, reactionLocation, maxBAT, includeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_Inventory, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_Operating, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_LegalRoutine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::StrengthI_LegalSpecial, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::ServiceI_PermitRoutine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII_PermitRoutine, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetReaction( intervalIdx, pgsTypes::ServiceI_PermitSpecial, reactionLocation, maxBAT, includeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pForces->GetReaction( intervalIdx, pgsTypes::StrengthII_PermitSpecial, reactionLocation, maxBAT, includeImpact, &min, &max );
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
