///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include "TexasGirderSummaryChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"

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
CTexasGirderSummaryChapterBuilder::CTexasGirderSummaryChapterBuilder()
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


#if defined IGNORE_2007_CHANGES
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   // let the paragraph builder to all the work here...
   CTexasIBNSParagraphBuilder parabuilder;
   rptParagraph* pcontent = parabuilder.Build(pBroker,span,girder,pDisplayUnits,level);

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

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,"Girder Line Geometry");
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
   std::string strGirderSpacingMeasureAtStartOfSpan, strGirderSpacingMeasureAtEndOfSpan;
   std::string* pStr;
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

      if ( hash == HashGirderSpacing(pgsTypes::AtCenterlinePier,pgsTypes::AlongItem) )
         *pStr = "Measured at and along the centerline pier";
      else if ( hash == HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem) )
         *pStr = "Measured at and along the centerline bearing";
      else if ( hash == HashGirderSpacing(pgsTypes::AtCenterlinePier,pgsTypes::NormalToItem) )
         *pStr = "Measured normal to alignment at centerline pier";
      else if ( hash == HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem) )
         *pStr = "Measured normal to alignment at centerline bearing";
   }


   RowIndexType row = 0;

   // Populate the table
   (*pTable)(row,0) << "Girder Type";
   (*pTable)(row++,1) << pSpan->GetGirderTypes()->GetGirderName(girder);

   (*pTable)(row,0) << "Span Length, CL Bearing to CL Bearing" ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSpanLength(span,girder));

   (*pTable)(row,0) << "Girder Length" ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetGirderLength(span,girder));

   (*pTable)(row,0) << "Number of Girders";
   (*pTable)(row++,1) << pBridge->GetGirderCount(span);

   if ( pBridge->IsInteriorGirder(span,girder) )
   {
      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         (*pTable)(row,0) << "Girder Spacing Datum\nStart of Span";
         (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
         (*pTable)(row,0) << "Left Girder Spacing\nStart of Span";
      }
      else
      {
         (*pTable)(row,0) << "Joint Spacing Datum\nStart of Span";
         (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
         (*pTable)(row,0) << "Left Girder Joint\nStart of Span";
      }

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(girder-1));
      else
         (*pTable)(row++,1) << " - ";

      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         (*pTable)(row,0) << "Right Girder Spacing\nStart of Span";
      }
      else
      {
         (*pTable)(row,0) << "Right Joint Spacing\nStart of Span";
      }

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(girder));
      else
         (*pTable)(row++,1) << " - ";


      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         (*pTable)(row,0) << "Girder Spacing Datum\nEnd of Span";
         (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
         (*pTable)(row,0) << "Left Girder Spacing\nEnd of Span";
      }
      else
      {
         (*pTable)(row,0) << "Joint Spacing Datum\nEnd of Span";
         (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
         (*pTable)(row,0) << "Left Joint Spacing\nEnd of Span";
      }

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(girder-1));
      else
         (*pTable)(row++,1) << " - ";

      if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         (*pTable)(row,0) << "Right Girder Spacing\nEnd of Span";
      else
         (*pTable)(row,0) << "Right Joint Spacing\nEnd of Span";

      if ( 1 < pBridge->GetGirderCount(span) )
         (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(girder));
      else
         (*pTable)(row++,1) << " - ";
   }
   else
   {
      if ( girder == 0 )
      {
         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         {
            (*pTable)(row,0) << "Girder Spacing Datum\nStart of Span";
            (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
            (*pTable)(row,0) << "Right Girder Spacing\nStart of Span";
         }
         else
         {
            (*pTable)(row,0) << "Joint Spacing Datum\nStart of Span";
            (*pTable)(row++,1) << strGirderSpacingMeasureAtStartOfSpan;
            (*pTable)(row,0) << "Right Joint Spacing\nStart of Span";
         }

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(girder));
         else
            (*pTable)(row++,1) << " - ";

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
         {
            (*pTable)(row,0) << "Girder Spacing Datum\nEnd of Span";
            (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
            (*pTable)(row,0) << "Right Girder Spacing\nEnd of Span";
         }
         else
         {
            (*pTable)(row,0) << "Joint Spacing Datum\nEnd of Span";
            (*pTable)(row++,1) << strGirderSpacingMeasureAtEndOfSpan;
            (*pTable)(row,0) << "Right Joint Spacing\nEnd of Span";
         }

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(girder));
         else
            (*pTable)(row++,1) << " - ";
      }
      else
      {
         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << "Left Girder Spacing\nStart of Span";
         else
            (*pTable)(row,0) << "Left Joint Spacing\nStart of Span";

         GirderIndexType nGirders = pSpan->GetGirderCount();

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(SpacingIndexType(nGirders-2)));
         else
            (*pTable)(row++,1) << " - ";

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << "Left Girder Spacing\nEnd of Span";
         else
            (*pTable)(row,0) << "Left Joint Spacing\nEnd of Span";

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(SpacingIndexType(nGirders-2)));
         else
            (*pTable)(row++,1) << " - ";
      }
   }

#pragma Reminder("UPDATE: Assumes constant slab thickness")   
   pgsPointOfInterest poi(span,girder,0.00);
   (*pTable)(row,0) << "Slab Thickness for Design";
   (*pTable)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( poi ));

   (*pTable)(row,0) << "Slab Thickness for Construction";
   (*pTable)(row++,1) << component.SetValue(pBridge->GetGrossSlabDepth( poi ));

   (*pTable)(row,0) << "Slab Offset at Start (\"A\" Dimension)";
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metStart));

   (*pTable)(row,0) << "Slab Offset at End (\"A\" Dimension)";
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metEnd));

   (*pTable)(row,0) << "Overlay";
   (*pTable)(row++,1) << olay.SetValue(pDeck->OverlayWeight);

#pragma Reminder("#*#*#*#*#*# TXDOT girder summary - Diaphragms #*#*#*#*#*#")
//   (*pTable)(row,0) << "Intermediate Diaphragm (H x W)";
//   (*pTable)(row,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmHeight() );
//   (*pTable)(row,1) << " x ";
//   (*pTable)(row++,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmWidth() );


#pragma Reminder("UPDATE:: Update for new railing/sidewalk system")
   (*pTable)(row,0) << "Left Traffic Barrier";
   (*pTable)(row++,1) << pBridgeDesc->GetLeftRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << "Right Traffic Barrier";
   (*pTable)(row++,1) << pBridgeDesc->GetRightRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << "Traffic Barrier Weight (per girder)";
   (*pTable)(row++,1) << fpl.SetValue( -pProductLoads->GetTrafficBarrierLoad(span,girder) );

   (*pTable)(row,0) << "Connection type at Pier " << LABEL_PIER(span);
   (*pTable)(row++,1) << pPrevPier->GetConnection(pgsTypes::Ahead);

   (*pTable)(row,0) << "Connection type at Pier " << LABEL_PIER(span+1);
   (*pTable)(row++,1) << pNextPier->GetConnection(pgsTypes::Back);
}
