///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include <PgsExt\TimelineEvent.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\ClosureJointData.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Allowables.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

void CFlexuralStressCheckTable::Build(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task, bool bGirderStresses) const
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
               BuildAllowStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
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

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   // Write notes, then table
   BuildNotes(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
   BuildTable(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
}

void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   BuildSectionHeading(        pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
   BuildAllowStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task, bGirderStresses);
}

void CFlexuralStressCheckTable::BuildSectionHeading(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   bool bIsStressingInterval = pIntervals->IsStressingInterval(girderKey,task.intervalIdx);

   std::_tstring strLimitState = GetLimitStateString(task.limitState);
   std::_tstring strStress = (task.stressType == pgsTypes::Tension ? _T("Tension") : _T("Compression"));

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

   *pPara << strLimitState << rptNewLine;

   if ( bIsStressingInterval )
   {
      *pPara << _T("For Temporary Stresses before Losses [") << LrfdCw8th(_T("5.9.4.1"),_T("5.9.2.3.1")) << _T("]") << rptNewLine;
   }
   else
   {
      *pPara << _T("Stresses at Service Limit State after Losses [") << LrfdCw8th(_T("5.9.4.2"),_T("5.9.2.3.2")) << _T("]") << rptNewLine;
   }

   if ( task.stressType == pgsTypes::Compression )
   {
      if ( bIsStressingInterval )
      {
         *pPara << _T("Compression Stresses [") << LrfdCw8th(_T("5.9.4.1.1"),_T("5.9.2.3.1a")) << _T("]") << rptNewLine;
      }
      else
      {
         *pPara << _T("Compression Stresses [") << LrfdCw8th(_T("5.9.4.2.1"),_T("5.9.2.3.2a")) << _T("]") << rptNewLine;
      }
   }
   
   if ( task.stressType == pgsTypes::Tension )
   {
      if ( bIsStressingInterval )
      {
         *pPara << _T("Tension Stresses [") << LrfdCw8th(_T("5.9.4.1.2"),_T("5.9.2.3.1b")) << _T("]") << rptNewLine;
      }
      else
      {
         *pPara << _T("Tension Stresses [") << LrfdCw8th(_T("5.9.4.2.2"),_T("5.9.2.3.2b")) << _T("]") << rptNewLine;
      }
   }
}

void CFlexuralStressCheckTable::BuildAllowStressInformation(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
{
   if ( bGirderStresses )
   {
      BuildAllowGirderStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, task);
   }
   else
   {
      BuildAllowDeckStressInformation(pChapter, pBroker, pGirderArtifact, pDisplayUnits, task);
   }
}

void CFlexuralStressCheckTable::BuildTable(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task,bool bGirderStresses) const
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

      CollectionIndexType nArtifacts = pSegmentArtifact->GetFlexuralStressArtifactCount(task);

      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
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

#if defined _DEBUG
bool CFlexuralStressCheckTable::AssertValid() const
{
   return true;
}

void CFlexuralStressCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CCombinedMomentsTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CFlexuralStressCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CFlexuralStressCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CFlexuralStressCheckTable");

   TESTME_EPILOG("CFlexuralStressCheckTable");
}
#endif // _UNITTEST

void CFlexuralStressCheckTable::BuildAllowGirderStressInformation(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task) const
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
            BuildAllowSegmentStressInformation(p, pBroker, pSegmentArtifact, artifactIdx, pDisplayUnits, task);
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

         BuildAllowClosureJointStressInformation(p, pBroker, pSegmentArtifact, nArtifacts-1, pDisplayUnits, task);

         *p << rptNewLine;
      }
   }
}

