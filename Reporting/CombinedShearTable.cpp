///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>

#include <IFace\Project.h>
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
   CCombinedShearTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CCombinedShearTable::CCombinedShearTable()
{
}

CCombinedShearTable::CCombinedShearTable(const CCombinedShearTable& rOther)
{
   MakeCopy(rOther);
}

CCombinedShearTable::~CCombinedShearTable()
{
}

//======================== OPERATORS  =======================================
CCombinedShearTable& CCombinedShearTable::operator= (const CCombinedShearTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CCombinedShearTable::Build(IBroker* pBroker,rptChapter* pChapter,
                                const CGirderKey& girderKey,
                                IEAFDisplayUnits* pDisplayUnits,
                                IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                                bool bDesign,bool bRating) const
{
   BuildCombinedDeadTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, bDesign, bRating);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      if (bDesign)
      {
         BuildCombinedLiveTable(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, true, false);
      }

      if (bRating)
      {
         BuildCombinedLiveTable(pBroker, pChapter, girderKey, pDisplayUnits, analysisType, false, true);
      }

      if (bDesign)
      {
         BuildLimitStateTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, true, false);
      }

      if (bRating)
      {
         BuildLimitStateTable(pBroker, pChapter, girderKey, pDisplayUnits, intervalIdx, analysisType, false, true);
      }
   }
}


