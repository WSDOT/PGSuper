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
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

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

std::_tstring LiveLoadPrefix(pgsTypes::LiveLoadType llType)
{
   std::_tstring strPrefix;
   switch(llType)
   {
   case pgsTypes::lltDesign:
      strPrefix = _T("D");
      break;

   case pgsTypes::lltFatigue:
      strPrefix = _T("F");
      break;
      
   case pgsTypes::lltPedestrian:
      strPrefix = _T("Ped");
      break;

   case pgsTypes::lltPermit:
      strPrefix = _T("P");
      break;

   case pgsTypes::lltLegalRating_Routine:
      strPrefix = _T("R");
      break;

   case pgsTypes::lltLegalRating_Special:
      strPrefix = _T("S");
      break;

   case pgsTypes::lltPermitRating_Routine:
      strPrefix = _T("PR");
      break;

   case pgsTypes::lltPermitRating_Special:
      strPrefix = _T("PS");
      break;
   }

   return strPrefix;
}

void LiveLoadTableFooter(IBroker* pBroker,rptParagraph* pPara,GirderIndexType girder,bool bDesign,bool bRating)
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   std::vector<std::_tstring> strLLNames;
   std::vector<std::_tstring>::iterator iter;
   long j;

   if ( bDesign )
   {
      GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);

      j = 0;
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girder);
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << j << _T(") ") << *iter << rptNewLine;
      }

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPedestrian) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPedestrian,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPedestrian) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << j << _T(") ") << *iter << rptNewLine;
         }
      }
   }

   if ( bRating )
   {
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         j = 0;
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girder);
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltLegalRating_Routine,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltLegalRating_Special,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermitRating_Routine,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermitRating_Special,girder);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << j << _T(") ") << *iter << rptNewLine;
         }
      }
   }
}

ColumnIndexType GetProductLoadTableColumnCount(IBroker* pBroker,SpanIndexType span,GirderIndexType gdrIdx,pgsTypes::AnalysisType analysisType,bool bDesign,bool bRating,
                                               bool *pbConstruction,bool* pbDeckPanels,bool* pbSidewalk,bool* pbShearKey,bool* pbPedLoading,bool* pbPermit,pgsTypes::Stage* pContinuityStage,SpanIndexType* pStartSpan,SpanIndexType* pNSpans)
{
   ColumnIndexType nCols = 6; // location, girder, diaphragm, slab, overlay, traffic barrier
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);

   *pbDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   *pbPermit     = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   *pStartSpan = (span == ALL_SPANS ? 0 : span);
   *pNSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : (*pStartSpan)+1 );
   *pbPedLoading = pLoads->HasPedestrianLoad(*pStartSpan,gdrIdx);
   *pbSidewalk   = pLoads->HasSidewalkLoad(*pStartSpan,gdrIdx);
   *pbShearKey   = pLoads->HasShearKeyLoad(*pStartSpan,gdrIdx);
   *pbConstruction = !IsZero(pUserLoads->GetConstructionLoad());

   // determine continuity stage
   *pContinuityStage = pgsTypes::BridgeSite2;
   SpanIndexType spanIdx;
   for ( spanIdx = *pStartSpan; spanIdx < *pNSpans; spanIdx++ )
   {
      pgsTypes::Stage left_stage, right_stage;
      pBridge->GetContinuityStage(spanIdx,&left_stage,&right_stage);
      *pContinuityStage = _cpp_min(*pContinuityStage,left_stage);
      *pContinuityStage = _cpp_min(*pContinuityStage,right_stage);
   }

   // last pier
   pgsTypes::Stage left_stage, right_stage;
   pBridge->GetContinuityStage(spanIdx,&left_stage,&right_stage);
   *pContinuityStage = _cpp_min(*pContinuityStage,left_stage);
   *pContinuityStage = _cpp_min(*pContinuityStage,right_stage);

   if ( *pbConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && *pContinuityStage == pgsTypes::BridgeSite1)
         nCols += 2;
      else
         nCols++;
   }

   if ( *pbDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && *pContinuityStage == pgsTypes::BridgeSite1)
         nCols += 2;
      else
         nCols++;
   }

   if ( analysisType == pgsTypes::Envelope && *pContinuityStage == pgsTypes::BridgeSite1 )
      nCols++; // add on more column for min/max slab

   if ( analysisType == pgsTypes::Envelope )
      nCols += 2; // add one more each for min/max overlay and min/max traffic barrier

   if ( bDesign )
   {
      nCols += 2; // design live loads
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         nCols += 2; // fatigue live load

      if ( *pbPermit )
         nCols += 2; // for permit live loads

      if ( *pbPedLoading )
         nCols += 2;
   }

   if ( *pbSidewalk )
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

   if ( *pbShearKey )
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
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory)) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         nCols += 2;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         nCols += 2;
   }

   return nCols;
}

