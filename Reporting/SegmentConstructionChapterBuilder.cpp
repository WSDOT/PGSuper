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
#include <Reporting\SegmentConstructionChapterBuilder.h>
#include <Reporting\StrandEccTable.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\CastingYardStressTable.h>
#include <Reporting\PrestressStressTable.h>
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\LongReinfShearCheck.h>

#include <Reporting\PrestressLossTable.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Artifact.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>

#include <PgsExt\TimelineManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma Reminder("OBSOLETE") // this chapter builder was creating during PGSplice development
// for testing purposes... remove it and the source files when they are no longer needed

/****************************************************************************
CLASS
   CSegmentConstructionChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSegmentConstructionChapterBuilder::CSegmentConstructionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSegmentConstructionChapterBuilder::GetName() const
{
   return TEXT("Segment Construction");
}

rptChapter* CSegmentConstructionChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   // This is a "Test" spec check chapter builder for PGSplice.... the ultimate goal is to have a single
   // spec check for PGSuper/PGSplice

   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   ATLASSERT(pGdrRptSpec != NULL);

   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);

   // Spec check all segments in the girder
   CSegmentKey segmentKey;
   segmentKey.groupIndex   = pGdrRptSpec->GetGroupIndex();
   segmentKey.girderIndex  = pGdrRptSpec->GetGirderIndex();
   segmentKey.segmentIndex = ALL_SEGMENTS;

   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker, IEAFDisplayUnits,   pDisplayUnits);
   GET_IFACE2(pBroker, ILosses,            pILosses );
   GET_IFACE2(pBroker, IArtifact,          pIArtifact);

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   std::vector<IntervalIndexType> vIntervals( pIntervals->GetSpecCheckIntervals(CGirderKey(0,0)) );
   std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vIntervals.end());
   for ( ; iter != end; iter++ )
   {
      IntervalIndexType intervalIdx = *iter;

      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      (*pPara) << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" ") << pIntervals->GetDescription(intervalIdx) << rptNewLine;

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

               pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               *pChapter << pPara;
               (*pPara) << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;

               const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(thisSegmentKey);

               bool bPermit = pLimitStateForces->IsStrengthIIApplicable(thisSegmentKey);

               // Check Flexural Stresses
//               CFlexuralStressCheckTable().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::ServiceI,pgsTypes::Compression);

               if ( intervalIdx == lastIntervalIdx)
               {
//                  CFlexuralStressCheckTable().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension);

                  // Moment Capacity
                  rptParagraph* p = new rptParagraph;
                  bool bOverReinforced;
                  p->SetName(_T("Moment Capacities"));
//                  *p << CFlexuralCapacityCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
                  if ( bOverReinforced )
                  {
                     *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
                     *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
                     *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
                  }
                  *pChapter << p;

                  // Strength II if permit load exists
                  if (bPermit)
                  {
                     p = new rptParagraph;
//                     *p << CFlexuralCapacityCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII,true,&bOverReinforced) << rptNewLine;
                     if ( bOverReinforced )
                     {
                        *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
                        *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
                        *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
                     }
                     *pChapter << p;
                  }
                  
                  *p << rptNewLine;

#pragma Reminder("UPDATE: report moment capacity for negative moments")
                  // Not reporting it now because "span" doesn't fit with how we are reporting here 

                  //if ( pBridge->ProcessNegativeMoments(span) )
                  //{
                  //   p = new rptParagraph;
                  //   *p << CFlexuralCapacityCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,false,&bOverReinforced) << rptNewLine;
                  //   if ( bOverReinforced )
                  //   {
                  //      *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
                  //      *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
                  //      *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
                  //   }
                  //   *pChapter << p;

                  //   // Strength II if permit load exists
                  //   if (bPermit)
                  //   {
                  //      p = new rptParagraph;
                  //      *p << CFlexuralCapacityCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII,false,&bOverReinforced) << rptNewLine;
                  //      if ( bOverReinforced )
                  //      {
                  //         *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
                  //         *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
                  //         *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
                  //      }
                  //      *pChapter << p;
                  //   }
                  //}


                  // Vertical Shear check
                  p = new rptParagraph;
                  p->SetName(_T("Shear"));
                  *pChapter << p;
                  bool bStrutAndTieRequired;
                  //*p << CShearCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired) << rptNewLine;

                  //CShearCheckTable().BuildNotes(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired);

                  //if ( bPermit )
                  //{
                  //   p = new rptParagraph;
                  //   *pChapter << p;
                  //   *p << CShearCheckTable().Build(pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;

                  //   CShearCheckTable().BuildNotes(pChapter,pBroker,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired);
                  //}

                  // Interface Shear check
                  //if ( pBridge->IsCompositeDeck() )
                  //{
                  //   CInterfaceShearTable().Build(pBroker,pChapter,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI);

                  //   if ( bPermit )
                  //      CInterfaceShearTable().Build(pBroker,pChapter,pSegmentArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII);
                  //}

                  // these two check are independent of interval... move them
#pragma Reminder("UPDATE:")
                  //if (pSpecEntry->IsSplittingCheckEnabled())
                  //{
                  //   // Splitting Zone check
                  //   write_splitting_zone_check(pBroker,pDisplayUnits,segmentKey,pChapter);
                  //}

                  //if (pSpecEntry->IsConfinementCheckEnabled())
                  //{
                  //   // confinement check
                  //   write_confinement_check(pBroker,pDisplayUnits,segmentKey,pChapter);
                  //}

                  // Longitudinal reinforcement for shear
                  //CLongReinfShearCheck().Build(pChapter,pBroker,pSegmentArtifact,intervalIdx,pgsTypes::StrengthI,pDisplayUnits);
                  //if ( bPermit )
                  //{
                  //   CLongReinfShearCheck().Build(pChapter,pBroker,pSegmentArtifact,intervalIdx,pgsTypes::StrengthII,pDisplayUnits);
                  //}

#pragma Reminder("UPDATE:")
               //   // Optional live load deflection
               //   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetGirderArtifact(girderKey);
               //#pragma Reminder("UPDATE: assuming precast girder bridge") // there could be multiple deflection checks for a girder (one of each span)
               //   COptionalDeflectionCheck().Build(pChapter,pGdrArtifact->GetDeflectionCheckArtifact(0),pDisplayUnits);

               //   // Lifting
               //   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

               //   if (pGirderLiftingSpecCriteria->IsLiftingAnalysisEnabled())
               //   {
               //      p = new rptParagraph;
               //      p->SetName(_T("Lifting"));
               //      *pChapter << p;

               //      CLiftingCheck().Build(pChapter,pBroker,segmentKey,pDisplayUnits);
               //   }

               //   // Hauling
               //   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
               //   if (pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
               //   {
               //      p = new rptParagraph;
               //      p->SetName(_T("Hauling"));
               //      *pChapter << p;

               //      CHaulingCheck().Build(pChapter,pBroker,segmentKey,pDisplayUnits);
               //   }

               //   p = new rptParagraph;
               //   p->SetName(_T("Constructability"));
               //   *pChapter << p;

               //   // Girder Detailing
               //   CGirderDetailingCheck().Build(pChapter,pBroker,pSegmentArtifact,pDisplayUnits);

               //   // Debonding
               //   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
               //   if ( pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight) )
               //   {
               //      CDebondCheckTable().Build(pChapter, pBroker, pSegmentArtifact, pgsTypes::Straight, pDisplayUnits);
               //   }

               //   if ( pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Harped) )
               //   {
               //      CDebondCheckTable().Build(pChapter, pBroker, pSegmentArtifact, pgsTypes::Harped, pDisplayUnits);
               //   }

               //   if ( pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Temporary) )
               //   {
               //      CDebondCheckTable().Build(pChapter, pBroker, pSegmentArtifact, pgsTypes::Temporary, pDisplayUnits);
               //   }

               //   // Strand Slope
               //   CStrandSlopeCheck().Build(pChapter,pBroker,pSegmentArtifact->GetStrandSlopeArtifact(),pDisplayUnits);

               //   // Hold Down Force
               //   rptRcTable* hdtable = CHoldDownForceCheck().Build(pBroker,pSegmentArtifact->GetHoldDownForceArtifact(),pDisplayUnits);
               //   if (hdtable != NULL)
               //   {
               //      p = new rptParagraph;
               //      *p << hdtable << rptNewLine;
               //      *pChapter << p;
               //   }

               //   // "A" Dimension check
               //   std::vector<CSegmentKey> segmentList;
               //   segmentList.push_back(segmentKey);
               //   rptRcTable* atable = CConstructabilityCheckTable().BuildSlabOffsetTable(pBroker,segmentList,pDisplayUnits);
               //   if (atable!=NULL)
               //   { 
               //      rptParagraph* p = new rptParagraph;
               //      *p << atable << rptNewLine;
               //      *pChapter << p;
               //   }

               //   // Global Stability Check
               //   CConstructabilityCheckTable().BuildGlobalGirderStabilityCheck(pChapter,pBroker,pSegmentArtifact,pDisplayUnits);


               //   // Load rating
               //   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
               //   if ( !pRatingSpec->AlwaysLoadRate() )
               //      return pChapter;

               //   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
               //   {
               //      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //      (*pChapter) << pPara;
               //      pPara->SetName(_T("Design Load Rating"));
               //      (*pPara) << pPara->GetName() << rptNewLine;
               //      pPara = new rptParagraph;
               //      (*pChapter) << pPara;
               //      (*pPara) << CRatingSummaryTable().BuildByLimitState(pBroker, segmentKey, CRatingSummaryTable::Design ) << rptNewLine;
               //   }

               //   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               //   {
               //      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //      (*pChapter) << pPara;
               //      pPara->SetName(_T("Legal Load Rating"));
               //      (*pPara) << pPara->GetName() << rptNewLine;
               //      pPara = new rptParagraph;
               //      (*pChapter) << pPara;

               //      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
               //      {
               //         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker, segmentKey, pgsTypes::lrLegal_Routine);
               //         if ( pTable )
               //            (*pPara) << pTable << rptNewLine;

               //         pTable = CRatingSummaryTable().BuildLoadPosting(pBroker, segmentKey, pgsTypes::lrLegal_Routine);
               //         if ( pTable )
               //            (*pPara) << pTable << rptNewLine;
               //      }

               //      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
               //      {
               //         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker, segmentKey, pgsTypes::lrLegal_Special);
               //         if ( pTable )
               //            (*pPara) << pTable << rptNewLine;

               //         pTable = CRatingSummaryTable().BuildLoadPosting(pBroker, segmentKey, pgsTypes::lrLegal_Special);
               //         if ( pTable )
               //            (*pPara) << pTable << rptNewLine;
               //      }
               //   }

               //   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               //   {
               //      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               //      (*pChapter) << pPara;
               //      pPara->SetName(_T("Permit Load Rating"));
               //      (*pPara) << pPara->GetName() << rptNewLine;
               //      pPara = new rptParagraph;
               //      (*pChapter) << pPara;
               //      (*pPara) << Super(_T("*")) << _T("MBE 6A.4.5.2 Permit load rating should only be used if the bridge has a rating factor greater than 1.0 when evaluated for AASHTO legal loads.") << rptNewLine;

               //      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
               //      {
               //         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker, segmentKey, pgsTypes::lrPermit_Routine);
               //         if ( pTable )
               //            (*pPara) << pTable << rptNewLine;
               //      }

               //      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
               //      {
               //         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker, segmentKey, pgsTypes::lrPermit_Special);
               //         if ( pTable )
               //            (*pPara) << pTable << rptNewLine;
               //      }
               //   }


               } // if live load interval
            } // next segment
         } // next girder
      } // next group
   } // next interval

   return pChapter;
}

CChapterBuilder* CSegmentConstructionChapterBuilder::Clone() const
{
   return new CSegmentConstructionChapterBuilder;
}
