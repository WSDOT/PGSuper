///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <Reporting\ProductStressTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
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
   CProductStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductStressTable::CProductStressTable()
{
}

CProductStressTable::CProductStressTable(const CProductStressTable& rOther)
{
   MakeCopy(rOther);
}

CProductStressTable::~CProductStressTable()
{
}

//======================== OPERATORS  =======================================
CProductStressTable& CProductStressTable::operator= (const CProductStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CProductStressTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                       bool bDesign,bool bRating,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : startSpan+1 );

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(span,gdr);
   bool bPedLoading = pLoads->HasPedestrianLoad(startSpan,gdr);
   bool bSidewalk = pLoads->HasSidewalkLoad(startSpan,gdr);
   bool bShearKey = pLoads->HasShearKeyLoad(startSpan,gdr);



   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   int loss_method = pSpecEntry->GetLossMethod();
   bool bSlabShrinkage = ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() && 
                         (loss_method == LOSSES_AASHTO_REFINED || loss_method == LOSSES_WSDOT_REFINED) ? true : false);

   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
   bool bConstruction = !IsZero(pUserLoads->GetConstructionLoad());

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   pgsTypes::Stage continuity_stage = pgsTypes::BridgeSite2;
   SpanIndexType spanIdx;
   for ( spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(spanIdx,&left_stage,&right_stage);
      continuity_stage = _cpp_min(continuity_stage,left_stage);
      continuity_stage = _cpp_min(continuity_stage,right_stage);
   }
   // last pier
   pgsTypes::Stage left_stage, right_stage;
   pBridge->GetContinuityStage(spanIdx,&left_stage,&right_stage);
   continuity_stage = _cpp_min(continuity_stage,left_stage);
   continuity_stage = _cpp_min(continuity_stage,right_stage);

   ColumnIndexType nCols = 7;

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1)
         nCols += 2;
      else
         nCols++;
   }

   if ( bConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1)
         nCols += 2;
      else
         nCols++;
   }

   if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      nCols += 2; // add one more each for min/max slab and slab pad

   if ( analysisType == pgsTypes::Envelope )
      nCols += 2; // add one more each for min/max overlay and min/max traffic barrier

   if ( bDesign )
   {
      if ( bPedLoading )
         nCols += 2;


      nCols += 2;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         nCols += 2; // fatigue

      if ( bSlabShrinkage )
      {
         nCols++; // slab shrikage
      }

      if ( bPermit )
         nCols += 2;
   }


   if ( bSidewalk )
   {
      if (analysisType == pgsTypes::Envelope )
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }

   if ( bShearKey )
   {
      if (analysisType == pgsTypes::Envelope )
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }

   if ( bRating )
   {
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         nCols += 2;
   }

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Bridge Site Stress"));

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   RowIndexType row = ConfigureProductLoadTableHeading<rptStressUnitTag,unitmgtStressData>(p_table,false,bSlabShrinkage,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,bPermit,bRating,analysisType,continuity_stage,pRatingSpec,pDisplayUnits,pDisplayUnits->GetStressUnit());


   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   // Get all the tabular poi's for flexure and shear
   // Merge the two vectors to form one vector to report on.
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite3,POI_ALL, POIFIND_OR);

   std::vector<Float64> fTopGirder, fBotGirder;
   std::vector<Float64> fTopDiaphragm, fBotDiaphragm;
   std::vector<Float64> fTopMaxConstruction, fTopMinConstruction, fBotMaxConstruction, fBotMinConstruction;
   std::vector<Float64> fTopMaxSlab, fTopMinSlab, fBotMaxSlab, fBotMinSlab;
   std::vector<Float64> fTopMaxSlabPad, fTopMinSlabPad, fBotMaxSlabPad, fBotMinSlabPad;
   std::vector<Float64> fTopMaxSlabPanel, fTopMinSlabPanel, fBotMaxSlabPanel, fBotMinSlabPanel;
   std::vector<Float64> fTopMaxOverlay, fTopMinOverlay, fBotMaxOverlay, fBotMinOverlay;
   std::vector<Float64> fTopMaxSidewalk, fTopMinSidewalk, fBotMaxSidewalk, fBotMinSidewalk;
   std::vector<Float64> fTopMaxShearKey, fTopMinShearKey, fBotMaxShearKey, fBotMinShearKey;
   std::vector<Float64> fTopMaxTrafficBarrier, fTopMinTrafficBarrier, fBotMaxTrafficBarrier, fBotMinTrafficBarrier;
   std::vector<Float64> fTopMaxPedestrianLL, fBotMaxPedestrianLL;
   std::vector<Float64> fTopMinPedestrianLL, fBotMinPedestrianLL;
   std::vector<Float64> fTopMaxDesignLL, fBotMaxDesignLL;
   std::vector<Float64> fTopMinDesignLL, fBotMinDesignLL;
   std::vector<Float64> fTopMaxFatigueLL, fBotMaxFatigueLL;
   std::vector<Float64> fTopMinFatigueLL, fBotMinFatigueLL;
   std::vector<Float64> fTopMaxPermitLL, fBotMaxPermitLL;
   std::vector<Float64> fTopMinPermitLL, fBotMinPermitLL;
   std::vector<Float64> fTopMaxLegalRoutineLL, fBotMaxLegalRoutineLL;
   std::vector<Float64> fTopMinLegalRoutineLL, fBotMinLegalRoutineLL;
   std::vector<Float64> fTopMaxLegalSpecialLL, fBotMaxLegalSpecialLL;
   std::vector<Float64> fTopMinLegalSpecialLL, fBotMinLegalSpecialLL;
   std::vector<Float64> fTopMaxPermitRoutineLL, fBotMaxPermitRoutineLL;
   std::vector<Float64> fTopMinPermitRoutineLL, fBotMinPermitRoutineLL;
   std::vector<Float64> fTopMaxPermitSpecialLL, fBotMaxPermitSpecialLL;
   std::vector<Float64> fTopMinPermitSpecialLL, fBotMinPermitSpecialLL;
   std::vector<Float64> dummy1, dummy2;

   pForces2->GetStress( girderLoadStage, pftGirder, vPoi, SimpleSpan, &fTopGirder, &fBotGirder);
   pForces2->GetStress( pgsTypes::BridgeSite1, pftDiaphragm, vPoi, SimpleSpan, &fTopDiaphragm, &fBotDiaphragm);

   if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
   {
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSlab, &fBotMaxSlab );
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope, &fTopMinSlab, &fBotMinSlab );

      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPad, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSlabPad, &fBotMaxSlabPad );
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPad, vPoi, MinSimpleContinuousEnvelope, &fTopMinSlabPad, &fBotMinSlabPad );
   }
   else
   {
      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSlab, &fBotMaxSlab );

      pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPad, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSlabPad, &fBotMaxSlabPad );
   }

   if ( bConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftConstruction, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxConstruction, &fBotMaxConstruction );
         pForces2->GetStress( pgsTypes::BridgeSite1, pftConstruction, vPoi, MinSimpleContinuousEnvelope, &fTopMinConstruction, &fBotMinConstruction );
      }
      else
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftConstruction, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxConstruction, &fBotMaxConstruction );
      }
   }

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSlabPanel, &fBotMaxSlabPanel );
         pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope, &fTopMinSlabPanel, &fBotMinSlabPanel );
      }
      else
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSlabPanel, &fBotMaxSlabPanel );
      }
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      if ( bSidewalk )
      {
         pForces2->GetStress( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxSidewalk, &fBotMaxSidewalk);
         pForces2->GetStress( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope, &fTopMinSidewalk, &fBotMinSidewalk);
      }

      if ( bShearKey )
      {
         pForces2->GetStress( pgsTypes::BridgeSite1, pftShearKey, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxShearKey, &fBotMaxShearKey);
         pForces2->GetStress( pgsTypes::BridgeSite1, pftShearKey, vPoi, MinSimpleContinuousEnvelope, &fTopMinShearKey, &fBotMinShearKey);
      }

      pForces2->GetStress( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxTrafficBarrier, &fBotMaxTrafficBarrier);
      pForces2->GetStress( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope, &fTopMinTrafficBarrier, &fBotMinTrafficBarrier);

      pForces2->GetStress( overlay_stage, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, MaxSimpleContinuousEnvelope, &fTopMaxOverlay, &fBotMaxOverlay);
      pForces2->GetStress( overlay_stage, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, MinSimpleContinuousEnvelope, &fTopMinOverlay, &fBotMinOverlay);

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxPedestrianLL, &dummy2, &fBotMaxPedestrianLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinPedestrianLL, &dummy1, &fBotMinPedestrianLL, &dummy2);
         }

         pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxDesignLL, &dummy2, &fBotMaxDesignLL);
         pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinDesignLL, &dummy1, &fBotMinDesignLL, &dummy2);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxFatigueLL, &dummy2, &fBotMaxFatigueLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinFatigueLL, &dummy1, &fBotMinFatigueLL, &dummy2);
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxPermitLL, &dummy2, &fBotMaxPermitLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinPermitLL, &dummy1, &fBotMinPermitLL, &dummy2);
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxDesignLL, &dummy2, &fBotMaxDesignLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinDesignLL, &dummy1, &fBotMinDesignLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxLegalRoutineLL, &dummy2, &fBotMaxLegalRoutineLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinLegalRoutineLL, &dummy1, &fBotMinLegalRoutineLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxLegalSpecialLL, &dummy2, &fBotMaxLegalSpecialLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinLegalSpecialLL, &dummy1, &fBotMinLegalSpecialLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxPermitRoutineLL, &dummy2, &fBotMaxPermitRoutineLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinPermitRoutineLL, &dummy1, &fBotMinPermitRoutineLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy1, &fTopMaxPermitSpecialLL, &dummy2, &fBotMaxPermitSpecialLL);
            pForces2->GetLiveLoadStress(pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &fTopMinPermitSpecialLL, &dummy1, &fBotMinPermitSpecialLL, &dummy2);
         }
      }
   }
   else
   {
      if ( bSidewalk )
         pForces2->GetStress( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxSidewalk, &fBotMaxSidewalk);

      if ( bShearKey )
         pForces2->GetStress( pgsTypes::BridgeSite1, pftShearKey, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxShearKey, &fBotMaxShearKey);

      pForces2->GetStress( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxTrafficBarrier, &fBotMaxTrafficBarrier);
      pForces2->GetStress( overlay_stage, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMaxOverlay, &fBotMaxOverlay);

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinPedestrianLL, &fTopMaxPedestrianLL, &fBotMinPedestrianLL, &fBotMaxPedestrianLL);
         }

         pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinFatigueLL, &fTopMaxFatigueLL, &fBotMinFatigueLL, &fBotMaxFatigueLL);
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinPermitLL, &fTopMaxPermitLL, &fBotMinPermitLL, &fBotMaxPermitLL);
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinLegalRoutineLL, &fTopMaxLegalRoutineLL, &fBotMinLegalRoutineLL, &fBotMaxLegalRoutineLL);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinLegalSpecialLL, &fTopMaxLegalSpecialLL, &fBotMinLegalSpecialLL, &fBotMaxLegalSpecialLL);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinPermitRoutineLL, &fTopMaxPermitRoutineLL, &fBotMinPermitRoutineLL, &fBotMaxPermitRoutineLL);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadStress(pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMinPermitSpecialLL, &fTopMaxPermitSpecialLL, &fBotMinPermitSpecialLL, &fBotMaxPermitSpecialLL);
         }
      }
   }



   // Fill up the table
   Uint32 index = 0;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++, index++ )
   {
      const pgsPointOfInterest& poi = *i;
      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx  = poi.GetGirder();
   
      Float64 end_size = pBridge->GetGirderStartConnectionLength(spanIdx,gdrIdx);

      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      ColumnIndexType col = 0;

      (*p_table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

      (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopGirder[index]) << rptNewLine;
      (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotGirder[index]);
      col++;

      (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDiaphragm[index]) << rptNewLine;
      (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDiaphragm[index]);
      col++;

      if ( bShearKey )
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxShearKey[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxShearKey[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinShearKey[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinShearKey[index]);
            col++;
         }
         else
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxShearKey[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxShearKey[index]);
            col++;
         }
      }

      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxConstruction[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxConstruction[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinConstruction[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinConstruction[index]);
            col++;
         }
         else
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxConstruction[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxConstruction[index]);
            col++;
         }
      }

      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlab[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlab[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSlab[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSlab[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPad[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPad[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSlabPad[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSlabPad[index]);
         col++;
      }
      else
      {
         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlab[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlab[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPad[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPad[index]);
         col++;
      }

      if ( bSlabShrinkage )
      {
         Float64 ft_ss, fb_ss;
         pForces->GetDeckShrinkageStresses(poi,&ft_ss,&fb_ss);

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(ft_ss) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fb_ss);
         col++;
      }

      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPanel[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPanel[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSlabPanel[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSlabPanel[index]);
            col++;
         }
         else
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSlabPanel[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSlabPanel[index]);
            col++;
         }
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSidewalk[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSidewalk[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinSidewalk[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinSidewalk[index]);
            col++;
         }

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxTrafficBarrier[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxTrafficBarrier[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinTrafficBarrier[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinTrafficBarrier[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxOverlay[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinOverlay[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinOverlay[index]);
         col++;
      }
      else
      {
         if ( bSidewalk )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxSidewalk[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxSidewalk[index]);
            col++;
         }

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxTrafficBarrier[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxTrafficBarrier[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxOverlay[index]);
         col++;
      }


      if ( bDesign )
      {
         if ( bPedLoading )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPedestrianLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPedestrianLL[index]);
            col++;
         }

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);
         col++;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueLL[index]);
            col++;
         }

         if ( bPermit )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPermitLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPermitLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPermitLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPermitLL[index]);
            col++;
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);
            col++;
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalRoutineLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalRoutineLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalRoutineLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalRoutineLL[index]);
            col++;
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalSpecialLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalSpecialLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalSpecialLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalSpecialLL[index]);
            col++;
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPermitRoutineLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPermitRoutineLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPermitRoutineLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPermitRoutineLL[index]);
            col++;
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPermitSpecialLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPermitSpecialLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPermitSpecialLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPermitSpecialLL[index]);
            col++;
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
void CProductStressTable::MakeCopy(const CProductStressTable& rOther)
{
   // Add copy code here...
}

void CProductStressTable::MakeAssignment(const CProductStressTable& rOther)
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
