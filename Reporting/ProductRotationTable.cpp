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
#include <Reporting\ProductRotationTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

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
   CProductRotationTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductRotationTable::CProductRotationTable()
{
}

CProductRotationTable::CProductRotationTable(const CProductRotationTable& rOther)
{
   MakeCopy(rOther);
}

CProductRotationTable::~CProductRotationTable()
{
}

//======================== OPERATORS  =======================================
CProductRotationTable& CProductRotationTable::operator= (const CProductRotationTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductRotationTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                         bool bIncludeImpact, bool bIncludeLLDF,bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();
   PierIndexType nPiers = pBridge->GetPierCount();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   bool bSegments, bConstruction, bDeck, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bLongitudinalJoint, bPermit;
   bool bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeck,&bDeckPanels,&bSidewalk,&bShearKey,&bLongitudinalJoint,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Rotations"));
   RowIndexType row = ConfigureProductLoadTableHeading<rptAngleUnitTag,WBFL::Units::AngleData>(pBroker,p_table,true,false,bSegments,bConstruction,bDeck,bDeckPanels,bSidewalk,bShearKey,bLongitudinalJoint,bHasOverlay,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,bContinuousBeforeDeckCasting,pRatingSpec,pDisplayUnits,pDisplayUnits->GetRadAngleUnit());

   p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   // get poi where pier rotations occur
   PoiList vPoi;
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey.girderIndex, startGroup, endGroup, &vGirderKeys);
   for (const auto& thisGirderKey : vGirderKeys)
   {
      PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(thisGirderKey.groupIndex);
      PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(thisGirderKey.groupIndex);
      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
      {
         if ( pierIdx == startPierIdx )
         {
            CSegmentKey segmentKey(thisGirderKey,0);
            PoiList segPoi;
            pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &segPoi);
            vPoi.push_back(segPoi.front());
         }
         else if ( pierIdx == endPierIdx )
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
            CSegmentKey segmentKey(thisGirderKey,nSegments-1);
            PoiList segPoi;
            pPOI->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &segPoi);
            vPoi.push_back(segPoi.front());
         }
         else
         {
            Float64 Xgp;
            VERIFY(pBridge->GetPierLocation(thisGirderKey,pierIdx,&Xgp));
            pgsPointOfInterest poi = pPOI->ConvertGirderPathCoordinateToPoi(thisGirderKey,Xgp);
            vPoi.push_back(poi);
         }
      }
   }

   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey) );

   // Fill up the table
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);
   iter.First();
   PierIndexType startPierIdx = (iter.IsDone() ? INVALID_INDEX : iter.CurrentItem().PierIdx);

   // Use iterator to walk locations
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;

      const ReactionLocation& reactionLocation( iter.CurrentItem() );
      const CGirderKey& thisGirderKey(reactionLocation.GirderKey);

      (*p_table)(row,col++) << reactionLocation.PierLabel;

      ReactionDecider reactionDecider(BearingReactionsTable,reactionLocation,thisGirderKey,pBridge,pIntervals);

      const pgsPointOfInterest& poi = vPoi[reactionLocation.PierIdx-startPierIdx];

      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(poi.GetSegmentKey());

      if ( bSegments )
      {
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false) );
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx,         pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false) );
      }
      else
      {
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, maxBAT, rtCumulative, false) );
      }

      if ( reactionDecider.DoReport(lastIntervalIdx) )
      {
         (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftDiaphragm, poi, maxBAT, rtCumulative, false) );
      }
      else
      {
         (*p_table)(row,col++) << RPT_NA;
      }

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, minBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftShearKey, poi, maxBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }
      }


      if (bLongitudinalJoint)
      {
         if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
         {
            if (reactionDecider.DoReport(lastIntervalIdx))
            {
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false));
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, minBAT, rtCumulative, false));
            }
            else
            {
               (*p_table)(row, col++) << RPT_NA;
               (*p_table)(row, col++) << RPT_NA;
            }
         }
         else
         {
            if (reactionDecider.DoReport(lastIntervalIdx))
            {
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, maxBAT, rtCumulative, false));
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
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction,   poi, maxBAT, rtCumulative, false ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction,   poi, minBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftConstruction,   poi, maxBAT, rtCumulative, false ) );
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
            if (reactionDecider.DoReport(lastIntervalIdx))
            {
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false));
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, minBAT, rtCumulative, false));

               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, maxBAT, rtCumulative, false));
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, minBAT, rtCumulative, false));
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
            if (reactionDecider.DoReport(lastIntervalIdx))
            {
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlab, poi, maxBAT, rtCumulative, false));
               (*p_table)(row, col++) << rotation.SetValue(pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPad, poi, maxBAT, rtCumulative, false));
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
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel,   poi, maxBAT, rtCumulative, false ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel,   poi, minBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }
         else
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSlabPanel,   poi, maxBAT, rtCumulative, false ) );
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
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, minBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( reactionDecider.DoReport(lastIntervalIdx) )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false ) );
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, minBAT, rtCumulative, false ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
            (*p_table)(row,col++) << RPT_NA;
         }

         if ( bHasOverlay )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false ) );
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, poi, minBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bDesign )
         {
            if ( bPedLoading )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, maxBAT, bIncludeImpact, true, &min, &max );
                  (*p_table)(row,col++) << rotation.SetValue( max );

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, minBAT, bIncludeImpact, true, &min, &max );
                  (*p_table)(row,col++) << rotation.SetValue( min );
               }
               else
               {
                  (*p_table)(row,col++) << RPT_NA;
                  (*p_table)(row,col++) << RPT_NA;
               }
            }

            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
               col++;

               pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltFatigue, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltFatigue, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermit, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermit, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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

            // Legal Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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

            // Legal Rating - Special
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Special, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << rotation.SetValue(max);
                  if (bIndicateControllingLoad && 0 <= maxConfig)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Special, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << rotation.SetValue(min);
                  if (bIndicateControllingLoad && 0 <= minConfig)
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

            // Legal Rating - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << rotation.SetValue(max);
                  if (bIndicateControllingLoad && 0 <= maxConfig)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << rotation.SetValue(min);
                  if (bIndicateControllingLoad && 0 <= minConfig)
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermitRating_Special, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermitRating_Special, poi, minBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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
         Float64 min, max;
         VehicleIndexType minConfig, maxConfig;
         if ( bSidewalk )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftSidewalk, poi, maxBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( reactionDecider.DoReport(lastIntervalIdx) )
         {
            (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, pgsTypes::pftTrafficBarrier, poi, maxBAT, rtCumulative, false ) );
         }
         else
         {
            (*p_table)(row,col++) << RPT_NA;
         }

         if ( bHasOverlay )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               (*p_table)(row,col++) << rotation.SetValue( pProductForces->GetRotation(lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, poi, maxBAT, rtCumulative, false ) );
            }
            else
            {
               (*p_table)(row,col++) << RPT_NA;
            }
         }

         if ( bPedLoading )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPedestrian, poi, maxBAT, bIncludeImpact, true, &min, &max );
               (*p_table)(row,col++) << rotation.SetValue( max );
               (*p_table)(row,col++) << rotation.SetValue( min );
            }
            else
            {
               (*p_table)(row, col++) << RPT_NA;
               (*p_table)(row, col++) << RPT_NA;
            }
         }

         if ( bDesign )
         {
            if ( reactionDecider.DoReport(lastIntervalIdx) )
            {
               pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
               (*p_table)(row,col) << rotation.SetValue( max );
               if ( bIndicateControllingLoad && 0 <= maxConfig )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
               }
               col++;

               (*p_table)(row,col) << rotation.SetValue( min );
               if ( bIndicateControllingLoad && 0 <= minConfig )
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltFatigue, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermit, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minConfig << _T(")");
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltDesign, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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

            // Legal Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               if ( reactionDecider.DoReport(lastIntervalIdx) )
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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

            // Legal Rating - Special
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Special, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << rotation.SetValue(max);
                  if (bIndicateControllingLoad && 0 <= maxConfig)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row, col) << rotation.SetValue(min);
                  if (bIndicateControllingLoad && 0 <= minConfig)
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

            // Legal Rating - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               if (reactionDecider.DoReport(lastIntervalIdx))
               {
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig);
                  (*p_table)(row, col) << rotation.SetValue(max);
                  if (bIndicateControllingLoad && 0 <= maxConfig)
                  {
                     (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row, col) << rotation.SetValue(min);
                  if (bIndicateControllingLoad && 0 <= minConfig)
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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
                  pProductForces->GetLiveLoadRotation(lastIntervalIdx, pgsTypes::lltPermitRating_Special, poi, maxBAT, bIncludeImpact, bIncludeLLDF, &min, &max, &minConfig, &maxConfig );
                  (*p_table)(row,col) << rotation.SetValue( max );
                  if ( bIndicateControllingLoad && 0 <= maxConfig )
                  {
                     (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxConfig << _T(")");
                  }
                  col++;

                  (*p_table)(row,col) << rotation.SetValue( min );
                  if ( bIndicateControllingLoad && 0 <= minConfig )
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
void CProductRotationTable::MakeCopy(const CProductRotationTable& rOther)
{
   // Add copy code here...
}

void CProductRotationTable::MakeAssignment(const CProductRotationTable& rOther)
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
