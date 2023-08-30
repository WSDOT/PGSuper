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
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductMomentsTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#include <PgsExt\PierData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductReactionTable::CProductReactionTable()
{
}

CProductReactionTable::CProductReactionTable(const CProductReactionTable& rOther)
{
   MakeCopy(rOther);
}

CProductReactionTable::~CProductReactionTable()
{
}

//======================== OPERATORS  =======================================
CProductReactionTable& CProductReactionTable::operator= (const CProductReactionTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductReactionTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                         ReactionTableType tableType, bool bIncludeImpact,bool bDesign,bool bRating,bool bIndicateControllingLoad,
                                         IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, reactu, pDisplayUnits->GetShearUnit(), false );

   // Tricky: the reaction tool below will dump out two lines per cell for bearing reactions with more than one bearing
   ReactionUnitValueTool reaction(tableType, reactu);

   GET_IFACE2_NOCHECK(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();
   PierIndexType nPiers = pBridge->GetPierCount();

   bool bIncludeLLDF = false; // this table never distributes live load

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType diaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType lastCastDeckIntervalIdx = pIntervals->GetLastCastDeckInterval(); // deck cast be cast in multiple stages, use interval after entire deck is cast
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType ljIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType shearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType constructionIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   bool bSegments, bConstruction, bDeck, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bLongitudinalJoint, bPermit;
   bool bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeck,&bDeckPanels,&bSidewalk,&bShearKey,&bLongitudinalJoint,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);
   
   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,
                         tableType==PierReactionsTable ?_T("Girder Line Pier Reactions"): _T("Girder Bearing Reactions") );
   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,WBFL::Units::ForceData>(pBroker,p_table,true,false,bSegments,bConstruction,bDeck,bDeckPanels,bSidewalk,bShearKey,bLongitudinalJoint,bHasOverlay,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,bContinuousBeforeDeckCasting,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   pgsTypes::BridgeAnalysisType batSS = pgsTypes::SimpleSpan;
   pgsTypes::BridgeAnalysisType batCS = pgsTypes::ContinuousSpan;

   // TRICKY: use adapter class to get correct reaction interfaces
   std::unique_ptr<IProductReactionAdapter> pForces;
   if( tableType == PierReactionsTable )
   {
      GET_IFACE2(pBroker,IReactions,pReactions);
      pForces =  std::make_unique<ProductForcesReactionAdapter>(pReactions,girderKey);
   }
   else
   {
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
      pForces =  std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, diaphragmIntervalIdx, girderKey);
   }

   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& reactionLocation( iter.CurrentItem() );

      const CGirderKey& thisGirderKey(reactionLocation.GirderKey);
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);

      const CBearingData2* pbd = pIBridgeDesc->GetBearingData(reactionLocation.PierIdx, (reactionLocation.Face==rftBack? pgsTypes::Back : pgsTypes::Ahead), girderKey.girderIndex);
      IndexType numBearings = pbd->BearingCount;

      reaction.SetNumBearings(numBearings); // class will dump per-bearing reaction if applicable:

     (*p_table)(row,col) << reactionLocation.PierLabel;
     if (tableType == BearingReactionsTable && numBearings > 1) // add second line for per-bearing value
     {
        (*p_table)(row, col) << _T(" - Total") << rptNewLine << _T("Per Bearing");
     }

     col++;

     ReactionDecider reactionDecider(tableType,reactionLocation,thisGirderKey,pBridge,pIntervals);
   
     if ( bSegments )
     {
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( erectSegmentIntervalIdx, reactionLocation, pgsTypes::pftGirder,    analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction( lastIntervalIdx,         reactionLocation, pgsTypes::pftGirder,    analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
     }
     else
     {
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(erectSegmentIntervalIdx, reactionLocation, pgsTypes::pftGirder,    analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
     }

     if ( reactionDecider.DoReport(diaphragmIntervalIdx) )
     {
        (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(diaphragmIntervalIdx,     reactionLocation, pgsTypes::pftDiaphragm, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
     }
     else
     {
        (*p_table)(row,col++) << RPT_NA;
     }

     if (bShearKey)
     {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
           if (reactionDecider.DoReport(shearKeyIntervalIdx))
           {
              (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(shearKeyIntervalIdx, reactionLocation, pgsTypes::pftShearKey, maxBAT));
              (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(shearKeyIntervalIdx, reactionLocation, pgsTypes::pftShearKey, minBAT));
           }
           else
           {
              (*p_table)(row, col++) << RPT_NA;
              (*p_table)(row, col++) << RPT_NA;
           }
        }
        else
        {
           if (reactionDecider.DoReport(shearKeyIntervalIdx))
           {
              (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(shearKeyIntervalIdx, reactionLocation, pgsTypes::pftShearKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
           }
           else
           {
              (*p_table)(row, col++) << RPT_NA;
           }
        }
     }

     if (bLongitudinalJoint)
     {
        if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
        {
           if (reactionDecider.DoReport(ljIntervalIdx))
           {
              (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(ljIntervalIdx, reactionLocation, pgsTypes::pftLongitudinalJoint, maxBAT));
              (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(ljIntervalIdx, reactionLocation, pgsTypes::pftLongitudinalJoint, minBAT));
           }
           else
           {
              (*p_table)(row, col++) << RPT_NA;
              (*p_table)(row, col++) << RPT_NA;
           }
        }
        else
        {
           if (reactionDecider.DoReport(ljIntervalIdx))
           {
              (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(ljIntervalIdx, reactionLocation, pgsTypes::pftLongitudinalJoint, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
           }
           else
           {
              (*p_table)(row, col++) << RPT_NA;
           }
        }
     }

      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
         {
            if ( reactionDecider.DoReport(constructionIntervalIdx) )
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(constructionIntervalIdx, reactionLocation, pgsTypes::pftConstruction,   maxBAT ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(constructionIntervalIdx, reactionLocation, pgsTypes::pftConstruction, minBAT ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if ( reactionDecider.DoReport(constructionIntervalIdx) )
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(constructionIntervalIdx, reactionLocation, pgsTypes::pftConstruction, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if (bDeck)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            if (reactionDecider.DoReport(lastCastDeckIntervalIdx))
            {
               (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlab, maxBAT));
               (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlab, minBAT));

               (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, maxBAT));
               (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, minBAT));
            }
            else
            {
               (*p_table)(row, col++) << RPT_NA;
               (*p_table)(row, col++) << RPT_NA;
               (*p_table)(row, col++) << RPT_NA;
               (*p_table)(row, col++) << RPT_NA;
            }
         }
         else
         {
            if (reactionDecider.DoReport(lastCastDeckIntervalIdx))
            {
               (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlab, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
               (*p_table)(row, col++) << reaction.SetValue(pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPad, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
            }
            else
            {
               (*p_table)(row, col++) << RPT_NA;
               (*p_table)(row, col++) << RPT_NA;
            }
         }
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
         {
            if ( reactionDecider.DoReport(lastCastDeckIntervalIdx) )
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, maxBAT ) );
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, minBAT ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if ( reactionDecider.DoReport(lastCastDeckIntervalIdx) )
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lastCastDeckIntervalIdx, reactionLocation, pgsTypes::pftSlabPanel, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            if ( reactionDecider.DoReport(railingSystemIntervalIdx) )
            {
               Float64 R1 = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftSidewalk, batSS);
               Float64 R2 = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftSidewalk, batCS);
               (*p_table)(row,col++) << reaction.SetValue( Max(R1,R2) );
               (*p_table)(row,col++) << reaction.SetValue( Min(R1,R2) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( reactionDecider.DoReport(railingSystemIntervalIdx) )
         {
            Float64 R1 = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftTrafficBarrier, batSS);
            Float64 R2 = pForces->GetReaction(railingSystemIntervalIdx, reactionLocation, pgsTypes::pftTrafficBarrier, batCS);
            (*p_table)(row,col++) << reaction.SetValue( Max(R1,R2) );
            (*p_table)(row,col++) << reaction.SetValue( Min(R1,R2) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if ( bHasOverlay )
         {
            if ( reactionDecider.DoReport(overlayIntervalIdx) )
            {
               Float64 R1 = pForces->GetReaction(overlayIntervalIdx, reactionLocation, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, batSS);
               Float64 R2 = pForces->GetReaction(overlayIntervalIdx, reactionLocation, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, batCS);
               (*p_table)(row,col++) << reaction.SetValue( Max(R1,R2) );
               (*p_table)(row,col++) << reaction.SetValue( Min(R1,R2) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }


         if ( bPedLoading )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               Float64 R1min, R1max, R2min, R2max;
               pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, batSS, bIncludeImpact, true, &R1min, &R2max);
               pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, batCS, bIncludeImpact, true, &R2min, &R2max);
               (*p_table)(row,col++) << reaction.SetValue( Max(R1max,R2max) );
               (*p_table)(row,col++) << reaction.SetValue( Min(R1min,R2min) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( bDesign )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               Float64 R1min, R1max, R2min, R2max;
               VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;
               pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
               pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

               Float64 max = Max(R1max, R2max);
               VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

               Float64 min = Min(R1min, R2min);
               VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

               (*p_table)(row,col) << reaction.SetValue( max );

               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );

               if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << _T(")");
               }

               col++;
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }

            if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltFatigue, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltFatigue, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            if ( bPermit )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermit, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermit, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine <<  _T("(") << LiveLoadPrefix(pgsTypes::lltDesign)<< minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal - Special
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row, col) << reaction.SetValue(max);

                  if (bIndicateControllingLoad && maxConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row, col) << reaction.SetValue(min);

                  if (bIndicateControllingLoad && minConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row, col++) << RPT_NA;
                  (*p_table)(row, col++) << RPT_NA;
               }
            }

            // Legal - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row, col) << reaction.SetValue(max);

                  if (bIndicateControllingLoad && maxConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row, col) << reaction.SetValue(min);

                  if (bIndicateControllingLoad && minConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row, col++) << RPT_NA;
                  (*p_table)(row, col++) << RPT_NA;
               }
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  Float64 R1min, R1max, R2min, R2max;
                  VehicleIndexType minConfig1, maxConfig1, minConfig2, maxConfig2;

                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, batSS, bIncludeImpact, bIncludeLLDF, &R1min, &R1max, &minConfig1, &maxConfig1);
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, batCS, bIncludeImpact, bIncludeLLDF, &R2min, &R2max, &minConfig2, &maxConfig2);

                  Float64 max = Max(R1max, R2max);
                  VehicleIndexType maxConfig = MaxIndex(R1max, R2max) == 0 ? maxConfig1 : maxConfig2;

                  Float64 min = Min(R1min, R2min);
                  VehicleIndexType minConfig = MinIndex(R1min, R2min) == 0 ? minConfig1 : minConfig2;

                  (*p_table)(row,col) << reaction.SetValue( max );

                  if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );

                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }
         }
      }
      else
      {
         if ( bSidewalk )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftSidewalk, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( reactionDecider.DoReport(lastIntervalIdx) )
         {
            (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lastIntervalIdx, reactionLocation, pgsTypes::pftTrafficBarrier, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
         }

         if ( bHasOverlay )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << reaction.SetValue( pForces->GetReaction(lastIntervalIdx, reactionLocation, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bPedLoading )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPedestrian, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << reaction.SetValue( max );
               (*p_table)(row,col++) << reaction.SetValue( min );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( bDesign )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << reaction.SetValue( max );
               if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX)
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }

               col++;

               (*p_table)(row,col) << reaction.SetValue( min );
               if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
               }

               col++;
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }

            if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltFatigue, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            if ( bPermit )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermit, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig!=INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))  )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltDesign, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }

                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minConfig << _T(")");
                  }

                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Legal - Special
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Special, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << reaction.SetValue(max);
                  if (bIndicateControllingLoad && maxConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row, col) << reaction.SetValue(min);
                  if (bIndicateControllingLoad && minConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row, col++) << RPT_NA;
                  (*p_table)(row, col++) << RPT_NA;
               }
            }

            // Legal - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << reaction.SetValue(max);
                  if (bIndicateControllingLoad && maxConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row, col) << reaction.SetValue(min);
                  if (bIndicateControllingLoad && minConfig != INVALID_INDEX)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row, col++) << RPT_NA;
                  (*p_table)(row, col++) << RPT_NA;
               }
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pForces->GetLiveLoadReaction(lastIntervalIdx, pgsTypes::lltPermitRating_Special, reactionLocation, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << reaction.SetValue( max );
                  if ( bIndicateControllingLoad && maxConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << reaction.SetValue( min );
                  if ( bIndicateControllingLoad && minConfig != INVALID_INDEX )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special)<< minConfig << _T(")");
                  }
                  col++;
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }
         }
      }

      row++;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CProductReactionTable::MakeCopy(const CProductReactionTable& rOther)
{
   // Add copy code here...
}

void CProductReactionTable::MakeAssignment(const CProductReactionTable& rOther)
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
