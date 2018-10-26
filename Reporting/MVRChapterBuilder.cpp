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

#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ConcurrentShearTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <Reporting\TSRemovalMomentsTable.h>
#include <Reporting\TSRemovalShearTable.h>
#include <Reporting\TSRemovalDisplacementsTable.h>
#include <Reporting\TSRemovalReactionTable.h>
#include <Reporting\TSRemovalRotationTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

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
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(girderKey);
   IntervalIndexType lastIntervalIdx = nIntervals-1;
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(girderKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(girderKey);

   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
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


   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   bool bIndicateControllingLoad = true;

   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2( pBroker, ILibrary, pLib );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );


   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   // Casting Yard Results
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
            *p << CCastingYardMomentsTable().Build(pBroker,segmentKey,pDisplayUnits) << rptNewLine;
         }
      }
   }

   // Bridge Site Results
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);

      std::vector<IntervalIndexType> vIntervals(pIntervals->GetSpecCheckIntervals(thisGirderKey));

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *p << _T("Load Responses - Bridge Site")<<rptNewLine;
      p->SetName(_T("Bridge Site Results"));
      *pChapter << p;
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductMomentsTable().Build(pBroker,thisGirderKey,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      LiveLoadTableFooter(pBroker,p,thisGirderKey,bDesign,bRating);

      bool bAreThereUserLoads = pUDL->DoUserLoadsExist(thisGirderKey);
      if (bAreThereUserLoads)
      {
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;
            if ( pUDL->DoUserLoadsExist(thisGirderKey,intervalIdx) )
            {
               *p << CUserMomentsTable().Build(pBroker,thisGirderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
            }
         }
      }

      CTSRemovalMomentsTable().Build(pChapter,pBroker,thisGirderKey,analysisType,pDisplayUnits);

      // Product Shears
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductShearTable().Build(pBroker,thisGirderKey,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,thisGirderKey,bDesign,bRating);
      *p << rptNewLine;

      if (bAreThereUserLoads)
      {
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;

            if ( pUDL->DoUserLoadsExist(thisGirderKey,intervalIdx) )
            {
               *p << CUserShearTable().Build(pBroker,thisGirderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
            }
         }
      }

      CTSRemovalShearTable().Build(pChapter,pBroker,thisGirderKey,analysisType,pDisplayUnits);

      // Product Reactions
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductReactionTable().Build(pBroker,thisGirderKey,analysisType,PierReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

      if ( bPedestrian )
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;

      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,thisGirderKey,bDesign,bRating);
      *p << rptNewLine;

      CTSRemovalReactionTable().Build(pChapter,pBroker,thisGirderKey,analysisType,PierReactionsTable,pDisplayUnits);

      // For girder bearing reactions
      GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
      bool bDoBearingReaction, bDummy;
      bDoBearingReaction = pBearingDesign->AreBearingReactionsAvailable(lastIntervalIdx,thisGirderKey,&bDummy,&bDummy);
      if(bDoBearingReaction)
      {
         *p << CProductReactionTable().Build(pBroker,thisGirderKey,analysisType,BearingReactionsTable,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

         if ( bPedestrian )
            *p << _T("$ Pedestrian values are per girder") << rptNewLine;

         *p << LIVELOAD_PER_LANE << rptNewLine;
         *p << rptNewLine;
         LiveLoadTableFooter(pBroker,p,thisGirderKey,bDesign,bRating);
      }

      if (bAreThereUserLoads)
      {
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;
            if ( pUDL->DoUserLoadsExist(thisGirderKey,intervalIdx) )
            {
               *p << CUserReactionTable().Build(pBroker,thisGirderKey,analysisType,PierReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
               if(bDoBearingReaction)
               {
                  *p << CUserReactionTable().Build(pBroker,thisGirderKey,analysisType,BearingReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
               }
            }
         }
      }

      // Product Deflections
      if ( bDesign )
      {
         p = new rptParagraph;
         *pChapter << p;
         *p << CProductDeflectionsTable().Build(pBroker,thisGirderKey,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

         if ( bPedestrian )
            *p << _T("$ Pedestrian values are per girder") << rptNewLine;

         *p << LIVELOAD_PER_LANE << rptNewLine;
         *p << rptNewLine;
         LiveLoadTableFooter(pBroker,p,thisGirderKey,bDesign,bRating);

         if (bAreThereUserLoads)
         {
            std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
            std::vector<IntervalIndexType>::iterator end(vIntervals.end());
            for ( ; iter != end; iter++ )
            {
               IntervalIndexType intervalIdx = *iter;

               if ( pUDL->DoUserLoadsExist(thisGirderKey,intervalIdx) )
               {
                  *p << CUserDeflectionsTable().Build(pBroker,thisGirderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
               }
            }
         }

         CTSRemovalDeflectionsTable().Build(pChapter,pBroker,thisGirderKey,analysisType,pDisplayUnits);

         // Product Rotations
         p = new rptParagraph;
         *pChapter << p;
         *p << CProductRotationTable().Build(pBroker,thisGirderKey,analysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;

         if ( bPedestrian )
            *p << _T("$ Pedestrian values are per girder") << rptNewLine;

         *p << LIVELOAD_PER_LANE << rptNewLine;
         *p << rptNewLine;
         LiveLoadTableFooter(pBroker,p,thisGirderKey,bDesign,bRating);

         if (bAreThereUserLoads)
         {
            std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
            std::vector<IntervalIndexType>::iterator end(vIntervals.end());
            for ( ; iter != end; iter++ )
            {
               IntervalIndexType intervalIdx = *iter;

               if ( pUDL->DoUserLoadsExist(thisGirderKey,intervalIdx) )
               {
                  *p << CUserRotationTable().Build(pBroker,thisGirderKey,analysisType,intervalIdx,pDisplayUnits) << rptNewLine;
               }
            }
         }

         CTSRemovalRotationTable().Build(pChapter,pBroker,thisGirderKey,analysisType,pDisplayUnits);

         if (pSpecEntry->GetDoEvaluateLLDeflection())
         {
            // Optional Live Load Deflections
            p = new rptParagraph;
            p->SetName(_T("Live Load Deflections"));
            *pChapter << p;
            *p << CProductDeflectionsTable().BuildLiveLoadTable(pBroker,thisGirderKey,pDisplayUnits) << rptNewLine;
            *p << _T("D1 = LRFD Design truck without lane load and including impact")<< rptNewLine;
            *p << _T("D2 = 0.25*(Design truck) + lane load, including impact")<< rptNewLine;
            *p << _T("D(Controlling) = Max(D1, D2)")<< rptNewLine;
            *p << _T("EI = Bridge EI / Number of Girders") << rptNewLine;
            *p << _T("Live Load Distribution Factor = (Multiple Presence Factor)(Number of Lanes)/(Number of Girders)") << rptNewLine;
            *p << rptNewLine;
         }
      } // if design


      bDoBearingReaction = pBearingDesign->AreBearingReactionsAvailable(lastIntervalIdx,thisGirderKey,&bDummy,&bDummy);

      // Load Combinations (DC, DW, etc) & Limit States
      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;

         p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << p;
         CString strName;
         strName.Format(_T("Combined Results - Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(thisGirderKey,intervalIdx));
         p->SetName(strName);
         *p << p->GetName() << rptNewLine;

         if ( liveLoadIntervalIdx == intervalIdx )
         {
            CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,thisGirderKey,pDisplayUnits);
         }

         CCombinedMomentsTable().Build(pBroker,pChapter,thisGirderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
         CCombinedShearTable().Build(  pBroker,pChapter,thisGirderKey,pDisplayUnits,intervalIdx, analysisType, bDesign, bRating);
         if ( castDeckIntervalIdx <= intervalIdx )
         {
            CCombinedReactionTable().Build(pBroker,pChapter,thisGirderKey,pDisplayUnits,intervalIdx,analysisType,PierReactionsTable, bDesign, bRating);
            if( bDoBearingReaction )
            {
               CCombinedReactionTable().Build(pBroker,pChapter,thisGirderKey,pDisplayUnits,intervalIdx,analysisType,BearingReactionsTable, bDesign, bRating);
            }

            if ( liveLoadIntervalIdx <= intervalIdx )
            {
               p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               *pChapter << p;
               *p << _T("Live Load Reactions Without Impact") << rptNewLine;
               p->SetName(_T("Live Load Reactions Without Impact"));
               CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,thisGirderKey,pDisplayUnits,analysisType,PierReactionsTable, false, true, false);
               if(bDoBearingReaction)
               {
                  CCombinedReactionTable().BuildLiveLoad(pBroker,pChapter,thisGirderKey,pDisplayUnits,analysisType,BearingReactionsTable, false, true, false);
               }

               if ( pSpecEntry->GetShearCapacityMethod() == scmVciVcw )
               {
                  p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
                  *pChapter << p;
                  *p << _T("Concurrent Shears") << rptNewLine;
                  p->SetName(_T("Concurrent Shears"));
                  CConcurrentShearTable().Build(pBroker,pChapter,thisGirderKey,pDisplayUnits,liveLoadIntervalIdx, analysisType);
               }
            }
         }
      } // next interval
   } // next group

   return pChapter;
}

CChapterBuilder* CMVRChapterBuilder::Clone() const
{
   return new CMVRChapterBuilder(m_bDesign,m_bRating);
}
