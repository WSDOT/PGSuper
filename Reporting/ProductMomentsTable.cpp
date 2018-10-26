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
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>

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

void LiveLoadTableFooter(IBroker* pBroker,rptParagraph* pPara,const CGirderKey& girderKey,bool bDesign,bool bRating)
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
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girderKey);
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << j << _T(") ") << *iter << rptNewLine;
      }

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPedestrian) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPedestrian,girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPedestrian) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girderKey);
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
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girderKey);
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltLegalRating_Routine,girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltLegalRating_Special,girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermitRating_Routine,girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << j << _T(") ") << *iter << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermitRating_Special,girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << j << _T(") ") << *iter << rptNewLine;
         }
      }
   }
}

ColumnIndexType GetProductLoadTableColumnCount(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,bool bDesign,bool bRating,
                                               bool *pbConstruction,bool* pbDeckPanels,bool* pbSidewalk,bool* pbShearKey,bool* pbPedLoading,bool* pbPermit,IntervalIndexType* pContinuityInterval,GroupIndexType* pStartGroup,GroupIndexType* pNGroups)
{
   ColumnIndexType nCols = 7; // location, girder, diaphragm, slab, slab pad, overlay, traffic barrier
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   *pbDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   *pbPermit     = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   *pStartGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   *pNGroups    = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount() : (*pStartGroup)+1 );

   CGirderKey key(*pStartGroup,girderKey.girderIndex);
   *pbPedLoading = pLoads->HasPedestrianLoad(key);
   *pbSidewalk   = pLoads->HasSidewalkLoad(key);
   *pbShearKey   = pLoads->HasShearKeyLoad(key);
   *pbConstruction = !IsZero(pUserLoads->GetConstructionLoad());

   // determine continuity stage
   EventIndexType continuityEventIdx = MAX_INDEX;
   PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(*pStartGroup);
   PierIndexType lastPierIdx  = pBridge->GetGirderGroupEndPier(*pStartGroup + *pNGroups - 1);
   for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++ )
   {
      EventIndexType left_event_index, right_event_index;
      pBridge->GetContinuityEventIndex(pierIdx,&left_event_index,&right_event_index);
      continuityEventIdx = _cpp_min(continuityEventIdx,left_event_index);
      continuityEventIdx = _cpp_min(continuityEventIdx,right_event_index);
   }
   *pContinuityInterval = pIntervals->GetInterval(continuityEventIdx);

   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();

   if ( *pbConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && continuityEventIdx == castDeckEventIdx)
         nCols += 2;
      else
         nCols++;
   }

   if ( *pbDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuityEventIdx == castDeckEventIdx)
         nCols += 2;
      else
         nCols++;
   }

   if ( analysisType == pgsTypes::Envelope && continuityEventIdx == castDeckEventIdx )
      nCols += 2; // add one more each for min/max slab and min/max slab pad

   if ( analysisType == pgsTypes::Envelope )
      nCols += 2; // add one more each for min/max overlay and min/max traffic barrier

   if ( bDesign )
   {
      nCols += 2; // design live loads
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         nCols += 2; // fatigue live load

      if ( *pbPermit )
         nCols += 2; // for permit live loads
   }

   if ( *pbPedLoading )
      nCols += 2;

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
rptRcTable* CProductMomentsTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                        bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue,     moment,   pDisplayUnits->GetMomentUnit(),     false );

   bool bConstruction, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bPermit;
   GroupIndexType startGroup, nGroups;
   IntervalIndexType continuity_interval;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&continuity_interval,&startGroup,&nGroups);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Moments"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   RowIndexType row = ConfigureProductLoadTableHeading<rptMomentUnitTag,unitmgtMomentData>(pBroker,p_table,false,false,bConstruction,bDeckPanels,bSidewalk,bShearKey,bDesign,bPedLoading,
                                                                                           bPermit,bRating,analysisType,continuity_interval,
                                                                                           pRatingSpec,pDisplayUnits,pDisplayUnits->GetMomentUnit());
   // Get the results
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstErectedSegmentInterval();

   for ( GroupIndexType grpIdx = startGroup; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = min(girderKey.girderIndex,nGirders-1);

      CSegmentKey allSegmentsKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey) );

      // Get the results for this span (it is faster to get them as a vector rather than individually)
      std::vector<Float64> girder    = pForces2->GetMoment(erectSegmentIntervalIdx, pftGirder,    vPoi, maxBAT);
      std::vector<Float64> diaphragm = pForces2->GetMoment(castDeckIntervalIdx,     pftDiaphragm, vPoi, maxBAT);

      std::vector<Float64> minSlab, maxSlab;
      std::vector<Float64> minSlabPad, maxSlabPad;
      maxSlab = pForces2->GetMoment( castDeckIntervalIdx, pftSlab, vPoi, maxBAT );
      minSlab = pForces2->GetMoment( castDeckIntervalIdx, pftSlab, vPoi, minBAT );

      maxSlabPad = pForces2->GetMoment( castDeckIntervalIdx, pftSlabPad, vPoi, maxBAT );
      minSlabPad = pForces2->GetMoment( castDeckIntervalIdx, pftSlabPad, vPoi, minBAT );

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         maxDeckPanel = pForces2->GetMoment( castDeckIntervalIdx, pftSlabPanel, vPoi, maxBAT );
         minDeckPanel = pForces2->GetMoment( castDeckIntervalIdx, pftSlabPanel, vPoi, minBAT );
      }

      std::vector<Float64> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         maxConstruction = pForces2->GetMoment( castDeckIntervalIdx, pftConstruction, vPoi, maxBAT );
         minConstruction = pForces2->GetMoment( castDeckIntervalIdx, pftConstruction, vPoi, minBAT );
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
         maxSidewalk = pForces2->GetMoment( railingSystemIntervalIdx, pftSidewalk, vPoi, maxBAT );
         minSidewalk = pForces2->GetMoment( railingSystemIntervalIdx, pftSidewalk, vPoi, minBAT );
      }

      if ( bShearKey )
      {
         maxShearKey = pForces2->GetMoment( castDeckIntervalIdx, pftShearKey, vPoi, maxBAT );
         minShearKey = pForces2->GetMoment( castDeckIntervalIdx, pftShearKey, vPoi, minBAT );
      }

      maxTrafficBarrier = pForces2->GetMoment( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, maxBAT );
      minTrafficBarrier = pForces2->GetMoment( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, minBAT );
      if ( overlayIntervalIdx != INVALID_INDEX )
      {
         maxOverlay = pForces2->GetMoment( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, maxBAT );
         minOverlay = pForces2->GetMoment( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, minBAT );
      }

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPoi, maxBAT, true, true, &dummy, &maxPedestrian );
         pForces2->GetLiveLoadMoment( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPoi, minBAT, true, true, &minPedestrian, &dummy );
      }

      if ( bDesign )
      {
         pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltFatigue, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermit, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltDesign, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Routine, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltLegalRating_Special, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Routine, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, vPoi, maxBAT, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
            pForces2->GetLiveLoadMoment( pgsTypes::lltPermitRating_Special, liveLoadIntervalIdx, vPoi, minBAT, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
         }
      }


      // write out the results
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      long index = 0;
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
         const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

         ColumnIndexType col = 0;

         Float64 end_size = pBridge->GetSegmentStartEndDistance(thisSegmentKey);

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
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
            if ( analysisType == pgsTypes::Envelope && continuity_interval == castDeckIntervalIdx )
            {
               (*p_table)(row,col++) << moment.SetValue( maxConstruction[index] );
               (*p_table)(row,col++) << moment.SetValue( minConstruction[index] );
            }
            else
            {
               (*p_table)(row,col++) << moment.SetValue( maxConstruction[index] );
            }
         }

         if ( analysisType == pgsTypes::Envelope && continuity_interval == castDeckIntervalIdx )
         {
            (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );
            (*p_table)(row,col++) << moment.SetValue( minSlab[index] );

            (*p_table)(row,col++) << moment.SetValue( maxSlabPad[index] );
            (*p_table)(row,col++) << moment.SetValue( minSlabPad[index] );
         }
         else
         {
            (*p_table)(row,col++) << moment.SetValue( maxSlab[index] );

            (*p_table)(row,col++) << moment.SetValue( maxSlabPad[index] );
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && continuity_interval == castDeckIntervalIdx )
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

         if ( bPedLoading )
         {
            (*p_table)(row,col++) << moment.SetValue( maxPedestrian[index] );
            (*p_table)(row,col++) << moment.SetValue( minPedestrian[index] );
         }

         if ( bDesign )
         {
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

