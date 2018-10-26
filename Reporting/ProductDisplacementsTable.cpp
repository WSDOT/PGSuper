///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include <PgsExt\PointOfInterest.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
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
rptRcTable* CProductDisplacementsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                              bool bDesign,bool bRating,bool bIndicateControllingLoad,IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   bool bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   SpanIndexType startSpan, nSpans;
   pgsTypes::Stage continuity_stage;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,span,gdr,analysisType,bDesign,bRating,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_stage,&startSpan,&nSpans);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"Displacements");
   RowIndexType row = ConfigureProductLoadTableHeading<rptLengthUnitTag,unitmgtLengthData>(p_table,false,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continuity_stage,pRatingSpec,pDisplayUnits,pDisplayUnits->GetDisplacementUnit());

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, spanIdx, gdr, POI_ALL, POIFIND_OR );

      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder = pForces2->GetDisplacement(girderLoadStage,pftGirder,vPoi,SimpleSpan);
      std::vector<Float64> diaphragm = pForces2->GetDisplacement(pgsTypes::BridgeSite1,pftDiaphragm,vPoi,SimpleSpan);

      std::vector<Float64> minSlab, maxSlab;
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         maxSlab = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope );
         minSlab = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxSlab = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDeckPanel = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope );
            minDeckPanel = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope );
         }
         else
         {
            maxDeckPanel = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }
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

      std::vector<long> dummyTruck;
      std::vector<long> minDesignLLtruck;
      std::vector<long> maxDesignLLtruck;
      std::vector<long> minFatigueLLtruck;
      std::vector<long> maxFatigueLLtruck;
      std::vector<long> minPermitLLtruck;
      std::vector<long> maxPermitLLtruck;
      std::vector<long> minLegalRoutineLLtruck;
      std::vector<long> maxLegalRoutineLLtruck;
      std::vector<long> minLegalSpecialLLtruck;
      std::vector<long> maxLegalSpecialLLtruck;
      std::vector<long> minPermitRoutineLLtruck;
      std::vector<long> maxPermitRoutineLLtruck;
      std::vector<long> minPermitSpecialLLtruck;
      std::vector<long> maxPermitSpecialLLtruck;

      if (analysisType == pgsTypes::Envelope)
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope );
            minSidewalk = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftShearKey, vPoi, MaxSimpleContinuousEnvelope );
            minShearKey = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftShearKey, vPoi, MinSimpleContinuousEnvelope );
         }

         maxTrafficBarrier = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope );
         minTrafficBarrier = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope );
         maxOverlay = pForces2->GetDisplacement( overlay_stage, pftOverlay, vPoi, MaxSimpleContinuousEnvelope );
         minOverlay = pForces2->GetDisplacement( overlay_stage, pftOverlay, vPoi, MinSimpleContinuousEnvelope );

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPedestrian );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPedestrian, &dummy );
            }

            pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
            }

            if ( bPermit )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
            }
         }
      }
      else
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetDisplacement( pgsTypes::BridgeSite1, pftShearKey, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         maxTrafficBarrier = pForces2->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxOverlay = pForces2->GetDisplacement( overlay_stage, pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPedestrian, &maxPedestrian );
            }

            pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minFatigueLL, &maxFatigueLL, &minFatigueLLtruck, &maxFatigueLLtruck );
            }

            if ( bPermit )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitLL, &maxPermitLL, &minPermitLLtruck, &maxPermitLLtruck );
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minLegalRoutineLL, &maxLegalRoutineLL, &minLegalRoutineLLtruck, &maxLegalRoutineLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minLegalSpecialLL, &maxLegalSpecialLL, &minLegalSpecialLLtruck, &maxLegalSpecialLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitRoutineLL, &maxPermitRoutineLL, &minPermitRoutineLLtruck, &maxPermitRoutineLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces2->GetLiveLoadDisplacement( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitSpecialLL, &maxPermitSpecialLL, &minPermitSpecialLLtruck, &maxPermitSpecialLLtruck );
            }
         }
      }


      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i;
      long index = 0;
      for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;

         int col = 0;

         Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());

         (*p_table)(row,col++) << location.SetValue( poi, end_size );
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

         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << displacement.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << displacement.SetValue( minSlab[index] );
         }
         else
         {
            (*p_table)(row,col++) << displacement.SetValue( maxSlab[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
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
               (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << ")";

            col++;

            (*p_table)(row,col) << displacement.SetValue( minDesignLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << ")";

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << displacement.SetValue( maxFatigueLL[index] );

               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minFatigueLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << ")";

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << displacement.SetValue( maxPermitLL[index] );

               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size())
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minPermitLL[index] );

               if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size())
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitLLtruck[index] << ")";

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxDesignLL[index] );

               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minDesignLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << ")";

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << ")";

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << ")";

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << ")";

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << displacement.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << ")";

               col++;

               (*p_table)(row,col) << displacement.SetValue( minPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << "(" << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << minPermitSpecialLLtruck[index] << ")";

               col++;
            }
         }

         row++;
      }
   }

   return p_table;
}

rptRcTable* CProductDisplacementsTable::BuildLiveLoadTable(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                                           IDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, displacement, pDisplayUnits->GetDisplacementUnit(), false );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(4,"Displacements For The LRFD Optional Deflection Live Load (LRFD 3.6.1.3.2)");

   // Set up table headings
   (*p_table)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,        rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,1) << COLHDR("D1",          rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*p_table)(0,2) << COLHDR("D2",       rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );
   (*p_table)(0,3) << COLHDR("D" << rptNewLine << "Controlling",            rptLengthUnitTag, pDisplayUnits->GetDisplacementUnit() );

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, span, girder, POI_TABULAR );

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;

      (*p_table)(row,0) << location.SetValue( poi, end_size );

      Float64 min, max;
      pForces->GetDeflLiveLoadDisplacement( IProductForces::DesignTruckAlone, poi, &min, &max );
      (*p_table)(row,1) << displacement.SetValue( min );
      pForces->GetDeflLiveLoadDisplacement( IProductForces::Design25PlusLane, poi, &min, &max );
      (*p_table)(row,2) << displacement.SetValue( min );
      pForces->GetDeflLiveLoadDisplacement( IProductForces::DeflectionLiveLoadEnvelope, poi, &min, &max );
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
   os << "Dump for CProductDisplacementsTable" << endl;
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
