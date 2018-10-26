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
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>

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
   CProductDisplacementsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductDisplacementsTable::CProductDisplacementsTable()
{
}

CProductDisplacementsTable::CProductDisplacementsTable(const CProductDisplacementsTable& rOther)
{
   MakeCopy(rOther);
}

CProductDisplacementsTable::~CProductDisplacementsTable()
{
}

//======================== OPERATORS  =======================================
CProductDisplacementsTable& CProductDisplacementsTable::operator= (const CProductDisplacementsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductDisplacementsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                              bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   GroupIndexType startGroup, nGroups;
   IntervalIndexType continuityIntervalIdx;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   GET_IFACE2(pBroker, IProductForces, pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuityIntervalIdx,&startGroup,&nGroups);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Displacements"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   RowIndexType row = ConfigureProductLoadTableHeading<rptLengthUnitTag,unitmgtLengthData>(pBroker,p_table,false,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continuityIntervalIdx,pRatingSpec,pDisplayUnits,pDisplayUnits->GetDisplacementUnit());

   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetFirstErectedSegmentInterval();

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   for ( GroupIndexType grpIdx = startGroup; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = min(girderKey.girderIndex,nGirders-1);

      CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey) );


      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder    = pForces2->GetDisplacement(erectSegmentIntervalIdx,pftGirder,vPoi,maxBAT);
      std::vector<Float64> diaphragm = pForces2->GetDisplacement(castDeckIntervalIdx,pftDiaphragm,vPoi,maxBAT);

      std::vector<Float64> minSlab, maxSlab;
      std::vector<Float64> minSlabPad, maxSlabPad;
      maxSlab = pForces2->GetDisplacement( castDeckIntervalIdx, pftSlab, vPoi, maxBAT );
      minSlab = pForces2->GetDisplacement( castDeckIntervalIdx, pftSlab, vPoi, minBAT );

      maxSlabPad = pForces2->GetDisplacement( castDeckIntervalIdx, pftSlabPad, vPoi, maxBAT );
      minSlabPad = pForces2->GetDisplacement( castDeckIntervalIdx, pftSlabPad, vPoi, minBAT );

      std::vector<Float64> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         maxConstruction = pForces2->GetDisplacement( castDeckIntervalIdx, pftConstruction, vPoi, maxBAT);
         minConstruction = pForces2->GetDisplacement( castDeckIntervalIdx, pftConstruction, vPoi, minBAT);
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         maxDeckPanel = pForces2->GetDisplacement( castDeckIntervalIdx, pftSlabPanel, vPoi, maxBAT );
         minDeckPanel = pForces2->GetDisplacement( castDeckIntervalIdx, pftSlabPanel, vPoi, minBAT );
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
         maxSidewalk = pForces2->GetDisplacement( railingSystemIntervalIdx, pftSidewalk, vPoi, maxBAT );
         minSidewalk = pForces2->GetDisplacement( railingSystemIntervalIdx, pftSidewalk, vPoi, minBAT );
      }

      if ( bShearKey )
      {
         maxShearKey = pForces2->GetDisplacement( castDeckIntervalIdx, pftShearKey, vPoi, maxBAT );
         minShearKey = pForces2->GetDisplacement( castDeckIntervalIdx, pftShearKey, vPoi, minBAT );
      }

      maxTrafficBarrier = pForces2->GetDisplacement( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, maxBAT );
      minTrafficBarrier = pForces2->GetDisplacement( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, minBAT );
      maxOverlay = pForces2->GetDisplacement( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, maxBAT );
      minOverlay = pForces2->GetDisplacement( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, minBAT );

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPoi, maxBAT, true, true, &dummy, &maxPedestrian );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPoi, minBAT, true, true, &minPedestrian, &dummy );
         }

         pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
         }
      }


      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      CollectionIndexType index = 0;
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
         const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

         ColumnIndexType col = 0;

         Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
         (*p_table)(row,col++) << displacement.SetValue( girder[index] );
         (*p_table)(row,col++) << displacement.SetValue( diaphragm[index] );

         if ( bShearKey )
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxShearKey[index] );
               (*p_table)(row,col++) << displacement.SetValue( minShearKey[index] );
            }
            else
            {
               (*p_table)(row,col++) << displacement.SetValue( maxShearKey[index] );
            }
         }

         if ( bConstruction )
         {
            if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxConstruction[index] );
               (*p_table)(row,col++) << displacement.SetValue( minConstruction[index] );
            }
            else
            {
               (*p_table)(row,col++) << displacement.SetValue( maxConstruction[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << displacement.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << displacement.SetValue( minSlab[index] );
            
            (*p_table)(row,col++) << displacement.SetValue( maxSlabPad[index] );
            (*p_table)(row,col++) << displacement.SetValue( minSlabPad[index] );
         }
         else
         {
            (*p_table)(row,col++) << displacement.SetValue( maxSlab[index] );
            
            (*p_table)(row,col++) << displacement.SetValue( maxSlabPad[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuityIntervalIdx == castDeckIntervalIdx )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << displacement.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << displacement.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << displacement.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << displacement.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << displacement.SetValue( minTrafficBarrier[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxOverlay[index] );
            (*p_table)(row,col++) << displacement.SetValue( minOverlay[index] );
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << displacement.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << displacement.SetValue( maxOverlay[index] );
         }

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               (*p_table)(row,col++) << displacement.SetValue( maxPedestrian[index] );
               (*p_table)(row,col++) << displacement.SetValue( minPedestrian[index] );
            }

            (*p_table)(row,col) << displacement.SetValue( maxDesignLL[index] );

            if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");

            col++;

            (*p_table)(row,col) << displacement.SetValue( minDesignLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << displacement.SetValue( maxFatigueLL[index] );

               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minFatigueLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << _T(")");

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << displacement.SetValue( maxPermitLL[index] );

               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size())
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minPermitLL[index] );

               if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size())
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitLLtruck[index] << _T(")");

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxDesignLL[index] );

               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minDesignLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << _T(")");

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << _T(")");

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << _T(")");

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << displacement.SetValue( minPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minPermitSpecialLLtruck[index] << _T(")");

               col++;
            }
         }

         row++;
      }
   }

   return p_table;
}

