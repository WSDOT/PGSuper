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
#include <Reporting\PretensionStressTable.h>
#include <Reporting\PosttensionStressTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <Reporting\TSRemovalStressTable.h>

#include <IFace\Bridge.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

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
   p->SetName(_T("Product Load Stresses"));
   *p << p->GetName() << rptNewLine;
   *pChapter << p;

   if ( bDesign )
   {
      p = new rptParagraph;
      *pChapter << p;

      for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

         rptRcTable* pReleaseLayoutTable = NULL;
         rptRcTable* pStorageLayoutTable = NULL;
         rptRcTable* pLayoutTable        = NULL;
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         if ( 1 < nSegments )
         {
            pReleaseLayoutTable = pgsReportStyleHolder::CreateLayoutTable(nSegments,_T("Segment Stresses at Prestress Release"));
            *p << pReleaseLayoutTable << rptNewLine;

            pStorageLayoutTable = pgsReportStyleHolder::CreateLayoutTable(nSegments,_T("Segment Stresses during Storage"));
            *p << pStorageLayoutTable << rptNewLine;
         }
         else
         {
            pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2);
            *p << pLayoutTable << rptNewLine;
         }

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

            if ( pLayoutTable )
            {
               (*pLayoutTable)(0,0) << CCastingYardStressTable().Build(pBroker,segmentKey,releaseIntervalIdx,_T("Stresses at Release"),pDisplayUnits) << rptNewLine;
               (*pLayoutTable)(0,1) << CCastingYardStressTable().Build(pBroker,segmentKey,storageIntervalIdx,_T("Stresses during Storage"),pDisplayUnits) << rptNewLine;
            }
            else
            {
               CString strTableTitle;
               strTableTitle.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));

               (*pReleaseLayoutTable)(0,segIdx) << CCastingYardStressTable().Build(pBroker,segmentKey,releaseIntervalIdx,strTableTitle.GetBuffer(),pDisplayUnits) << rptNewLine;
               (*pStorageLayoutTable)(0,segIdx) << CCastingYardStressTable().Build(pBroker,segmentKey,storageIntervalIdx,strTableTitle.GetBuffer(),pDisplayUnits) << rptNewLine;
            }
         }
      }
   }

   // Bridge Site Results
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(thisGirderKey);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(thisGirderKey);

      // product stresses in girder
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductStressTable().Build(pBroker,thisGirderKey,analysisType,bDesign,bRating,pDisplayUnits,true/*girder stresses*/) << rptNewLine;
      *p << LIVELOAD_PER_LANE << rptNewLine;

      // product stresses in deck
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductStressTable().Build(pBroker,thisGirderKey,analysisType,bDesign,bRating,pDisplayUnits,false/*deck stresses*/) << rptNewLine;
      *p << LIVELOAD_PER_LANE << rptNewLine;

      // stresses in girder
      GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
      bool bAreThereUserLoads = pUDL->DoUserLoadsExist(thisGirderKey);
      if (bAreThereUserLoads)
      {
         std::vector<IntervalIndexType> vIntervals(pIntervals->GetSpecCheckIntervals(thisGirderKey));
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;
            if ( pUDL->DoUserLoadsExist(thisGirderKey,intervalIdx))
            {
               *p << CUserStressTable().Build(pBroker,thisGirderKey,analysisType,intervalIdx,pDisplayUnits,true/*girder stresses*/) << rptNewLine;
               if ( compositeDeckIntervalIdx <= intervalIdx )
               {
                  *p << CUserStressTable().Build(pBroker,thisGirderKey,analysisType,intervalIdx,pDisplayUnits,false/*deck stresses*/) << rptNewLine;
               }
            }
         }
      }

       CTSRemovalStressTable().Build(pChapter,pBroker,thisGirderKey,analysisType,pDisplayUnits,true/*girder stresses*/);
       CTSRemovalStressTable().Build(pChapter,pBroker,thisGirderKey,analysisType,pDisplayUnits,false/*deck stresses*/);

      // Load Combinations (DC, DW, etc) & Limit States

      std::vector<IntervalIndexType> vIntervals(pIntervals->GetSpecCheckIntervals(thisGirderKey));
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;

         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(thisGirderKey);
         IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(thisGirderKey);

         p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << p;
         CString strName;
         strName.Format(_T("Combined Stresses - Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(thisGirderKey,intervalIdx));
         p->SetName(strName);
         *p << p->GetName();

         CCombinedStressTable().Build(pBroker,pChapter,thisGirderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating, true/*girder stresses*/);
         if ( liveLoadIntervalIdx <= intervalIdx )
         {
            p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
            *pChapter << p;
            *p << LIVELOAD_PER_GIRDER << rptNewLine;
         }

         if ( compositeDeckIntervalIdx <= intervalIdx )
         {
            CCombinedStressTable().Build(pBroker,pChapter,thisGirderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating, false/*deck stresses*/);
            if ( liveLoadIntervalIdx <= intervalIdx )
            {
               p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
               *pChapter << p;
               *p << LIVELOAD_PER_GIRDER << rptNewLine;
            }
         }
      } // next interval
   } // next group


   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   p->SetName(_T("Stress due to Prestress"));
   *p << p->GetName() << rptNewLine;
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         if ( 1 < nSegments )
         {
            p = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << p;
            *p << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         }

         p = new rptParagraph;
         *pChapter << p;
         *p << CPretensionStressTable().Build(pBroker,segmentKey,bDesign,pDisplayUnits) << rptNewLine;
      }
   }


   GET_IFACE2(pBroker,IDocumentType, pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      p->SetName(_T("Stresses due to Post-tensioning"));
      *p << p->GetName() << rptNewLine;
      for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

         CGirderKey girderKey(grpIdx,gdrIdx);

         if ( 0 < pTendonGeom->GetDuctCount(girderKey) )
         {
            p = new rptParagraph;
            *pChapter << p;
            *p << CPosttensionStressTable().Build(pBroker,girderKey,bDesign,pDisplayUnits,true /*girder stresses*/) << rptNewLine;
            *p << CPosttensionStressTable().Build(pBroker,girderKey,bDesign,pDisplayUnits,false/*deck stresses*/) << rptNewLine;
         }
      }
   }

   return pChapter;
}

CChapterBuilder* CStressChapterBuilder::Clone() const
{
   return new CStressChapterBuilder(m_bDesign,m_bRating);
}
