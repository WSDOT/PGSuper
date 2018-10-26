///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CProductDeflectionsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductDeflectionsTable::CProductDeflectionsTable()
{
}

CProductDeflectionsTable::CProductDeflectionsTable(const CProductDeflectionsTable& rOther)
{
   MakeCopy(rOther);
}

CProductDeflectionsTable::~CProductDeflectionsTable()
{
}

//======================== OPERATORS  =======================================
CProductDeflectionsTable& CProductDeflectionsTable::operator= (const CProductDeflectionsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductDeflectionsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                              bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   GroupIndexType startGroup, endGroup;
   IntervalIndexType continuityIntervalIdx;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   GET_IFACE2(pBroker, IProductForces, pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx     = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType overlayIntervalIdx      = pIntervals->GetOverlayInterval(girderKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetLastSegmentErectionInterval(girderKey);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuityIntervalIdx,&startGroup,&endGroup);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Deflections"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   RowIndexType row = ConfigureProductLoadTableHeading<rptLengthUnitTag,unitmgtLengthData>(pBroker,p_table,false,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,overlayIntervalIdx != INVALID_INDEX,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,continuityIntervalIdx,castDeckIntervalIdx,pRatingSpec,pDisplayUnits,pDisplayUnits->GetDeflectionUnit());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);


   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(thisGirderKey);
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(thisGirderKey);
      IntervalIndexType loadRatingIntervalIdx    = pIntervals->GetLoadRatingInterval(thisGirderKey);

      CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey,POI_ERECTED_SEGMENT) );

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder    = pForces2->GetDeflection(erectSegmentIntervalIdx,pftGirder,vPoi,maxBAT, rtCumulative, false);
      std::vector<Float64> diaphragm = pForces2->GetDeflection(castDeckIntervalIdx,pftDiaphragm,vPoi,maxBAT, rtCumulative, false);

      std::vector<Float64> minSlab, maxSlab;
      std::vector<Float64> minSlabPad, maxSlabPad;
      maxSlab = pForces2->GetDeflection( castDeckIntervalIdx, pftSlab, vPoi, maxBAT, rtCumulative, false );
      minSlab = pForces2->GetDeflection( castDeckIntervalIdx, pftSlab, vPoi, minBAT, rtCumulative, false );

      maxSlabPad = pForces2->GetDeflection( castDeckIntervalIdx, pftSlabPad, vPoi, maxBAT, rtCumulative, false );
      minSlabPad = pForces2->GetDeflection( castDeckIntervalIdx, pftSlabPad, vPoi, minBAT, rtCumulative, false );

      std::vector<Float64> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         maxConstruction = pForces2->GetDeflection( castDeckIntervalIdx, pftConstruction, vPoi, maxBAT, rtCumulative, false);
         minConstruction = pForces2->GetDeflection( castDeckIntervalIdx, pftConstruction, vPoi, minBAT, rtCumulative, false);
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         maxDeckPanel = pForces2->GetDeflection( castDeckIntervalIdx, pftSlabPanel, vPoi, maxBAT, rtCumulative, false);
         minDeckPanel = pForces2->GetDeflection( castDeckIntervalIdx, pftSlabPanel, vPoi, minBAT, rtCumulative, false);
      }

      std::vector<Float64> dummy;
      std::vector<Float64> minOverlay, maxOverlay;
      std::vector<Float64> minTrafficBarrier, maxTrafficBarrier;
      std::vector<Float64> minSidewalk, maxSidewalk;
      std::vector<Float64> minShearKey, maxShearKey;
      std::vector<Float64> minPedestrian, maxPedestrian;
      std::vector<Float64> minDesignLL, maxDesignLL;
      std::vector<Float64> minFatigueLL, maxFatigueLL;
      std::vector<Float64> minPermitLL, maxPermitLL;
      std::vector<Float64> minLegalRoutineLL, maxLegalRoutineLL;
      std::vector<Float64> minLegalSpecialLL, maxLegalSpecialLL;
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
      std::vector<VehicleIndexType> minPermitRoutineLLtruck;
      std::vector<VehicleIndexType> maxPermitRoutineLLtruck;
      std::vector<VehicleIndexType> minPermitSpecialLLtruck;
      std::vector<VehicleIndexType> maxPermitSpecialLLtruck;

      if ( bSidewalk )
      {
         maxSidewalk = pForces2->GetDeflection( railingSystemIntervalIdx, pftSidewalk, vPoi, maxBAT, rtCumulative, false );
         minSidewalk = pForces2->GetDeflection( railingSystemIntervalIdx, pftSidewalk, vPoi, minBAT, rtCumulative, false );
      }

      if ( bShearKey )
      {
         maxShearKey = pForces2->GetDeflection( castDeckIntervalIdx, pftShearKey, vPoi, maxBAT, rtCumulative, false );
         minShearKey = pForces2->GetDeflection( castDeckIntervalIdx, pftShearKey, vPoi, minBAT, rtCumulative, false );
      }

      maxTrafficBarrier = pForces2->GetDeflection( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, maxBAT, rtCumulative, false );
      minTrafficBarrier = pForces2->GetDeflection( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, minBAT, rtCumulative, false );

      if ( overlayIntervalIdx != INVALID_INDEX )
      {
         maxOverlay = pForces2->GetDeflection( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, maxBAT, rtCumulative, false );
         minOverlay = pForces2->GetDeflection( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, minBAT, rtCumulative, false );
      }

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltPedestrian, vPoi, maxBAT, true, true, &dummy, &maxPedestrian );
            pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltPedestrian, vPoi, minBAT, true, true, &minPedestrian, &dummy );
         }

         pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltFatigue, vPoi, maxBAT, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltFatigue, vPoi, minBAT, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltPermit, vPoi, maxBAT, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadDeflection( liveLoadIntervalIdx, pgsTypes::lltPermit, vPoi, minBAT, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, minBAT, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, maxBAT, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck );
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, minBAT, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, minBAT, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, maxBAT, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
            pForces2->GetLiveLoadDeflection( loadRatingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, minBAT, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
         }
      }


      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      CollectionIndexType index = 0;
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );
         (*p_table)(row,col++) << deflection.SetValue( girder[index] );
         (*p_table)(row,col++) << deflection.SetValue( diaphragm[index] );

         if ( bShearKey )
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxShearKey[index] );
               (*p_table)(row,col++) << deflection.SetValue( minShearKey[index] );
            }
            else
            {
               (*p_table)(row,col++) << deflection.SetValue( maxShearKey[index] );
            }
         }

         if ( bConstruction )
         {
            if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxConstruction[index] );
               (*p_table)(row,col++) << deflection.SetValue( minConstruction[index] );
            }
            else
            {
               (*p_table)(row,col++) << deflection.SetValue( maxConstruction[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << deflection.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << deflection.SetValue( minSlab[index] );
            
            (*p_table)(row,col++) << deflection.SetValue( maxSlabPad[index] );
            (*p_table)(row,col++) << deflection.SetValue( minSlabPad[index] );
         }
         else
         {
            (*p_table)(row,col++) << deflection.SetValue( maxSlab[index] );
            
            (*p_table)(row,col++) << deflection.SetValue( maxSlabPad[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << deflection.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << deflection.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << deflection.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << deflection.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << deflection.SetValue( minTrafficBarrier[index] );

            if ( overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxOverlay[index] );
               (*p_table)(row,col++) << deflection.SetValue( minOverlay[index] );
            }
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << deflection.SetValue( maxTrafficBarrier[index] );

            if ( overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxOverlay[index] );
            }
         }

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               (*p_table)(row,col++) << deflection.SetValue( maxPedestrian[index] );
               (*p_table)(row,col++) << deflection.SetValue( minPedestrian[index] );
            }

            (*p_table)(row,col) << deflection.SetValue( maxDesignLL[index] );

            if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
            }

            col++;

            (*p_table)(row,col) << deflection.SetValue( minDesignLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << deflection.SetValue( maxFatigueLL[index] );

               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minFatigueLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << _T(")");
               }

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << deflection.SetValue( maxPermitLL[index] );

               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size())
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minPermitLL[index] );

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
               (*p_table)(row,col) << deflection.SetValue( maxDesignLL[index] );

               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minDesignLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << deflection.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col) << deflection.SetValue( maxLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << deflection.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << deflection.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << deflection.SetValue( minPermitSpecialLL[index] );
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

rptRcTable* CProductDeflectionsTable::BuildLiveLoadTable(IBroker* pBroker,const CGirderKey& girderKey,
                                                           IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection, pDisplayUnits->GetDeflectionUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Deflections For The LRFD Optional Deflection Live Load (LRFD 3.6.1.3.2)"));

   if (girderKey.groupIndex == ALL_GROUPS)
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Set up table headings
   (*p_table)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("D1"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*p_table)(0,2) << COLHDR(_T("D2"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*p_table)(0,3) << COLHDR(_T("D") << rptNewLine << _T("Controlling"), rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS),POI_ERECTED_SEGMENT ) );

   GET_IFACE2(pBroker,IProductForces,pForces);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::BridgeAnalysisType bat = (pSpec->GetAnalysisType() == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      (*p_table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi );

      Float64 min, max;
      pForces->GetDeflLiveLoadDeflection( IProductForces::DesignTruckAlone, poi, bat, &min, &max );
      (*p_table)(row,1) << deflection.SetValue( min );

      pForces->GetDeflLiveLoadDeflection( IProductForces::Design25PlusLane, poi, bat, &min, &max );
      (*p_table)(row,2) << deflection.SetValue( min );

      pForces->GetDeflLiveLoadDeflection( IProductForces::DeflectionLiveLoadEnvelope, poi, bat, &min, &max );
      (*p_table)(row,3) << deflection.SetValue( min );

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
void CProductDeflectionsTable::MakeCopy(const CProductDeflectionsTable& rOther)
{
   // Add copy code here...
}

void CProductDeflectionsTable::MakeAssignment(const CProductDeflectionsTable& rOther)
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
bool CProductDeflectionsTable::AssertValid() const
{
   return true;
}

void CProductDeflectionsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CProductDeflectionsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductDeflectionsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductDeflectionsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductDeflectionsTable");

   TESTME_EPILOG("CProductDeflectionsTable");
}
#endif // _UNITTEST
