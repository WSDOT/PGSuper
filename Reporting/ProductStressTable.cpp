///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
rptRcTable* CProductStressTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                                       bool bDesign,bool bRating,IEAFDisplayUnits* pDisplayUnits,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS ? true : false);

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bDeckPanels = (pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP ? true : false);
   bool bHasOverlay    = pBridge->HasOverlay();
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroup = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroup   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroup);

   GET_IFACE2(pBroker,IProductForces,pForces);
   GET_IFACE2(pBroker,IProductForces2,pForces2);

   pgsTypes::BridgeAnalysisType maxBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   bool bPedLoading = pLoads->HasPedestrianLoad(girderKey);
   bool bSidewalk = pLoads->HasSidewalkLoad(girderKey);
   bool bShearKey = pLoads->HasShearKeyLoad(girderKey);

   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   pgsTypes::LossMethod loss_method = pLossParams->GetLossMethod();
   
   bool bSlabShrinkage = ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() && 
                         (loss_method == pgsTypes::AASHTO_REFINED || loss_method == pgsTypes::WSDOT_REFINED) ? true : false);
   if ( !bGirderStresses )
   {
      // assume deck shrinkage does not cause shrinkage stresses in the deck itself
      bSlabShrinkage = false;
   }

   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);
   bool bConstruction = !IsZero(pUserLoads->GetConstructionLoad());

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   EventIndexType continuityEventIdx = MAX_INDEX;
   PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(startGroup);
   PierIndexType lastPierIdx  = pBridge->GetGirderGroupEndPier(endGroup);
   for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++ )
   {
      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         EventIndexType leftContinuityEventIdx, rightContinuityEventIdx;
         pBridge->GetContinuityEventIndex(pierIdx,&leftContinuityEventIdx,&rightContinuityEventIdx);
         continuityEventIdx = Min(continuityEventIdx,leftContinuityEventIdx);
         continuityEventIdx = Min(continuityEventIdx,rightContinuityEventIdx);
      }
   }

   IntervalIndexType continuityIntervalIdx    = pIntervals->GetInterval(girderKey,continuityEventIdx);

   bool bContinuousBeforeDeckCasting;
   ColumnIndexType nCols = GetProductLoadTableColumnCount(pBroker,girderKey,analysisType,bDesign,bRating,bSlabShrinkage,&bConstruction,&bDeckPanels,&bSidewalk,&bShearKey,&bPedLoading,&bPermit,&bContinuousBeforeDeckCasting,&startGroup,&endGroup);

   std::_tstring strTitle(bGirderStresses ? _T("Girder Stresses") : _T("Deck Stresses"));
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,strTitle);

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   RowIndexType row = ConfigureProductLoadTableHeading<rptStressUnitTag,unitmgtStressData>(pBroker,p_table,false,bSlabShrinkage,bConstruction,bDeckPanels,bSidewalk,bShearKey,bHasOverlay,bFutureOverlay,bDesign,bPedLoading,bPermit,bRating,analysisType,bContinuousBeforeDeckCasting,pRatingSpec,pDisplayUnits,pDisplayUnits->GetStressUnit());


   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   for ( GroupIndexType grpIdx = startGroup; grpIdx <= endGroup; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      CSegmentKey allSegmentsKey(thisGirderKey,ALL_SEGMENTS);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(allSegmentsKey,POI_ERECTED_SEGMENT) );

      IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetLastSegmentErectionInterval(thisGirderKey);
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(thisGirderKey);
      IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval(thisGirderKey);
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval(thisGirderKey);
      IntervalIndexType loadRatingIntervalIdx    = pIntervals->GetLoadRatingInterval(thisGirderKey);


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

      pForces2->GetStress( erectSegmentIntervalIdx, pftGirder, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopGirder, &fBotGirder);
      pForces2->GetStress( castDeckIntervalIdx, pftDiaphragm, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopDiaphragm, &fBotDiaphragm);

      pForces2->GetStress( castDeckIntervalIdx, pftSlab, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxSlab, &fBotMaxSlab );
      pForces2->GetStress( castDeckIntervalIdx, pftSlab, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinSlab, &fBotMinSlab );

      pForces2->GetStress( castDeckIntervalIdx, pftSlabPad, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxSlabPad, &fBotMaxSlabPad );
      pForces2->GetStress( castDeckIntervalIdx, pftSlabPad, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinSlabPad, &fBotMinSlabPad );

      if ( bConstruction )
      {
         pForces2->GetStress( castDeckIntervalIdx, pftConstruction, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxConstruction, &fBotMaxConstruction );
         pForces2->GetStress( castDeckIntervalIdx, pftConstruction, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinConstruction, &fBotMinConstruction );
      }

      if ( bDeckPanels )
      {
         pForces2->GetStress( castDeckIntervalIdx, pftSlabPanel, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxSlabPanel, &fBotMaxSlabPanel );
         pForces2->GetStress( castDeckIntervalIdx, pftSlabPanel, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinSlabPanel, &fBotMinSlabPanel );
      }

      if ( bSidewalk )
      {
         pForces2->GetStress( railingSystemIntervalIdx, pftSidewalk, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxSidewalk, &fBotMaxSidewalk);
         pForces2->GetStress( railingSystemIntervalIdx, pftSidewalk, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinSidewalk, &fBotMinSidewalk);
      }

      if ( bShearKey )
      {
         pForces2->GetStress( castDeckIntervalIdx, pftShearKey, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxShearKey, &fBotMaxShearKey);
         pForces2->GetStress( castDeckIntervalIdx, pftShearKey, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinShearKey, &fBotMinShearKey);
      }

      pForces2->GetStress( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxTrafficBarrier, &fBotMaxTrafficBarrier);
      pForces2->GetStress( railingSystemIntervalIdx, pftTrafficBarrier, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinTrafficBarrier, &fBotMinTrafficBarrier);

      if ( bHasOverlay )
      {
         pForces2->GetStress( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, maxBAT, rtCumulative, topLocation, botLocation, &fTopMaxOverlay, &fBotMaxOverlay);
         pForces2->GetStress( overlayIntervalIdx, bRating && !bDesign ? pftOverlayRating : pftOverlay, vPoi, minBAT, rtCumulative, topLocation, botLocation, &fTopMinOverlay, &fBotMinOverlay);
      }

      if ( bPedLoading )
      {
         pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltPedestrian, vPoi, maxBAT, true, true, topLocation, botLocation, &dummy1, &fTopMaxPedestrianLL, &dummy2, &fBotMaxPedestrianLL);
         pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltPedestrian, vPoi, minBAT, true, true, topLocation, botLocation, &fTopMinPedestrianLL, &dummy1, &fBotMinPedestrianLL, &dummy2);
      }

      if ( bDesign )
      {
         pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxDesignLL, &dummy2, &fBotMaxDesignLL);
         pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinDesignLL, &dummy1, &fBotMinDesignLL, &dummy2);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltFatigue, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxFatigueLL, &dummy2, &fBotMaxFatigueLL);
            pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltFatigue, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinFatigueLL, &dummy1, &fBotMinFatigueLL, &dummy2);
         }

         if ( bPermit )
         {
            pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltPermit, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxPermitLL, &dummy2, &fBotMaxPermitLL);
            pForces2->GetLiveLoadStress(liveLoadIntervalIdx, pgsTypes::lltPermit, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinPermitLL, &dummy1, &fBotMinPermitLL, &dummy2);
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxDesignLL, &dummy2, &fBotMaxDesignLL);
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinDesignLL, &dummy1, &fBotMinDesignLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxLegalRoutineLL, &dummy2, &fBotMaxLegalRoutineLL);
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinLegalRoutineLL, &dummy1, &fBotMinLegalRoutineLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxLegalSpecialLL, &dummy2, &fBotMaxLegalSpecialLL);
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinLegalSpecialLL, &dummy1, &fBotMinLegalSpecialLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxPermitRoutineLL, &dummy2, &fBotMaxPermitRoutineLL);
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinPermitRoutineLL, &dummy1, &fBotMinPermitRoutineLL, &dummy2);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, maxBAT, true, false, topLocation, botLocation, &dummy1, &fTopMaxPermitSpecialLL, &dummy2, &fBotMaxPermitSpecialLL);
            pForces2->GetLiveLoadStress(loadRatingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, minBAT, true, false, topLocation, botLocation, &fTopMinPermitSpecialLL, &dummy1, &fBotMinPermitSpecialLL, &dummy2);
         }
      }

      // Fill up the table
      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
      
         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopGirder[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotGirder[index]);
         col++;

         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDiaphragm[index]) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDiaphragm[index]);
         col++;

         if ( bShearKey )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
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
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
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

         if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
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
            ATLASSERT(bGirderStresses); // slab shrinkage stresses only applicable to girder stresses
            Float64 ft_ss, fb_ss;
            pForces->GetDeckShrinkageStresses(poi,&ft_ss,&fb_ss);

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(ft_ss) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fb_ss);
            col++;
         }

         if ( bDeckPanels )
         {
            if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
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

            if ( bHasOverlay )
            {
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxOverlay[index]);
               col++;

               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinOverlay[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinOverlay[index]);
               col++;
            }
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

            if ( bHasOverlay )
            {
               (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxOverlay[index]) << rptNewLine;
               (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxOverlay[index]);
               col++;
            }
         }

         if ( bPedLoading )
         {
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPedestrianLL[index]);
            col++;

            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPedestrianLL[index]);
            col++;
         }

         if ( bDesign )
         {
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
      } // next poi
   } // next group

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