void CCombinedShearTable::BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptRcTable* p_table = 0;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GroupIndexType startGroupIdx = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
   GroupIndexType endGroupIdx   = girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx;
   
   RowIndexType row = CreateCombinedDeadLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,pBroker,CGirderKey(startGroupIdx,girderKey.girderIndex),_T("Shear"),false,bRating,intervalIdx,
                                 analysisType,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   RowIndexType row2 = 1;

   // Get the interface pointers we need
   GET_IFACE2(pBroker,ICombinedForces2,pForces2);
   GET_IFACE2_NOCHECK(pBroker,ILimitStateForces2,pLsForces2); // only used if liveLoadIntervalIdx <= intervalIdx

   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // Fill up the table
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      PoiAttributeType poiRefAttribute;
      std::vector<pgsPointOfInterest> vPoi;
      GetCombinedResultsPoi(pBroker,thisGirderKey,intervalIdx,&vPoi,&poiRefAttribute);
      poiRefAttribute = (girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : poiRefAttribute);

      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minServiceI, maxServiceI;
      std::vector<sysSectionValue> minDCinc, maxDCinc;
      std::vector<sysSectionValue> minDCcum, maxDCcum;
      std::vector<sysSectionValue> minDWinc, maxDWinc;
      std::vector<sysSectionValue> minDWcum, maxDWcum;
      std::vector<sysSectionValue> minDWRatinginc, maxDWRatinginc;
      std::vector<sysSectionValue> minDWRatingcum, maxDWRatingcum;
      std::vector<sysSectionValue> minCRinc, maxCRinc;
      std::vector<sysSectionValue> minCRcum, maxCRcum;
      std::vector<sysSectionValue> minSHinc, maxSHinc;
      std::vector<sysSectionValue> minSHcum, maxSHcum;
      std::vector<sysSectionValue> minREinc, maxREinc;
      std::vector<sysSectionValue> minREcum, maxREcum;
      std::vector<sysSectionValue> minPSinc, maxPSinc;
      std::vector<sysSectionValue> minPScum, maxPScum;

      if ( minBAT == maxBAT )
      {
         maxDCinc = pForces2->GetShear( intervalIdx, lcDC, vPoi, maxBAT, rtIncremental );
         minDCinc = maxDCinc;

         maxDWinc = pForces2->GetShear( intervalIdx, lcDW, vPoi, maxBAT, rtIncremental );
         minDWinc = maxDWinc;

         if ( bRating )
         {
            maxDWRatinginc = pForces2->GetShear( intervalIdx, lcDWRating, vPoi, maxBAT, rtIncremental );
            minDWRatinginc = maxDWRatinginc;
         }

         maxDCcum = pForces2->GetShear( intervalIdx, lcDC, vPoi, maxBAT, rtCumulative );
         minDCcum = maxDCcum;

         maxDWcum = pForces2->GetShear( intervalIdx, lcDW, vPoi, maxBAT, rtCumulative );
         minDWcum = maxDWcum;

         if ( bRating )
         {
            maxDWRatingcum = pForces2->GetShear( intervalIdx, lcDWRating, vPoi, maxBAT, rtCumulative );
            minDWRatingcum = maxDWRatingcum;
         }

         if ( bTimeStepMethod )
         {
            maxCRinc = pForces2->GetShear( intervalIdx, lcCR, vPoi, maxBAT, rtIncremental );
            minCRinc = maxCRinc;

            maxSHinc = pForces2->GetShear( intervalIdx, lcSH, vPoi, maxBAT, rtIncremental );
            minSHinc = maxSHinc;

            maxREinc = pForces2->GetShear( intervalIdx, lcRE, vPoi, maxBAT, rtIncremental );
            minREinc = maxREinc;

            maxPSinc = pForces2->GetShear( intervalIdx, lcPS, vPoi, maxBAT, rtIncremental );
            minPSinc = maxPSinc;

            maxCRcum = pForces2->GetShear( intervalIdx, lcCR, vPoi, maxBAT, rtCumulative );
            minCRcum = maxCRcum;

            maxSHcum = pForces2->GetShear( intervalIdx, lcSH, vPoi, maxBAT, rtCumulative );
            minSHcum = maxSHcum;

            maxREcum = pForces2->GetShear( intervalIdx, lcRE, vPoi, maxBAT, rtCumulative );
            minREcum = maxREcum;

            maxPScum = pForces2->GetShear( intervalIdx, lcPS, vPoi, maxBAT, rtCumulative );
            minPScum = minPScum;
         }

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI, vPoi, maxBAT, &minServiceI, &maxServiceI );
         }
      }
      else
      {
         maxDCinc = pForces2->GetShear( intervalIdx, lcDC, vPoi, maxBAT, rtIncremental );
         minDCinc = pForces2->GetShear( intervalIdx, lcDC, vPoi, minBAT, rtIncremental );
         maxDWinc = pForces2->GetShear( intervalIdx, lcDW, vPoi, maxBAT, rtIncremental );
         minDWinc = pForces2->GetShear( intervalIdx, lcDW, vPoi, minBAT, rtIncremental );

         if ( bRating )
         {
            maxDWRatinginc = pForces2->GetShear( intervalIdx, lcDWRating, vPoi, maxBAT, rtIncremental );
            minDWRatinginc = pForces2->GetShear( intervalIdx, lcDWRating, vPoi, minBAT, rtIncremental );
         }

         maxDCcum = pForces2->GetShear( intervalIdx, lcDC, vPoi, maxBAT, rtCumulative );
         minDCcum = pForces2->GetShear( intervalIdx, lcDC, vPoi, minBAT, rtCumulative );
         maxDWcum = pForces2->GetShear( intervalIdx, lcDW, vPoi, maxBAT, rtCumulative );
         minDWcum = pForces2->GetShear( intervalIdx, lcDW, vPoi, minBAT, rtCumulative );

         if(bRating)
         {
            maxDWRatingcum = pForces2->GetShear( intervalIdx, lcDWRating, vPoi, maxBAT, rtCumulative );
            minDWRatingcum = pForces2->GetShear( intervalIdx, lcDWRating, vPoi, minBAT, rtCumulative );
         }

         if ( bTimeStepMethod )
         {
            maxCRinc = pForces2->GetShear( intervalIdx, lcCR, vPoi, maxBAT, rtIncremental );
            minCRinc = pForces2->GetShear( intervalIdx, lcCR, vPoi, minBAT, rtIncremental );
            maxSHinc = pForces2->GetShear( intervalIdx, lcSH, vPoi, maxBAT, rtIncremental );
            minSHinc = pForces2->GetShear( intervalIdx, lcSH, vPoi, minBAT, rtIncremental );
            maxREinc = pForces2->GetShear( intervalIdx, lcRE, vPoi, maxBAT, rtIncremental );
            minREinc = pForces2->GetShear( intervalIdx, lcRE, vPoi, minBAT, rtIncremental );
            maxPSinc = pForces2->GetShear( intervalIdx, lcPS, vPoi, maxBAT, rtIncremental );
            minPSinc = pForces2->GetShear( intervalIdx, lcPS, vPoi, minBAT, rtIncremental );

            maxCRcum = pForces2->GetShear( intervalIdx, lcCR, vPoi, maxBAT, rtCumulative );
            minCRcum = pForces2->GetShear( intervalIdx, lcCR, vPoi, minBAT, rtCumulative );
            maxSHcum = pForces2->GetShear( intervalIdx, lcSH, vPoi, maxBAT, rtCumulative );
            minSHcum = pForces2->GetShear( intervalIdx, lcSH, vPoi, minBAT, rtCumulative );
            maxREcum = pForces2->GetShear( intervalIdx, lcRE, vPoi, maxBAT, rtCumulative );
            minREcum = pForces2->GetShear( intervalIdx, lcRE, vPoi, minBAT, rtCumulative );
            maxPScum = pForces2->GetShear( intervalIdx, lcPS, vPoi, maxBAT, rtCumulative );
            minPScum = pForces2->GetShear( intervalIdx, lcPS, vPoi, minBAT, rtCumulative );
         }

         if ( intervalIdx < liveLoadIntervalIdx )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI, vPoi, maxBAT, &dummy, &maxServiceI );
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI, vPoi, minBAT, &minServiceI, &dummy );
         }
      }

      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
         const CSegmentKey& thisSegmentKey = poi.GetSegmentKey();

         IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
         IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(thisSegmentKey);
         IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(thisSegmentKey);

         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( poiRefAttribute, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
            (*p_table)(row,col++) << shear.SetValue( minDCinc[index] );
            (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );
            (*p_table)(row,col++) << shear.SetValue( minDWinc[index] );

            if ( bRating )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDWRatinginc[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWRatinginc[index] );
            }

            if ( bTimeStepMethod )
            {
               (*p_table)(row,col++) << shear.SetValue( maxCRinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minCRinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxSHinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minSHinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxREinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minREinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxPSinc[index] );
               (*p_table)(row,col++) << shear.SetValue( minPSinc[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
            (*p_table)(row,col++) << shear.SetValue( minDCcum[index] );
            (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );
            (*p_table)(row,col++) << shear.SetValue( minDWcum[index] );

            if ( bRating )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDWRatingcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minDWRatingcum[index] );
            }

            if ( bTimeStepMethod )
            {
               (*p_table)(row,col++) << shear.SetValue( maxCRcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minCRcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxSHcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minSHcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxREcum[index] );
               (*p_table)(row,col++) << shear.SetValue( minREcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxPScum[index] );
               (*p_table)(row,col++) << shear.SetValue( minPScum[index] );
            }

            if ( intervalIdx < liveLoadIntervalIdx )
            {
               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table)(row,col++) << shear.SetValue( minServiceI[index] );
            }
         }
         else
         {
            (*p_table)(row,col++) << shear.SetValue( maxDCinc[index] );
            (*p_table)(row,col++) << shear.SetValue( maxDWinc[index] );

            if ( bRating )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDWRatinginc[index] );
            }

            if ( bTimeStepMethod )
            {
               (*p_table)(row,col++) << shear.SetValue( maxCRinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxSHinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxREinc[index] );
               (*p_table)(row,col++) << shear.SetValue( maxPSinc[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxDCcum[index] );
            (*p_table)(row,col++) << shear.SetValue( maxDWcum[index] );

            if ( bRating )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDWRatingcum[index] );
            }

            if ( bTimeStepMethod )
            {
               (*p_table)(row,col++) << shear.SetValue( maxCRcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxSHcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxREcum[index] );
               (*p_table)(row,col++) << shear.SetValue( maxPScum[index] );
            }

            if ( intervalIdx < liveLoadIntervalIdx )
            {
               (*p_table)(row,col++) << shear.SetValue( maxServiceI[index] );
            }
         }
         row++;
      }
   }
}

