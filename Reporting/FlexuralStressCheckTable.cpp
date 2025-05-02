///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PsgLib\TimelineEvent.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PsgLib\ClosureJointData.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace/Limits.h>
#include <IFace\AnalysisResults.h>
#include <IFace/DocumentType.h>
#include <IFace\ReportOptions.h>


/****************************************************************************
CLASS
   CFlexuralStressCheckTable
****************************************************************************/

CFlexuralStressCheckTable::CFlexuralStressCheckTable()
{
}

CFlexuralStressCheckTable::CFlexuralStressCheckTable(const CFlexuralStressCheckTable& rOther)
{
   MakeCopy(rOther);
}

CFlexuralStressCheckTable::~CFlexuralStressCheckTable()
{
}

CFlexuralStressCheckTable& CFlexuralStressCheckTable::operator= (const CFlexuralStressCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CFlexuralStressCheckTable::Build(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses) const
{
   GET_IFACE2_NOCHECK(pBroker,IIntervals,pIntervals); // only used if there are more than one segment in the girder

   bool bAreSegmentsJoined = true;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      CClosureKey closureKey(pGirderArtifact->GetGirderKey(),segIdx);
      IntervalIndexType compClosureJointIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
      if ( task.intervalIdx < compClosureJointIntervalIdx )
      {
         bAreSegmentsJoined = false;
         break;
      }
   }

   if ( !bAreSegmentsJoined )
   {
      CGirderKey girderKey = pGirderArtifact->GetGirderKey();

      bool bIsSegmentStressingInterval = false; // for this interval, are segment tendons in any segment of this girder stressed?
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(girderKey, segIdx);
         if (pIntervals->IsSegmentTendonStressingInterval(segmentKey, task.intervalIdx))
         {
            bIsSegmentStressingInterval = true;
            break;
         }
      }

      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      // segments are not yet connected and act independently...
      // report segments individually
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey, segIdx);

         const auto* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

         if ( 1 < nSegments )
         {
            // Write out one section header for all segments
            if ( segIdx == 0 )
            {
               BuildSectionHeading(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
            }

            rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            if (bIsSegmentStressingInterval && pSegment->Tendons.GetDuctCount() == 0)
            {
               *pPara << _T("This segment does not have tendons that are stressed during this interval. Stress limits are not applicable to this segment during this interval.") << rptNewLine;
            }
            else
            {
               BuildStressLimitInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
               BuildTable(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
            }
         }
         else
         {
            Build(pChapter,pBroker,pGirderArtifact,segIdx,pDisplayUnits, task, bGirderStresses);
         }
      }
   }
   else
   {
      // all girder segments are connected... report as a single girder
      Build(pChapter,pBroker,pGirderArtifact,nSegments == 1 ? 0 : ALL_SEGMENTS,pDisplayUnits,task, bGirderStresses);
   }
}

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   // Write notes, then table
   BuildNotes(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
   BuildTable(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
}

void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   BuildSectionHeading(        pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
   BuildStressLimitInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
}

void CFlexuralStressCheckTable::BuildSectionHeading(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   CSegmentKey segmentKey(girderKey, segIdx == ALL_SEGMENTS ? 0 : segIdx);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   bool bIsStressingInterval = pIntervals->IsStressingInterval(girderKey, task.intervalIdx);

   std::_tstring strLimitState = GetLimitStateString(task.limitState);
   std::_tstring strStress = (task.stressType == pgsTypes::Tension ? _T("Tension") : _T("Compression"));

   if (task.limitState == pgsTypes::FatigueI && task.stressType == pgsTypes::Tension)
   {
      strLimitState = _T("Service I, components subjected to cyclic stresses");
      strStress = _T("");
   }

   std::_tostringstream os;
   os << _T("Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(": ") << pIntervals->GetDescription(task.intervalIdx) << _T(" : ") << strLimitState << _T(" ") << strStress;
   if (liveLoadIntervalIdx <= task.intervalIdx && !task.bIncludeLiveLoad)
   {
      os << _T(" without live load");
   }
   os << std::endl;

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << os.str() << rptNewLine;
   pTitle->SetName(os.str().c_str());

   rptParagraph* pPara = new rptParagraph( rptStyleManager::GetSubheadingStyle() );
   *pChapter << pPara;

   GET_IFACE2(pBroker, IDocumentType, pDocType);

   if (task.limitState == pgsTypes::FatigueI && task.stressType == pgsTypes::Tension)
   {
      // use a different title for this special case of Fatigue I and Tension - this is an UHPC check
      // and the other titles and spec references don't make sense in that context
      *pPara << _T("Service I stresses are used in this limit state check. See GS 1.5.2.3.") << rptNewLine;
   }
   else
   {
      // The storage interval represents a change in loading conditions because supports move relative to release.
      // Per LRFD 5.12.3.4.3 for spliced girder segments the concrete stress limits for after losses in
      // LRFD 5.9.2.3.2. However this doesn't may any sense. At release the BeforeLosses case applies and
      // a short time later the AfterLosses case applies, and the stress limit goes from 0.65f'ci to 0.45f'ci.
      // For this reason we use the "Before Losses" case for storage

      if (bIsStressingInterval || (storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulIntervalIdx) )
      {
         *pPara << _T("For Temporary Stresses") << WBFL::LRFD::LrfdLosses10th(WBFL::LRFD::ltTemporary) << _T("(LRFD )") << WBFL::LRFD::LrfdCw8th(_T("5.9.4.1"), _T("5.9.2.3.1")) << _T(")") << rptNewLine;
      }
      else
      {
         *pPara << _T("Stresses at Service Limit State") << WBFL::LRFD::LrfdLosses10th(WBFL::LRFD::ltService) << _T("(LRFD )") << WBFL::LRFD::LrfdCw8th(_T("5.9.4.2"), _T("5.9.2.3.2")) << _T(")") << rptNewLine;
      }

      if ( task.stressType == pgsTypes::Compression )
      {
         if ( bIsStressingInterval || (storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulIntervalIdx))
         {
            *pPara << _T("Compression Stresses (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.4.1.1"), _T("5.9.2.3.1a"));
         }
         else
         {
            *pPara << _T("Compression Stresses (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.4.2.1"), _T("5.9.2.3.2a"));
         }
      }

      if ( task.stressType == pgsTypes::Tension )
      {
         if ( bIsStressingInterval || (storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulIntervalIdx))
         {
            *pPara << _T("Tension Stresses (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.4.1.2"),_T("5.9.2.3.1b"));
         }
         else
         {
            *pPara << _T("Tension Stresses (LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.9.4.2.2"),_T("5.9.2.3.2b"));
         }
      }

      if (pDocType->IsPGSpliceDocument())
      {
         *pPara << _T(", ") << WBFL::LRFD::LrfdCw8th(_T("5.14.1.3.2d"), _T("5.12.3.4.2d")) << _T(", ") << WBFL::LRFD::LrfdCw8th(_T("5.14.1.3.3"), _T("5.12.3.4.3"));
      }

      *pPara << _T(")") << rptNewLine;
   }
}

