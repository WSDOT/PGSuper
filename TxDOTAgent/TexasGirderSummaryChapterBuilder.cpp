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
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <ReportManager\ReportManager.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include "TexasGirderSummaryChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"

#include <ReportManager\ReportManager.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PierData.h>
#include <PgsExt\BridgeDescription.h>

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

static void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);

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

rptChapter* CTexasGirderSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   // let the paragraph builder to all the work here...
   std::vector<SpanGirderHashType> spanGirders;
   spanGirders.push_back( HashSpanGirder(span, girder) );

   CTexasIBNSParagraphBuilder parabuilder;
   rptParagraph* pcontent = parabuilder.Build(pBroker,spanGirders,pDisplayUnits,level);

   *pChapter << pcontent;

   // girder line geometry table
   girder_line_geometry( pChapter, pBroker, span, girder, pDisplayUnits );


   return pChapter;
}

CChapterBuilder* CTexasGirderSummaryChapterBuilder::Clone() const
{
   return new CTexasGirderSummaryChapterBuilder;
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
void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Girder Line Geometry"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, olay,      pDisplayUnits->GetOverlayWeightUnit(), true );

   // Get the interfaces we need
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();

   const CPierData* pPrevPier = pSpan->GetPrevPier();
   const CPierData* pNextPier = pSpan->GetNextPier();

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
         hash = HashGirderSpacing(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetMeasurementLocation(),pSpan->GetGirderSpacing(pgsTypes::metStart)->GetMeasurementType());
      }
      else
      {
         pStr = &strGirderSpacingMeasureAtEndOfSpan;
         hash = HashGirderSpacing(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetMeasurementLocation(),pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetMeasurementType());
      }

      if ( hash == HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem) )
      {
         if ( (i == 0 && pPrevPier->GetPrevSpan() == NULL) || (i == 1 && pNextPier->GetNextSpan() == NULL) )
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
         if ( (i == 0 && pPrevPier->GetPrevSpan() == NULL) || (i == 1 && pNextPier->GetNextSpan() == NULL) )
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
   (*pTable)(row++,1) << pSpan->GetGirderTypes()->GetGirderName(girder);

   (*pTable)(row,0) << _T("Span Length, CL Bearing to CL Bearing") ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSpanLength(span,girder));

   (*pTable)(row,0) << _T("Girder Length") ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetGirderLength(span,girder));

   (*pTable)(row,0) << _T("Number of Girders");
   (*pTable)(row++,1) << pBridge->GetGirderCount(span);

   if ( pBridge->IsInteriorGirder(span,girder) )
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

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(girder-1));
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

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(girder));
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

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(girder-1));
      else
         (*pTable)(row++,1) << _T(" - ");

      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         (*pTable)(row,0) << _T("Right Girder Spacing\nEnd of Span");
      else
         (*pTable)(row,0) << _T("Right Joint Spacing\nEnd of Span");

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(girder));
      else
         (*pTable)(row++,1) << _T(" - ");
   }
   else
   {
      if ( girder == 0 )
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

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(girder));
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

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(girder));
         else
            (*pTable)(row++,1) << _T(" - ");
      }
      else
      {
         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << _T("Left Girder Spacing\nStart of Span");
         else
            (*pTable)(row,0) << _T("Left Joint Spacing\nStart of Span");

         GirderIndexType nGirders = pSpan->GetGirderCount();

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(SpacingIndexType(nGirders-2)));
         else
            (*pTable)(row++,1) << _T(" - ");

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << _T("Left Girder Spacing\nEnd of Span");
         else
            (*pTable)(row,0) << _T("Left Joint Spacing\nEnd of Span");

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(SpacingIndexType(nGirders-2)));
         else
            (*pTable)(row++,1) << _T(" - ");
      }
   }

#pragma Reminder("UPDATE: Assumes constant slab thickness")   
   pgsPointOfInterest poi(span,girder,0.00);
   (*pTable)(row,0) << _T("Slab Thickness for Design");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( poi ));

   (*pTable)(row,0) << _T("Slab Thickness for Construction");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetGrossSlabDepth( poi ));

   (*pTable)(row,0) << _T("Slab Offset at Start (\"A\" Dimension)");
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metStart));

   (*pTable)(row,0) << _T("Slab Offset at End (\"A\" Dimension)");
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metEnd));

   (*pTable)(row,0) << _T("Overlay");
   (*pTable)(row++,1) << olay.SetValue(pDeck->OverlayWeight);

#pragma Reminder("#*#*#*#*#*# TXDOT girder summary - Diaphragms #*#*#*#*#*#")
//   (*pTable)(row,0) << _T("Intermediate Diaphragm (H x W)");
//   (*pTable)(row,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmHeight() );
//   (*pTable)(row,1) << _T(" x ");
//   (*pTable)(row++,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmWidth() );


#pragma Reminder("UPDATE:: Update for new railing/sidewalk system")
   (*pTable)(row,0) << _T("Left Traffic Barrier");
   (*pTable)(row++,1) << pBridgeDesc->GetLeftRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << _T("Right Traffic Barrier");
   (*pTable)(row++,1) << pBridgeDesc->GetRightRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << _T("Traffic Barrier Weight (per girder)");
   (*pTable)(row++,1) << fpl.SetValue( -pProductLoads->GetTrafficBarrierLoad(span,girder) );

   const CPierData* pPier = pBridgeDesc->GetPier(span);
   std::_tstring strPierLabel(pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));
   Float64 brgOffset, endDistance;
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
   ConnectionLibraryEntry::EndDistanceMeasurementType endDistanceMeasure;
   pPier->GetBearingOffset(pgsTypes::Ahead,&brgOffset,&brgOffsetMeasure);
   pPier->GetGirderEndDistance(pgsTypes::Ahead,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(span);
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pPier->IsAbutment()) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pPier->IsAbutment());
   row++;

   pPier = pBridgeDesc->GetPier(span+1);
   strPierLabel = (pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));
   pPier->GetBearingOffset(pgsTypes::Back,&brgOffset,&brgOffsetMeasure);
   pPier->GetGirderEndDistance(pgsTypes::Back,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(span+1);
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pPier->IsAbutment()) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pPier->IsAbutment());
   row++;
}