void CCombinedShearTable::BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
 
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   LPCTSTR strLabel = bDesign ? _T("Shear - Design Vehicles") : _T("Shear - Rating Vehicles");

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table;
   RowIndexType Nhrows = CreateCombinedLiveLoadingTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table,strLabel,false,bDesign,bPermit,bPedLoading,bRating,false,true,
                           analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());

   RowIndexType row = Nhrows;

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
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // Fill up the table
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType liveLoadInteravlIdx = pIntervals->GetLiveLoadInterval();

      PoiAttributeType poiRefAttribute;
      std::vector<pgsPointOfInterest> vPoi;
      GetCombinedResultsPoi(pBroker,thisGirderKey,liveLoadInteravlIdx,&vPoi,&poiRefAttribute);
      poiRefAttribute = (girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : poiRefAttribute);

      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minPedestrianLL,    maxPedestrianLL;
      std::vector<sysSectionValue> minDesignLL,        maxDesignLL;
      std::vector<sysSectionValue> minFatigueLL,       maxFatigueLL;
      std::vector<sysSectionValue> minPermitLL,        maxPermitLL;
      std::vector<sysSectionValue> minLegalRoutineLL,  maxLegalRoutineLL;
      std::vector<sysSectionValue> minLegalSpecialLL, maxLegalSpecialLL;
      std::vector<sysSectionValue> minLegalEmergencyLL, maxLegalEmergencyLL;
      std::vector<sysSectionValue> minPermitRoutineLL, maxPermitRoutineLL;
      std::vector<sysSectionValue> minPermitSpecialLL, maxPermitSpecialLL;

      if ( bPedLoading )
      {
         pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltPedestrian, vPoi, maxBAT, true, &dummy, &maxPedestrianLL );
         pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltPedestrian, vPoi, minBAT, true, &minPedestrianLL, &dummy );
      }

      if ( bDesign )
      {
         pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, &dummy, &maxDesignLL );
         pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltDesign, vPoi, minBAT, true, &minDesignLL, &dummy );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltFatigue, vPoi, maxBAT, true, &dummy, &maxFatigueLL );
            pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltFatigue, vPoi, minBAT, true, &minFatigueLL, &dummy );
         }

         if ( bPermit )
         {
            pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltPermit, vPoi, maxBAT, true, &dummy, &maxPermitLL );
            pForces2->GetCombinedLiveLoadShear( liveLoadInteravlIdx, pgsTypes::lltPermit, vPoi, minBAT, true, &minPermitLL, &dummy );
         }
      }

      if ( bRating )
      {
         IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();

         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory)) )
         {
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltDesign, vPoi, maxBAT, true, &dummy, &maxDesignLL );
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltDesign, vPoi, minBAT, true, &minDesignLL, &dummy );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, maxBAT, true, &dummy, &maxLegalRoutineLL );
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltLegalRating_Routine, vPoi, minBAT, true, &minLegalRoutineLL, &dummy );
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pForces2->GetCombinedLiveLoadShear(ratingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, maxBAT, true, &dummy, &maxLegalSpecialLL);
            pForces2->GetCombinedLiveLoadShear(ratingIntervalIdx, pgsTypes::lltLegalRating_Special, vPoi, minBAT, true, &minLegalSpecialLL, &dummy);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pForces2->GetCombinedLiveLoadShear(ratingIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, maxBAT, true, &dummy, &maxLegalEmergencyLL);
            pForces2->GetCombinedLiveLoadShear(ratingIntervalIdx, pgsTypes::lltLegalRating_Emergency, vPoi, minBAT, true, &minLegalEmergencyLL, &dummy);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, maxBAT, true, &dummy, &maxPermitRoutineLL );
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltPermitRating_Routine, vPoi, minBAT, true, &minPermitRoutineLL, &dummy );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, maxBAT, true, &dummy, &maxPermitSpecialLL );
            pForces2->GetCombinedLiveLoadShear( ratingIntervalIdx, pgsTypes::lltPermitRating_Special, vPoi, minBAT, true, &minPermitSpecialLL, &dummy );
         }
      }

      // fill table (first half if design/ped load)
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      IndexType index = 0;
      ColumnIndexType col = 0;
      RowIndexType row2 = row;
      for ( ; i != end; i++, index++ )
      {
         const pgsPointOfInterest& poi = *i;
         col = 0;

         (*p_table)(row,col++) << location.SetValue( poiRefAttribute, poi );

         if ( bDesign )
         {
            if ( bPedLoading )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPedestrianLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPedestrianLL[index] );
            }

            (*p_table)(row,col++) << shear.SetValue( maxDesignLL[index] );
            (*p_table)(row,col++) << shear.SetValue( minDesignLL[index] );
         
            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row,col++) << shear.SetValue( maxFatigueLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minFatigueLL[index] );
            }

            if ( bPermit )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPermitLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPermitLL[index] );
            }
         }

         if ( bRating )
         {
            if ( bPedLoading && pRatingSpec->IncludePedestrianLiveLoad() )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPedestrianLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPedestrianLL[index] );
            }

            if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            {
               (*p_table)(row,col++) << shear.SetValue( maxDesignLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minDesignLL[index] );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            {
               (*p_table)(row,col++) << shear.SetValue( maxLegalRoutineLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minLegalRoutineLL[index] );
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
            {
               (*p_table)(row, col++) << shear.SetValue(maxLegalSpecialLL[index]);
               (*p_table)(row, col++) << shear.SetValue(minLegalSpecialLL[index]);
            }

            if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
            {
               (*p_table)(row, col++) << shear.SetValue(maxLegalEmergencyLL[index]);
               (*p_table)(row, col++) << shear.SetValue(minLegalEmergencyLL[index]);
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPermitRoutineLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPermitRoutineLL[index] );
            }

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            {
               (*p_table)(row,col++) << shear.SetValue( maxPermitSpecialLL[index] );
               (*p_table)(row,col++) << shear.SetValue( minPermitSpecialLL[index] );
            }
         }

         row++;
      }

      // fill second half of table if design/ped load
      if ( bDesign && bPedLoading )
      {
         // Sum or envelope pedestrian values with live loads to give final LL

         GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
         ILiveLoads::PedestrianLoadApplicationType DesignPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign);
         ILiveLoads::PedestrianLoadApplicationType FatiguePedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue);
         ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);

         SumPedAndLiveLoad(DesignPedLoad, minDesignLL, maxDesignLL, minPedestrianLL, maxPedestrianLL);

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            SumPedAndLiveLoad(FatiguePedLoad, minFatigueLL, maxFatigueLL, minPedestrianLL, maxPedestrianLL);
         }

         if ( bPermit )
         {
            SumPedAndLiveLoad(PermitPedLoad, minPermitLL, maxPermitLL, minPedestrianLL, maxPedestrianLL);
         }

         // Now we can fill table
         ColumnIndexType recCol = col;
         IndexType psiz = (IndexType)vPoi.size();
         for ( index=0; index<psiz; index++ )
         {
            col = recCol;

            (*p_table)(row2,col++) << shear.SetValue( maxDesignLL[index] );
            (*p_table)(row2,col++) << shear.SetValue( minDesignLL[index] );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*p_table)(row2,col++) << shear.SetValue( maxFatigueLL[index] );
               (*p_table)(row2,col++) << shear.SetValue( minFatigueLL[index] );
            }

            if ( bPermit )
            {
               (*p_table)(row2,col++) << shear.SetValue( maxPermitLL[index] );
               (*p_table)(row2,col++) << shear.SetValue( minPermitLL[index] );
            }

            row2++;
         }

         // footnotes for pedestrian loads
         if ( grpIdx == startGroupIdx )
         {
            int lnum=1;
            *pNote<< lnum++ << PedestrianFootnote(DesignPedLoad) << rptNewLine;

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               *pNote << lnum++ << PedestrianFootnote(FatiguePedLoad) << rptNewLine;
            }

            if ( bPermit )
            {
               *pNote << lnum++ << PedestrianFootnote(PermitPedLoad) << rptNewLine;
            }
         }
      }

      if ( bRating && pRatingSpec->IncludePedestrianLiveLoad())
      {
         // Note for rating and pedestrian load
         *pNote << _T("$ Pedestrian load results will be summed with vehicular load results at time of load combination.");
      }
   }
}

