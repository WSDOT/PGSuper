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
#include <Reporting\FlexuralStressCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\TimelineEvent.h>
#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\ClosureJointData.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Allowables.h>

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

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,
                                      IBroker* pBroker,
                                      const pgsGirderArtifact* pGirderArtifact,
                                      IEAFDisplayUnits* pDisplayUnits,
                                      IntervalIndexType intervalIdx,
                                      pgsTypes::LimitState limitState,
                                      bool bGirderStresses
                                      ) const
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   bool bAreSegmentsJoined = true;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
   {
      CClosureKey closureKey(pGirderArtifact->GetGirderKey(),segIdx);
      IntervalIndexType compClosureJointIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
      if ( intervalIdx < compClosureJointIntervalIdx )
      {
         bAreSegmentsJoined = false;
         break;
      }
   }

   if ( !bAreSegmentsJoined )
   {
      // segments are not yet connected and act independently...
      // report segments individually
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         if ( 1 < nSegments )
         {
            // Write out one section header for all segments
            if ( segIdx == 0 )
            {
               BuildSectionHeading(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
            }

            rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
            BuildAllowStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
            BuildTable(                 pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
         }
         else
         {
            Build(pChapter,pBroker,pGirderArtifact,segIdx,pDisplayUnits,intervalIdx,limitState, bGirderStresses);
         }
      }
   }
   else
   {
      // all girder segments are connected... report as a single girder
      Build(pChapter,pBroker,pGirderArtifact,ALL_SEGMENTS,pDisplayUnits,intervalIdx,limitState, bGirderStresses);
   }
}

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,
                                      const pgsGirderArtifact* pGirderArtifact,
                                      SegmentIndexType segIdx,
                                      IEAFDisplayUnits* pDisplayUnits,
                                      IntervalIndexType intervalIdx,
                                      pgsTypes::LimitState limitState,
                                      bool bGirderStresses) const
{
   // Write notes, then table
   BuildNotes(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
   BuildTable(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
}

void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           bool bGirderStresses) const
{
   BuildSectionHeading(        pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
   BuildAllowStressInformation(pChapter, pBroker, pGirderArtifact, segIdx, pDisplayUnits, intervalIdx, limitState, bGirderStresses);
}

void CFlexuralStressCheckTable::BuildSectionHeading(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           bool bGirderStresses) const
{
   USES_CONVERSION;

   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(CSegmentKey(pGirderArtifact->GetGirderKey(),segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker, IEventMap, pEventMap );
   std::_tstring strLimitState = OLE2T(pEventMap->GetLimitStateName(limitState));

   std::_tostringstream os;
   os << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(": ") << pIntervals->GetDescription(intervalIdx) << std::endl;

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
   bool bCompression = pAllowable->IsStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression);
   bool bTension     = pAllowable->IsStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension);

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << os.str() << rptNewLine;

   rptParagraph* pPara = new rptParagraph( pgsReportStyleHolder::GetSubheadingStyle() );
   *pChapter << pPara;

   *pPara << strLimitState << rptNewLine;

   if ( intervalIdx < compositeDeckIntervalIdx )
   {
      *pPara << _T("For Temporary Stresses before Losses [5.9.4.1]") << rptNewLine;
   }
   else
   {
      *pPara << _T("Stresses at Service Limit State after Losses [5.9.4.2]") << rptNewLine;
   }

   if ( bCompression )
   {
      if ( intervalIdx < compositeDeckIntervalIdx )
         *pPara << _T("Compression Stresses [5.9.4.1.1]") << rptNewLine;
      else
         *pPara << _T("Compression Stresses [5.9.4.2.1]") << rptNewLine;
   }
   
   if ( bTension )
   {
      if ( intervalIdx < compositeDeckIntervalIdx )
         *pPara << _T("Tension Stresses [5.9.4.1.2]") << rptNewLine;
      else
         *pPara << _T("Tension Stresses [5.9.4.2.2]") << rptNewLine;
   }
}

void CFlexuralStressCheckTable::BuildAllowStressInformation(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           bool bGirderStresses) const
{
   if ( bGirderStresses )
   {
      BuildAllowGirderStressInformation(pChapter, 
                                        pBroker,
                                        pGirderArtifact,
                                        segIdx,
                                        pDisplayUnits,
                                        intervalIdx,
                                        limitState);
   }
   else
   {
      BuildAllowDeckStressInformation(pChapter, 
                                      pBroker,
                                      pGirderArtifact,
                                      segIdx,
                                      pDisplayUnits,
                                      intervalIdx,
                                      limitState);
   }
}

void CFlexuralStressCheckTable::BuildTable(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           bool bGirderStresses) const
{
   USES_CONVERSION;

   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // Build table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue,   stress,   pDisplayUnits->GetStressUnit(),     false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS || segIdx == ALL_SEGMENTS);
   rptCapacityToDemand cap_demand;

   bool bIncludePrestress = (bGirderStresses ? true : false);

   bool bIncludeTendons = false;
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      if ( pIntervals->GetStressTendonInterval(girderKey,ductIdx) <= intervalIdx )
      {
         bIncludeTendons = true;
         break;
      }
   }


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   // create and set up table
   rptRcTable* p_table;
   ColumnIndexType nColumns = 1; // location column

   // Is allowable stress check at top of girder applicable anywhere along the girder
   bool bApplicableTensionTop     = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,    topLocation);
   bool bApplicableCompressionTop = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression,topLocation);

   // Is allowable stress check at bottom of girder applicable anywhere along the girder
   bool bApplicableTensionBot     = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,    botLocation);
   bool bApplicableCompressionBot = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression,botLocation);

   // Was allowable with mild rebar used anywhere along the girder
   bool bWasWithRebarAllowableUsedTop = pGirderArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,topLocation);
   bool bWasWithRebarAllowableUsedBot = pGirderArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,botLocation);


   ColumnIndexType columnInc = 0;
   if ( bApplicableTensionTop || bApplicableCompressionTop )
   {
      columnInc++; // we'll be adding columns for top of girder stresses
   }

   if ( bApplicableTensionBot || bApplicableCompressionBot )
   {
      columnInc++; // we'll be adding columns for bottom of girder stresses
   }

   // Pre-tension
   if ( bIncludePrestress )
   {
      nColumns += columnInc;
   }

   // Post-tension
   if ( bIncludeTendons )
   {
      nColumns += columnInc;
   }

   // Limit State
   nColumns += columnInc;

   // Demand
   nColumns += columnInc;

   // Tension allowable with rebar
   if ( bApplicableTensionTop && bWasWithRebarAllowableUsedTop )
   {
      nColumns++;
   }

   if ( bApplicableTensionBot && bWasWithRebarAllowableUsedBot )
   {
      nColumns++;
   }

   // Precompressed Tensile Zone
   nColumns += 2;

   // Status
   if ( bApplicableTensionTop || bApplicableTensionBot )
      nColumns++; // tension status

   if ( bApplicableCompressionTop || bApplicableCompressionBot )
      nColumns++; // compression status


   p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   *p << p_table << rptNewLine;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   if ( intervalIdx == releaseIntervalIdx )
      (*p_table)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );


   GET_IFACE2(pBroker, IEventMap, pEventMap );
   std::_tstring strLimitState = OLE2T(pEventMap->GetLimitStateName(limitState));


   if ( bIncludePrestress && ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot ) )
   {
      if ( columnInc == 2 )
      {
         p_table->SetColumnSpan(0,col1,2);
      }
      (*p_table)(0,col1++) << _T("Pre-tension");
      if ( bApplicableTensionTop || bApplicableCompressionTop )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      if ( bApplicableTensionBot || bApplicableCompressionBot )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }

   if ( bIncludeTendons && ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot ) )
   {
      if ( columnInc == 2 )
      {
         p_table->SetColumnSpan(0,col1,2);
      }
      (*p_table)(0,col1++) << _T("Post-tension");
      if ( bApplicableTensionTop || bApplicableCompressionTop )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      if ( bApplicableTensionBot || bApplicableCompressionBot )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }

   if ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot )
   {
      if ( columnInc == 2 )
      {
         p_table->SetColumnSpan(0,col1,2);
      }
      (*p_table)(0,col1++) << strLimitState;
      if ( bApplicableTensionTop || bApplicableCompressionTop )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      if ( bApplicableTensionBot || bApplicableCompressionBot )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      if ( columnInc == 2 )
      {
         p_table->SetColumnSpan(0,col1,2);
      }
      (*p_table)(0,col1++) << _T("Demand");
      if ( bApplicableTensionTop || bApplicableCompressionTop )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FTOP, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      if ( bApplicableTensionBot || bApplicableCompressionBot )
      {
         (*p_table)(1,col2++) << COLHDR(RPT_FBOT, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }

   if ( (bApplicableTensionTop && bWasWithRebarAllowableUsedTop) || (bApplicableTensionBot && bWasWithRebarAllowableUsedBot) )
   {
      if ( bWasWithRebarAllowableUsedTop && bWasWithRebarAllowableUsedBot )
      {
         p_table->SetColumnSpan(0,col1,2);
         (*p_table)(0,col1++) << _T("Tensile Capacity");
         (*p_table)(1,col2++) << COLHDR(_T("Top"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*p_table)(1,col2++) << COLHDR(_T("Bottom"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         if ( bWasWithRebarAllowableUsedTop )
         {
            (*p_table)(0,col1++) << COLHDR(_T("Tension") << rptNewLine << _T("Capacity") << rptNewLine << _T("Top"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
         else
         {
            (*p_table)(0,col1++) << COLHDR(_T("Tension") << rptNewLine << _T("Capacity") << rptNewLine << _T("Bottom"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
      }
   }

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Precompressed") << rptNewLine << _T("Tensile Zone");
   (*p_table)(1,col2++) << _T("Top");
   (*p_table)(1,col2++) << _T("Bottom");

   if ( bApplicableTensionTop || bApplicableTensionBot )
   {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1++) <<_T("Tension") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   }

   if ( bApplicableCompressionTop || bApplicableCompressionBot )
   {
         p_table->SetRowSpan(0,col1,2);
         p_table->SetRowSpan(1,col2++,SKIP_CELL);
         (*p_table)(0,col1++) <<_T("Compression") << rptNewLine << _T("Status") << rptNewLine << _T("(C/D)");
   }

   p_table->SetNumberOfHeaderRows(2);
   for ( ColumnIndexType i = col1; i < p_table->GetNumberOfColumns(); i++ )
   {
      p_table->SetColumnSpan(0,i,SKIP_CELL);
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx  = (segIdx == ALL_SEGMENTS ? nSegments-1 : firstSegIdx);

   // Fill up the table
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   if ( intervalIdx == releaseIntervalIdx )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);

#pragma Reminder("UPDATE: get list of POIs from segment artifact to report on")
      // Use the list instead of the loop on artifact index...
      // Use the Find artifact method to get the artifact at a POI
      //
      // It may be a little slower, but we are guarenteed to get the report correct

      CollectionIndexType nArtifacts = Max(
         pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression),
         pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension)
            );
      for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
      {
         ColumnIndexType col = 0;

         const pgsFlexuralStressArtifact* pTensionArtifact     = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,     idx );
         const pgsFlexuralStressArtifact* pCompressionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression, idx );

#if defined _DEBUG
         if ( pTensionArtifact && pCompressionArtifact )
         {
            // artifacts must be for the same POI!
            ATLASSERT(pTensionArtifact->GetPointOfInterest() == pCompressionArtifact->GetPointOfInterest());
         }
#endif

         if ( pTensionArtifact == NULL && pCompressionArtifact == NULL )
            continue;


         const pgsPointOfInterest& poi( pTensionArtifact ? pTensionArtifact->GetPointOfInterest() : pCompressionArtifact->GetPointOfInterest());

         (*p_table)(row,col++) << location.SetValue( intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT, poi, end_size );

         if ( pTensionArtifact )
         {
            bool bIsTopTensionApplicableAtThisPOI = pTensionArtifact->IsApplicable(topLocation);
            bool bIsBotTensionApplicableAtThisPOI = pTensionArtifact->IsApplicable(botLocation);

            Float64 fTop, fBot;

            if ( bIncludePrestress && ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot ) )
            {
               fTop = pTensionArtifact->GetPretensionEffects(topLocation);
               fBot = pTensionArtifact->GetPretensionEffects(botLocation);

               if ( bApplicableTensionTop || bApplicableCompressionTop )
               {
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
               if ( bApplicableTensionBot || bApplicableCompressionBot )
               {
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }
            }

            if ( bIncludeTendons && ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot ) )
            {
               fTop = pTensionArtifact->GetPosttensionEffects(topLocation);
               fBot = pTensionArtifact->GetPosttensionEffects(botLocation);

               if ( bApplicableTensionTop || bApplicableCompressionTop )
               {
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
               if ( bApplicableTensionBot || bApplicableCompressionBot )
               {
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }
            }

            if ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot )
            {
               fTop = pTensionArtifact->GetExternalEffects(topLocation);
               fBot = pTensionArtifact->GetExternalEffects(botLocation);
               if ( bApplicableTensionTop || bApplicableCompressionTop )
               {
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
               if ( bApplicableTensionBot || bApplicableCompressionBot )
               {
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }

               fTop = pTensionArtifact->GetDemand(topLocation);
               fBot = pTensionArtifact->GetDemand(botLocation);
               if ( bApplicableTensionTop || bApplicableCompressionTop )
               {
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
               if ( bApplicableTensionBot || bApplicableCompressionBot )
               {
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }
            }

            bool bWasWithRebarUsedAtThisPoiTop = pTensionArtifact->WasWithRebarAllowableStressUsed(topLocation);
            bool bWasWithRebarUsedAtThisPoiBot = pTensionArtifact->WasWithRebarAllowableStressUsed(botLocation);

            if ( bIsTopTensionApplicableAtThisPOI )
            {
               if ( bWasWithRebarUsedAtThisPoiTop )
               {
                  fTop = pTensionArtifact->GetAlternativeAllowableTensileStress(topLocation);
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
               else if ( bWasWithRebarAllowableUsedTop )
               {
                  fTop = pTensionArtifact->GetCapacity(topLocation);
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
            }
            else if ( bWasWithRebarAllowableUsedTop )
            {
               (*p_table)(row,col++) << _T("-");
            }

            if ( bIsBotTensionApplicableAtThisPOI )
            {
               if ( bWasWithRebarUsedAtThisPoiBot )
               {
                  fBot = pTensionArtifact->GetAlternativeAllowableTensileStress(botLocation);
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }
               else if ( bWasWithRebarAllowableUsedBot )
               {
                  fBot = pTensionArtifact->GetCapacity(botLocation);
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }
            }
            else if ( bWasWithRebarAllowableUsedBot )
            {
               (*p_table)(row,col++) << _T("-");
            }

            bool bIsTopInPTZ = pTensionArtifact->IsInPrecompressedTensileZone(topLocation);
            bool bIsBotInPTZ = pTensionArtifact->IsInPrecompressedTensileZone(botLocation);
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


            if ( bIsTopTensionApplicableAtThisPOI || bIsBotTensionApplicableAtThisPOI )
            {
               bool bPassed = bGirderStresses ? pTensionArtifact->BeamPassed()     : pTensionArtifact->DeckPassed();
               Float64 cdr  = bGirderStresses ? pTensionArtifact->GetBeamCDRatio() : pTensionArtifact->GetDeckCDRatio();

               if ( bPassed )
                 (*p_table)(row,col) << RPT_PASS;
               else
                 (*p_table)(row,col) << RPT_FAIL;

               (*p_table)(row,col) << rptNewLine << _T("(") << cap_demand.SetValue(cdr) << _T(")");
               
               col++;
            }
            else
            {
               ATLASSERT( !bIsTopTensionApplicableAtThisPOI && !bIsBotTensionApplicableAtThisPOI );
               (*p_table)(row,col++) << RPT_NA; // stress check isn't applicable at this location
            }
         }

         // Compression
         if ( pCompressionArtifact )
         {
            Float64 fTop, fBot;
            if ( pTensionArtifact == NULL )
            {
               // if there is a tension artifact, then this stuff is already reported
               // if not, report it here
               if ( bIncludePrestress && ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot ) )
               {
                  fTop = pCompressionArtifact->GetPretensionEffects(topLocation);
                  fBot = pCompressionArtifact->GetPretensionEffects(botLocation);
                  if ( bApplicableTensionTop || bApplicableCompressionTop )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fTop );
                  }
                  if ( bApplicableTensionBot || bApplicableCompressionBot )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fBot );
                  }
               }

               if ( bIncludeTendons && ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot ) )
               {
                  fTop = pCompressionArtifact->GetPosttensionEffects(topLocation);
                  fBot = pCompressionArtifact->GetPosttensionEffects(botLocation);
                  if ( bApplicableTensionTop || bApplicableCompressionTop )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fTop );
                  }
                  if ( bApplicableTensionBot || bApplicableCompressionBot )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fBot );
                  }
               }

               if ( bApplicableTensionTop || bApplicableCompressionTop || bApplicableTensionBot || bApplicableCompressionBot )
               {
                  fTop = pCompressionArtifact->GetExternalEffects(topLocation);
                  fBot = pCompressionArtifact->GetExternalEffects(botLocation);
                  if ( bApplicableTensionTop || bApplicableCompressionTop )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fTop );
                  }
                  if ( bApplicableTensionBot || bApplicableCompressionBot )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fBot );
                  }

                  fTop = pCompressionArtifact->GetDemand(topLocation);
                  fBot = pCompressionArtifact->GetDemand(botLocation);
                  if ( bApplicableTensionTop || bApplicableCompressionTop )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fTop );
                  }
                  if ( bApplicableTensionBot || bApplicableCompressionBot )
                  {
                     (*p_table)(row,col++) << stress.SetValue( fBot );
                  }
               }

               if ( bApplicableTensionTop && bWasWithRebarAllowableUsedTop )
               {
                  (*p_table)(row,col++) << _T("-");
               }

               if ( bApplicableTensionBot && bWasWithRebarAllowableUsedBot )
               {
                  (*p_table)(row,col++) << _T("-");
               }

               bool bIsTopInPTZ = pCompressionArtifact->IsInPrecompressedTensileZone(topLocation);
               bool bIsBotInPTZ = pCompressionArtifact->IsInPrecompressedTensileZone(botLocation);
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
            }

            if ( bApplicableCompressionTop || bApplicableCompressionBot )
            {
               bool bPassed = bGirderStresses ? pCompressionArtifact->BeamPassed()     : pCompressionArtifact->DeckPassed();
               Float64 cdr  = bGirderStresses ? pCompressionArtifact->GetBeamCDRatio() : pCompressionArtifact->GetDeckCDRatio();
               if ( bPassed )
                 (*p_table)(row,col) << RPT_PASS;
               else
                 (*p_table)(row,col) << RPT_FAIL;

               (*p_table)(row,col) << rptNewLine << _T("(") << cap_demand.SetValue(cdr) << _T(")");
               
               col++;
            }
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

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,IBroker* pBroker,const pgsSegmentArtifact* pSegmentArtifact,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState,
                                           pgsTypes::StressType stressType) const
{
#pragma Reminder("OBSOLETE: remove obsolete code") // this method should not be called... remove it
#pragma Reminder("UPDATE: some things still call this method (TxDOT Agent - TOGA)")
   ATLASSERT(false); // should never get here
   //// Write notes, then table
   //BuildNotes(pChapter, pBroker, pSegmentArtifact, pDisplayUnits, intervalIdx, limitState, stressType);
   //BuildTable(pChapter, pBroker, pSegmentArtifact, pDisplayUnits, intervalIdx, limitState, stressType);
}
   
