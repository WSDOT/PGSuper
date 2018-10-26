///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Reporting\DetailsChapterBuilder.h>
#include <Reporting\StrandEccTable.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\CastingYardStressTable.h>
#include <Reporting\PrestressStressTable.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\PrestressLossTable.h>
#include <Reporting\LiftingCheck.h>
#include <Reporting\HaulingCheck.h>
#include <Reporting\StrandStressCheckTable.h>
#include <Reporting\StrandSlopeCheck.h>
#include <Reporting\HoldDownForceCheck.h>
#include <Reporting\GirderDetailingCheck.h>
#include <Reporting\DebondCheckTable.h>
#include <Reporting\ConstructabilityCheckTable.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
#include <IFace\Artifact.h>
#include <PgsExt\GirderArtifact.h>

#include <PgsExt\StageManager.h>
#include <PgsExt\PrecastSegmentData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDetailsChapterBuilder::CDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDetailsChapterBuilder::GetName() const
{
   return TEXT("Stage by Stage Details");
}

rptChapter* CDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSegmentReportSpecification* pSegmentRptSpec = dynamic_cast<CSegmentReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   CSegmentKey segmentKey;

   if ( pSegmentRptSpec )
   {
      pSegmentRptSpec->GetBroker(&pBroker);
      segmentKey = pSegmentRptSpec->GetSegmentKey();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      segmentKey.groupIndex = pGdrRptSpec->GetGroupIndex();
      segmentKey.girderIndex = pGdrRptSpec->GetGirderIndex();
      segmentKey.segmentIndex = ALL_SEGMENTS;
   }

   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker, IEAFDisplayUnits,   pDisplayUnits);
   GET_IFACE2(pBroker, ILosses,            pILosses );
   GET_IFACE2(pBroker, IArtifact,          pIArtifact);
   GET_IFACE2(pBroker, IStrandGeometry,    pStrandGeometry);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      std::_tostringstream os;
      os << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx) << std::endl;

      rptParagraph* p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << p;
      p->SetName(os.str().c_str());
      (*p) << p->GetName() << rptNewLine;

      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
      GroupIndexType lastGroupIdx  = (segmentKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
      for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType firstGirderIdx = (segmentKey.girderIndex == ALL_GIRDERS ? 0 : segmentKey.girderIndex);
         GirderIndexType lastGirderIdx  = (segmentKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
         
         for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
            SegmentIndexType firstSegmentIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
            SegmentIndexType lastSegmentIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegmentIdx);

            for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
            {
               CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);
               const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);

               SegmentIDType segID = pSegment->GetID();

               const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(thisSegmentKey);

               p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               (*pChapter) << p;

               (*p) << _T("Group ") << LABEL_GROUP(grpIdx) 
                    << _T(" Girder ") << LABEL_GIRDER(gdrIdx)
                    << _T(" Segment " ) << LABEL_SEGMENT(segIdx) << rptNewLine;


               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
               if ( intervalIdx < releaseIntervalIdx )
               {
                  // prestress not released yet
                  continue;
               }

               p = new rptParagraph;
               (*pChapter) << p;

               if ( releaseIntervalIdx == intervalIdx )
               {
                  p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
                  (*pChapter) << p;
                  (*p) << _T("Activity: Construct Segment") << rptNewLine;

                  //
                  // Casting Yard Moments
                  //
                  p = new rptParagraph;
                  *pChapter << p;

                  *p << CCastingYardMomentsTable().Build(pBroker,thisSegmentKey,pDisplayUnits) << rptNewLine;

                  //
                  // Casting yard stresses
                  //
                  p = new rptParagraph;
                  *pChapter << p;

                  *p << CCastingYardStressTable().Build(pBroker,thisSegmentKey,pDisplayUnits) << rptNewLine;

                  //
                  // Stresses due to prestressing
                  //
                  p = new rptParagraph;
                  *pChapter << p;

                  *p << CPrestressStressTable().Build(pBroker,thisSegmentKey,true,pDisplayUnits) << rptNewLine;

                  //
                  // Spec Check
                  //
                  CFlexuralStressCheckTable().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::ServiceI);
               }

               if ( intervalIdx == castDeckIntervalIdx )
               {
                  CFlexuralStressCheckTable().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::ServiceI,pgsTypes::Tension/*this is a dummy value*/);
               }

               if ( liveLoadIntervalIdx <= intervalIdx )
               {
                  CFlexuralStressCheckTable().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::ServiceIII);
                  CFlexuralStressCheckTable().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::FatigueI,pgsTypes::Compression);

                  p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
                  (*pChapter) << p;
                  bool bOverReinforced;
                  *p << CFlexuralCapacityCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
                  if ( bOverReinforced )
                  {
                     *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
                     *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
                     *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
                  }


                  // Vertical Shear check
                  p = new rptParagraph;
                  *pChapter << p;
                  bool bStrutAndTieRequired;
                  *p << CShearCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired) << rptNewLine;

                  if ( bStrutAndTieRequired )
                  {
                     p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
                     *pChapter << p;
                     *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
                  }
                  else
                  {
                     p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
                     *pChapter << p;
                     *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;
                  }

                  //if ( bPermit )
                  //{
                  //   p = new rptParagraph;
                  //   *pChapter << p;
                  //   *p << CShearCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;

                  //   if ( bStrutAndTieRequired )
                  //   {
                  //      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
                  //      *pChapter << p;
                  //      *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
                  //   }
                  //   else
                  //   {
                  //      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
                  //      *pChapter << p;
                  //      *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;
                  //   }
                  //}
               }

               //if ( pStageMgr->GetSegmentErectionStageIndex(thisSegmentKey) == stageIdx )
               //{
               //   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //   (*pChapter) << p;
               //   (*p) << _T("Activity: Erect Segment") << rptNewLine;
               //}

               //if ( pStageMgr->GetCastDeckStageIndex() == stageIdx )
               //{
               //   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //   (*pChapter) << p;
               //   (*p) << _T("Activity: Cast Deck") << rptNewLine;
               //}

               //if ( pStageMgr->GetRailingSystemLoadStageIndex() == stageIdx )
               //{
               //   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //   (*pChapter) << p;
               //   (*p) << _T("Activity: Cast Railing System") << rptNewLine;
               //}

               //if ( pStageMgr->GetOverlayLoadStageIndex() == stageIdx )
               //{
               //   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //   (*pChapter) << p;
               //   (*p) << _T("Activity: Install Overlay") << rptNewLine;
               //}

               //if ( pStageMgr->IsTendonStressedByIndex(stageIdx) )
               //{
               //   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //   (*pChapter) << p;
               //   (*p) << _T("Activity: Stress Tendons") << rptNewLine;
               //}


               ////
               //// Losses
               ////
               //p = new rptParagraph;
               //*pChapter << p;

               //*p << CPrestressLossTable().Build(pBroker,thisSegmentKey,pDisplayUnits);

               //pILosses->ReportLosses(thisSegmentKey,pChapter,pDisplayUnits);

               //
               // Closure Pour
               //
               //if ( segIdx < nSegments-1 )
               //{
               //   // there is one fewer closure pour then segments

               //   // closure key is the same as the segment key... just changing names
               //   // here so the context is clear
               //   CSegmentKey thisClosureKey(thisSegmentKey);

               //   IntervalIndexType castClosurePourIntervalIdx = pIntervals->GetCastClosurePourInterval(thisClosureKey);
               //   if ( castClosurePourIntervalIdx < intervalIdx )
               //   {
               //      // closure pour has strength, and thus has stress limits, one interval after it is cast
               //      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //      (*pChapter) << p;

               //      (*p) << _T("Group ")                          << LABEL_GROUP(  thisClosureKey.groupIndex) 
               //           << _T(" Girder ")                        << LABEL_GIRDER( thisClosureKey.girderIndex)
               //           << _T(" Closure Pour between Segment " ) << LABEL_SEGMENT(thisClosureKey.segmentIndex) 
               //           << _T(" and Segment ")                   << LABEL_SEGMENT(thisClosureKey.segmentIndex+1)
               //           << rptNewLine;

               //      p = new rptParagraph;
               //      (*pChapter) << p;

               //      const pgsClosurePourArtifact* pClosureArtifact = pIArtifact->GetClosurePourArtifact(thisClosureKey);
               //      CFlexuralStressCheckTable().Build(pChapter,
               //                                        pBroker,
               //                                        pClosureArtifact,
               //                                        pDisplayUnits,
               //                                        intervalIdx,
               //                                        pgsTypes::ServiceI);

               //      if ( liveLoadIntervalIdx <= intervalIdx )
               //      {
               //         CFlexuralStressCheckTable().Build(pChapter,pBroker,pClosureArtifact,pDisplayUnits,intervalIdx,pgsTypes::ServiceIII);
               //         CFlexuralStressCheckTable().Build(pChapter,pBroker,pClosureArtifact,pDisplayUnits,intervalIdx,pgsTypes::FatigueI);

               //         p = new rptParagraph;
               //         (*pChapter) << p;
               //         bool bOverReinforced;
               //         *p << CFlexuralCapacityCheckTable().Build(pBroker,pClosureArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
               //         if ( bOverReinforced )
               //         {
               //            *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
               //            *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
               //            *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
               //         }


               //      }
               //   } // if closure pour stage
               //} // if closure pour

            } // segmentIdx
         } // girderIdx
      } // groupIdx
   } // stageIdx


   // Results that are independent of stage
   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
   GroupIndexType lastGroupIdx  = (segmentKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (segmentKey.girderIndex == ALL_GIRDERS ? 0 : segmentKey.girderIndex);
      GirderIndexType lastGirderIdx  = (segmentKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         SegmentIndexType firstSegmentIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
         SegmentIndexType lastSegmentIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegmentIdx);

         const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetGirderArtifact(girderKey);

         // Girder Results

         //
         // Tendon Stresses
         //
#pragma Reminder("UPDATE: dummy implementation")
         // This is a dummy implementation of reporting the tendon stresses.
         // This code is just a stub and needs to be cleaned up as tendon stresses
         // are limited at various stages and locations along the tendon
         rptParagraph* p = new rptParagraph;
         *pChapter << p;
         *p << _T("Tendon Stress Check") << rptNewLine;
         if ( pGdrArtifact->GetTendonStressArtifact()->Passed() )
         {
            *p << RPT_PASS << rptNewLine;
         }
         else
         {
            *p << RPT_PASS << rptNewLine;
         }

         // Segment Results
         for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            p = new rptParagraph;
            *pChapter << p;
            (*p) << _T("Group ")     << LABEL_GROUP(grpIdx) 
                 << _T(" Girder ")   << LABEL_GIRDER(gdrIdx)
                 << _T(" Segment " ) << LABEL_SEGMENT(segIdx) 
                 << rptNewLine;

            //
            // Strand Stresses
            //
            const pgsSegmentArtifact* pSegmentArtifact = pArtifacts->GetSegmentArtifact(thisSegmentKey);
            p = new rptParagraph;
            *pChapter << p;
            *p << CStrandStressCheckTable().Build(pBroker,pSegmentArtifact->GetStrandStressArtifact(),pDisplayUnits) << rptNewLine;

            //
            // Most of the checks below this could be logically grouped with the casting yard results...
            //

            // Segment Detailing
            CGirderDetailingCheck().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits);

            // Debonding
            if ( 0 < pStrandGeometry->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Straight) )
            {
               CDebondCheckTable().Build(pChapter, pBroker, pSegmentArtifact, pgsTypes::Straight, pDisplayUnits);
            }

            if ( 0 < pStrandGeometry->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Harped) )
            {
               CDebondCheckTable().Build(pChapter, pBroker, pSegmentArtifact, pgsTypes::Harped, pDisplayUnits);
            }

            if ( 0 < pStrandGeometry->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Temporary) )
            {
               CDebondCheckTable().Build(pChapter, pBroker, pSegmentArtifact, pgsTypes::Temporary, pDisplayUnits);
            }

            // Strand Slope
            p = new rptParagraph;
            *pChapter << p;
            CStrandSlopeCheck().Build(pChapter,pBroker,pSegmentArtifact->GetStrandSlopeArtifact(),pDisplayUnits);

            // Hold Down Force
            rptRcTable* hdtable = CHoldDownForceCheck().Build(pBroker,pSegmentArtifact->GetHoldDownForceArtifact(),pDisplayUnits);
            if (hdtable!=NULL)
            {
               p = new rptParagraph;
               *p << hdtable << rptNewLine;
               *pChapter << p;
            }

            // "A" Dimension check
            rptRcTable* atable = CConstructabilityCheckTable().BuildSlabOffsetTable(pBroker,pSegmentArtifact,pDisplayUnits);
            if (atable!=NULL)
            { 
               rptParagraph* p = new rptParagraph;
               *p << atable << rptNewLine;
               *pChapter << p;
            }

            // Global Stability Check
            CConstructabilityCheckTable().BuildGlobalGirderStabilityCheck(pChapter,pBroker,pSegmentArtifact,pDisplayUnits);
         } // segIdx
      } // gdrIdx
   } // grpIdx

   return pChapter;
}

CChapterBuilder* CDetailsChapterBuilder::Clone() const
{
   return new CDetailsChapterBuilder;
}