void CCombinedShearTable::BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                                         const CGirderKey& girderKey,
                                         IEAFDisplayUnits* pDisplayUnits,
                                         IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                                         bool bDesign,bool bRating) const
{
   ATLASSERT(!(bDesign&&bRating)); // these are separate tables, can't do both

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceSectionValue, shear, pDisplayUnits->GetShearUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
 
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);

   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);
   bool bPedLoading = pProductLoads->HasPedestrianLoad(girderKey);

   rptRcTable* p_table2;
   RowIndexType row2 = CreateLimitStateTableHeading<rptForceUnitTag,unitmgtForceData>(&p_table2,_T("Shear, Vu"),false,bDesign,bPermit,bRating,false,analysisType,pRatingSpec,pDisplayUnits,pDisplayUnits->GetShearUnit());
   *p << p_table2;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table2->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table2->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,ILimitStateForces2,pLsForces2);
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

   // Get data and fill up the table
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      PoiAttributeType poiRefAttribute;
      std::vector<pgsPointOfInterest> vPoi;
      GetCombinedResultsPoi(pBroker,thisGirderKey,intervalIdx,&vPoi,&poiRefAttribute);
      poiRefAttribute = (girderKey.groupIndex == ALL_GROUPS ? POI_SPAN : poiRefAttribute);

      // create second table for BSS3 Limit states
      std::vector<sysSectionValue> dummy;
      std::vector<sysSectionValue> minServiceI,   maxServiceI;
      std::vector<sysSectionValue> minServiceIA,  maxServiceIA;
      std::vector<sysSectionValue> minServiceIII, maxServiceIII;
      std::vector<sysSectionValue> minFatigueI,   maxFatigueI;
      std::vector<sysSectionValue> minStrengthI,  maxStrengthI;
      std::vector<sysSectionValue> minStrengthII, maxStrengthII;
      std::vector<sysSectionValue> minStrengthI_Inventory, maxStrengthI_Inventory;
      std::vector<sysSectionValue> minStrengthI_Operating, maxStrengthI_Operating;
      std::vector<sysSectionValue> minStrengthI_Legal_Routine, maxStrengthI_Legal_Routine;
      std::vector<sysSectionValue> minStrengthI_Legal_Special, maxStrengthI_Legal_Special;
      std::vector<sysSectionValue> minStrengthI_Legal_Emergency, maxStrengthI_Legal_Emergency;
      std::vector<sysSectionValue> minStrengthII_Permit_Routine, maxStrengthII_Permit_Routine;
      std::vector<sysSectionValue> minServiceI_Permit_Routine, maxServiceI_Permit_Routine;
      std::vector<sysSectionValue> minStrengthII_Permit_Special, maxStrengthII_Permit_Special;
      std::vector<sysSectionValue> minServiceI_Permit_Special, maxServiceI_Permit_Special;

      if ( bDesign )
      {
         pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI, vPoi, maxBAT, &dummy, &maxServiceI );
         pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI, vPoi, minBAT, &minServiceI, &dummy );

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceIA, vPoi, maxBAT, &dummy, &maxServiceIA );
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceIA, vPoi, minBAT, &minServiceIA, &dummy );
         }
         else
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::FatigueI, vPoi, maxBAT, &dummy, &maxFatigueI );
            pLsForces2->GetShear( intervalIdx, pgsTypes::FatigueI, vPoi, minBAT, &minFatigueI, &dummy );
         }

         pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceIII, vPoi, maxBAT, &dummy, &maxServiceIII );
         pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceIII, vPoi, minBAT, &minServiceIII, &dummy );

         pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI, vPoi, maxBAT, &dummy, &maxStrengthI );
         pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI, vPoi, minBAT, &minStrengthI, &dummy );

         if ( bPermit )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthII, vPoi, maxBAT, &dummy, &maxStrengthII );
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthII, vPoi, minBAT, &minStrengthII, &dummy );
         }
      }

      if ( bRating )
      {
         // Design - Inventory
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI_Inventory, vPoi, maxBAT, &dummy, &maxStrengthI_Inventory );
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI_Inventory, vPoi, minBAT, &minStrengthI_Inventory, &dummy );
         }

         // Design - Operating
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI_Operating, vPoi, maxBAT, &dummy, &maxStrengthI_Operating );
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI_Operating, vPoi, minBAT, &minStrengthI_Operating, &dummy );
         }

         // Legal - Routine
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI_LegalRoutine, vPoi, maxBAT, &dummy, &maxStrengthI_Legal_Routine );
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthI_LegalRoutine, vPoi, minBAT, &minStrengthI_Legal_Routine, &dummy );
         }

         // Legal - Special
         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
         {
            pLsForces2->GetShear(intervalIdx, pgsTypes::StrengthI_LegalSpecial, vPoi, maxBAT, &dummy, &maxStrengthI_Legal_Special);
            pLsForces2->GetShear(intervalIdx, pgsTypes::StrengthI_LegalSpecial, vPoi, minBAT, &minStrengthI_Legal_Special, &dummy);
         }

         // Legal - Emergency
         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
         {
            pLsForces2->GetShear(intervalIdx, pgsTypes::StrengthI_LegalEmergency, vPoi, maxBAT, &dummy, &maxStrengthI_Legal_Emergency);
            pLsForces2->GetShear(intervalIdx, pgsTypes::StrengthI_LegalEmergency, vPoi, minBAT, &minStrengthI_Legal_Emergency, &dummy);
         }

         // Permit Rating - Routine
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthII_PermitRoutine, vPoi, maxBAT, &dummy, &maxStrengthII_Permit_Routine );
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthII_PermitRoutine, vPoi, minBAT, &minStrengthII_Permit_Routine, &dummy );
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI_PermitRoutine,   vPoi, maxBAT, &dummy, &maxServiceI_Permit_Routine );
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI_PermitRoutine,   vPoi, minBAT, &minServiceI_Permit_Routine, &dummy );
         }

         // Permit Rating - Special
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthII_PermitSpecial, vPoi, maxBAT, &dummy, &maxStrengthII_Permit_Special );
            pLsForces2->GetShear( intervalIdx, pgsTypes::StrengthII_PermitSpecial, vPoi, minBAT, &minStrengthII_Permit_Special, &dummy );
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI_PermitSpecial,   vPoi, maxBAT, &dummy, &maxServiceI_Permit_Special );
            pLsForces2->GetShear( intervalIdx, pgsTypes::ServiceI_PermitSpecial,   vPoi, minBAT, &minServiceI_Permit_Special, &dummy );
         }
      }

      // Fill table
      IndexType index = 0;
      std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      for ( ; i != end; i++, index++ )
      {
         ColumnIndexType col = 0;

         const pgsPointOfInterest& poi = *i;

         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());

         (*p_table2)(row2,col++) << location.SetValue( poiRefAttribute, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            if ( bDesign )
            {
               (*p_table2)(row2,col++) << shear.SetValue( maxServiceI[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minServiceI[index] );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceIA[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minServiceIA[index] );
               }

               (*p_table2)(row2,col++) << shear.SetValue( maxServiceIII[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minServiceIII[index] );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxFatigueI[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minFatigueI[index] );
               }

               (*p_table2)(row2,col++) << shear.SetValue( maxStrengthI[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minStrengthI[index] );

               if ( bPermit )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxStrengthII[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minStrengthII[index] );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Inventory[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Inventory[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Operating[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Operating[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Legal_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Legal_Routine[index]);
               }

               if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
               {
                  (*p_table2)(row2, col++) << shear.SetValue(maxStrengthI_Legal_Special[index]);
                  (*p_table2)(row2, col++) << shear.SetValue(minStrengthI_Legal_Special[index]);
               }

               if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
               {
                  (*p_table2)(row2, col++) << shear.SetValue(maxStrengthI_Legal_Emergency[index]);
                  (*p_table2)(row2, col++) << shear.SetValue(minStrengthI_Legal_Emergency[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Routine[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Special[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Special[index]);
               }
            }
         }
         else
         {
            if ( bDesign )
            {
               (*p_table2)(row2,col++) << shear.SetValue( maxServiceI[index] );

               if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
                  (*p_table2)(row2,col++) << shear.SetValue( maxServiceIA[index] );

               (*p_table2)(row2,col++) << shear.SetValue( maxServiceIII[index] );

               if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
                  (*p_table2)(row2,col++) << shear.SetValue( maxFatigueI[index] );

               (*p_table2)(row2,col++) << shear.SetValue( maxStrengthI[index] );
               (*p_table2)(row2,col++) << shear.SetValue( minStrengthI[index] );

               if ( bPermit )
               {
                  (*p_table2)(row2,col++) << shear.SetValue( maxStrengthII[index] );
                  (*p_table2)(row2,col++) << shear.SetValue( minStrengthII[index] );
               }
            }

            if ( bRating )
            {
               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Inventory[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Inventory[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Operating[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Operating[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthI_Legal_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthI_Legal_Routine[index]);
               }

               if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
               {
                  (*p_table2)(row2, col++) << shear.SetValue(maxStrengthI_Legal_Special[index]);
                  (*p_table2)(row2, col++) << shear.SetValue(minStrengthI_Legal_Special[index]);
               }

               if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
               {
                  (*p_table2)(row2, col++) << shear.SetValue(maxStrengthI_Legal_Special[index]);
                  (*p_table2)(row2, col++) << shear.SetValue(minStrengthI_Legal_Special[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Routine[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Routine[index]);
               }

               if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               {
                  (*p_table2)(row2,col++) << shear.SetValue(maxStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minStrengthII_Permit_Special[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(maxServiceI_Permit_Special[index]);
                  (*p_table2)(row2,col++) << shear.SetValue(minServiceI_Permit_Special[index]);
               }
            }
         }

         row2++;
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CCombinedShearTable::MakeCopy(const CCombinedShearTable& rOther)
{
   // Add copy code here...
}

void CCombinedShearTable::MakeAssignment(const CCombinedShearTable& rOther)
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
bool CCombinedShearTable::AssertValid() const
{
   return true;
}

void CCombinedShearTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedShearTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CCombinedShearTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CCombinedShearTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CCombinedShearTable");

   TESTME_EPILOG("CCombinedShearTable");
}
#endif // _UNITTEST
