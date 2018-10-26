///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

void CFlexuralStressCheckTable::Build(rptChapter* pChapter,
                                      IBroker* pBroker,
                                      const pgsGirderArtifact* pGirderArtifact,
                                      IEAFDisplayUnits* pDisplayUnits,
                                      IntervalIndexType intervalIdx,
                                      pgsTypes::LimitState limitState,
                                      bool bGirderStresses
                                      ) const
{
   GET_IFACE2_NOCHECK(pBroker,IIntervals,pIntervals); // only used if there are more than one segment in the girder

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
   // Build table
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(girderKey,segIdx == ALL_SEGMENTS ? 0 : segIdx));
   bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(girderKey,intervalIdx);
   bool bIsStressingInterval = (intervalIdx == releaseIntervalIdx || bIsTendonStressingInterval);

   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   std::_tstring strLimitState = pProductLoads->GetLimitStateName(limitState);

   std::_tostringstream os;
   os << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(": ") << pIntervals->GetDescription(intervalIdx) << _T(" : ") << strLimitState << std::endl;

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
   bool bCompression = pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Compression);
   bool bTension     = pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Tension);

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << os.str() << rptNewLine;
   pTitle->SetName(os.str().c_str());

   rptParagraph* pPara = new rptParagraph( pgsReportStyleHolder::GetSubheadingStyle() );
   *pChapter << pPara;

   *pPara << strLimitState << rptNewLine;

   if ( bIsStressingInterval )
   {
      *pPara << _T("For Temporary Stresses before Losses [5.9.4.1]") << rptNewLine;
   }
   else
   {
      *pPara << _T("Stresses at Service Limit State after Losses [5.9.4.2]") << rptNewLine;
   }

   if ( bCompression )
   {
      if ( bIsStressingInterval )
      {
         *pPara << _T("Compression Stresses [5.9.4.1.1]") << rptNewLine;
      }
      else
      {
         *pPara << _T("Compression Stresses [5.9.4.2.1]") << rptNewLine;
      }
   }
   
   if ( bTension )
   {
      if ( bIsStressingInterval )
      {
         *pPara << _T("Tension Stresses [5.9.4.1.2]") << rptNewLine;
      }
      else
      {
         *pPara << _T("Tension Stresses [5.9.4.2.2]") << rptNewLine;
      }
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

   if ( segIdx == ALL_SEGMENTS )
   {
      nColumns++; // second location column
   }


   // Is allowable stress check at top of girder applicable anywhere along the girder
   bool bApplicableTensionTop     = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,    topLocation);
   bool bApplicableCompressionTop = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression,topLocation);

   // Is allowable stress check at bottom of girder applicable anywhere along the girder
   bool bApplicableTensionBot     = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Tension,    botLocation);
   bool bApplicableCompressionBot = pGirderArtifact->IsFlexuralStressCheckApplicable(intervalIdx,limitState,pgsTypes::Compression,botLocation);

   // Was allowable with mild rebar used anywhere along the girder
   bool bIsWithRebarAllowableApplicableTop = pGirderArtifact->IsWithRebarAllowableStressApplicable(intervalIdx,limitState,topLocation);
   bool bIsWithRebarAllowableApplicableBot = pGirderArtifact->IsWithRebarAllowableStressApplicable(intervalIdx,limitState,botLocation);


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
   if ( bIsWithRebarAllowableApplicableTop && bIsWithRebarAllowableApplicableBot )
   {
      nColumns += 2;
   }
   else
   {
      if ( bApplicableTensionTop && bIsWithRebarAllowableApplicableTop )
      {
         nColumns++;
      }

      if ( bApplicableTensionBot && bIsWithRebarAllowableApplicableBot )
      {
         nColumns++;
      }
   }

   // Precompressed Tensile Zone
   nColumns += 2;

   // Status
   if ( bApplicableTensionTop || bApplicableTensionBot )
   {
      nColumns++; // tension status
   }

   if ( bApplicableCompressionTop || bApplicableCompressionBot )
   {
      nColumns++; // compression status
   }

   p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns);
   *p << p_table << rptNewLine;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;

   if ( segIdx == ALL_SEGMENTS )
   {
      p_table->SetRowSpan(0,col1,2);
      p_table->SetRowSpan(1,col2++,SKIP_CELL);
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   p_table->SetRowSpan(0,col1,2);
   p_table->SetRowSpan(1,col2++,SKIP_CELL);
   if ( intervalIdx == releaseIntervalIdx )
   {
      (*p_table)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }
   else
   {
      (*p_table)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }


   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   std::_tstring strLimitState = pProductLoads->GetLimitStateName(limitState);


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

   if ( (bApplicableTensionTop && bIsWithRebarAllowableApplicableTop) || (bApplicableTensionBot && bIsWithRebarAllowableApplicableBot) )
   {
      if ( bIsWithRebarAllowableApplicableTop && bIsWithRebarAllowableApplicableBot )
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
         if ( bIsWithRebarAllowableApplicableTop )
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
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   for ( SegmentIndexType sIdx = firstSegIdx; sIdx <= lastSegIdx; sIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(sIdx);

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
         {
            continue;
         }

#if defined _DEBUG
         if ( pTensionArtifact && pCompressionArtifact )
         {
            // artifacts should be reporting at the same poi
            ATLASSERT(pTensionArtifact->GetPointOfInterest().GetID() == pCompressionArtifact->GetPointOfInterest().GetID());
            ATLASSERT(pTensionArtifact->GetPointOfInterest().GetID() != INVALID_ID);
         }
#endif


         const pgsPointOfInterest& poi( pTensionArtifact ? pTensionArtifact->GetPointOfInterest() : pCompressionArtifact->GetPointOfInterest());

         if ( segIdx == ALL_SEGMENTS )
         {
            (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );
         }

         (*p_table)(row,col++) << location.SetValue( intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT, poi );

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
               else if ( bIsWithRebarAllowableApplicableTop )
               {
                  fTop = pTensionArtifact->GetCapacity(topLocation);
                  (*p_table)(row,col++) << stress.SetValue( fTop );
               }
            }
            else if ( bIsWithRebarAllowableApplicableTop )
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
               else if ( bIsWithRebarAllowableApplicableBot )
               {
                  fBot = pTensionArtifact->GetCapacity(botLocation);
                  (*p_table)(row,col++) << stress.SetValue( fBot );
               }
            }
            else if ( bIsWithRebarAllowableApplicableBot )
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
            else if ( bApplicableTensionTop || bApplicableTensionBot )
            {
               // tension check isn't applicable at this location, but it is applicable
               // somewhere along the girder, so report N/A for this locatino
               (*p_table)(row,col++) << RPT_NA;
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

               if ( bApplicableTensionTop && bIsWithRebarAllowableApplicableTop )
               {
                  (*p_table)(row,col++) << _T("-");
               }

               if ( bApplicableTensionBot && bIsWithRebarAllowableApplicableBot )
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


   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2);
   *pPara << pLayoutTable;

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

      Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);
      if ( intervalIdx == releaseIntervalIdx )
      {
         *p << RPT_FCI << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
      }
      else
      {
         *p << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
      }

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
         {
            *p << _T("Closure Joint ") << LABEL_SEGMENT(sIdx) << rptNewLine;
         }

         IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);

         Float64 fc = pMaterials->GetClosureJointDesignFc(segmentKey,intervalIdx);
         if ( intervalIdx == compositeClosureIntervalIdx )
         {
            *p << RPT_FCI << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
         }
         else
         {
            *p << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;
         }

         BuildAllowClosureJointStressInformation(p, pBroker, pSegmentArtifact, nArtifacts-1, pDisplayUnits, intervalIdx, limitState);

         *p << rptNewLine;
      }
   }
}

