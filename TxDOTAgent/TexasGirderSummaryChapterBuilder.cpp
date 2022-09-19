///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <Reporting\SpanGirderReportSpecification.h>
#include <ReportManager\ReportManager.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include "TexasGirderSummaryChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"

#include <ReportManager\ReportManager.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\BridgeDescription2.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>

#include <psgLib\ConnectionLibraryEntry.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);

/****************************************************************************
CLASS
   CTexasGirderSummaryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasGirderSummaryChapterBuilder::CTexasGirderSummaryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasGirderSummaryChapterBuilder::GetName() const
{
   return TEXT("Girder Summary");
}

rptChapter* CTexasGirderSummaryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   // This is a single segment report
   CSegmentKey segmentKey(girderKey,0);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   // eject a page break before this chapter
   pChapter->SetEjectPageBreakBefore(true);

   // let the paragraph builder to all the work here...
   bool doEjectPage;
   CTexasIBNSParagraphBuilder parabuilder;
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   rptParagraph* pcontent = parabuilder.Build(pBroker,segmentKeys,pDisplayUnits,level,doEjectPage);

   *pChapter << pcontent;

   // girder line geometry table
   girder_line_geometry( pChapter, pBroker, segmentKey, pDisplayUnits );

   // put a page break at bottom of table
   if (doEjectPage)
   {
      rptParagraph* p = new rptParagraph;
      *pChapter << p;
      *p << rptNewPage;
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTexasGirderSummaryChapterBuilder::Clone() const
{
   return std::make_unique<CTexasGirderSummaryChapterBuilder>();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////
void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("Girder Line Geometry"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, olay,      pDisplayUnits->GetOverlayWeightUnit(), true );

   // Get the interfaces we need
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   const CPierData2* pPrevPier = pGroup->GetPier(pgsTypes::metStart);
   const CPierData2* pNextPier = pGroup->GetPier(pgsTypes::metEnd);

   rptLengthUnitValue* pUnitValue = (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) ? &length : &component);


   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   std::_tstring strGirderSpacingMeasureAtStartOfSpan, strGirderSpacingMeasureAtEndOfSpan;
   std::_tstring* pStr;
   for ( int i = 0; i < 2; i++ )
   {
      long hash;
      if ( i == 0 )
      {
         pStr = &strGirderSpacingMeasureAtStartOfSpan;
         hash = HashGirderSpacing(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetMeasurementLocation(),pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetMeasurementType());
      }
      else
      {
         pStr = &strGirderSpacingMeasureAtEndOfSpan;
         hash = HashGirderSpacing(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetMeasurementLocation(),pNextPier->GetGirderSpacing(pgsTypes::Back)->GetMeasurementType());
      }

      if ( hash == HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem) )
      {
         if ( (i == 0 && pPrevPier->GetPrevSpan() == nullptr) || (i == 1 && pNextPier->GetNextSpan() == nullptr) )
            *pStr = _T("Measured at and along the abutment line");
         else
            *pStr = _T("Measured at and along the pier line");
      }
      else if ( hash == HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem) )
      {
         *pStr = _T("Measured at and along the centerline bearing");
      }
      else if ( hash == HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem) )
      {
         if ( (i == 0 && pPrevPier->GetPrevSpan() == nullptr) || (i == 1 && pNextPier->GetNextSpan() == nullptr) )
            *pStr = _T("Measured normal to alignment at abutment line");
         else
            *pStr = _T("Measured normal to alignment at pier line");
      }
      else if ( hash == HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem) )
	   {
         *pStr = _T("Measured normal to alignment at centerline bearing");
	   }
   }


   RowIndexType row = 0;

   // Populate the table
   (*pTable)(row,0) << _T("Girder Type");
   (*pTable)(row++,1) << pGroup->GetGirder(segmentKey.girderIndex)->GetGirderName();

   (*pTable)(row,0) << _T("Span Length, CL Bearing to CL Bearing") ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSegmentSpanLength(segmentKey));

   (*pTable)(row,0) << _T("Girder Length") ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSegmentLength(segmentKey));

   (*pTable)(row,0) << _T("Number of Girders");
   (*pTable)(row++,1) << pBridge->GetGirderCount(segmentKey.groupIndex);

   if ( pBridge->IsInteriorGirder( segmentKey ) )
   {
      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         (*pTable)(row,0) << _T("Girder Spacing Datum\nStart of Span");
         (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
         (*pTable)(row,0) << _T("Left Girder Spacing\nStart of Span");
      }
      else
      {
         (*pTable)(row,0) << _T("Joint Spacing Datum\nStart of Span");
         (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
         (*pTable)(row,0) << _T("Left Girder Joint\nStart of Span");
      }

      if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(segmentKey.girderIndex-1));
      else
         (*pTable)(row++,1) << _T(" - ");

      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         (*pTable)(row,0) << _T("Right Girder Spacing\nStart of Span");
      }
      else
      {
         (*pTable)(row,0) << _T("Right Joint Spacing\nStart of Span");
      }

      if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(segmentKey.girderIndex));
      else
         (*pTable)(row++,1) << _T(" - ");


      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         (*pTable)(row,0) << _T("Girder Spacing Datum\nEnd of Span");
         (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
         (*pTable)(row,0) << _T("Left Girder Spacing\nEnd of Span");
      }
      else
      {
         (*pTable)(row,0) << _T("Joint Spacing Datum\nEnd of Span");
         (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
         (*pTable)(row,0) << _T("Left Joint Spacing\nEnd of Span");
      }

      if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetGirderSpacing(segmentKey.girderIndex-1));
      else
         (*pTable)(row++,1) << _T(" - ");

      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         (*pTable)(row,0) << _T("Right Girder Spacing\nEnd of Span");
      else
         (*pTable)(row,0) << _T("Right Joint Spacing\nEnd of Span");

      if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetGirderSpacing(segmentKey.girderIndex));
      else
         (*pTable)(row++,1) << _T(" - ");
   }
   else
   {
      if ( segmentKey.girderIndex == 0 )
      {
         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         {
            (*pTable)(row,0) << _T("Girder Spacing Datum\nStart of Span");
            (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
            (*pTable)(row,0) << _T("Right Girder Spacing\nStart of Span");
         }
         else
         {
            (*pTable)(row,0) << _T("Joint Spacing Datum\nStart of Span");
            (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
            (*pTable)(row,0) << _T("Right Joint Spacing\nStart of Span");
         }

         if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(segmentKey.girderIndex));
         else
            (*pTable)(row++,1) << _T(" - ");

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         {
            (*pTable)(row,0) << _T("Girder Spacing Datum\nEnd of Span");
            (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
            (*pTable)(row,0) << _T("Right Girder Spacing\nEnd of Span");
         }
         else
         {
            (*pTable)(row,0) << _T("Joint Spacing Datum\nEnd of Span");
            (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
            (*pTable)(row,0) << _T("Right Joint Spacing\nEnd of Span");
         }

         if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetGirderSpacing(segmentKey.girderIndex));
         else
            (*pTable)(row++,1) << _T(" - ");
      }
      else
      {
         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << _T("Left Girder Spacing\nStart of Span");
         else
            (*pTable)(row,0) << _T("Left Joint Spacing\nStart of Span");

         GirderIndexType nGirders = pGroup->GetGirderCount();

         if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing(SpacingIndexType(nGirders-2)));
         else
            (*pTable)(row++,1) << _T(" - ");

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << _T("Left Girder Spacing\nEnd of Span");
         else
            (*pTable)(row,0) << _T("Left Joint Spacing\nEnd of Span");

         if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetGirderSpacing(SpacingIndexType(nGirders-2)));
         else
            (*pTable)(row++,1) << _T(" - ");
      }
   }

   pgsPointOfInterest poi(segmentKey,0.00);
   (*pTable)(row,0) << _T("Slab Thickness for Design");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( poi ));

   (*pTable)(row,0) << _T("Slab Thickness for Construction");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetGrossSlabDepth( poi ));

   (*pTable)(row,0) << _T("Slab Offset at Start (\"A\" Dimension)");
   (*pTable)(row++,1) << component.SetValue(pSegment->GetSlabOffset(pgsTypes::metStart));

   (*pTable)(row,0) << _T("Slab Offset at End (\"A\" Dimension)");
   (*pTable)(row++,1) << component.SetValue(pSegment->GetSlabOffset(pgsTypes::metEnd));

   (*pTable)(row,0) << _T("Overlay");
   (*pTable)(row++,1) << olay.SetValue(pDeck->OverlayWeight);

   (*pTable)(row,0) << _T("Left Traffic Barrier");
   (*pTable)(row++,1) << pBridgeDesc->GetLeftRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << _T("Right Traffic Barrier");
   (*pTable)(row++,1) << pBridgeDesc->GetRightRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << _T("Traffic Barrier Weight (per girder)");
   (*pTable)(row++,1) << fpl.SetValue( -pProductLoads->GetTrafficBarrierLoad(segmentKey) );

   Float64 brgOffset, endDistance;
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
   ConnectionLibraryEntry::EndDistanceMeasurementType endDistanceMeasure;
   pPrevPier->GetBearingOffset(pgsTypes::Ahead,&brgOffset,&brgOffsetMeasure);
   pPrevPier->GetGirderEndDistance(pgsTypes::Ahead,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << LABEL_PIER_EX(pPrevPier->IsAbutment(),pPrevPier->GetIndex());
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pPrevPier->IsAbutment(),false) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pPrevPier->IsAbutment(),false);
   row++;

   pNextPier->GetBearingOffset(pgsTypes::Back,&brgOffset,&brgOffsetMeasure);
   pNextPier->GetGirderEndDistance(pgsTypes::Back,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << LABEL_PIER_EX(pNextPier->IsAbutment(), pNextPier->GetIndex());
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pNextPier->IsAbutment(),false) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pNextPier->IsAbutment(),false);
   row++;

}
