///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

   case pgsTypes::lltLegalRating_Emergency:
      strPrefix = _T("E");
      break;

   case pgsTypes::lltPermitRating_Routine:
      strPrefix = _T("PR");
      break;

   case pgsTypes::lltPermitRating_Special:
      strPrefix = _T("PS");
      break;

   default:
      ATLASSERT(false);
   }

   return strPrefix;
}

void LiveLoadTableFooter(IBroker* pBroker,rptParagraph* pPara,const CGirderKey& girderKey,bool bDesign,bool bRating)
{
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
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
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

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

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltLegalRating_Emergency, girderKey);
         j = 0;
         for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++)
         {
            *pPara << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << j << _T(") ") << *iter << rptNewLine;
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

ColumnIndexType GetProductLoadTableColumnCount(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,bool bDesign,bool bRating,bool bSlabShrinkage,
                                               bool* pbSegments,bool *pbConstruction,bool* pbDeck,bool* pbDeckPanels,bool* pbSidewalk,bool* pbShearKey,bool* pbLongitudinalJoint,bool* pbPedLoading,bool* pbPermit,bool* pbContinuousBeforeDeckCasting,GroupIndexType* pStartGroup,GroupIndexType* pEndGroup)
{
   ColumnIndexType nCols = 4; // location, girder, diaphragm, and traffic barrier

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);

   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   *pbDeck = false;
   if (deckType != pgsTypes::sdtNone)
   {
      *pbDeck = true;
      nCols += 2; // slab + slab pad
      if (bSlabShrinkage)
      {
         nCols++;
      }
   }

   *pbDeckPanels = (deckType == pgsTypes::sdtCompositeSIP ? true : false);
   *pbPermit     = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   *pStartGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   *pEndGroup   = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : *pStartGroup );

   CGirderKey key(*pStartGroup,girderKey.girderIndex);
   *pbSegments = (1 < pBridge->GetSegmentCount(key) ? true : false);
   *pbPedLoading = pLoads->HasPedestrianLoad(key);
   *pbSidewalk   = pLoads->HasSidewalkLoad(key);
   *pbShearKey   = pLoads->HasShearKeyLoad(key);
   *pbLongitudinalJoint = pLoads->HasLongitudinalJointLoad();
   *pbConstruction = !IsZero(pUserLoads->GetConstructionLoad());

   if ( *pbSegments )
   {
      nCols++;
      ATLASSERT(analysisType == pgsTypes::Continuous);
   }

   if ( pBridge->HasOverlay() )
   {
      nCols++;

      if ( analysisType == pgsTypes::Envelope )
         nCols++;
   }

   // determine continuity stage
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType continuityIntervalIdx = MAX_INDEX;
   PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(*pStartGroup);
   PierIndexType lastPierIdx  = pBridge->GetGirderGroupEndPier(*pEndGroup);
   for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++ )
   {
      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         IntervalIndexType left_interval_index, right_interval_index;
         pIntervals->GetContinuityInterval(pierIdx,&left_interval_index,&right_interval_index);
         continuityIntervalIdx = Min(continuityIntervalIdx,left_interval_index);
         continuityIntervalIdx = Min(continuityIntervalIdx,right_interval_index);
      }
   }

   IntervalIndexType firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   if (*pbDeck)
   {
      *pbContinuousBeforeDeckCasting = (continuityIntervalIdx <= firstCastDeckIntervalIdx) ? true : false;
   }
   else
   {
      *pbContinuousBeforeDeckCasting = false;
   }

   if ( *pbConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && *pbContinuousBeforeDeckCasting == true)
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }

   if ( *pbDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && *pbContinuousBeforeDeckCasting == true)
      {
         nCols += 2;
      }
      else
      {
         nCols++;
      }
   }

   if ( *pbDeck && analysisType == pgsTypes::Envelope && *pbContinuousBeforeDeckCasting == true)
   {
      nCols += 2; // add one more each for min/max slab and min/max slab pad
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      nCols ++; // add for min/max traffic barrier
   }

   if ( bDesign )
   {
      nCols += 2; // design live loads
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         nCols += 2; // fatigue live load
      }

      if ( *pbPermit )
      {
         nCols += 2; // for permit live loads
      }
   }

   if ( *pbPedLoading )
   {
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

   if (*pbLongitudinalJoint)
   {
      if (analysisType == pgsTypes::Envelope && *pbContinuousBeforeDeckCasting == true)
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
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory)) )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         nCols += 2;
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
      {
         nCols += 2;
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         nCols += 2;
      }
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

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasOverlay = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   bool bSegments, bConstruction, bDeck, bDeckPanels, bPedLoading, bSidewalk, bShearKey, bLongitudinalJoint, bPermit;
   bool bContinuousBeforeDeckCasting;
   GroupIndexType startGroup, endGroup;

   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,false,&bSegments,&bConstruction,&bDeck,&bDeckPanels,&bSidewalk,&bShearKey,&bLongitudinalJoint,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T("Moments"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);
   PoiAttributeType poiRefAttribute(girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : POI_ERECTED_SEGMENT);

   RowIndexType row = ConfigureProductLoadTableHeading<rptMomentUnitTag,WBFL::Units::MomentData>(pBroker,p_table,false,false,bSegments,bConstruction,bDeck,bDeckPanels,bSidewalk,bShearKey,bLongitudinalJoint,bHasOverlay,bFutureOverlay,bDesign,bPedLoading,
                                                                                           bPermit,bRating,analysisType,bContinuousBeforeDeckCasting,
                                                                                           pRatingSpec,pDisplayUnits,pDisplayUnits->GetMomentUnit());
   // Get the results
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey.girderIndex, startGroup, endGroup, &vGirderKeys);
   for (const auto& thisGirderKey : vGirderKeys)
   {
      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);

      CSegmentKey allSegmentsKey(thisGirderKey,ALL_SEGMENTS);
      PoiList vPoi;
      pIPoi->GetPointsOfInterest(allSegmentsKey, poiRefAttribute, &vPoi);

      // Add PSXFER poi's (but only at the ends... don't need them all from debonding)
      PoiList vPoi2;
      pIPoi->GetPointsOfInterest(allSegmentsKey, POI_PSXFER, &vPoi2);
      if (0 < vPoi2.size())
      {
         vPoi.push_back(vPoi2.front());
         vPoi.push_back(vPoi2.back());
         pIPoi->SortPoiList(&vPoi);
      }

      PoiList csPois;
      pIPoi->GetCriticalSections(pgsTypes::StrengthI, thisGirderKey, &csPois);
      pIPoi->MergePoiLists(vPoi, csPois, &vPoi);

      std::vector<Float64> segment;
      std::vector<Float64> girder;
      if ( bSegments )
      {
         segment = pForces2->GetMoment(erectSegmentIntervalIdx, pgsTypes::pftGirder,    vPoi, maxBAT, rtCumulative);
         girder  = pForces2->GetMoment(lastIntervalIdx,         pgsTypes::pftGirder,    vPoi, maxBAT, rtCumulative);
      }
      else
      {
         girder = pForces2->GetMoment(erectSegmentIntervalIdx, pgsTypes::pftGirder,    vPoi, maxBAT, rtCumulative);
      }
      std::vector<Float64> diaphragm = pForces2->GetMoment(lastIntervalIdx,     pgsTypes::pftDiaphragm, vPoi, maxBAT, rtCumulative);

      std::vector<Float64> minSlab, maxSlab;
      std::vector<Float64> minSlabPad, maxSlabPad;
      if (bDeck)
      {
         maxSlab = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftSlab, vPoi, maxBAT, rtCumulative);
         minSlab = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftSlab, vPoi, minBAT, rtCumulative);

         maxSlabPad = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftSlabPad, vPoi, maxBAT, rtCumulative);
         minSlabPad = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftSlabPad, vPoi, minBAT, rtCumulative);
      }

      std::vector<Float64> minDeckPanel, maxDeckPanel;
      if ( bDeckPanels )
      {
         maxDeckPanel = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftSlabPanel, vPoi, maxBAT, rtCumulative );
         minDeckPanel = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftSlabPanel, vPoi, minBAT, rtCumulative );
      }

      std::vector<Float64> minConstruction, maxConstruction;
      if ( bConstruction )
      {
         maxConstruction = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftConstruction, vPoi, maxBAT, rtCumulative );
         minConstruction = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftConstruction, vPoi, minBAT, rtCumulative );
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
         maxSidewalk = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftSidewalk, vPoi, maxBAT, rtCumulative );
         minSidewalk = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftSidewalk, vPoi, minBAT, rtCumulative );
      }

      if (bShearKey)
      {
         maxShearKey = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftShearKey, vPoi, maxBAT, rtCumulative);
         minShearKey = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftShearKey, vPoi, minBAT, rtCumulative);
      }

      if (bLongitudinalJoint)
      {
         maxLongitudinalJoint = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, vPoi, maxBAT, rtCumulative);
         minLongitudinalJoint = pForces2->GetMoment(lastIntervalIdx, pgsTypes::pftLongitudinalJoint, vPoi, minBAT, rtCumulative);
      }

      maxTrafficBarrier = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftTrafficBarrier, vPoi, maxBAT, rtCumulative );
      minTrafficBarrier = pForces2->GetMoment( lastIntervalIdx, pgsTypes::pftTrafficBarrier, vPoi, minBAT, rtCumulative );
      if ( bHasOverlay )
      {
         maxOverlay = pForces2->GetMoment( lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, vPoi, maxBAT, rtCumulative );
         minOverlay = pForces2->GetMoment( lastIntervalIdx, bRating && !bDesign ? pgsTypes::pftOverlayRating : pgsTypes::pftOverlay, vPoi, minBAT, rtCumulative );
      }

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPedestrian, vPoi, maxBAT, true, true, &dummy, &maxPedestrian );
         pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPedestrian, vPoi, minBAT, true, true, &minPedestrian, &dummy );
      }

      if ( bDesign )
      {
         pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
         pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltFatigue, vPoi, maxBAT, true, false, &dummy, &maxFatigueLL, &dummyTruck, &maxFatigueLLtruck );
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltFatigue, vPoi, minBAT, true, false, &minFatigueLL, &dummy, &minFatigueLLtruck, &dummyTruck );
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPermit, vPoi, maxBAT, true, false, &dummy, &maxPermitLL, &dummyTruck, &maxPermitLLtruck );
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPermit, vPoi, minBAT, true, false, &minPermitLL, &dummy, &minPermitLLtruck, &dummyTruck );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, &dummy, &maxDesignLL, &dummyTruck, &maxDesignLLtruck );
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, &minDesignLL, &dummy, &minDesignLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxLegalRoutineLL, &dummyTruck, &maxLegalRoutineLLtruck );
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, minBAT, true, false, &minLegalRoutineLL, &dummy, &minLegalRoutineLLtruck, &dummyTruck );
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pForces2->GetLiveLoadMoment(lastIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, maxBAT, true, false, &dummy, &maxLegalSpecialLL, &dummyTruck, &maxLegalSpecialLLtruck);
            pForces2->GetLiveLoadMoment(lastIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, minBAT, true, false, &minLegalSpecialLL, &dummy, &minLegalSpecialLLtruck, &dummyTruck);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pForces2->GetLiveLoadMoment(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, maxBAT, true, false, &dummy, &maxLegalEmergencyLL, &dummyTruck, &maxLegalEmergencyLLtruck);
            pForces2->GetLiveLoadMoment(lastIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, minBAT, true, false, &minLegalEmergencyLL, &dummy, &minLegalEmergencyLLtruck, &dummyTruck);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, maxBAT, true, false, &dummy, &maxPermitRoutineLL, &dummyTruck, &maxPermitRoutineLLtruck );
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, minBAT, true, false, &minPermitRoutineLL, &dummy, &minPermitRoutineLLtruck, &dummyTruck );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, maxBAT, true, false, &dummy, &maxPermitSpecialLL, &dummyTruck, &maxPermitSpecialLLtruck );
            pForces2->GetLiveLoadMoment( lastIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, minBAT, true, false, &minPermitSpecialLL, &dummy, &minPermitSpecialLLtruck, &dummyTruck );
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
            (*p_table)(row, col++) << moment.SetValue(segment[index]);
         }

         (*p_table)(row, col++) << moment.SetValue(girder[index]);

         (*p_table)(row, col++) << moment.SetValue(diaphragm[index]);

         if (bShearKey)
         {
            if (analysisType == pgsTypes::Envelope)
            {
               (*p_table)(row, col++) << moment.SetValue(maxShearKey[index]);
               (*p_table)(row, col++) << moment.SetValue(minShearKey[index]);
            }
            else
            {
               (*p_table)(row, col++) << moment.SetValue(maxShearKey[index]);
            }
         }

         if (bLongitudinalJoint)
         {
            if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row, col++) << moment.SetValue(maxLongitudinalJoint[index]);
               (*p_table)(row, col++) << moment.SetValue(minLongitudinalJoint[index]);
            }
            else
            {
               (*p_table)(row, col++) << moment.SetValue(maxLongitudinalJoint[index]);
            }
         }

         if (bConstruction)
         {
            if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row, col++) << moment.SetValue(maxConstruction[index]);
               (*p_table)(row, col++) << moment.SetValue(minConstruction[index]);
            }
            else
            {
               (*p_table)(row, col++) << moment.SetValue(maxConstruction[index]);
            }
         }

         if (bDeck)
         {
            if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
            {
               (*p_table)(row, col++) << moment.SetValue(maxSlab[index]);
               (*p_table)(row, col++) << moment.SetValue(minSlab[index]);

               (*p_table)(row, col++) << moment.SetValue(maxSlabPad[index]);
               (*p_table)(row, col++) << moment.SetValue(minSlabPad[index]);
            }
            else
            {
               (*p_table)(row, col++) << moment.SetValue(maxSlab[index]);
               (*p_table)(row, col++) << moment.SetValue(maxSlabPad[index]);
            }
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
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

            if ( bHasOverlay && overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
               (*p_table)(row,col++) << moment.SetValue( minOverlay[index] );
            }
         }
         else
         {
            if ( bSidewalk )
            {
               (*p_table)(row,col++) << moment.SetValue( maxSidewalk[index] );
            }

            (*p_table)(row,col++) << moment.SetValue( maxTrafficBarrier[index] );

            if ( bHasOverlay && overlayIntervalIdx != INVALID_INDEX )
            {
               (*p_table)(row,col++) << moment.SetValue( maxOverlay[index] );
            }
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
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
            }

            col++;

            (*p_table)(row,col) << moment.SetValue( minDesignLL[index] );
            
            if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
            {
               (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
            }

            col++;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col) << moment.SetValue( maxFatigueLL[index] );

               if ( bIndicateControllingLoad && 0 < maxFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << maxFatigueLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minFatigueLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minFatigueLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltFatigue) << minFatigueLLtruck[index] << _T(")");
               }

               col++;
            }

            if ( bPermit )
            {
               (*p_table)(row,col) << moment.SetValue( maxPermitLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermit) << maxPermitLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minPermitLL[index] );
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
               (*p_table)(row,col) << moment.SetValue( maxDesignLL[index] );

               if ( bIndicateControllingLoad && 0 < maxDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << maxDesignLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minDesignLL[index] );
               
               if ( bIndicateControllingLoad && 0 < minDesignLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltDesign) << minDesignLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col) << moment.SetValue( maxLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << maxLegalRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minLegalRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Routine) << minLegalRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            {
               (*p_table)(row,col) << moment.SetValue( maxLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxLegalSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << maxLegalSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minLegalSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < minLegalSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Special) << minLegalSpecialLLtruck[index] << _T(")");
               }

               col++;
            }

            // Legal - Emergency
            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               (*p_table)(row, col) << moment.SetValue(maxLegalEmergencyLL[index]);
               if (bIndicateControllingLoad && 0 < maxLegalEmergencyLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << maxLegalEmergencyLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row, col) << moment.SetValue(minLegalEmergencyLL[index]);
               if (bIndicateControllingLoad && 0 < minLegalEmergencyLLtruck.size())
               {
                  (*p_table)(row, col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltLegalRating_Emergency) << minLegalEmergencyLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Routine
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col) << moment.SetValue( maxPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << maxPermitRoutineLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minPermitRoutineLL[index] );
               if ( bIndicateControllingLoad && 0 < minPermitRoutineLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Routine) << minPermitRoutineLLtruck[index] << _T(")");
               }

               col++;
            }

            // Permit Rating - Special
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col) << moment.SetValue( maxPermitSpecialLL[index] );
               if ( bIndicateControllingLoad && 0 < maxPermitSpecialLLtruck.size() )
               {
                  (*p_table)(row,col) << rptNewLine << _T("(") << LiveLoadPrefix(pgsTypes::lltPermitRating_Special) << maxPermitSpecialLLtruck[index] << _T(")");
               }

               col++;

               (*p_table)(row,col) << moment.SetValue( minPermitSpecialLL[index] );
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

void CProductMomentsTable::MakeCopy(const CProductMomentsTable& rOther)
{
   // Add copy code here...
}

void CProductMomentsTable::MakeAssignment(const CProductMomentsTable& rOther)
{
   MakeCopy( rOther );
}