void CFlexuralStressCheckTable::BuildNotes(rptChapter* pChapter, 
                   IBroker* pBroker,
                   const pgsSegmentArtifact* pSegmentArtifact,
                   IEAFDisplayUnits* pDisplayUnits,
                   IntervalIndexType intervalIdx,
                   pgsTypes::LimitState ls,
                   pgsTypes::StressType stress) const
{
#pragma Reminder("OBSOLETE: remove obsolete code") // this method should not be called... remove it
#pragma Reminder("UPDATE: some things still call this method (TxDOT Agent - TOGA)")
   ATLASSERT(false); // should never get here
}

void CFlexuralStressCheckTable::BuildAllowGirderStressInformation(rptChapter* pChapter, 
                                           IBroker* pBroker,
                                           const pgsGirderArtifact* pGirderArtifact,
                                           SegmentIndexType segIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState) const
{
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
   
   USES_CONVERSION;

   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IMaterials,pMaterials);


   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateTableNoHeading(2);
   *pPara << pLayoutTable;
   pLayoutTable->SetInsideBorderStyle(rptRiStyle::NOBORDER);
   pLayoutTable->SetOutsideBorderStyle(rptRiStyle::NOBORDER);


   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx  = (segIdx == ALL_SEGMENTS ? nSegments-1 : firstSegIdx);

   RowIndexType rowIdx = 0;
   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++, rowIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);

      rptParagraph* p = &(*pLayoutTable)(rowIdx,0);

      const CSegmentKey& segmentKey(pSegmentArtifact->GetSegmentKey());

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      if ( firstSegIdx != lastSegIdx )
      {
         *p << _T("Segment ") << LABEL_SEGMENT(sIdx) << rptNewLine;
      }

      Float64 fc = pMaterials->GetSegmentFc(segmentKey,intervalIdx);
      if ( intervalIdx == releaseIntervalIdx )
         *p << RPT_FCI << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
      else
         *p << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

      IndexType nArtifacts = Max(pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension),
                                 pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression));

      for ( IndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
      {
         const pgsFlexuralStressArtifact* pTensionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,artifactIdx);
         bool bIsTopApplicableTension = false;
         bool bIsBotApplicableTension = false;
         if ( pTensionArtifact )
         {
            bIsTopApplicableTension = pTensionArtifact->IsApplicable(topLocation);
            bIsBotApplicableTension = pTensionArtifact->IsApplicable(botLocation);
         }

         const pgsFlexuralStressArtifact* pCompressionArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,artifactIdx);
         bool bIsTopApplicableCompression = false;
         bool bIsBotApplicableCompression = false;
         if ( pCompressionArtifact )
         {
            bIsTopApplicableCompression = pCompressionArtifact->IsApplicable(topLocation);
            bIsBotApplicableCompression = pCompressionArtifact->IsApplicable(botLocation);
         }

         if ( bIsTopApplicableTension || bIsBotApplicableTension || bIsTopApplicableCompression || bIsBotApplicableCompression )
         {
            // report the allowable stress information from the first applicable artifact
            BuildAllowSegmentStressInformation(p, pBroker, pSegmentArtifact, artifactIdx, pDisplayUnits, intervalIdx, limitState);
            break;
         }
      }

      //
      // Closure Joint
      //
      CClosureKey closureKey(pGirderArtifact->GetGirderKey(),sIdx);
      IntervalIndexType compositeClosureJointIntervalIdx = (sIdx == nSegments-1 ? INVALID_INDEX : pIntervals->GetCompositeClosureJointInterval(closureKey));
      if ( sIdx != nSegments-1 && compositeClosureJointIntervalIdx <= intervalIdx )
      {
         p = &(*pLayoutTable)(rowIdx,1);

         if ( firstSegIdx != lastSegIdx )
            *p << _T("Closure Joint ") << LABEL_SEGMENT(sIdx) << rptNewLine;

         IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);

         Float64 fc = pMaterials->GetClosureJointFc(segmentKey,intervalIdx);
         if ( intervalIdx == compositeClosureIntervalIdx )
            *p << RPT_FCI << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
         else
            *p << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

         BuildAllowSegmentStressInformation(p, pBroker, pSegmentArtifact, nArtifacts-1, pDisplayUnits, intervalIdx, limitState);

         *p << rptNewLine;
      }
   }
}