void CFlexuralStressCheckTable::BuildStressLimitInformation(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   if ( bGirderStresses )
   {
      BuildGirderStressLimitInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task);
   }
   else
   {
      BuildDeckStressLimitInformation(pChapter, pBroker, pGirderArtifact, pDisplayUnits, task);
   }
}

void CFlexuralStressCheckTable::BuildTable(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());
   GET_IFACE2(pBroker, IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx = (segIdx == ALL_SEGMENTS ? nSegments - 1 : firstSegIdx);

   IndexType nArtifacts = 0;
   for (SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++)
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);
      nArtifacts += pSegmentArtifact->GetFlexuralStressArtifactCount(task);
   }

   if (nArtifacts == 0)
   {
      rptParagraph* p = new rptParagraph;
      *pChapter << p;
      *p << _T("Stress limits are not applicable to this segment during this interval.") << rptNewLine;
      return;
   }

   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);


   GET_IFACE2_NOCHECK(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType overlayIntervalIdx  = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   pgsTypes::WearingSurfaceType wearingSurfaceType = pBridge->GetWearingSurfaceType();

   PoiAttributeType refAttribute = POI_SPAN;
   if ( task.intervalIdx == releaseIntervalIdx )
   {
      refAttribute = POI_RELEASED_SEGMENT;
   }
   else if (task.intervalIdx == storageIntervalIdx )
   {
      refAttribute = POI_STORAGE_SEGMENT;
   }
   else if ( task.intervalIdx == erectionIntervalIdx )
   {
      refAttribute = POI_ERECTED_SEGMENT;
   }

   // this table not used for lifting and hauling
   ATLASSERT(task.intervalIdx != pIntervals->GetLiftSegmentInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx)));
   ATLASSERT(task.intervalIdx != pIntervals->GetHaulSegmentInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx)));

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue,   stress,   pDisplayUnits->GetStressUnit(),     false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey) && segIdx==ALL_SEGMENTS);

   rptCapacityToDemand cap_demand;

   bool bIncludePrestress = (bGirderStresses ? true : false);

   bool bIncludeTendons = false;
   GET_IFACE2(pBroker,IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      if ( pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx) <= task.intervalIdx )
      {
         bIncludeTendons = true;
         break;
      }
   }

   if (!bIncludeTendons)
   {
      GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
      SegmentIndexType lastSegIdx = (segIdx == ALL_SEGMENTS ? nSegments - 1 : firstSegIdx);
      for (SegmentIndexType si = firstSegIdx; si <= lastSegIdx && !bIncludeTendons; si++)
      {
         CSegmentKey segmentKey(girderKey, si);
         DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
         IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
         for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
         {
            if (stressTendonIntervalIdx <= task.intervalIdx)
            {
               bIncludeTendons = true;
               break;
            }
         }
      }
   }



   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   // create and set up table
   rptRcTable* p_table;
   ColumnIndexType nColumns = 1; // location column

   if (refAttribute != POI_SPAN)
   {
      nColumns++; // second location column
   }

   // Was allowable with mild rebar used anywhere along the girder
   bool bIsWithRebarAllowableApplicableTop = pGirderArtifact->IsWithRebarAllowableStressApplicable(task,topLocation);
   bool bIsWithRebarAllowableApplicableBot = pGirderArtifact->IsWithRebarAllowableStressApplicable(task,botLocation);

   // Pre-tension
   if ( bIncludePrestress )
   {
      nColumns += 2;
   }

   // Post-tension
   if ( bIncludeTendons )
   {
      nColumns += 2;
   }

   // Limit State
   nColumns += 2;

   // Demand
   nColumns += 2;

   // Tension allowable with rebar
   if ( bIsWithRebarAllowableApplicableTop && bIsWithRebarAllowableApplicableBot )
   {
      nColumns += 2;
   }
   else
   {
      if ( task.stressType == pgsTypes::Tension && bIsWithRebarAllowableApplicableTop )
      {
         nColumns += 2;
      }
   }

   // Precompressed Tensile Zone
   nColumns += 2;

   // Status
   nColumns++; 

   p_table = rptStyleManager::CreateDefaultTable(nColumns);
   *p << p_table << rptNewLine;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;

   if (refAttribute != POI_SPAN)
   {
      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   p_table->SetRowSpan(0,col,2);
   if ( task.intervalIdx == releaseIntervalIdx )
   {
      (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }
   else
   {
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }


   std::_tstring strLimitState = GetLimitStateString(task.limitState);
   if (task.limitState == pgsTypes::FatigueI && task.stressType == pgsTypes::Tension)
   {
      // for UHPC fatigue tension check, stresses are actually based on the Service I limit state
      // so force that title here... see GS 1.5.3
      strLimitState = GetLimitStateString(pgsTypes::ServiceI);
   }


   if ( bIncludePrestress )
   {
      p_table->SetColumnSpan(0,col,2);
      (*p_table)(0,col) << _T("Pre-tension");

      (*p_table)(1,col++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   if ( bIncludeTendons )
   {
      p_table->SetColumnSpan(0,col,2);
      (*p_table)(0,col) << _T("Post-tension");
      (*p_table)(1,col++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*p_table)(1,col++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   p_table->SetColumnSpan(0,col,2);
   (*p_table)(0, col) << strLimitState;

   (*p_table)(1,col++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   p_table->SetColumnSpan(0,col,2);
  (*p_table)(0, col) << _T("Demand");

   (*p_table)(1,col++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(1,col++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if ( (task.stressType == pgsTypes::Tension && bIsWithRebarAllowableApplicableTop) || (task.stressType == pgsTypes::Tension && bIsWithRebarAllowableApplicableBot) )
   {
      if ( bIsWithRebarAllowableApplicableTop && bIsWithRebarAllowableApplicableBot )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("Tension Limit");
         (*p_table)(1,col++) << COLHDR(_T("Top"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(1,col++) << COLHDR(_T("Bottom"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         p_table->SetRowSpan(0,col,2);
         if ( bIsWithRebarAllowableApplicableTop )
         {
            (*p_table)(0,col) << COLHDR(_T("Tension") << rptNewLine << _T("Limit") << rptNewLine << _T("Top"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
         else
         {
            (*p_table)(0,col) << COLHDR(_T("Tension") << rptNewLine << _T("Limit") << rptNewLine << _T("Bottom"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
      }
   }

   p_table->SetColumnSpan(0,col,2);
   (*p_table)(0,col) << _T("Precompressed") << rptNewLine << _T("Tensile Zone");
   (*p_table)(1,col++) << _T("Top");
   (*p_table)(1,col++) << _T("Bottom");

   p_table->SetRowSpan(0,col,2);
  (*p_table)(0,col++) << _T("Status") << rptNewLine << _T("(C/D)");

   p_table->SetNumberOfHeaderRows(2);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);

      IndexType nArtifacts = pSegmentArtifact->GetFlexuralStressArtifactCount(task);

      for ( IndexType idx = 0; idx < nArtifacts; idx++ )
      {
         ColumnIndexType col = 0;

         const pgsFlexuralStressArtifact* pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task, idx );

         const pgsPointOfInterest& poi( pStressArtifact->GetPointOfInterest() );

         if (refAttribute != POI_SPAN)
         {
            (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );
         }

         (*p_table)(row,col++) << location.SetValue( refAttribute, poi );

         // applicability is related to interval and precompressed tensile zone
         bool bIsTopStressApplicableAtThisPOI = pStressArtifact->IsApplicable(topLocation);
         bool bIsBotStressApplicableAtThisPOI = pStressArtifact->IsApplicable(botLocation);

         Float64 fTop, fBot;

         if ( bIncludePrestress )
         {
            fTop = pStressArtifact->GetPretensionEffects(topLocation);
            fBot = pStressArtifact->GetPretensionEffects(botLocation);
            (*p_table)(row,col++) << stress.SetValue( fTop );
            (*p_table)(row,col++) << stress.SetValue( fBot );
         }

         if ( bIncludeTendons )
         {
            fTop = pStressArtifact->GetPosttensionEffects(topLocation);
            fBot = pStressArtifact->GetPosttensionEffects(botLocation);

            (*p_table)(row,col++) << stress.SetValue( fTop );
            (*p_table)(row,col++) << stress.SetValue( fBot );
         }

         fTop = pStressArtifact->GetExternalEffects(topLocation);
         fBot = pStressArtifact->GetExternalEffects(botLocation);
         (*p_table)(row,col++) << stress.SetValue( fTop );
         (*p_table)(row,col++) << stress.SetValue( fBot );

         fTop = pStressArtifact->GetDemand(topLocation);
         fBot = pStressArtifact->GetDemand(botLocation);
         (*p_table)(row,col++) << stress.SetValue( fTop );
         (*p_table)(row,col++) << stress.SetValue( fBot );

         bool bWasWithRebarUsedAtThisPoiTop = pStressArtifact->WasWithRebarAllowableStressUsed(topLocation);
         bool bWasWithRebarUsedAtThisPoiBot = pStressArtifact->WasWithRebarAllowableStressUsed(botLocation);

         if ( bIsTopStressApplicableAtThisPOI )
         {
            if ( bWasWithRebarUsedAtThisPoiTop )
            {
               fTop = pStressArtifact->GetAlternativeAllowableTensileStress(topLocation);
               (*p_table)(row,col++) << stress.SetValue( fTop );
            }
            else if ( bIsWithRebarAllowableApplicableTop )
            {
               fTop = pStressArtifact->GetCapacity(topLocation);
               (*p_table)(row,col++) << stress.SetValue( fTop );
            }
         }
         else if ( bIsWithRebarAllowableApplicableTop )
         {
            (*p_table)(row,col++) << _T("-");
         }

         if ( bIsBotStressApplicableAtThisPOI )
         {
            if ( bWasWithRebarUsedAtThisPoiBot )
            {
               fBot = pStressArtifact->GetAlternativeAllowableTensileStress(botLocation);
               (*p_table)(row,col++) << stress.SetValue( fBot );
            }
            else if ( bIsWithRebarAllowableApplicableBot )
            {
               fBot = pStressArtifact->GetCapacity(botLocation);
               (*p_table)(row,col++) << stress.SetValue( fBot );
            }
         }
         else if ( bIsWithRebarAllowableApplicableBot )
         {
            (*p_table)(row,col++) << _T("-");
         }

         bool bIsTopInPTZ = pStressArtifact->IsInPrecompressedTensileZone(topLocation);
         bool bIsBotInPTZ = pStressArtifact->IsInPrecompressedTensileZone(botLocation);
         if ( bIsTopInPTZ )
         {
            (*p_table)(row,col++) << _T("Yes");
         }
         else
         {
            (*p_table)(row,col++) << _T("No");
         }

         if ( bIsBotInPTZ )
         {
            (*p_table)(row,col++) << _T("Yes");
         }
         else
         {
            (*p_table)(row,col++) << _T("No");
         }


         if ( bIsTopStressApplicableAtThisPOI || bIsBotStressApplicableAtThisPOI )
         {
            bool bPassed = bGirderStresses ? pStressArtifact->BeamPassed()     : pStressArtifact->DeckPassed();
            Float64 cdr  = bGirderStresses ? pStressArtifact->GetBeamCDRatio() : pStressArtifact->GetDeckCDRatio();

            if ( bPassed )
            {
               (*p_table)(row,col) << RPT_PASS;
            }
            else
            {
               (*p_table)(row,col) << RPT_FAIL;
            }

            (*p_table)(row,col) << rptNewLine << _T("(") << cap_demand.SetValue(cdr) << _T(")");
               
            col++;
         }
         else if ( task.stressType == pgsTypes::Tension)
         {
            // tension check isn't applicable at this location, but it is applicable
            // somewhere along the girder, so report N/A for this location
            (*p_table)(row,col++) << RPT_NA;
         }
         row++;
      } // next artifact
   } // next segment
}

void CFlexuralStressCheckTable::MakeCopy(const CFlexuralStressCheckTable& rOther)
{
   // Add copy code here...
}

void CFlexuralStressCheckTable::MakeAssignment(const CFlexuralStressCheckTable& rOther)
{
   MakeCopy( rOther );
}

void CFlexuralStressCheckTable::BuildGirderStressLimitInformation(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const
{
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
   
   USES_CONVERSION;

   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );

   GET_IFACE2_NOCHECK(pBroker,IIntervals,pIntervals); // doesn't get used in all cases

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2_NOCHECK(pBroker,IMaterials,pMaterials); // doesn't get used in nArtifacts is 0


   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
   *pPara << pLayoutTable;

   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx  = (segIdx == ALL_SEGMENTS ? nSegments-1 : firstSegIdx);

   RowIndexType rowIdx = 0;
   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++, rowIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);

      IndexType nArtifacts = pSegmentArtifact->GetFlexuralStressArtifactCount(task);
      if (nArtifacts == 0)
      {
         continue;
      }


      rptParagraph* p = &(*pLayoutTable)(rowIdx,0);

      const CSegmentKey& segmentKey(pSegmentArtifact->GetSegmentKey());

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      if ( firstSegIdx != lastSegIdx )
      {
         *p << _T("Segment ") << LABEL_SEGMENT(sIdx) << rptNewLine;
      }

      Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,task.intervalIdx);
      if ( task.intervalIdx == releaseIntervalIdx )
      {
         *p << RPT_FCI << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
      }
      else
      {
         *p << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
      }

      for ( IndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
      {
         const pgsFlexuralStressArtifact* pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task, artifactIdx);
         bool bIsTopApplicableStress = false;
         bool bIsBotApplicableStress = false;
         if (pStressArtifact)
         {
            bIsTopApplicableStress = pStressArtifact->IsApplicable(topLocation);
            bIsBotApplicableStress = pStressArtifact->IsApplicable(botLocation);
         }

         if ( bIsTopApplicableStress || bIsBotApplicableStress )
         {
            // report the allowable stress information from the first applicable artifact
            BuildSegmentStressLimitInformation(p, pBroker, pSegmentArtifact, artifactIdx, pDisplayUnits, task);
            break;
         }
      }

      //
      // Closure Joint
      //
      CClosureKey closureKey(pGirderArtifact->GetGirderKey(),sIdx);
      IntervalIndexType compositeClosureJointIntervalIdx = (sIdx == nSegments-1 ? INVALID_INDEX : pIntervals->GetCompositeClosureJointInterval(closureKey));
      if ( sIdx != nSegments-1 && compositeClosureJointIntervalIdx <= task.intervalIdx )
      {
         p = &(*pLayoutTable)(rowIdx,1);

         if ( firstSegIdx != lastSegIdx )
         {
            *p << _T("Closure Joint ") << LABEL_SEGMENT(sIdx) << rptNewLine;
         }

         IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);

         Float64 fc = pMaterials->GetClosureJointDesignFc(segmentKey,task.intervalIdx);
         if ( task.intervalIdx < compositeClosureIntervalIdx )
         {
            *p << RPT_FCI << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
         }
         else
         {
            *p << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
         }

         BuildClosureJointStressLimitInformation(p, pBroker, pSegmentArtifact, nArtifacts-1, pDisplayUnits, task);

         *p << rptNewLine;
      }
   }
}

void CFlexuralStressCheckTable::BuildDeckStressLimitInformation(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   GET_IFACE2(pBroker,IConcreteStressLimits,pLimits);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   Float64 fc = pMaterials->GetDeckDesignFc(task.intervalIdx);
   *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

   // using a dummy location to get information... all location should be the same
   // so 0 is as good as any
   const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(0);

   //
   // Compression
   //
   if ( task.stressType == pgsTypes::Compression )
   {
      const pgsFlexuralStressArtifact* pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task, 0 );
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      Float64 c = pLimits->GetDeckConcreteCompressionStressLimitCoefficient(poi,task);
      Float64 fLimit = pLimits->GetDeckConcreteCompressionStressLimit(poi, task);

      *pPara << _T("Compression stress limit = -") << c << RPT_FC << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
   }

   //
   // Tension
   //

   if ( task.stressType == pgsTypes::Tension )
   {
      const pgsFlexuralStressArtifact* pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,0);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      bool bIsTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(girderKey,task.intervalIdx);
      // NOTE: don't need to worry about segment tendons here... we are checking deck stresses and segment tendons are stress
      // at the fabrication plant so they don't effect the deck

      auto tension_stress_limit = pLimits->GetDeckConcreteTensionStressLimitParameters(poi, task,false/*without rebar*/);

      if (bIsTendonStressingInterval)
      {
         if (WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition())
         {
            (*pPara) << _T("Tension stress limit = ");
         }
         else
         {
            (*pPara) << _T("Tension stress limit in areas other than the precompressed tensile zone = ");
         }
         tension_stress_limit.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc);
         Float64 fLimit = pLimits->GetDeckConcreteTensionStressLimit(poi, task,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;

         if ( pGirderArtifact->WasDeckWithRebarAllowableStressUsed(task) )
         {
            tension_stress_limit = pLimits->GetDeckConcreteTensionStressLimitParameters(poi, task,true/*with rebar*/);
            fLimit = pLimits->GetDeckConcreteTensionStressLimit(poi, task,true/*with rebar*/);

            (*pPara) << _T("Tension stress limit in areas with sufficient bonded reinforcement = ");
            tension_stress_limit.Report(pPara,pDisplayUnits,TensionStressLimit::ConcreteSymbol::fc);
            (*pPara) << rptNewLine;
         }
      }
      else
      {
         (*pPara) << _T("Tensile stress limit in the precompressed tensile zone = ");
         tension_stress_limit.Report(pPara, pDisplayUnits, TensionStressLimit::ConcreteSymbol::fc);

         Float64 fLimit = pLimits->GetDeckConcreteTensionStressLimit(poi, task,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
      }
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pGirderArtifact->GetRequiredDeckConcreteStrength(task);

   if ( 0 < fc_reqd )
   {
      *pPara << _T("Concrete strength required to satisfy this requirement = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
   }
   else if ( IsZero(fc_reqd) )
   {
      // do nothing if exactly zero
   }
   else
   {
      ATLASSERT(fc_reqd == NO_AVAILABLE_CONCRETE_STRENGTH); // NO_AVAILABLE_CONCRETE_STRENGTH means there is not a concrete strength that will work
      *pPara << _T("Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
   }
}

void CFlexuralStressCheckTable::BuildSegmentStressLimitInformation(rptParagraph* pPara, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsSegmentArtifact* pSegmentArtifact, IndexType artifactIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const
{
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   const CSegmentKey& segmentKey(pSegmentArtifact->GetSegmentKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   const pgsFlexuralStressArtifact* pArtifact;

   GET_IFACE2(pBroker,IConcreteStressLimits,pLimits);

   //
   // Compression
   //
   if ( task.stressType == pgsTypes::Compression )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(task));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(!poi.HasAttribute(POI_CLOSURE));

      pLimits->ReportSegmentConcreteCompressionStressLimit(poi, task, pPara, pDisplayUnits);
   }

   //
   // Tension
   //
   if ( task.stressType == pgsTypes::Tension )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(task));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT( !poi.HasAttribute(POI_CLOSURE) );
      ATLASSERT( poi.GetSegmentKey() == segmentKey);
      
      pLimits->ReportSegmentConcreteTensionStressLimit(poi, task, pSegmentArtifact, pPara, pDisplayUnits);
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pSegmentArtifact->GetRequiredSegmentConcreteStrength(task);

   GET_IFACE2(pBroker, IMaterials, pMaterials);
   auto name = pLimits->GetConcreteStressLimitParameterName(task.stressType, pMaterials->GetSegmentConcreteType(segmentKey));

   if ( 0 < fc_reqd )
   {
      name[0] = std::toupper(name[0]);
      *pPara << name << _T(" required to satisfy this requirement = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
   }
   else if ( IsZero(fc_reqd) )
   {
      // do nothing if exactly zero
   }
   else
   {
      *pPara << _T("Regardless of the ") << name << _T(", the stress requirements will not be satisfied.") << rptNewLine;
   }
}

void CFlexuralStressCheckTable::BuildClosureJointStressLimitInformation(rptParagraph* pPara, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsSegmentArtifact* pSegmentArtifact, IndexType artifactIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const
{
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   const CSegmentKey& segmentKey(pSegmentArtifact->GetSegmentKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   const pgsFlexuralStressArtifact* pArtifact;

   GET_IFACE2(pBroker,IConcreteStressLimits,pLimits);

   //
   // Compression
   //
   if ( task.stressType == pgsTypes::Compression )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(task));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(poi.HasAttribute(POI_CLOSURE));

      pLimits->ReportClosureJointConcreteCompressionStressLimit(poi, task, pPara, pDisplayUnits);
   }

   //
   // Tension
   //
   if ( task.stressType == pgsTypes::Tension )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(task));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(poi.HasAttribute(POI_CLOSURE));

      pLimits->ReportClosureJointConcreteTensionStressLimit(poi, task, pSegmentArtifact, pPara, pDisplayUnits);
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pSegmentArtifact->GetRequiredClosureJointConcreteStrength(task);
   if ( 0 < fc_reqd )
   {
      *pPara << _T("Concrete strength required to satisfy this requirement = ") << stress_u.SetValue( fc_reqd ) << rptNewLine;
   }
   else if ( IsZero(fc_reqd) )
   {
      // do nothing if exactly zero
   }
   else
   {
      *pPara << _T("Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
   }
}
