///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\CombinedStressTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
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
   CCombinedStressTable
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedStressTable::CCombinedStressTable()
{
}

CCombinedStressTable::CCombinedStressTable(const CCombinedStressTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedStressTable::~CCombinedStressTable()
{
}

//======================== OPERATORS  =======================================
CCombinedStressTable& CCombinedStressTable::operator= (const CCombinedStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedStressTable::Build(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating,bool bGirderStresses) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval();

#if defined _DEBUG
   if ( !bGirderStresses )
   {
      // only report deck stresses after the deck is composite
      IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
      ATLASSERT(lastCompositeDeckIntervalIdx <= intervalIdx);
   }
#endif

   BuildCombinedDeadTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, bDesign, bRating, bGirderStresses);

   if (liveLoadIntervalIdx <= intervalIdx)
   {
      if (bDesign)
      {
         BuildCombinedLiveTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, true, false, bGirderStresses);
      }

      if (bRating)
      {
         BuildCombinedLiveTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, false, true, bGirderStresses);
      }

      if (bDesign)
      {
         BuildLimitStateTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, true, false, bGirderStresses);
      }

      if (bRating)
      {
         BuildLimitStateTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, false, true, bGirderStresses);
      }
   }
}

void CCombinedStressTable::BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   IntervalIndexType continuityIntervalIndex = MAX_INDEX;
   PierIndexType firstPierIdx = pBridge->GetGirderGroupStartPier(firstGroupIdx);
   PierIndexType lastPierIdx  = pBridge->GetGirderGroupEndPier(lastGroupIdx);
   for (PierIndexType pierIdx = firstPierIdx; pierIdx <= lastPierIdx; pierIdx++ )
   {
      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         IntervalIndexType leftContinuityIntervalIdx, rightContinuityIntervalIdx;
         pIntervals->GetContinuityInterval(pierIdx,&leftContinuityIntervalIdx,&rightContinuityIntervalIdx);
         continuityIntervalIndex = Min(continuityIntervalIndex,leftContinuityIntervalIdx);
         continuityIntervalIndex = Min(continuityIntervalIndex,rightContinuityIntervalIdx);
      }
   }

   // Set up table headings
   ColumnIndexType nCols = (bTimeStepMethod ? 13 : 5);
   if ( bRating )
   {
      nCols += 2;
   }

   std::_tstring strTitle(bGirderStresses ? _T("Girder Stresses") : _T("Deck Stresses"));
   pTable = rptStyleManager::CreateDefaultTable(nCols,strTitle);

   ColumnIndexType col  = 0;

   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << COLHDR(_T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0,col++) << COLHDR(_T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
   if ( bRating )
   {
      (*pTable)(0,col++) << COLHDR(_T("DW") << rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }
   if ( bTimeStepMethod )
   {
      (*pTable)(0,col++) << COLHDR(_T("CR"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << COLHDR(_T("SH"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << COLHDR(_T("RE"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(0,col++) << COLHDR(_T("PS"),          rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }
   (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("DC"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("DW"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( bRating )
   {
      (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("DW") << rptNewLine << _T("Rating"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }
   if ( bTimeStepMethod )
   {
      (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("CR"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("SH"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("RE"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pTable)(0,col++) << COLHDR(symbol(SUM) << _T("PS"),          rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *p << pTable << rptNewLine;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      std::vector<Float64> fTopDCinc, fBotDCinc;
      std::vector<Float64> fTopDWinc, fBotDWinc;
      std::vector<Float64> fTopDWRatinginc, fBotDWRatinginc;
      std::vector<Float64> fTopCRinc, fBotCRinc;
      std::vector<Float64> fTopSHinc, fBotSHinc;
      std::vector<Float64> fTopREinc, fBotREinc;
      std::vector<Float64> fTopPSinc, fBotPSinc;
      std::vector<Float64> fTopDCcum, fBotDCcum;
      std::vector<Float64> fTopDWcum, fBotDWcum;
      std::vector<Float64> fTopDWRatingcum, fBotDWRatingcum;
      std::vector<Float64> fTopCRcum, fBotCRcum;
      std::vector<Float64> fTopSHcum, fBotSHcum;
      std::vector<Float64> fTopREcum, fBotREcum;
      std::vector<Float64> fTopPScum, fBotPScum;

      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      PoiAttributeType poiRefAttribute;
      PoiList vPoi;
      GetCombinedResultsPoi(pBroker,thisGirderKey,intervalIdx,&vPoi,&poiRefAttribute);
      poiRefAttribute = (girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : poiRefAttribute);

      pForces2->GetStress( intervalIdx, lcDC, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopDCinc, &fBotDCinc);
      pForces2->GetStress( intervalIdx, lcDW, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopDWinc, &fBotDWinc);
      if ( bRating )
      {
         pForces2->GetStress( intervalIdx, lcDWRating, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopDWRatinginc, &fBotDWRatinginc);
      }
      pForces2->GetStress( intervalIdx, lcDC, vPoi, bat, rtCumulative, topLocation, botLocation, &fTopDCcum, &fBotDCcum);
      pForces2->GetStress( intervalIdx, lcDW, vPoi, bat, rtCumulative, topLocation, botLocation, &fTopDWcum, &fBotDWcum);
      if ( bRating )
      {
         pForces2->GetStress( intervalIdx, lcDWRating, vPoi, bat, rtCumulative, topLocation, botLocation, &fTopDWRatingcum, &fBotDWRatingcum);
      }

      if ( bTimeStepMethod )
      {
         pForces2->GetStress( intervalIdx, lcCR, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopCRinc, &fBotCRinc);
         pForces2->GetStress( intervalIdx, lcCR, vPoi, bat, rtCumulative,  topLocation, botLocation, &fTopCRcum, &fBotCRcum);
         pForces2->GetStress( intervalIdx, lcSH, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopSHinc, &fBotSHinc);
         pForces2->GetStress( intervalIdx, lcSH, vPoi, bat, rtCumulative,  topLocation, botLocation, &fTopSHcum, &fBotSHcum);
         pForces2->GetStress( intervalIdx, lcRE, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopREinc, &fBotREinc);
         pForces2->GetStress( intervalIdx, lcRE, vPoi, bat, rtCumulative,  topLocation, botLocation, &fTopREcum, &fBotREcum);
         pForces2->GetStress( intervalIdx, lcPS, vPoi, bat, rtIncremental, topLocation, botLocation, &fTopPSinc, &fBotPSinc);
         pForces2->GetStress( intervalIdx, lcPS, vPoi, bat, rtCumulative,  topLocation, botLocation, &fTopPScum, &fBotPScum);
      }

      // Fill up the table
      RowIndexType row = pTable->GetNumberOfHeaderRows();

      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

         col = 0;

         (*pTable)(row,col++) << location.SetValue( poiRefAttribute, poi );
         (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCinc[index]) << rptNewLine;
         (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCinc[index]);

         (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWinc[index]) << rptNewLine;
         (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWinc[index]);

         if ( bRating )
         {
            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWRatinginc[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWRatinginc[index]);
         }

         if ( bTimeStepMethod )
         {
            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopCRinc[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotCRinc[index]);

            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopSHinc[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotSHinc[index]);

            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopREinc[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotREinc[index]);

            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopPSinc[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotPSinc[index]);
         }

         (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDCcum[index]) << rptNewLine;
         (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDCcum[index]);

         (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWcum[index]) << rptNewLine;
         (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWcum[index]);

         if ( bRating )
         {
            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopDWRatingcum[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotDWRatingcum[index]);
         }

         if ( bTimeStepMethod )
         {
            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopCRcum[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotCRcum[index]);

            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopSHcum[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotSHcum[index]);

            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopREcum[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotREcum[index]);

            (*pTable)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopPScum[index]) << rptNewLine;
            (*pTable)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotPScum[index]);
         }

         row++;
         index++;
      }
   } // next group
}

void CCombinedStressTable::BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS ? true : false);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
 
   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads); // not used if bRating = true and bDesign = false
   bool bPedLoading = bDesign && pProductLoads->HasPedestrianLoad(girderKey) || 
                      bRating && pRatingSpec->IncludePedestrianLiveLoad();

   bool bPermit = false;// never for stress

   std::_tstring strBasicTitle;
   if ( bGirderStresses )
   {
      strBasicTitle = (bDesign ? _T("Girder Stresses - Design Vehicles") : _T("Girder Stresses - Rating Vehicles"));
   }
   else
   {
      strBasicTitle = (bDesign ? _T("Deck Stresses - Design Vehicles") : _T("Deck Stresses - Rating Vehicles"));
   }

   CString strTitle;
   strTitle.Format(_T("%s - Interval %d %s"),strBasicTitle.c_str(),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptStressUnitTag,WBFL::Units::StressData>(&p_table,strTitle,false,bDesign,bPermit,bPedLoading,bRating,true,true,
                           analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetStressUnit());
   *p << p_table;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   rptParagraph* pNote = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pNote;
   *pNote << LIVELOAD_PER_GIRDER << rptNewLine;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      PoiAttributeType poiRefAttribute;
      PoiList vPoi;
      GetCombinedResultsPoi(pBroker,thisGirderKey,intervalIdx,&vPoi,&poiRefAttribute);
      poiRefAttribute = (girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : poiRefAttribute);

      std::vector<Float64> fTopMinPedestrianLL,   fBotMinPedestrianLL;
      std::vector<Float64> fTopMaxPedestrianLL,   fBotMaxPedestrianLL;
      std::vector<Float64> fTopMinDesignLL,       fBotMinDesignLL;
      std::vector<Float64> fTopMaxDesignLL,       fBotMaxDesignLL;
      std::vector<Float64> fTopMinFatigueLL,      fBotMinFatigueLL;
      std::vector<Float64> fTopMaxFatigueLL,      fBotMaxFatigueLL;
      std::vector<Float64> fTopMinLegalRoutineLL, fBotMinLegalRoutineLL;
      std::vector<Float64> fTopMaxLegalRoutineLL, fBotMaxLegalRoutineLL;
      std::vector<Float64> fTopMinLegalSpecialLL, fBotMinLegalSpecialLL;
      std::vector<Float64> fTopMaxLegalSpecialLL, fBotMaxLegalSpecialLL;
      std::vector<Float64> fTopMinLegalEmergencyLL, fBotMinLegalEmergencyLL;
      std::vector<Float64> fTopMaxLegalEmergencyLL, fBotMaxLegalEmergencyLL;

      // Bridge site 3
      if ( bPedLoading )
      {
         pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltPedestrian, vPoi, bat, topLocation, botLocation, &fTopMinPedestrianLL, &fTopMaxPedestrianLL, &fBotMinPedestrianLL, &fBotMaxPedestrianLL );
      }

      if ( bDesign )
      {
         pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltDesign, vPoi, bat, topLocation, botLocation, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltFatigue, vPoi, bat, topLocation, botLocation, &fTopMinFatigueLL, &fTopMaxFatigueLL, &fBotMinFatigueLL, &fBotMaxFatigueLL );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltDesign, vPoi, bat, topLocation, botLocation, &fTopMinDesignLL, &fTopMaxDesignLL, &fBotMinDesignLL, &fBotMaxDesignLL );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, bat, topLocation, botLocation, &fTopMinLegalRoutineLL, &fTopMaxLegalRoutineLL, &fBotMinLegalRoutineLL, &fBotMaxLegalRoutineLL );
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltLegalRating_Special, vPoi, bat, topLocation, botLocation, &fTopMinLegalSpecialLL, &fTopMaxLegalSpecialLL, &fBotMinLegalSpecialLL, &fBotMaxLegalSpecialLL);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pForces2->GetCombinedLiveLoadStress(intervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, bat, topLocation, botLocation, &fTopMinLegalEmergencyLL, &fTopMaxLegalEmergencyLL, &fBotMinLegalEmergencyLL, &fBotMaxLegalEmergencyLL);
         }
      }

      // Fill up the table
      RowIndexType row = Nhrows;
      ColumnIndexType col = 0;
      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         const CSegmentKey& thisSegmentKey(poi.GetSegmentKey());

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

         col = 0;

         (*p_table)(row,col++) << location.SetValue( poiRefAttribute, poi );

         if ( bPedLoading )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxPedestrianLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxPedestrianLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinPedestrianLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinPedestrianLL[index]);
         }

         if ( bDesign )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueLL[index]);
            }
         }

         if ( bRating )
         {
            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalRoutineLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalRoutineLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalRoutineLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalRoutineLL[index]);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalSpecialLL[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalSpecialLL[index]);

               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalSpecialLL[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalSpecialLL[index]);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxLegalEmergencyLL[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxLegalEmergencyLL[index]);

               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinLegalEmergencyLL[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinLegalEmergencyLL[index]);
            }
         }

         row++;
         index++;
      }


      // fill second half of table if design & ped load
      if ( bDesign && bPedLoading )
      {
         // Sum or envelope pedestrian values with live loads to give final LL

         GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
         ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
         ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);
         ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);

         SumPedAndLiveLoad(DesignPedLoad, fTopMinDesignLL, fTopMaxDesignLL, fTopMinPedestrianLL, fTopMaxPedestrianLL);
         SumPedAndLiveLoad(DesignPedLoad, fBotMinDesignLL, fBotMaxDesignLL, fBotMinPedestrianLL, fBotMaxPedestrianLL);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            SumPedAndLiveLoad(FatiguePedLoad, fTopMinFatigueLL, fTopMaxFatigueLL, fTopMinPedestrianLL, fTopMaxPedestrianLL);
            SumPedAndLiveLoad(FatiguePedLoad, fBotMinFatigueLL, fBotMaxFatigueLL, fBotMinPedestrianLL, fBotMaxPedestrianLL);
         }

         // Now we can fill table
         RowIndexType    row = Nhrows;
         ColumnIndexType recCol = col;
         IndexType psiz = (IndexType)vPoi.size();
         for ( index=0; index<psiz; index++ )
         {
            col = recCol;

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxDesignLL[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinDesignLL[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinDesignLL[index]);

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueLL[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueLL[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueLL[index]);
            }

            row++;
         }

         // footnotes for pedestrian loads
         int lnum=1;
         *pNote<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            *pNote << lnum++ << PedestrianFootnote(FatiguePedLoad) << rptNewLine;
         }
      }
   } // next group

   if ( bRating && pRatingSpec->IncludePedestrianLiveLoad())
   {
      // Note for rating and pedestrian load
      *pNote << _T("$ Pedestrian load results will be summed with vehicular load results at time of load combination.");
   }
}

