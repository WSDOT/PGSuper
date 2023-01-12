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

#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductAxialTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\CombinedAxialTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ConcurrentShearTable.h>
#include <Reporting\UserAxialTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Allowables.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\RatingLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CMVRChapterBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CMVRChapterBuilder::CMVRChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CMVRChapterBuilder::GetName() const
{
   return TEXT("Moments, Shears, and Reactions");
}

rptChapter* CMVRChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptParagraph* p = nullptr;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bDesign = m_bDesign;
   bool bRating = m_bRating;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();
   bool bReportAxial = pProductLoads->ReportAxialResults();

   bool bIndicateControllingLoad = true;

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);

   GET_IFACE2( pBroker, ILibrary, pLib );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   // Casting Yard Results
   std::vector<CGirderKey> vGirderKeys;
   if ( bDesign )
   {
      GET_IFACE2(pBroker, IBridge, pBridge);

      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *p << _T("Load Responses - Casting Yard")<<rptNewLine;
      p->SetName(_T("Casting Yard Results"));
      *pChapter << p;

      pBridge->GetGirderline(girderKey, &vGirderKeys);
      for(const auto& thisGirderKey : vGirderKeys)
      {
         rptRcTable* pReleaseLayoutTable = nullptr;
         rptRcTable* pStorageLayoutTable = nullptr;
         rptRcTable* pLayoutTable        = nullptr;

         SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
         if ( 1 < nSegments )
         {
            pReleaseLayoutTable = rptStyleManager::CreateLayoutTable(nSegments,_T("Girder Dead Load Moment/Shear at Prestress Release"));
            *p << pReleaseLayoutTable << rptNewLine;

            pStorageLayoutTable = rptStyleManager::CreateLayoutTable(nSegments,_T("Girder Dead Load Moment/Shear during Storage"));
            *p << pStorageLayoutTable << rptNewLine;
         }
         else
         {
            pLayoutTable = rptStyleManager::CreateLayoutTable(2);
            *p << pLayoutTable << rptNewLine;
         }

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(thisGirderKey,segIdx);

            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

            if ( pLayoutTable )
            {
               (*pLayoutTable)(0,0) << CCastingYardMomentsTable().Build(pBroker,segmentKey,releaseIntervalIdx,POI_RELEASED_SEGMENT,_T("At Release"),    pDisplayUnits) << rptNewLine;
               (*pLayoutTable)(0,1) << CCastingYardMomentsTable().Build(pBroker,segmentKey,storageIntervalIdx,POI_STORAGE_SEGMENT, _T("During Storage"),pDisplayUnits) << rptNewLine;
            }
            else
            {
               CString strTableTitle;
               strTableTitle.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));

               (*pReleaseLayoutTable)(0,segIdx) << CCastingYardMomentsTable().Build(pBroker,segmentKey,releaseIntervalIdx,POI_RELEASED_SEGMENT,strTableTitle.GetBuffer(),pDisplayUnits) << rptNewLine;
               (*pStorageLayoutTable)(0,segIdx) << CCastingYardMomentsTable().Build(pBroker,segmentKey,storageIntervalIdx,POI_STORAGE_SEGMENT, strTableTitle.GetBuffer(),pDisplayUnits) << rptNewLine;
            }
         }
      }
   }

   // Bridge Site Results
   std::vector<IntervalIndexType> vUserLoadIntervals = pUDL->GetUserDefinedLoadIntervals(girderKey);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType lastIntervalIdx = nIntervals-1;
   IntervalIndexType lastCastDeckIntervalIdx = pIntervals->GetLastCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker, IStressCheck, pStressCheck);
   std::vector<IntervalIndexType> vIntervals(pStressCheck->GetStressCheckIntervals(girderKey));

   p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *p << _T("Responses from Externally Applied Loads at the Bridge Site")<<rptNewLine;
   p->SetName(_T("Bridge Site Results"));
   *pChapter << p;

   // Product Axial
   if ( bReportAxial )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductAxialTable().Build(pBroker, girderKey,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
      {
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;
      }

      *p << LIVELOAD_PER_LANE << rptNewLine;
      LiveLoadTableFooter(pBroker,p, girderKey,bDesign,bRating);

      for(auto intervalIdx : vUserLoadIntervals)
      {
         ATLASSERT(pUDL->DoUserLoadsExist(girderKey, intervalIdx));
         *p << CUserAxialTable().Build(pBroker, girderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
      }
   }

   // Product Moments
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker, girderKey,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,p, girderKey,bDesign,bRating);

   for(auto intervalIdx : vUserLoadIntervals)
   {
     ATLASSERT( pUDL->DoUserLoadsExist(girderKey,intervalIdx) );
     *p << CUserMomentsTable().Build(pBroker, girderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
   }

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker, girderKey,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p, girderKey,bDesign,bRating);
   *p << rptNewLine;

   for(auto intervalIdx : vUserLoadIntervals)
   {
      ATLASSERT(pUDL->DoUserLoadsExist(girderKey, intervalIdx));
      *p << CUserShearTable().Build(pBroker, girderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
   }

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker, girderKey,analysisType,PierReactionsTable,true,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

   if ( bPedestrian )
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p, girderKey,bDesign,bRating);
   *p << rptNewLine;

   // For girder bearing reactions
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(lastIntervalIdx, girderKey);
   if( 0 < vPiers.size() )
   {
      *p << CProductReactionTable().Build(pBroker, girderKey,analysisType,BearingReactionsTable,pBearingDesign->BearingLiveLoadReactionsIncludeImpact(),bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
      {
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;
      }

      *p << (pBearingDesign->BearingLiveLoadReactionsIncludeImpact() ? LIVELOAD_PER_LANE : LIVELOAD_PER_LANE_NO_IMPACT) << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p, girderKey,bDesign,bRating);
   }

   for (auto intervalIdx : vUserLoadIntervals)
   {
      ATLASSERT(pUDL->DoUserLoadsExist(girderKey, intervalIdx));
      *p << CUserReactionTable().Build(pBroker, girderKey,analysisType,PierReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
      if( 0 < vPiers.size() )
      {
         *p << CUserReactionTable().Build(pBroker, girderKey,analysisType,BearingReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
      }
   }

   // Product Deflections
   if ( bDesign )
   {
      for (const auto& thisGirderKey : vGirderKeys)
      {
         p = new rptParagraph;
         *pChapter << p;
         *p << CProductDeflectionsTable().Build(pBroker, thisGirderKey, analysisType, bDesign, bRating, bIndicateControllingLoad, pDisplayUnits) << rptNewLine;

         if (bPedestrian)
         {
            *p << _T("$ Pedestrian values are per girder") << rptNewLine;
         }

         *p << LIVELOAD_PER_LANE << rptNewLine;
         *p << rptNewLine;
         LiveLoadTableFooter(pBroker, p, thisGirderKey, bDesign, bRating);

         for (auto intervalIdx : vUserLoadIntervals)
               {
            ATLASSERT(pUDL->DoUserLoadsExist(thisGirderKey, intervalIdx));
                  *p << CUserDeflectionsTable().Build(pBroker, thisGirderKey, analysisType, intervalIdx, pDisplayUnits) << rptNewLine;
               }

         // Product Rotations
         p = new rptParagraph;
         *pChapter << p;
         *p << CProductRotationTable().Build(pBroker, thisGirderKey, analysisType, pBearingDesign->BearingLiveLoadReactionsIncludeImpact(), false, bDesign, bRating, bIndicateControllingLoad, pDisplayUnits) << rptNewLine;

         if (bPedestrian)
         {
            *p << _T("$ Pedestrian values are per girder") << rptNewLine;
         }

         *p << (pBearingDesign->BearingLiveLoadReactionsIncludeImpact() ? LIVELOAD_PER_LANE : LIVELOAD_PER_LANE_NO_IMPACT) << rptNewLine;
         *p << rptNewLine;
         LiveLoadTableFooter(pBroker, p, thisGirderKey, bDesign, bRating);

         for (auto intervalIdx : vUserLoadIntervals)
               {
            ATLASSERT(pUDL->DoUserLoadsExist(thisGirderKey, intervalIdx));
                  *p << CUserRotationTable().Build(pBroker, thisGirderKey, analysisType, intervalIdx, pDisplayUnits) << rptNewLine;
               }

         if (pSpecEntry->GetDoEvaluateLLDeflection())
         {
            // Optional Live Load Deflections
            p = new rptParagraph;
            p->SetName(_T("Live Load Deflections"));
            *pChapter << p;
            *p << CProductDeflectionsTable().BuildLiveLoadTable(pBroker, thisGirderKey, pDisplayUnits) << rptNewLine;
            *p << _T("D1 = LRFD Design truck without lane load and including impact") << rptNewLine;
            *p << _T("D2 = 0.25*(Design truck) + lane load, including impact") << rptNewLine;
            *p << _T("D(Controlling) = Max(D1, D2)") << rptNewLine;
            *p << _T("EI = Bridge EI / Number of Girders") << rptNewLine;
            *p << _T("Live Load Distribution Factor = (Multiple Presence Factor)(Number of Lanes)/(Number of Girders)") << rptNewLine;
            *p << rptNewLine;
         }
      }
   } // if design


   vPiers = pBearingDesign->GetBearingReactionPiers(lastIntervalIdx,girderKey);

   // Load Combinations (DC, DW, etc) & Limit States
   // if we are doing a time-step analysis, we need to report for all intervals from
   // the first prestress release to the end to report all time-dependent effects
   bool bTimeDependentNote = false;
   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      bTimeDependentNote = true;
      IntervalIndexType firstReleaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
      vIntervals.clear();
      vIntervals.resize(nIntervals-firstReleaseIntervalIdx);
      std::generate(vIntervals.begin(),vIntervals.end(),IncrementValue<IntervalIndexType>(firstReleaseIntervalIdx));
      // when we go to C++ 11, use the std::itoa algorithm
   }

   for (const auto& intervalIdx : vIntervals)
   {
      p = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << p;
      CString strName;
      strName.Format(_T("Combined Results - Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      p->SetName(strName);
      *p << p->GetName() << rptNewLine;

      p = new rptParagraph;
      *pChapter << p;

      if ( bReportAxial )
      {
         CCombinedAxialTable().Build(pBroker,pChapter, girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
         if ( bTimeDependentNote )
         {
            p = new rptParagraph(rptStyleManager::GetFootnoteStyle());
            *pChapter << p;
            *p << TIME_DEPENDENT_NOTE << rptNewLine;
            p = new rptParagraph;
            *pChapter << p;
         }
      }
      CCombinedMomentsTable().Build(pBroker,pChapter, girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
      if ( bTimeDependentNote )
      {
         p = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << p;
         *p << TIME_DEPENDENT_NOTE << rptNewLine;
         p = new rptParagraph;
         *pChapter << p;
      }
      CCombinedShearTable().Build(  pBroker,pChapter, girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
      if ( bTimeDependentNote )
      {
         p = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << p;
         *p << TIME_DEPENDENT_NOTE << rptNewLine;
         p = new rptParagraph;
         *pChapter << p;
      }

      if ( lastCastDeckIntervalIdx <= intervalIdx )
      {
         CCombinedReactionTable().Build(pBroker,pChapter, girderKey,pDisplayUnits,intervalIdx,analysisType,PierReactionsTable, bDesign, bRating);
         if( 0 < vPiers.size() )
         {
            CCombinedReactionTable().Build(pBroker,pChapter, girderKey,pDisplayUnits,intervalIdx,analysisType,BearingReactionsTable, bDesign, bRating);
         }

         if ( pSpecEntry->GetShearCapacityMethod() == pgsTypes::scmVciVcw )
         {
            p = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << p;
            *p << _T("Concurrent Shears") << rptNewLine;
            p->SetName(_T("Concurrent Shears"));
            CConcurrentShearTable().Build(pBroker,pChapter, girderKey,pDisplayUnits,intervalIdx, analysisType);
         }
      }
   } // next interval

   p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Live Load Reactions Without Impact") << rptNewLine;
   p->SetName(_T("Live Load Reactions Without Impact"));
   if( 0 < vPiers.size() )
   {
      CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter, girderKey,pDisplayUnits,analysisType, false, true, false);
   }

   return pChapter;
}

CChapterBuilder* CMVRChapterBuilder::Clone() const
{
   return new CMVRChapterBuilder(m_bDesign,m_bRating);
}