void CFlexuralStressCheckTable::BuildAllowDeckStressInformation(rptChapter* pChapter, 
                                                                IBroker* pBroker,
                                                                const pgsGirderArtifact* pGirderArtifact,
                                                                IEAFDisplayUnits* pDisplayUnits,
                                                                IntervalIndexType intervalIdx,
                                                                pgsTypes::LimitState limitState) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress,   pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   GET_IFACE2(pBroker,IAllowableConcreteStress,pAllowable);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   Float64 fc = pMaterials->GetDeckDesignFc(intervalIdx);
   *pPara << RPT_FC << _T(" = ") << stress_u.SetValue(fc) << rptNewLine;

   // using a dummy location to get information... all location should be the same
   // so 0 is as good as any
   const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(0);

   //
   // Compression
   //
   if ( pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Compression) )
   {
      const pgsFlexuralStressArtifact* pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,0);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      Float64 c = pAllowable->GetDeckAllowableCompressionStressCoefficient(poi,intervalIdx,limitState);
      Float64 fAllowable = pAllowable->GetDeckAllowableCompressionStress(poi,intervalIdx,limitState);

      *pPara << _T("Allowable compressive stress = -") << c << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
   }

   //
   // Tension
   //

   if ( pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Tension) )
   {
      const pgsFlexuralStressArtifact* pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,0);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(girderKey,intervalIdx);

      Float64 t;            // tension coefficient
      Float64 t_max;        // maximum allowable tension
      bool b_t_max;         // true if max allowable tension is applicable
      pAllowable->GetDeckAllowableTensionStressCoefficient(poi,intervalIdx,limitState,false/*without rebar*/,&t,&b_t_max,&t_max);

      if ( bIsTendonStressingInterval )
      {
         (*pPara) << _T("Allowable tensile stress in areas other than the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FC;

         if ( b_t_max )
         {
            *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
         }

         Float64 fAllowable = pAllowable->GetDeckAllowableTensionStress(poi,intervalIdx,limitState,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;

         if ( pGirderArtifact->WasDeckWithRebarAllowableStressUsed(intervalIdx,limitState) )
         {
            Float64 t_with_rebar; // allowable tension when sufficient rebar is used
            pAllowable->GetDeckAllowableTensionStressCoefficient(poi,intervalIdx,limitState,true/*with rebar*/,&t_with_rebar,&b_t_max,&t_max);
            fAllowable = pAllowable->GetDeckAllowableTensionStress(poi,intervalIdx,limitState,true/*with rebar*/);

            (*pPara) << _T("Allowable tensile stress in areas with sufficient bonded reinforcement in the precompressed tensile zone = ") 
                     << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT) << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowable)
                     << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

         }
      }
      else
      {
         (*pPara) << _T("Allowable tensile stress in the precompressed tensile zone = ") << tension_coeff.SetValue(t) << symbol(ROOT) << RPT_FC;

         if ( b_t_max )
         {
            *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
         }

         Float64 fAllowable = pAllowable->GetDeckAllowableTensionStress(poi,intervalIdx,limitState,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
      }
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pGirderArtifact->GetRequiredDeckConcreteStrength(intervalIdx,limitState);

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
   if ( pAllowable->IsStressCheckApplicable(segmentKey,intervalIdx,limitState,pgsTypes::Compression) )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(!poi.HasAttribute(POI_CLOSURE));

      // use f'ci if this is at release, otherwise use f'c
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      bool bFci = (intervalIdx == releaseIntervalIdx ? true : false);

      Float64 c = pAllowable->GetSegmentAllowableCompressionStressCoefficient(poi,intervalIdx,limitState);
      Float64 fAllowable = pAllowable->GetSegmentAllowableCompressionStress(poi,intervalIdx,limitState);

      *pPara << _T("Allowable compressive stress = -") << c;

      if ( bFci )
      {
         (*pPara) << RPT_FCI;
      }
      else
      {
         (*pPara) << RPT_FC;
      }
      
      *pPara << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
   }

   //
   // Tension
   //
   if ( pAllowable->IsStressCheckApplicable(segmentKey,intervalIdx,limitState,pgsTypes::Tension) )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT( !poi.HasAttribute(POI_CLOSURE) );
      
      // use f'ci if this is at release, otherwise use f'c
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
      bool bFci = (intervalIdx == releaseIntervalIdx ? true : false);

      bool bIsTendonStressingInterval = pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx);
      bool bIsStressingInterval = (intervalIdx == releaseIntervalIdx || bIsTendonStressingInterval ? true : false);

      Float64 t;            // tension coefficient
      Float64 t_max;        // maximum allowable tension
      bool b_t_max;         // true if max allowable tension is applicable
      pAllowable->GetSegmentAllowableTensionStressCoefficient(poi,intervalIdx,limitState,false/*without rebar*/,&t,&b_t_max,&t_max);

      if ( bIsStressingInterval )
      {
         (*pPara) << _T("Allowable tensile stress in areas other than the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t) << symbol(ROOT);
         
         if ( bFci )
         {
            (*pPara) << RPT_FCI;
         }
         else
         {
            (*pPara) << RPT_FC;
         }

         if ( b_t_max )
         {
            *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
         }

         Float64 fAllowable = pAllowable->GetSegmentAllowableTensionStress(poi,intervalIdx,limitState,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;

         if ( pSegmentArtifact->IsSegmentWithRebarAllowableStressApplicable(intervalIdx,limitState) )
         {
            Float64 t_with_rebar; // allowable tension when sufficient rebar is used
            pAllowable->GetSegmentAllowableTensionStressCoefficient(poi,intervalIdx,limitState,true/*with rebar*/,&t_with_rebar,&b_t_max,&t_max);
            fAllowable = pAllowable->GetSegmentAllowableTensionStress(poi,intervalIdx,limitState,true/*with rebar*/);

            (*pPara) << _T("Allowable tensile stress in areas with sufficient bonded reinforcement in the precompressed tensile zone = ") 
                     << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT);

            if ( bFci )
            {
               (*pPara) << RPT_FCI;
            }
            else
            {
               (*pPara) << RPT_FC;
            }

            (*pPara) << _T(" = ") << stress_u.SetValue(fAllowable)
                     << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

         }
      }
      else
      {
         (*pPara) << _T("Allowable tensile stress in the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t) << symbol(ROOT);

         if ( bFci )
         {
            (*pPara) << RPT_FCI;
         }
         else
         {
            (*pPara) << RPT_FC;
         }

         if ( b_t_max )
         {
            *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
         }

         Float64 fAllowable = pAllowable->GetSegmentAllowableTensionStress(poi,intervalIdx,limitState,false/*without rebar*/);
         *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
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

void CFlexuralStressCheckTable::BuildAllowClosureJointStressInformation(rptParagraph* pPara, 
                                           IBroker* pBroker,
                                           const pgsSegmentArtifact* pSegmentArtifact,
                                           IndexType artifactIdx,
                                           IEAFDisplayUnits* pDisplayUnits,
                                           IntervalIndexType intervalIdx,
                                           pgsTypes::LimitState limitState) const
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
   if ( pAllowable->IsStressCheckApplicable(segmentKey,intervalIdx,limitState,pgsTypes::Compression) )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Compression));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Compression,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(poi.HasAttribute(POI_CLOSURE));

      // use f'ci for all intervals up to and including
      // when the closure joint becomes composite (initial loading of closure joint)
      // otherwise use f'c
      IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(segmentKey);
      bool bFci = (intervalIdx <= compositeClosureIntervalIdx ? true : false);

      Float64 c = pAllowable->GetClosureJointAllowableCompressionStressCoefficient(poi,intervalIdx,limitState);
      Float64 fAllowable = pAllowable->GetClosureJointAllowableCompressionStress(poi,intervalIdx,limitState);
      *pPara << _T("Allowable compressive stress = -") << c;
      if (bFci)
      {
         *pPara << RPT_FCI;
      }
      else
      {
         *pPara << RPT_FC;
      }

      *pPara << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;
   }

   //
   // Tension
   //
   if ( pAllowable->IsStressCheckApplicable(segmentKey,intervalIdx,limitState,pgsTypes::Tension) )
   {
      ATLASSERT( 0 < pSegmentArtifact->GetFlexuralStressArtifactCount(intervalIdx,limitState,pgsTypes::Tension));
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifact( intervalIdx,limitState,pgsTypes::Tension,artifactIdx);
      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());
      ATLASSERT(poi.HasAttribute(POI_CLOSURE));

      // use f'ci for all intervals up to and including
      // when the closure joint becomes composite (initial loading of closure joint)
      // otherwise use f'c
      IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(poi.GetSegmentKey());
      bool bFci = (intervalIdx <= compositeClosureIntervalIdx ? true : false);

      Float64 t;            // tension coefficient
      Float64 t_max;        // maximum allowable tension
      bool b_t_max;         // true if max allowable tension is applicable

      // Precompressed tensile zone
      pAllowable->GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,limitState,false/*without rebar*/,true/*in PTZ*/,&t,&b_t_max,&t_max);

      (*pPara) << _T("Allowable tensile stress in the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t) << symbol(ROOT);
         
      if ( bFci )
      {
         *pPara << RPT_FCI;
      }
      else
      {
         *pPara << RPT_FC;
      }

      if ( b_t_max )
      {
         *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
      }

      Float64 fAllowable = pAllowable->GetClosureJointAllowableTensionStress(poi,intervalIdx,limitState,false/*without rebar*/,true/*in PTZ*/);
      *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;

      if ( pSegmentArtifact->WasClosureJointWithRebarAllowableStressUsed(intervalIdx,limitState,true/*in PTZ*/) )
      {
         Float64 t_with_rebar; // allowable tension when sufficient rebar is used
         pAllowable->GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,limitState,true/*with rebar*/,true/*in PTZ*/,&t_with_rebar,&b_t_max,&t_max);
         fAllowable = pAllowable->GetClosureJointAllowableTensionStress(poi,intervalIdx,limitState,true/*with rebar*/,true/*in PTZ*/);

         (*pPara) << _T("Allowable tensile stress in joints with minimum bonded auxiliary reinforcement in the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT) << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowable)
                  << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

      }


      // Other than Precompressed tensile zone
      pAllowable->GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,limitState,false/*without rebar*/,false/*not in PTZ*/,&t,&b_t_max,&t_max);

      (*pPara) << _T("Allowable tensile stress in areas other than the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t) << symbol(ROOT);
         
      if ( bFci )
      {
         *pPara << RPT_FCI;
      }
      else
      {
         *pPara << RPT_FC;
      }

      if ( b_t_max )
      {
         *pPara << _T(" but not more than ") << stress_u.SetValue(t_max);
      }

      fAllowable = pAllowable->GetClosureJointAllowableTensionStress(poi,intervalIdx,limitState,false/*without rebar*/,false/*not in PTZ*/);
      *pPara  << _T(" = ") << stress_u.SetValue(fAllowable) << rptNewLine;

      if ( pSegmentArtifact->WasClosureJointWithRebarAllowableStressUsed(intervalIdx,limitState,false/*not in PTZ*/) )
      {
         Float64 t_with_rebar; // allowable tension when sufficient rebar is used
         pAllowable->GetClosureJointAllowableTensionStressCoefficient(poi,intervalIdx,limitState,true/*with rebar*/,false/*not in PTZ*/,&t_with_rebar,&b_t_max,&t_max);
         fAllowable = pAllowable->GetClosureJointAllowableTensionStress(poi,intervalIdx,limitState,true/*with rebar*/,false/*not in PTZ*/);

         (*pPara) << _T("Allowable tensile stress in joints with minimum bonded auxiliary reinforcement in areas other than the precompressed tensile zone = ") 
                  << tension_coeff.SetValue(t_with_rebar) << symbol(ROOT) << RPT_FC << _T(" = ") << stress_u.SetValue(fAllowable)
                  << _T(" if bonded reinforcement sufficient to resist the tensile force in the concrete is provided.") << rptNewLine;

      }
   }

   //
   // Required Strength
   //
   Float64 fc_reqd = pSegmentArtifact->GetRequiredClosureJointConcreteStrength(intervalIdx,limitState);
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