void CFlexuralStressCheckTable::BuildAllowDeckStressInformation(rptChapter* pChapter, 
                                                                IBroker* pBroker,
                                                                const pgsGirderArtifact* pGirderArtifact,
                                                                SegmentIndexType segIdx,
                                                                IEAFDisplayUnits* pDisplayUnits,
                                                                IntervalIndexType intervalIdx,
                                                                pgsTypes::LimitState limitState) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   pgsTypes::StressLocation topLocation = pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomDeck;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   Float64 fc = pMaterials->GetDeckFc(intervalIdx);
   *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

   // using a dummy location to get information... all location should be the same
   // so 0 is as good as any
   const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(0);

   //
   // Compression
   //
   if ( pAllowable->IsStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression) )
   {
      const pgsFlexuralStressArtifact* pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,0);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      Float64 c = pAllowable->GetAllowableCompressiveStressCoefficient(poi,topLocation,intervalIdx,limitState);

      bool bIsTopApplicable = pArtifact->IsApplicable(topLocation);
      bool bIsBotApplicable = pArtifact->IsApplicable(botLocation);

      Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
      Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

      if ( bIsTopApplicable && bIsBotApplicable )
      {
         // Compression stress is applicable at top and bottom... only want to report
         // allowable once if it is the same value
         if ( IsEqual(fAllowableTop,fAllowableBot) )
         {
            *pPara << _T("Allowable compressive stress = -") << c;
            *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;
         }
         else
         {
            // Not the same value... report each one individually

            *pPara << _T("Top of Girder - Allowable compressive stress = -") << c;
            *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;

            *pPara << _T("Bottom of Girder - Allowable compressive stress = -") << c;
            *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;
         }
      }
      else
      {
         // Stress check only applicable at the top or the bottom (not both)
         if ( bIsTopApplicable )
         {
            *pPara << _T("Top of Girder - Allowable compressive stress = -") << c;
            *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;
         }

         if ( bIsBotApplicable )
         {
            *pPara << _T("Bottom of Girder - Allowable compressive stress = -") << c;
            *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;
         }
      }
   }

   //
   // Tension
   //

   int fPTZTop    = 0;
   int fPTZBottom = 0;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   SegmentIndexType firstSegIdx = (segIdx == ALL_SEGMENTS ? 0 : segIdx);
   SegmentIndexType lastSegIdx  = (segIdx == ALL_SEGMENTS ? nSegments-1 : firstSegIdx);
   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);
      fPTZTop    |= pSegArtifact->GetPrecompressedTensileZone(intervalIdx,topLocation);
      fPTZBottom |= pSegArtifact->GetPrecompressedTensileZone(intervalIdx,botLocation);
   }

   if ( pAllowable->IsStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension) )
   {
      const pgsFlexuralStressArtifact* pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,0);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      bool bIsClosure = poi.HasAttribute(POI_CLOSURE);

      bool bIsTopInPTZ = pArtifact->IsInPrecompressedTensileZone(topLocation);
      bool bIsBotInPTZ = pArtifact->IsInPrecompressedTensileZone(botLocation);

      bool bIsTopApplicable = pSegmentArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,topLocation);

      if ( bIsTopApplicable )
      {
         // stresses are checked in the top of the girder so report the
         // allowable tension for the top of the girder

         Float64 t;            // tension coefficient
         Float64 t_max;        // maximum allowable tension
         bool b_t_max;         // true if max allowable tension is applicable

         // Get allowable tension stress parameters
         pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,false/*without rebar*/,bIsTopInPTZ,&t,&b_t_max,&t_max);

         if ( sysFlags<int>::IsSet(fPTZTop,PTZ_TOP_YES) )
         {
            (*pPara) << _T("Top of Deck - Allowable tensile stress in the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,topLocation,0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsTopInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Top of Deck - Allowable tensile stress in areas with sufficient bonded reinforcement in the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               *pPara << RPT_FC;

               fAllowableTop  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,topLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }

         if ( sysFlags<int>::IsSet(fPTZTop,PTZ_TOP_NO) )
         {
            (*pPara) << _T("Top of Deck - Allowable tensile stress in areas other than the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,topLocation,bIsClosure?POI_CLOSURE:0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsTopInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Top of Deck - Allowable tensile stress in areas with sufficient bonded reinforcement in areas other than the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

                *pPara << RPT_FC;

               fAllowableTop  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,topLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }
      }

      bool bIsBotApplicable = pSegmentArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,botLocation);

      if ( bIsBotApplicable )
      {
         // stresses are checked in the bottom of the girder so report the
         // allowable tension for the bottom of the girder

         Float64 t;            // tension coefficient
         Float64 t_max;        // maximum allowable tension
         bool b_t_max;         // true if max allowable tension is applicable

         // Get allowable tension stress parameters
         pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,false/*without rebar*/,bIsBotInPTZ,&t,&b_t_max,&t_max);

         if ( sysFlags<int>::IsSet(fPTZBottom,PTZ_BOTTOM_YES) )
         {
            (*pPara) << _T("Bottom of Deck - Allowable tensile stress in the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,botLocation,0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsBotInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Bottom of Deck - Allowable tensile stress in areas with sufficient bonded reinforcement in the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               *pPara << RPT_FC;

               fAllowableBot  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,botLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }

         if ( sysFlags<int>::IsSet(fPTZBottom,PTZ_BOTTOM_NO) )
         {
            (*pPara) << _T("Bottom of Deck - Allowable tensile stress in areas other than the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,botLocation,0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsBotInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Bottom of Deck - Allowable tensile stress in areas with sufficient bonded reinforcement in areas other than the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               *pPara << RPT_FC;

               fAllowableBot  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,botLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }
      }
   }

   //
   // Required Strength
   //
   Float64 fc_reqd;
   if ( segIdx == ALL_SEGMENTS )
   {
      fc_reqd = pGirderArtifact->GetRequiredDeckConcreteStrength(intervalIdx,limitState);
   }
   else
   {
      fc_reqd = pSegmentArtifact->GetRequiredDeckConcreteStrength(intervalIdx,limitState);
   }

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
      ATLASSERT(fc_reqd != -99999); // -99999 means the value was never set
      *pPara << _T("Regardless of the concrete strength, the stress requirements will not be satisfied.") << rptNewLine;
   }
}