void CCombinedStressTable::BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,IntervalIndexType intervalIdx,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   // NOTE - Stregth II stresses not reported because they aren't used for anything

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS ? true : false);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);

   GET_IFACE2_NOCHECK(pBroker,IRatingSpecification,pRatingSpec); // only used if bRating is true

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
 
   GET_IFACE2_NOCHECK(pBroker,IProductLoads,pProductLoads); // not used if bRating = true and bDesign = false
   bool bPedLoading = bDesign && pProductLoads->HasPedestrianLoad(girderKey) || 
                      bRating && pRatingSpec->IncludePedestrianLiveLoad();

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // create second table for BSS3 Limit states
   p = new rptParagraph;
   *pChapter << p;

   ColumnIndexType nCols = 1; 
   if ( bDesign )
   {
      nCols += 6;
   }

   if ( bRating )
   {
      if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
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
   }

   std::_tstring strTitle(bGirderStresses ? _T("Girder Stresses") : _T("Deck Stresses"));
   p_table = rptStyleManager::CreateDefaultTable(nCols,strTitle);

   *p << p_table;

   p_table->SetNumberOfHeaderRows(3);

   ColumnIndexType col = 0;
   p_table->SetRowSpan(0,col,3);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   if ( bDesign )
   {
      p_table->SetColumnSpan(0,col,6);
      (*p_table)(0,col) << _T("Design");
      p_table->SetColumnSpan(1,col,2);
      (*p_table)(1,col) << _T("Service I");
      (*p_table)(2,col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(2,col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      
      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         p_table->SetColumnSpan(1,col,2);
         (*p_table)(1,col) << _T("Service IA");
         (*p_table)(2,col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(2,col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      
      p_table->SetColumnSpan(1,col,2);
      (*p_table)(1,col) << _T("Service III");
      (*p_table)(2,col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(2,col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         p_table->SetColumnSpan(1,col,2);
         (*p_table)(1,col) << _T("Fatigue I");
         (*p_table)(2,col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(2,col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }

   if ( bRating )
   {
      ColumnIndexType colSpan = 0;

      if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory)  )
      {
         colSpan += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         colSpan += 2;
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
      {
         colSpan += 2;
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         colSpan += 2;
      }

      if ( 0 < colSpan )
      {
         p_table->SetColumnSpan(0,col,colSpan);
         (*p_table)(0,col) << _T("Rating");

         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            p_table->SetColumnSpan(1,col,2);
            (*p_table)(1,col) << _T("Service III") << rptNewLine << _T("Inventory");
            (*p_table)(2,col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(2,col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            p_table->SetColumnSpan(1,col,2);
            (*p_table)(1,col) << _T("Service III") << rptNewLine << _T("Legal Routine");
            (*p_table)(2,col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
            (*p_table)(2,col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Service III") << rptNewLine << _T("Legal Special");
            (*p_table)(2, col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            (*p_table)(2, col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            p_table->SetColumnSpan(1, col, 2);
            (*p_table)(1, col) << _T("Service III") << rptNewLine << _T("Legal Emergency");
            (*p_table)(2, col++) << COLHDR(_T("Max"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            (*p_table)(2, col++) << COLHDR(_T("Min"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         }
      }
   }

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      std::vector<Float64> fTopMinServiceI,   fBotMinServiceI;
      std::vector<Float64> fTopMaxServiceI,   fBotMaxServiceI;
      std::vector<Float64> fTopMinServiceIA,  fBotMinServiceIA;
      std::vector<Float64> fTopMaxServiceIA,  fBotMaxServiceIA;
      std::vector<Float64> fTopMinServiceIII, fBotMinServiceIII;
      std::vector<Float64> fTopMaxServiceIII, fBotMaxServiceIII;
      std::vector<Float64> fTopMinFatigueI,   fBotMinFatigueI;
      std::vector<Float64> fTopMaxFatigueI,   fBotMaxFatigueI;
      std::vector<Float64> fTopMinServiceIII_Inventory, fBotMinServiceIII_Inventory;
      std::vector<Float64> fTopMaxServiceIII_Inventory, fBotMaxServiceIII_Inventory;
      std::vector<Float64> fTopMinServiceIII_Routine,   fBotMinServiceIII_Routine;
      std::vector<Float64> fTopMaxServiceIII_Routine,   fBotMaxServiceIII_Routine;
      std::vector<Float64> fTopMinServiceIII_Special,   fBotMinServiceIII_Special;
      std::vector<Float64> fTopMaxServiceIII_Special,   fBotMaxServiceIII_Special;
      std::vector<Float64> fTopMinServiceIII_Emergency, fBotMinServiceIII_Emergency;
      std::vector<Float64> fTopMaxServiceIII_Emergency, fBotMaxServiceIII_Emergency;

      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      PoiAttributeType poiRefAttribute;
      PoiList vPoi;
      GetCombinedResultsPoi(pBroker,thisGirderKey,intervalIdx,&vPoi,&poiRefAttribute);
      poiRefAttribute = (girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : poiRefAttribute);

      if ( bDesign )
      {
         pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceI, vPoi, bat, false, topLocation, &fTopMinServiceI, &fTopMaxServiceI);
         pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceI, vPoi, bat, false, botLocation, &fBotMinServiceI, &fBotMaxServiceI);

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIA, vPoi, bat, false, topLocation, &fTopMinServiceIA, &fTopMaxServiceIA);
            pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIA, vPoi, bat, false, botLocation, &fBotMinServiceIA, &fBotMaxServiceIA);
         }
         else
         {
            pLsForces2->GetStress( intervalIdx, pgsTypes::FatigueI, vPoi, bat, false, topLocation, &fTopMinFatigueI, &fTopMaxFatigueI);
            pLsForces2->GetStress( intervalIdx, pgsTypes::FatigueI, vPoi, bat, false, botLocation, &fBotMinFatigueI, &fBotMaxFatigueI);
         }

         pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIII, vPoi, bat, false, topLocation, &fTopMinServiceIII, &fTopMaxServiceIII);
         pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIII, vPoi, bat, false, botLocation, &fBotMinServiceIII, &fBotMaxServiceIII);
      }

      if ( bRating )
      {
         if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIII_Inventory, vPoi, bat, false, topLocation, &fTopMinServiceIII_Inventory, &fTopMaxServiceIII_Inventory);
            pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIII_Inventory, vPoi, bat, false, botLocation, &fBotMinServiceIII_Inventory, &fBotMaxServiceIII_Inventory);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIII_LegalRoutine, vPoi, bat, false, topLocation, &fTopMinServiceIII_Routine, &fTopMaxServiceIII_Routine);
            pLsForces2->GetStress( intervalIdx, pgsTypes::ServiceIII_LegalRoutine, vPoi, bat, false, botLocation, &fBotMinServiceIII_Routine, &fBotMaxServiceIII_Routine);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pLsForces2->GetStress(intervalIdx, pgsTypes::ServiceIII_LegalSpecial, vPoi, bat, false, topLocation, &fTopMinServiceIII_Special, &fTopMaxServiceIII_Special);
            pLsForces2->GetStress(intervalIdx, pgsTypes::ServiceIII_LegalSpecial, vPoi, bat, false, botLocation, &fBotMinServiceIII_Special, &fBotMaxServiceIII_Special);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pLsForces2->GetStress(intervalIdx, pgsTypes::ServiceIII_LegalEmergency, vPoi, bat, false, topLocation, &fTopMinServiceIII_Emergency, &fTopMaxServiceIII_Emergency);
            pLsForces2->GetStress(intervalIdx, pgsTypes::ServiceIII_LegalEmergency, vPoi, bat, false, botLocation, &fBotMinServiceIII_Emergency, &fBotMaxServiceIII_Emergency);
         }
      }

      RowIndexType row = p_table->GetNumberOfHeaderRows();

      IndexType index = 0;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;
         const CSegmentKey& thisSegmentKey(poi.GetSegmentKey());
         
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

         (*p_table)(row,col++) << location.SetValue( poiRefAttribute, poi );

         if ( bDesign )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceI[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceI[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceI[index]);

            if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIA[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIA[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIA[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIA[index]);
            }

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII[index]);

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII[index]) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII[index]);

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxFatigueI[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxFatigueI[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinFatigueI[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinFatigueI[index]);
            }
         }

         if ( bRating )
         {
            if ( !bDesign && pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Inventory[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Inventory[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Inventory[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Inventory[index]);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Routine[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Routine[index]);

               (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Routine[index]) << rptNewLine;
               (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Routine[index]);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Special[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Special[index]);

               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Special[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Special[index]);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMaxServiceIII_Emergency[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMaxServiceIII_Emergency[index]);

               (*p_table)(row, col) << RPT_FTOP << _T(" = ") << stress.SetValue(fTopMinServiceIII_Emergency[index]) << rptNewLine;
               (*p_table)(row, col++) << RPT_FBOT << _T(" = ") << stress.SetValue(fBotMinServiceIII_Emergency[index]);
            }
         }

         row++;
         index++;
      }
   } // next group
}

void CCombinedStressTable::MakeCopy(const CCombinedStressTable& rOther)
{
   // Add copy code here...
}

void CCombinedStressTable::MakeAssignment(const CCombinedStressTable& rOther)
{
   MakeCopy( rOther );
}
