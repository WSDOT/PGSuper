///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\StressChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardStressTable.h>
#include <Reporting\ProductStressTable.h>
#include <Reporting\UserStressTable.h>
#include <Reporting\CombinedStressTable.h>
#include <Reporting\PrestressStressTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>

#include <IFace\Bridge.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>

#include <psgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStressChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStressChapterBuilder::CStressChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CStressChapterBuilder::GetName() const
{
   return TEXT("Stresses");
}

rptChapter* CStressChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
         //pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) || // operating does not apply
           pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
           pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) 
         )
      {
         bRating = true;
      }
      else
      {
         // if only permit rating is enabled, there aren't any stresses to report
         bRating = false;
      }
   }
   else
   {
      // include load rating results if we are always load rating
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      bRating = pRatingSpec->AlwaysLoadRate();

      // if none of the rating types are enabled, skip the rating
      if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) 
         )
         bRating = false;
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   // Product Stresses
   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << _T("Product Load Stresses") << rptNewLine;
   p->SetName(_T("Product Load Stresses"));
   *pChapter << p;

   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *p << _T("Load Responses - Casting Yard")<<rptNewLine;
      p->SetName(_T("Casting Yard Results"));
      *pChapter << p;

      for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            p = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << p;
            *p << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;

            p = new rptParagraph;
            *pChapter << p;
            *p << CCastingYardStressTable().Build(pBroker,segmentKey,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Bridge Site Results
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductStressTable().Build(pBroker,thisGirderKey,analysisType,bDesign,bRating,pDisplayUnits) << rptNewLine;
      *p << LIVELOAD_PER_LANE << rptNewLine;

      GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
      bool bAreThereUserLoads = pUDL->DoUserLoadsExist(thisGirderKey);
      if (bAreThereUserLoads)
      {
         *p << CUserStressTable().Build(pBroker,thisGirderKey,analysisType,pDisplayUnits) << rptNewLine;
      }
   } // next group

   // Load Combinations (DC, DW, etc) & Limit States
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals          = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0)); // release interval is the same for all segments
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      CString strName;
      strName.Format(_T("Combined Stresses - Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      p->SetName(strName);
      *p << p->GetName();

      CCombinedStressTable().Build(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
      if ( liveLoadIntervalIdx <= intervalIdx )
      {
         p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << p;
         *p << LIVELOAD_PER_GIRDER << rptNewLine;
      }
   } // next interval


   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << _T("Stresses due to Prestress") << rptNewLine;
   p->SetName(_T("Prestress"));
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         p = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << p;
         *p << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;

         p = new rptParagraph;
         *pChapter << p;
         *p << CPrestressStressTable().Build(pBroker,segmentKey,bDesign,pDisplayUnits) << rptNewLine;
      }
   }

   return pChapter;
}

CChapterBuilder* CStressChapterBuilder::Clone() const
{
   return new CStressChapterBuilder(m_bDesign,m_bRating);
}
