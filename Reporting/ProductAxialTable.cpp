///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\ProductAxialTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

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
   CProductAxialTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductAxialTable::CProductAxialTable()
{
}

CProductAxialTable::CProductAxialTable(const CProductAxialTable& rOther)
{
   MakeCopy(rOther);
}

CProductAxialTable::~CProductAxialTable()
{
}

//======================== OPERATORS  =======================================
CProductAxialTable& CProductAxialTable::operator= (const CProductAxialTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


//======================== OPERATIONS =======================================
rptRcTable* CProductAxialTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                        bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,     axial,   pDisplayUnits->GetGeneralForceUnit(),     false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   bool bSegments,bConstruction, bDeck, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bLongitudinalJoint, bPermit;
   bool bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeck,&bDeckPanels,&bSidewalk,&bShearKey,&bLongitudinalJoint,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Axial"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);
   PoiAttributeType poiRefAttribute(girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : POI_ERECTED_SEGMENT);

   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(pBroker,p_table,false,false,bSegments,bConstruction,bDeck,bDeckPanels,bSidewalk,bShearKey,bLongitudinalJoint,bHasOverlay,bFutureOverlay,bDesign,bPedLoading,
                                                                                           bPermit,bRating,analysisType,bContinuousBeforeDeckCasting,
                                                                                           pRatingSpec,pDisplayUnits,pDisplayUnits->GetGeneralForceUnit());
   // Get the results
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);

      CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(allSegmentsKey, poiRefAttribute, &vPoi);

      std::vector<Float64> segment;
      std::vector<Float64> girder;
      if ( bSegments )
      {
         segment = pForces2->GetAxial(erectSegmentIntervalIdx, pgsTypes::pftGirder,    vPoi, maxBAT, rtCumulative);
         girder  = pForces2->GetAxial(lastIntervalIdx,         pgsTypes::pftGirder,    vPoi, maxBAT, rtCumulative);
      }
      else
      {
         girder = pForces2->GetAxial(erectSegmentIntervalIdx, pgsTypes::pftGirder,    vPoi, maxBAT, rtCumulative);
      }
      std::vector<Float64> diaphragm = pForces2->GetAxial(lastIntervalIdx,     pgsTypes::pftDiaphragm, vPoi, maxBAT, rtCumulative);

      std::vector<Float64> minSlab, maxSlab;
      std::vector<Float64> minSlabPad, maxSlabPad;
      if (bDeck)
      {
         maxSlab = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSlab, vPoi, maxBAT, rtCumulative);
         minSlab = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSlab, vPoi, minBAT, rtCumulative);

         maxSlabPad = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSlabPad, vPoi, maxBAT, rtCumulative);
         minSlabPad = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSlabPad, vPoi, minBAT, rtCumulative);
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         maxDeckPanel = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSlabPanel, vPoi, maxBAT, rtCumulative );
         minDeckPanel = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSlabPanel, vPoi, minBAT, rtCumulative );
      }

      std::vector<Float64> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         maxConstruction = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftConstruction, vPoi, maxBAT, rtCumulative );
         minConstruction = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftConstruction, vPoi, minBAT, rtCumulative );
      }

      std::vector<Float64> dummy;
      std::vector<Float64> minOverlay, maxOverlay;
      std::vector<Float64> minTrafficBarrier, maxTrafficBarrier;
      std::vector<Float64> minSidewalk, maxSidewalk;
      std::vector<Float64> minShearKey, maxShearKey;
      std::vector<Float64> minLongitudinalJoint, maxLongitudinalJoint;
      std::vector<Float64> minPedestrian, maxPedestrian;
      std::vector<Float64> minDesignLL, maxDesignLL;
      std::vector<Float64> minFatigueLL, maxFatigueLL;
      std::vector<Float64> minPermitLL, maxPermitLL;
      std::vector<Float64> minLegalRoutineLL, maxLegalRoutineLL;
      std::vector<Float64> minLegalSpecialLL, maxLegalSpecialLL;
      std::vector<Float64> minLegalEmergencyLL, maxLegalEmergencyLL;
      std::vector<Float64> minPermitRoutineLL, maxPermitRoutineLL;
      std::vector<Float64> minPermitSpecialLL, maxPermitSpecialLL;

      std::vector<VehicleIndexType> dummyTruck;
      std::vector<VehicleIndexType> minDesignLLtruck;
      std::vector<VehicleIndexType> maxDesignLLtruck;
      std::vector<VehicleIndexType> minFatigueLLtruck;
      std::vector<VehicleIndexType> maxFatigueLLtruck;
      std::vector<VehicleIndexType> minPermitLLtruck;
      std::vector<VehicleIndexType> maxPermitLLtruck;
      std::vector<VehicleIndexType> minLegalRoutineLLtruck;
      std::vector<VehicleIndexType> maxLegalRoutineLLtruck;
      std::vector<VehicleIndexType> minLegalSpecialLLtruck;
      std::vector<VehicleIndexType> maxLegalSpecialLLtruck;
      std::vector<VehicleIndexType> minLegalEmergencyLLtruck;
      std::vector<VehicleIndexType> maxLegalEmergencyLLtruck;
      std::vector<VehicleIndexType> minPermitRoutineLLtruck;
      std::vector<VehicleIndexType> maxPermitRoutineLLtruck;
      std::vector<VehicleIndexType> minPermitSpecialLLtruck;
      std::vector<VehicleIndexType> maxPermitSpecialLLtruck;

      if ( bSidewalk )
      {
         maxSidewalk = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSidewalk, vPoi, maxBAT, rtCumulative );
         minSidewalk = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftSidewalk, vPoi, minBAT, rtCumulative );
      }

      if (bShearKey)
      {
         maxShearKey = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftShearKey, vPoi, maxBAT, rtCumulative);
         minShearKey = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftShearKey, vPoi, minBAT, rtCumulative);
      }

      if (bLongitudinalJoint)
      {
         maxLongitudinalJoint = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, vPoi, maxBAT, rtCumulative);
         minLongitudinalJoint = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, vPoi, minBAT, rtCumulative);
      }

      maxTrafficBarrier = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftTrafficBarrier, vPoi, maxBAT, rtCumulative );
      minTrafficBarrier = pForces2->GetAxial(lastIntervalIdx, pgsTypes::pftTrafficBarrier, vPoi, minBAT, rtCumulative );

      if ( bHasOverlay )
      {
         maxOverlay = pForces2->GetAxial(lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, vPoi, maxBAT, rtCumulative );
         minOverlay = pForces2->GetAxial(lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, vPoi, minBAT, rtCumulative );
      }

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPedestrian, vPoi, maxBAT, true, true, &dummy, &maxPedestrian );
         pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPedestrian, vPoi, minBAT, true, true, &minPedestrian, &dummy );
      }

      if ( bDesign )
      {
         pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltFatigue, vPoi, maxBAT, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltFatigue, vPoi, minBAT, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPermit, vPoi, maxBAT, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPermit, vPoi, minBAT, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, minBAT, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, maxBAT, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck);
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, minBAT, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, maxBAT, true, false, &dummy, &maxLegalEmergencyLL, &dummyTruck, &maxLegalEmergencyLLtruck);
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, minBAT, true, false, &minLegalEmergencyLL, &dummy, &minLegalEmergencyLLtruck, &dummyTruck);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, minBAT, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, maxBAT, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
            pForces2->GetLiveLoadAxial(lastIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, minBAT, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
         }
      }

      // write out the results
      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;

         (*p_table)(row, col++) << location.SetValue(poiRefAttribute, poi);
         if (bSegments)
         {
            (*p_table)(row, col++) << axial.SetValue(segment[index]);
         }

         (*p_table)(row, col++) << axial.SetValue(girder[index]);
         (*p_table)(row, col++) << axial.SetValue(diaphragm[index]);

         if (bShearKey)
         {
            if (analysisType == pgsTypes::Envelope)
            {
               (*p_table)(row, col++) << axial.SetValue(maxShearKey[index]);
               (*p_table)(row, col++) << axial.SetValue(minShearKey[index]);
            }
            else
            {
               (*p_table)(row, col++) << axial.SetValue(maxShearKey[index]);
            }
         }

         if (bLongitudinalJoint)
         {
            if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row, col++) << axial.SetValue(maxLongitudinalJoint[index]);
               (*p_table)(row, col++) << axial.SetValue(minLongitudinalJoint[index]);
            }
            else
            {
               (*p_table)(row, col++) << axial.SetValue(maxLongitudinalJoint[index]);
            }
         }

         if (bConstruction)
         {
            if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row, col++) << axial.SetValue(maxConstruction[index]);
               (*p_table)(row, col++) << axial.SetValue(minConstruction[index]);
            }
            else
            {
               (*p_table)(row, col++) << axial.SetValue(maxConstruction[index]);
            }
         }

         if (bDeck)
         {
            if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row, col++) << axial.SetValue(maxSlab[index]);
               (*p_table)(row, col++) << axial.SetValue(minSlab[index]);

               (*p_table)(row, col++) << axial.SetValue(maxSlabPad[index]);
               (*p_table)(row, col++) << axial.SetValue(minSlabPad[index]);
            }
            else
            {
               (*p_table)(row, col++) << axial.SetValue(maxSlab[index]);

               (*p_table)(row, col++) << axial.SetValue(maxSlabPad[index]);
            }
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
            {
               (*p_table)(row,col++) << axial.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << axial.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << axial.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << axial.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << axial.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << axial.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << axial.SetValue( minTrafficBarrier[index] );

            if ( bHasOverlay && overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << axial.SetValue( maxOverlay[index] );
               (*p_table)(row,col++) << axial.SetValue( minOverlay[index] );
            }
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << axial.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << axial.SetValue( maxTrafficBarrier[index] );

            if ( bHasOverlay && overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << axial.SetValue( maxOverlay[index] );
            }
         }

         if ( bPedLoading )
         {
            (*p_table)(row,col++) << axial.SetValue( maxPedestrian[index] );
            (*p_table)(row,col++) << axial.SetValue( minPedestrian[index] );
         }

         if ( bDesign )
         {
            (*p_table)(row,col) << axial.SetValue( maxDesignLL[index] );

            if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
            }

            col++;

            (*p_table)(row,col) << axial.SetValue( minDesignLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << axial.SetValue( maxFatigueLL[index] );

               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << axial.SetValue( minFatigueLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << _T(")");
               }

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << axial.SetValue( maxPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << axial.SetValue( minPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitLLtruck[index] << _T(")");
               }

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               (*p_table)(row,col) << axial.SetValue( maxDesignLL[index] );

               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << axial.SetValue( minDesignLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << axial.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << axial.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Special
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               (*p_table)(row, col) << axial.SetValue(maxLegalSpecialLL[index]);
               if (bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row, col) << axial.SetValue(minLegalSpecialLL[index]);
               if (bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               (*p_table)(row, col) << axial.SetValue(maxLegalEmergencyLL[index]);
               if (bIndicateControllingLoad && 0 < maxLegalEmergencyLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxLegalEmergencyLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row, col) << axial.SetValue(minLegalEmergencyLL[index]);
               if (bIndicateControllingLoad && 0 < minLegalEmergencyLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minLegalEmergencyLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << axial.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << axial.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << axial.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << axial.SetValue( minPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minPermitSpecialLLtruck[index] << _T(")");
               }

               col++;
            }
         }

         row++;
         index++;
      } // next poi
   } // next group

   return p_table;
}

void CProductAxialTable::MakeCopy(const CProductAxialTable& rOther)
{
   // Add copy code here...
}

void CProductAxialTable::MakeAssignment(const CProductAxialTable& rOther)
{
   MakeCopy( rOther );
}