rptRcTable* CProductDisplacementsTable::BuildLiveLoadTable(IBroker* pBroker,const CGirderKey& girderKey,
                                                           IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,_T("Displacements For The LRFD Optional Deflection Live Load (LRFD 3.6.1.3.2)"));

   if (girderKey.groupIndex == ALL_GROUPS)
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Set up table headings
   (*p_table)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR(_T("D1"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*p_table)(0,2) << COLHDR(_T("D2"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*p_table)(0,3) << COLHDR(_T("D") << rptNewLine << _T("Controlling"), rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS) ) );

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::BridgeAnalysisType bat = (pSpec->GetAnalysisType() == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      (*p_table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      Float64 min, max;
      pForces->GetDeflLiveLoadDisplacement( IProductForces::DesignTruckAlone, poi, bat, &min, &max );
      (*p_table)(row,1) << displacement.SetValue( min );

      pForces->GetDeflLiveLoadDisplacement( IProductForces::Design25PlusLane, poi, bat, &min, &max );
      (*p_table)(row,2) << displacement.SetValue( min );

      pForces->GetDeflLiveLoadDisplacement( IProductForces::DeflectionLiveLoadEnvelope, poi, bat, &min, &max );
      (*p_table)(row,3) << displacement.SetValue( min );

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
void CProductDisplacementsTable::MakeCopy(const CProductDisplacementsTable& rOther)
{
   // Add copy code here...
}

void CProductDisplacementsTable::MakeAssignment(const CProductDisplacementsTable& rOther)
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
bool CProductDisplacementsTable::AssertValid() const
{
   return true;
}

void CProductDisplacementsTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CProductDisplacementsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CProductDisplacementsTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CProductDisplacementsTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CProductDisplacementsTable");

   TESTME_EPILOG("CProductDisplacementsTable");
}
#endif // _UNITTEST
