///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CProductShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductShearTable::CProductShearTable()
{
}

CProductShearTable::CProductShearTable(const CProductShearTable& rOther)
{
   MakeCopy(rOther);
}

CProductShearTable::~CProductShearTable()
{
}

//======================== OPERATORS  =======================================
CProductShearTable& CProductShearTable::operator= (const CProductShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductShearTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                      bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   bool bSegments, bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   bool bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Shears"));


   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);
   PoiAttributeType poiRefAttribute(girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : POI_ERECTED_SEGMENT);

   RowIndexType row = ConfigureProductLoadTableHeading<rptForceUnitTag,unitmgtForceData>(pBroker,p_table,false,false,bSegments,bConstruction,bDeckPanels,bSidewalk,bShearKey,bHasOverlay,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,bContinuousBeforeDeckCasting,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);

   pgsTypes::BridgeAnalysisType maxBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
      IntervalIndexType loadRatingIntervalIdx    = pIntervals->GetLoadRatingInterval();

      CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey,poiRefAttribute) );

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<sysSectionValue> segment;
      std::vector<sysSectionValue> girder;
      if ( bSegments )
      {
         segment = pForces2->GetShear(erectSegmentIntervalIdx, pgsTypes::pftGirder,   vPoi,maxBAT, rtCumulative);
         girder  = pForces2->GetShear(lastIntervalIdx,         pgsTypes::pftGirder,   vPoi,maxBAT, rtCumulative);
      }
      else
      {
         girder = pForces2->GetShear(erectSegmentIntervalIdx, pgsTypes::pftGirder,   vPoi,maxBAT, rtCumulative);
      }
      std::vector<sysSectionValue> diaphragm = pForces2->GetShear(lastIntervalIdx,     pgsTypes::pftDiaphragm,vPoi,maxBAT, rtCumulative);

      std::vector<sysSectionValue> minSlab, maxSlab;
      std::vector<sysSectionValue> minSlabPad, maxSlabPad;
      maxSlab = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftSlab, vPoi, maxBAT, rtCumulative );
      minSlab = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftSlab, vPoi, minBAT, rtCumulative );

      maxSlabPad = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftSlabPad, vPoi, maxBAT, rtCumulative );
      minSlabPad = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftSlabPad, vPoi, minBAT, rtCumulative );

      std::vector<sysSectionValue> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         maxConstruction = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftConstruction, vPoi, maxBAT, rtCumulative );
         minConstruction = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftConstruction, vPoi, minBAT, rtCumulative );
      }

      std::vector<sysSectionValue> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         maxDeckPanel = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftSlabPanel, vPoi, maxBAT, rtCumulative );
         minDeckPanel = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftSlabPanel, vPoi, minBAT, rtCumulative );
      }

      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minOverlay, maxOverlay;
      std::vector<sysSectionValue> minTrafficBarrier, maxTrafficBarrier;
      std::vector<sysSectionValue> minSidewalk, maxSidewalk;
      std::vector<sysSectionValue> minShearKey, maxShearKey;
      std::vector<sysSectionValue> minPedestrian, maxPedestrian;
      std::vector<sysSectionValue> minDesignLL, maxDesignLL;
      std::vector<sysSectionValue> minFatigueLL, maxFatigueLL;
      std::vector<sysSectionValue> minPermitLL, maxPermitLL;
      std::vector<sysSectionValue> minLegalRoutineLL, maxLegalRoutineLL;
      std::vector<sysSectionValue> minLegalSpecialLL, maxLegalSpecialLL;
      std::vector<sysSectionValue> minLegalEmergencyLL, maxLegalEmergencyLL;
      std::vector<sysSectionValue> minPermitRoutineLL, maxPermitRoutineLL;
      std::vector<sysSectionValue> minPermitSpecialLL, maxPermitSpecialLL;

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
         maxSidewalk = pForces2->GetShear( railingSystemIntervalIdx, pgsTypes::pftSidewalk, vPoi, maxBAT, rtCumulative );
         minSidewalk = pForces2->GetShear( railingSystemIntervalIdx, pgsTypes::pftSidewalk, vPoi, minBAT, rtCumulative );
      }

      if ( bShearKey )
      {
         maxShearKey = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftShearKey, vPoi, maxBAT, rtCumulative );
         minShearKey = pForces2->GetShear( castDeckIntervalIdx, pgsTypes::pftShearKey, vPoi, minBAT, rtCumulative );
      }

      maxTrafficBarrier = pForces2->GetShear( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, vPoi, maxBAT, rtCumulative );
      minTrafficBarrier = pForces2->GetShear( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, vPoi, minBAT, rtCumulative );

      if ( bHasOverlay )
      {
         maxOverlay = pForces2->GetShear( overlayIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, vPoi, maxBAT, rtCumulative );
         minOverlay = pForces2->GetShear( overlayIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, vPoi, minBAT, rtCumulative );
      }

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltPedestrian, vPoi, maxBAT, true, true, &dummy, &maxPedestrian );
         pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltPedestrian, vPoi, minBAT, true, true, &minPedestrian, &dummy );
      }

      pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
      pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltFatigue, vPoi, maxBAT, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
         pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltFatigue, vPoi, minBAT, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
      }

      if ( bPermit )
      {
         pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltPermit, vPoi, maxBAT, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
         pForces2->GetLiveLoadShear( liveLoadIntervalIdx, pgsTypes::lltPermit, vPoi, minBAT, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
      }

      if ( bRating )
      {
         if (!bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, minBAT, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pForces2->GetLiveLoadShear(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, maxBAT, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck);
            pForces2->GetLiveLoadShear(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, minBAT, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pForces2->GetLiveLoadShear(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, maxBAT, true, false, &dummy, &maxLegalEmergencyLL, &dummyTruck, &maxLegalEmergencyLLtruck);
            pForces2->GetLiveLoadShear(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, minBAT, true, false, &minLegalEmergencyLL, &dummy, &minLegalEmergencyLLtruck, &dummyTruck);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, minBAT, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, maxBAT, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
            pForces2->GetLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, minBAT, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
         }
      }

      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      IntervalIndexType index = 0;
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( poiRefAttribute, poi );

         if ( bSegments )
         {
            (*p_table)(row,col++) << shear.SetValue( segment[index] );
         }

         (*p_table)(row,col++) << shear.SetValue( girder[index] );
         (*p_table)(row,col++) << shear.SetValue( diaphragm[index] );

         if ( bShearKey )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row,col++) << shear.SetValue( maxShearKey[index] );
               (*p_table)(row,col++) << shear.SetValue( minShearKey[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxShearKey[index] );
            }
         }

         if ( bConstruction )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
            {
               (*p_table)(row,col++) << shear.SetValue( maxConstruction[index] );
               (*p_table)(row,col++) << shear.SetValue( minConstruction[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxConstruction[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
         {
            (*p_table)(row,col++) << shear.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << shear.SetValue( minSlab[index] );

            (*p_table)(row,col++) << shear.SetValue( maxSlabPad[index] );
            (*p_table)(row,col++) << shear.SetValue( minSlabPad[index] );
         }
         else
         {
            (*p_table)(row,col++) << shear.SetValue( maxSlab[index] );

            (*p_table)(row,col++) << shear.SetValue( maxSlabPad[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << shear.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << shear.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << shear.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << shear.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << shear.SetValue( minTrafficBarrier[index] );

            if ( bHasOverlay && overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << shear.SetValue( maxOverlay[index] );
               (*p_table)(row,col++) << shear.SetValue( minOverlay[index] );
            }
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << shear.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxTrafficBarrier[index] );

            if ( bHasOverlay && overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << shear.SetValue( maxOverlay[index] );
            }
         }

         if ( bPedLoading )
         {
            (*p_table)(row,col++) << shear.SetValue( maxPedestrian[index] );
            (*p_table)(row,col++) << shear.SetValue( minPedestrian[index] );
         }

         if ( bDesign )
         {
            (*p_table)(row,col) << shear.SetValue( maxDesignLL[index] );
            if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
            }

            col++;

            (*p_table)(row,col) << shear.SetValue( minDesignLL[index] );
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size())
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << shear.SetValue( maxFatigueLL[index] );
               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << shear.SetValue( minFatigueLL[index] );
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size())
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << _T(")");
               }

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << shear.SetValue( maxPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size())
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << shear.SetValue( minPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size())
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
               (*p_table)(row,col) << shear.SetValue( maxDesignLL[index] );
               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << shear.SetValue( minDesignLL[index] );
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size())
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << shear.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << shear.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Special
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               (*p_table)(row, col) << shear.SetValue(maxLegalSpecialLL[index]);
               if (bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row, col) << shear.SetValue(minLegalSpecialLL[index]);
               if (bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               (*p_table)(row, col) << shear.SetValue(maxLegalEmergencyLL[index]);
               if (bIndicateControllingLoad && 0 < maxLegalEmergencyLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxLegalEmergencyLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row, col) << shear.SetValue(minLegalEmergencyLL[index]);
               if (bIndicateControllingLoad && 0 < minLegalEmergencyLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minLegalEmergencyLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << shear.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << shear.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << shear.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << shear.SetValue( minPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minPermitSpecialLLtruck[index] << _T(")");
               }

               col++;
            }
         }

         row++;
      }
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CProductShearTable::MakeCopy(const CProductShearTable& rOther)
{
   // Add copy code here...
}

void CProductShearTable::MakeAssignment(const CProductShearTable& rOther)
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
bool CProductShearTable::AssertValid() const
{
   return true;
}

void CProductShearTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CProductShearTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductShearTable");

   TESTME_EPILOG("CProductShearTable");
}
#endif // _UNITTEST
