///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
                              rptRcTable* pTable, ReactionUnitValueTool& reaction,
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

   if ( liveLoadIntervalIdx <= intervalIdx && BearingReactionsTable == tableType)
   {
      if (bDesign)
      {
         BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, true, true, false);
      }

      if (bRating)
      {
         BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, true, false, true);
      }

      if (bDesign)
      {
         BuildBearingLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, true, false);
      }

      if (bRating)
      {
         BuildBearingLimitStateTable(pBroker, pChapter, girderKey, true, pDisplayUnits, intervalIdx, analysisType, false, true);
      }
   }
}

void CCombinedReactionTable::BuildForBearingDesign(IBroker* pBroker, rptChapter* pChapter,
                                          const CGirderKey& girderKey, 
                                          IEAFDisplayUnits* pDisplayUnits,
                                          IntervalIndexType intervalIdx, pgsTypes::AnalysisType analysisType,bool bIncludeImpact) const
{
   ReactionTableType tableType = BearingReactionsTable;

   BuildCombinedDeadTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, tableType, true, false);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if (liveLoadIntervalIdx <= intervalIdx)
   {
      BuildLiveLoad(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, bIncludeImpact, true, false);
      BuildBearingLimitStateTable(pBroker, pChapter, girderKey, bIncludeImpact, pDisplayUnits, intervalIdx, analysisType, true, false);
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
   INIT_UV_PROTOTYPE( rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false );

   // Tricky: the reaction tool below will dump out two lines per cell for bearing reactions with more than one bearing
   ReactionUnitValueTool reaction(tableType, reactu);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2_NOCHECK(pBroker,IBearingDesign,pBearingDesign); // only used for certain reaction table types and intervals after live load is applied
   GET_IFACE2_NOCHECK(pBroker,ILimitStateForces,pLsForces); 

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   // TRICKY:
   // Use the adapter class to get the reaction response functions we need and to iterate piers
   std::unique_ptr<ICmbLsReactionAdapter> pForces;
   if(  tableType==PierReactionsTable )
   {
      GET_IFACE2(pBroker,IReactions,pReactions);
      pForces = std::make_unique<CombinedLsForcesReactionAdapter>(pReactions,pLsForces,girderKey);
   }
   else
   {
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
      pForces = std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, intervalIdx, girderKey);
   }

   // Use iterator to walk locations
   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   bool bDoLimitStates = false; // liveLoadIntervalIdx <= intervalIdx && tableType == BearingReactionsTable;
   rptRcTable* p_table = nullptr;

   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptForceUnitTag,WBFL::Units::ForceData>(&p_table,pBroker,(tableType==PierReactionsTable ? _T("Girder Line Pier Reactions"): _T("Girder Bearing Reactions")),
                                 true ,bRating,bDoLimitStates,
                                 analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());

   *p << p_table;

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // Fill up the table
   Float64 min, max;
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

      const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face==rftBack? pgsTypes::Back : pgsTypes::Ahead), girderKey.girderIndex);
      IndexType numBearings = pbd->BearingCount;

      reaction.SetNumBearings(numBearings); // class will dump per-bearing reaction if applicable:

      ColumnIndexType col = 0;
      (*p_table)(row,col) << reactionLocation.PierLabel;
      if (tableType == BearingReactionsTable && numBearings > 1) // add second line for per-bearing value
      {
         (*p_table)(row, col) << _T(" - Total") << rptNewLine << _T("Per Bearing");
      }
      col++;

      if ( analysisType == pgsTypes::Envelope )
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

         if ( bDoLimitStates )
         {
            pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::ServiceI, maxBAT, true, &min, &max);
            (*p_table)(row,col++) << reaction.SetValue( max );

            pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::ServiceI, minBAT, true, &min, &max);
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

         if ( bDoLimitStates )
         {
            pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::ServiceI, maxBAT, true, &min, &max);
            (*p_table)(row,col++) << reaction.SetValue( max );
         }
      }

      row++;
   }
}