void CFlexuralStressCheckTable::BuildAllowSegmentStressInformation(rptParagraph* pPara, 
                                           IBroker* pBroker,
                                           const pgsSegmentArtifact* pSegmentArtifact,
                                           IndexType artifactIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState) const
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
   if ( pAllowable->IsStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression) )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      bool bFci;
      if ( poi.HasAttribute(POI_CLOSURE) )
      {
         // if POI is at a closure joint, use f'ci for all intervals up to and including
         // when the closure joint becomes composite (initial loading of closure joint)
         // otherwise use f'c
         IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(poi.GetSegmentKey());
         bFci = (intervalIdx <= compositeClosureIntervalIdx ? true : false);
      }
      else
      {
         // Not at a closure joint... use f'ci if this is at release, otherwise use f'c
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
         bFci = (intervalIdx == releaseIntervalIdx ? true : false);
      }

      Float64 c = pAllowable->GetAllowableCompressiveStressCoefficient(poi,topLocation,intervalIdx,limitState);

      bool bIsTopApplicable = pArtifact->IsApplicable(topLocation);
      bool bIsBotApplicable = pArtifact->IsApplicable(botLocation);

      Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
      Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

      if ( bIsTopApplicable && bIsBotApplicable )
      {
         // Compression stress is applicable at top and bottom... only want to report
         // allowable once if it is the same value
         if ( IsEqual(fAllowableTop,fAllowableBot) )
         {
            *pPara << _T("Allowable compressive stress = -") << c;
            if (bFci)
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            *pPara << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;
         }
         else
         {
            // Not the same value... report each one individually

            *pPara << _T("Top of Girder - Allowable compressive stress = -") << c;
            if (bFci)
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            *pPara << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;

            *pPara << _T("Bottom of Girder - Allowable compressive stress = -") << c;
            if (bFci)
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            *pPara << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;
         }
      }
      else
      {
         // Stress check only applicable at the top or the bottom (not both)
         if ( bIsTopApplicable )
         {
            *pPara << _T("Top of Girder - Allowable compressive stress = -") << c;
            if (bFci)
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            *pPara << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;
         }

         if ( bIsBotApplicable )
         {
            *pPara << _T("Bottom of Girder - Allowable compressive stress = -") << c;
            if (bFci)
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            *pPara << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;
         }
      }
   }

   int fPTZTop    = pSegmentArtifact->GetPrecompressedTensileZone(intervalIdx,topLocation);
   int fPTZBottom = pSegmentArtifact->GetPrecompressedTensileZone(intervalIdx,botLocation);

   //
   // Tension
   //
   if ( pAllowable->IsStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension) )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      bool bIsClosure = poi.HasAttribute(POI_CLOSURE);

      bool bFci;
      if ( bIsClosure )
      {
         // if POI is at a closure joint, use f'ci for all intervals up to and including
         // when the closure joint becomes composite (initial loading of closure joint)
         // otherwise use f'c
         IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(poi.GetSegmentKey());
         bFci = (intervalIdx <= compositeClosureIntervalIdx ? true : false);
      }
      else
      {
         // Not at a closure joint... use f'ci if this is at release, otherwise use f'c
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
         bFci = (intervalIdx == releaseIntervalIdx ? true : false);
      }

      bool bIsTopInPTZ = pArtifact->IsInPrecompressedTensileZone(topLocation);
      bool bIsBotInPTZ = pArtifact->IsInPrecompressedTensileZone(botLocation);

      bool bIsTopApplicable = pSegmentArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,topLocation);

      if ( bIsTopApplicable )
      {
         // stresses are checked in the top of the girder so report the
         // allowable tension for the top of the girder

         Float64 t;            // tension coefficient
         Float64 t_max;        // maximum allowable tension
         bool b_t_max;         // true if max allowable tension is applicable

         // Get allowable tension stress parameters
         pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,false/*without rebar*/,bIsTopInPTZ,&t,&b_t_max,&t_max);

         if ( sysFlags<int>::IsSet(fPTZTop,PTZ_TOP_YES) )
         {
            (*pPara) << _T("Top of Girder - Allowable tensile stress in the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            if ( bFci )
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,topLocation,bIsClosure?POI_CLOSURE:0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsTopInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Top of Girder - Allowable tensile stress in areas with sufficient bonded reinforcement in the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               if ( bFci )
                  *pPara << RPT_FCI;
               else
                  *pPara << RPT_FC;

               fAllowableTop  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,topLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }

         if ( sysFlags<int>::IsSet(fPTZTop,PTZ_TOP_NO) )
         {
            (*pPara) << _T("Top of Girder - Allowable tensile stress in areas other than the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            if ( bFci )
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,topLocation,bIsClosure?POI_CLOSURE:0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsTopInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Top of Girder - Allowable tensile stress in areas with sufficient bonded reinforcement in areas other than the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               if ( bFci )
                  *pPara << RPT_FCI;
               else
                  *pPara << RPT_FC;

               fAllowableTop  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,topLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableTop);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }
      }

      bool bIsBotApplicable = pSegmentArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,botLocation);

      if ( bIsBotApplicable )
      {
         // stresses are checked in the bottom of the girder so report the
         // allowable tension for the bottom of the girder

         Float64 t;            // tension coefficient
         Float64 t_max;        // maximum allowable tension
         bool b_t_max;         // true if max allowable tension is applicable

         // Get allowable tension stress parameters
         pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,false/*without rebar*/,bIsBotInPTZ,&t,&b_t_max,&t_max);

         if ( sysFlags<int>::IsSet(fPTZBottom,PTZ_BOTTOM_YES) )
         {
            (*pPara) << _T("Bottom of Girder - Allowable tensile stress in the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            if ( bFci )
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,botLocation,bIsClosure?POI_CLOSURE:0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsBotInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Bottom of Girder - Allowable tensile stress in areas with sufficient bonded reinforcement in the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               if ( bFci )
                  *pPara << RPT_FCI;
               else
                  *pPara << RPT_FC;

               fAllowableBot  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,botLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }

         if ( sysFlags<int>::IsSet(fPTZBottom,PTZ_BOTTOM_NO) )
         {
            (*pPara) << _T("Bottom of Girder - Allowable tensile stress in areas other than the precompressed tensile zone");

            (*pPara) << _T(" = ") << tension_coeff.SetValue(t) << symbol(ROOT);

            if ( bFci )
               *pPara << RPT_FCI;
            else
               *pPara << RPT_FC;

            if ( b_t_max )
               *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);

            Float64 fAllowableTop = pArtifact->GetCapacity(topLocation);
            Float64 fAllowableBot = pArtifact->GetCapacity(botLocation);

            *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot) << rptNewLine;

            // report allowable stress if the with rebar stress was used anywhere in the segment
            if ( pSegmentArtifact->WasWithRebarAllowableStressUsed(intervalIdx,limitState,botLocation,bIsClosure?POI_CLOSURE:0) )
            {
               Float64 t_with_rebar; // allowable tension when sufficient rebar is used
               pAllowable->GetAllowableTensionStressCoefficient(poi,topLocation,intervalIdx,limitState,true/*with rebar*/,bIsBotInPTZ,&t_with_rebar,&b_t_max,&t_max);

               (*pPara) << _T("Bottom of Girder - Allowable tensile stress in areas with sufficient bonded reinforcement in areas other than the precompressed tensile zone");
            
               (*pPara) << _T(" = ") << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

               if ( bFci )
                  *pPara << RPT_FCI;
               else
                  *pPara << RPT_FC;

               fAllowableBot  = pSegmentArtifact->GetCapacityWithRebar(intervalIdx,limitState,botLocation);
               *pPara  << _T(" = ") << stress_u.SetValue(fAllowableBot);

               *pPara << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.");

               *pPara << rptNewLine;
            }
         }
      }
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pSegmentArtifact->GetRequiredSegmentConcreteStrength(intervalIdx,limitState);
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