/****************************************************************************
CLASS
   CProductMomentsTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CProductMomentsTable::CProductMomentsTable()
{
}

CProductMomentsTable::CProductMomentsTable(const CProductMomentsTable& rOther)
{
   MakeCopy(rOther);
}

CProductMomentsTable::~CProductMomentsTable()
{
}

//======================== OPERATORS  =======================================
CProductMomentsTable& CProductMomentsTable::operator= (const CProductMomentsTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


//======================== OPERATIONS =======================================
rptRcTable* CProductMomentsTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsTypes::AnalysisType analysisType,
                                        bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   SpanIndexType startSpan, nSpans;
   pgsTypes::Stage continuity_stage;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,span,gdr,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_stage,&startSpan,&nSpans);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Moments"));

   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   RowIndexType row = ConfigureProductLoadTableHeading<rptMomentUnitTag,unitmgtMomentData>(p_table,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,
                                                                                           bPermit,bRating,analysisType,continuity_stage,
                                                                                           pRatingSpec,pDisplayUnits,pDisplayUnits->GetMomentUnit());
   // Get the results
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(spanIdx,gdr, pgsTypes::BridgeSite3,POI_ALL, POIFIND_OR);
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(gdr,nGirders-1);

      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(spanIdx,gdrIdx);

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder = pForces2->GetMoment(girderLoadStage,pftGirder,vPoi,SimpleSpan);
      std::vector<Float64> diaphragm = pForces2->GetMoment(pgsTypes::BridgeSite1,pftDiaphragm,vPoi,SimpleSpan);

      std::vector<Float64> minSlab, maxSlab;
      if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
      {
         maxSlab = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlab, vPoi, MaxSimpleContinuousEnvelope );
         minSlab = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlab, vPoi, MinSimpleContinuousEnvelope );
      }
      else
      {
         maxSlab = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlab, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxDeckPanel = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MaxSimpleContinuousEnvelope );
            minDeckPanel = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, MinSimpleContinuousEnvelope );
         }
         else
         {
            maxDeckPanel = pForces2->GetMoment( pgsTypes::BridgeSite1, pftSlabPanel, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }
      }

      std::vector<Float64> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            maxConstruction = pForces2->GetMoment( pgsTypes::BridgeSite1, pftConstruction, vPoi, MaxSimpleContinuousEnvelope );
            minConstruction = pForces2->GetMoment( pgsTypes::BridgeSite1, pftConstruction, vPoi, MinSimpleContinuousEnvelope );
         }
         else
         {
            maxConstruction = pForces2->GetMoment( pgsTypes::BridgeSite1, pftConstruction, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
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
            maxSidewalk = pForces2->GetMoment( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MaxSimpleContinuousEnvelope );
            minSidewalk = pForces2->GetMoment( pgsTypes::BridgeSite2, pftSidewalk, vPoi, MinSimpleContinuousEnvelope );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetMoment( pgsTypes::BridgeSite1, pftShearKey, vPoi, MaxSimpleContinuousEnvelope );
            minShearKey = pForces2->GetMoment( pgsTypes::BridgeSite1, pftShearKey, vPoi, MinSimpleContinuousEnvelope );
         }

         maxTrafficBarrier = pForces2->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MaxSimpleContinuousEnvelope );
         minTrafficBarrier = pForces2->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, MinSimpleContinuousEnvelope );
         maxOverlay = pForces2->GetMoment( overlay_stage, bRating ? pftOverlayRating : pftOverlay, vPoi, MaxSimpleContinuousEnvelope );
         minOverlay = pForces2->GetMoment( overlay_stage, bRating ? pftOverlayRating : pftOverlay, vPoi, MinSimpleContinuousEnvelope );

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPedestrian );
               pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPedestrian, &dummy );
            }

            pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
            }

            if ( bPermit )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, MinSimpleContinuousEnvelope, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
            }
         }
      }
      else
      {
         if ( bSidewalk )
         {
            maxSidewalk = pForces2->GetMoment( pgsTypes::BridgeSite2, pftSidewalk, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         if ( bShearKey )
         {
            maxShearKey = pForces2->GetMoment( pgsTypes::BridgeSite1, pftShearKey, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         }

         maxTrafficBarrier = pForces2->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );
         maxOverlay = pForces2->GetMoment( overlay_stage, bRating ? pftOverlayRating : pftOverlay, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPedestrian, &maxPedestrian );
            }

            pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minFatigueLL, &maxFatigueLL, &minFatigueLLtruck, &maxFatigueLLtruck );
            }

            if ( bPermit )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitLL, &maxPermitLL, &minPermitLLtruck, &maxPermitLLtruck );
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minDesignLL, &maxDesignLL, &minDesignLLtruck, &maxDesignLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Routine, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minLegalRoutineLL, &maxLegalRoutineLL, &minLegalRoutineLLtruck, &maxLegalRoutineLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Special, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minLegalSpecialLL, &maxLegalSpecialLL, &minLegalSpecialLLtruck, &maxLegalSpecialLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Routine, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitRoutineLL, &maxPermitRoutineLL, &minPermitRoutineLLtruck, &maxPermitRoutineLLtruck );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Special, pgsTypes::BridgeSite3, vPoi, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &minPermitSpecialLL, &maxPermitSpecialLL, &minPermitSpecialLLtruck, &maxPermitSpecialLLtruck );
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

         (*p_table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
         (*p_table)(row,col++) << moment.SetValue( girder[index] );
         (*p_table)(row,col++) << moment.SetValue( diaphragm[index] );

         if ( bShearKey )
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               (*p_table)(row,col++) << moment.SetValue( maxShearKey[index] );
               (*p_table)(row,col++) << moment.SetValue( minShearKey[index] );
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxShearKey[index] );
            }
         }

         if ( bConstruction )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << moment.SetValue( maxConstruction[index] );
               (*p_table)(row,col++) << moment.SetValue( minConstruction[index] );
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxConstruction[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
         {
            (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << moment.SetValue( minSlab[index] );
         }
         else
         {
            (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_stage == pgsTypes::BridgeSite1 )
            {
               (*p_table)(row,col++) << moment.SetValue( maxDeckPanel[index] );
               (*p_table)(row,col++) << moment.SetValue( minDeckPanel[index] );
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxDeckPanel[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
               (*p_table)(row,col++) << moment.SetValue( minSidewalk[index] );
            }

            (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << moment.SetValue( minTrafficBarrier[index] );
            (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
            (*p_table)(row,col++) << moment.SetValue( minOverlay[index] );
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );
            (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
         }

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               (*p_table)(row,col++) << moment.SetValue( maxPedestrian[index] );
               (*p_table)(row,col++) << moment.SetValue( minPedestrian[index] );
            }

            (*p_table)(row,col) << moment.SetValue( maxDesignLL[index] );

            if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");

            col++;

            (*p_table)(row,col) << moment.SetValue( minDesignLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << moment.SetValue( maxFatigueLL[index] );

               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minFatigueLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << _T(")");

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << moment.SetValue( maxPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << minPermitLLtruck[index] << _T(")");

               col++;
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               (*p_table)(row,col) << moment.SetValue( maxDesignLL[index] );

               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minDesignLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << moment.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << _T(")");

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col) << moment.SetValue( maxLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << _T(")");

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << moment.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << _T(")");

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << moment.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << _T(")");

               col++;

               (*p_table)(row,col) << moment.SetValue( minPermitSpecialLL[index] );
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

void CProductMomentsTable::MakeCopy(const CProductMomentsTable& rOther)
{
   // Add copy code here...
}

void CProductMomentsTable::MakeAssignment(const CProductMomentsTable& rOther)
{
   MakeCopy( rOther );
}