void CCombinedReactionTable::BuildLiveLoad(IBroker* pBroker, rptChapter* pChapter,
                                          const CGirderKey& girderKey, 
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType, 
                                         bool bIncludeImpact, bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false );

   // Tricky: the reaction tool below will dump out two lines per cell for bearing reactions with more than one bearing
   ReactionUnitValueTool reaction(BearingReactionsTable, reactu);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);

   // TRICKY: Use the adapter class to get the reaction response functions we need and to iterate piers
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   IntervalIndexType intervalIdx = pIntervals->GetLiveLoadInterval();
   std::unique_ptr<ICmbLsReactionAdapter> pForces =  std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, intervalIdx, girderKey);

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

   std::_tstring strLabel(_T("Girder Bearing Reactions"));
   strLabel += std::_tstring(bDesign ? _T(" - Design Vehicles") : _T(" - Rating Vehicles"));
   strLabel += std::_tstring(bIncludeImpact ? _T(" (With Impact)") : _T(" (Without Impact)"));

   rptRcTable* p_table=0;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptForceUnitTag,WBFL::Units::ForceData>(&p_table, strLabel.c_str(),
                                 true,bDesign,bPermit,bPedLoading,bRating,false, bIncludeImpact,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

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

      const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face==rftBack? pgsTypes::Back : pgsTypes::Ahead), girderKey.girderIndex);
      IndexType nBearings = pbd->BearingCount;

      reaction.SetNumBearings(nBearings); // class will dump per-bearing reaction if applicable:

      ColumnIndexType col = 0;
      (*p_table)(row,col) << reactionLocation.PierLabel;
      if (1 < nBearings) // add second line for per-bearing value
      {
         (*p_table)(row, col) << _T(" - Total") << rptNewLine << _T("Per Bearing");
      }
      col++;

      ColumnIndexType pedCol = startPedCol;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, bIncludeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, minBAT, bIncludeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            // Design
            pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, minBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );


            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  row, pedCol, min, max, pedMin, pedMax);
            }

            // Fatigue
            if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltFatigue, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltFatigue, reactionLocation, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(FatiguePedLoad, p_table, reaction,  row, pedCol, min, max, pedMin, pedMax);
               }
            }

            if ( bPermit )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPermit, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPermit, reactionLocation, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               if (bPedLoading)
               {
                  CombineReportPedResult(PermitPedLoad, p_table, reaction,  row, pedCol, min, max, pedMin, pedMax);
               }
            }
         }

         if ( bRating )
         {
            IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();

            Float64 pedMin(0), pedMax(0);
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, bIncludeImpact, &min, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, minBAT, bIncludeImpact, &pedMin, &max );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltDesign, reactionLocation, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               pForces->GetCombinedLiveLoadReaction(ratingIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);

               pForces->GetCombinedLiveLoadReaction(ratingIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, minBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               pForces->GetCombinedLiveLoadReaction(ratingIntervalIdx, pgsTypes::lltLegalRating_Emergency, reactionLocation, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);

               pForces->GetCombinedLiveLoadReaction(ratingIntervalIdx, pgsTypes::lltLegalRating_Emergency, reactionLocation, minBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, minBAT, bIncludeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, bIncludeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if (bPedLoading)
            {
               CombineReportPedResult(DesignPedLoad, p_table, reaction,  
                                      row, pedCol, min, max, pedMin, pedMax);
            }

            if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
            {
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltFatigue, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( liveLoadIntervalIdx, pgsTypes::lltPermit, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
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
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, maxBAT, bIncludeImpact, &pedMin, &pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMax );
               (*p_table)(row,col++) << reaction.SetValue( pedMin );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltDesign, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               pForces->GetCombinedLiveLoadReaction(ratingIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               pForces->GetCombinedLiveLoadReaction(ratingIntervalIdx, pgsTypes::lltLegalRating_Emergency, reactionLocation, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces->GetCombinedLiveLoadReaction( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }

      row++;
   }

   p = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << p;
   *p << (bIncludeImpact ? LIVELOAD_PER_GIRDER : LIVELOAD_PER_GIRDER_NO_IMPACT) << rptNewLine;

   if (bDesign && bPedLoading)
   {
      // footnotes for pedestrian loads
      int lnum=1;
      *p<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

      if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
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

void CCombinedReactionTable::BuildBearingLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey, bool bIncludeImpact,
                                         IEAFDisplayUnits* pDisplayUnits,IntervalIndexType intervalIdx,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false );

   // Tricky: the reaction tool below will dump out two lines per cell for bearing reactions with more than one bearing
   ReactionUnitValueTool reaction(BearingReactionsTable, reactu);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILimitStateForces,pLsForces);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   bool bPermit = pLsForces->IsStrengthIIApplicable(girderKey);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   std::_tstring strLabel(_T("Girder Bearing Reactions"));
   strLabel += std::_tstring(bIncludeImpact ? _T(" (With Impact)") : _T(" (Without Impact)"));

   rptRcTable * p_table;
   RowIndexType row = CreateLimitStateTableHeading<rptForceUnitTag,WBFL::Units::ForceData>(&p_table, strLabel.c_str(),
                             true,bDesign,bPermit,bRating,false,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   Float64 min, max;
   // use adapter's static function to get locations
   ReactionLocationContainer Locations = CmbLsBearingDesignReactionAdapter::GetBearingReactionLocations(intervalIdx, girderKey, pBridge, pBearingDesign);
   ReactionLocationIter iter(Locations);
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face==rftBack? pgsTypes::Back : pgsTypes::Ahead), girderKey.girderIndex);
      IndexType nBearings = pbd->BearingCount;

      reaction.SetNumBearings(nBearings); // class will dump per-bearing reaction if applicable:

      ColumnIndexType col = 0;
      (*p_table)(row,col) << reactionLocation.PierLabel;
      if (1 < nBearings) // add second line for per-bearing value
      {
         (*p_table)(row, col) << _T(" - Total") << rptNewLine << _T("Per Bearing");
      }
      col++;

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bDesign )
         {
            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI, minBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( WBFL::LRFD::LRFDVersionMgr::GetVersion() < WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceIA, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceIA, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceIII, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceIII, minBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims  <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::FatigueI, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::FatigueI, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI, minBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_Inventory, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_Inventory, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_Operating, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_Operating, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalRoutine, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalRoutine, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalSpecial, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);

               pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalSpecial, minBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalEmergency, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);

               pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalEmergency, minBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI_PermitRoutine, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI_PermitRoutine, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII_PermitRoutine, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII_PermitRoutine, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI_PermitSpecial, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI_PermitSpecial, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII_PermitSpecial, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII_PermitSpecial, minBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }
      }
      else
      {
         if ( bDesign )
         {
            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( WBFL::LRFD::LRFDVersionMgr::GetVersion() < WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceIA, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceIII, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );

            if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::FatigueI, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
            }

            pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI, maxBAT, bIncludeImpact, &min, &max );
            (*p_table)(row,col++) << reaction.SetValue( max );
            (*p_table)(row,col++) << reaction.SetValue( min );

            if ( bPermit )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
         }

         if ( bRating )
         {
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_Inventory, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_Operating, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalRoutine, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalSpecial, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               pBearingDesign->GetBearingLimitStateReaction(intervalIdx, reactionLocation, pgsTypes::StrengthI_LegalEmergency, maxBAT, bIncludeImpact, &min, &max);
               (*p_table)(row, col++) << reaction.SetValue(max);
               (*p_table)(row, col++) << reaction.SetValue(min);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI_PermitRoutine, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII_PermitRoutine, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::ServiceI_PermitSpecial, maxBAT, bIncludeImpact, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );

               pBearingDesign->GetBearingLimitStateReaction( intervalIdx, reactionLocation, pgsTypes::StrengthII_PermitSpecial, maxBAT, bIncludeImpact, &min, &max );
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