void CFlexuralStressCheckTable::BuildAllowDeckStressInformation(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
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

      Float64 c = pAllowable->GetDeckAllowableCompressionStressCoefficient(poi,task);
      Float64 fAllowable = pAllowable->GetDeckAllowableCompressionStress(poi, task);

      *pPara << _T("Compression stress limit = -") << c << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
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

      Float64 t;            // tension coefficient
      Float64 t_max;        // maximum allowable tension
      bool b_t_max;         // true if max allowable tension is applicable
      pAllowable->GetDeckAllowableTensionStressCoefficient(poi, task,false/*without rebar*/,&t,&b_t_max,&t_max);

      if ( bIsTendonStressingInterval )
      {
         (*pPara) << _T("Tension stress limit in areas other than the precompressed tensile zone = ") << tension_coeff.SetValue(t);
         if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pPara) << symbol(lambda);
         }
         (*pPara) << symbol(ROOT) << RPT_FC;

         if ( b_t_max )
         {
            *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
         }

         Float64 fAllowable = pAllowable->GetDeckAllowableTensionStress(poi, task,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;

         if ( pGirderArtifact->WasDeckWithRebarAllowableStressUsed(task) )
         {
            Float64 t_with_rebar; // allowable tension when sufficient rebar is used
            pAllowable->GetDeckAllowableTensionStressCoefficient(poi, task,true/*with rebar*/,&t_with_rebar,&b_t_max,&t_max);
            fAllowable = pAllowable->GetDeckAllowableTensionStress(poi, task,true/*with rebar*/);

            (*pPara) << _T("Tension stress limit in areas with sufficient bonded reinforcement = ") << tension_coeff.SetValue(t_with_rebar);
            if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*pPara) << symbol(lambda);
            }
            (*pPara) << symbol(ROOT) << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;

         }
      }
      else
      {
         (*pPara) << _T("Allowable tensile stress in the precompressed tensile zone = ") << tension_coeff.SetValue(t);
         if ( lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pPara) << symbol(lambda);
         }
         (*pPara) << symbol(ROOT) << RPT_FC;

         if ( b_t_max )
         {
            *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
         }

         Float64 fAllowable = pAllowable->GetDeckAllowableTensionStress(poi, task,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
      }
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pGirderArtifact->GetRequiredDeckConcreteStrength(task.stressType, task.intervalIdx, task.limitState);

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

void CFlexuralStressCheckTable::BuildAllowSegmentStressInformation(rptParagraph* pPara, IBroker* pBroker, const pgsSegmentArtifact* pSegmentArtifact, IndexType artifactIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task) const
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

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);

   //
   // Compression
   //
   if ( task.stressType == pgsTypes::Compression )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(task));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(!poi.HasAttribute(POI_CLOSURE));

      pAllowable->ReportSegmentAllowableCompressionStress(poi, task, pPara, pDisplayUnits);
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
      
      pAllowable->ReportSegmentAllowableTensionStress(poi, task, pSegmentArtifact, pPara, pDisplayUnits);
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pSegmentArtifact->GetRequiredSegmentConcreteStrength(task.stressType, task.intervalIdx, task.limitState);
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

void CFlexuralStressCheckTable::BuildAllowClosureJointStressInformation(rptParagraph* pPara, IBroker* pBroker, const pgsSegmentArtifact* pSegmentArtifact, IndexType artifactIdx, IEAFDisplayUnits* pDisplayUnits, const StressCheckTask& task) const
{
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   const CSegmentKey& segmentKey(pSegmentArtifact->GetSegmentKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   const pgsFlexuralStressArtifact* pArtifact;

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);

   //
   // Compression
   //
   if ( task.stressType == pgsTypes::Compression )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(task));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( task,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(poi.HasAttribute(POI_CLOSURE));

      pAllowable->ReportClosureJointAllowableCompressionStress(poi, task, pPara, pDisplayUnits);
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

      pAllowable->ReportClosureJointAllowableTensionStress(poi, task, pSegmentArtifact, pPara, pDisplayUnits);
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pSegmentArtifact->GetRequiredClosureJointConcreteStrength(task.stressType, task.intervalIdx, task.limitState);
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
