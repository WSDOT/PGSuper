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
#include "InputSummaryChapter.h"
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PierData.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PgsExt\GirderData.h>

#include <Material\PsStrand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CInputSummaryChapter
****************************************************************************/


void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void concrete(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);
void prestressing(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CInputSummaryChapter::CInputSummaryChapter(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CInputSummaryChapter::GetName() const
{
   return TEXT("Input Summary");
}

rptChapter* CInputSummaryChapter::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   CSpanGirderReportSpecification* pSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType spanIdx = pSpec->GetSpan();
   GirderIndexType gdrIdx = pSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* p;
   
   p = new rptParagraph;
   *pChapter << p;
   *p << color(Red) << _T("NOTE: Several details have been omitted from this report") << color(Black) << rptNewLine;

   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeometry );
   if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(spanIdx, gdrIdx))
   {
      *p << color(Red) << Bold(_T("Warning: This is a non-standard girder because it utilizes straight web strands. WSDOT Standard Girders utilize harped strands.")) << color(Black) << rptNewLine;
   }

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);

   const CGirderData& girderData = pSpan->GetGirderTypes()->GetGirderData(gdrIdx);
   if (girderData.PrestressData.GetNumPermStrandsType() == NPS_DIRECT_SELECTION)
   {
      *p << color(Red) << Bold(_T("Warning: This is a non-standard girder because it utilizes Direct Strand Fill. WSDOT Standard Girders utilize sequentially filled strands.")) << color(Black) << rptNewLine;
   }

   girder_line_geometry( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   concrete( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );
   prestressing( pChapter, pBroker, spanIdx, gdrIdx, pDisplayUnits );

   return pChapter;
}

CChapterBuilder* CInputSummaryChapter::Clone() const
{
   return new CInputSummaryChapter;
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

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Girder Line Geometry"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   length,    pDisplayUnits->GetSpanLengthUnit(),    true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   component, pDisplayUnits->GetComponentDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, olay,      pDisplayUnits->GetOverlayWeightUnit(), true );
   
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( glength, IS_US_UNITS(pDisplayUnits), 4, pDisplayUnits->GetSpanLengthUnit(),   true, false );
   INIT_FRACTIONAL_LENGTH_PROTOTYPE( spacing, IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), true, false );

   // Get the interfaces we need
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();

   const CPierData* pPrevPier = pSpan->GetPrevPier();
   const CPierData* pNextPier = pSpan->GetNextPier();

   rptLengthUnitValue* pUnitValue = (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) ? &glength : &spacing);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);

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

   // Populate the table
   RowIndexType row = 0;

   (*pTable)(row,0) << _T("Girder Type");
   (*pTable)(row++,1) << pSpan->GetGirderTypes()->GetGirderName(girder);

   (*pTable)(row,0) << _T("Span Length, CL Bearing to CL Bearing") ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSpanLength(span,girder));

   (*pTable)(row,0) << _T("Girder Length") ;
   (*pTable)(row++,1) << glength.SetValue(pBridge->GetGirderLength(span,girder));

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
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing( SpacingIndexType(nGirders-2) ));
         else
            (*pTable)(row++,1) << _T(" - ");

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << _T("Left Girder Spacing\nEnd of Span");
         else
            (*pTable)(row,0) << _T("Left Joint Spacing\nEnd of Span");

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing( SpacingIndexType(nGirders-2) ));
         else
            (*pTable)(row++,1) << _T(" - ");
      }
   }

#pragma Reminder("UPDATE: Assumes constant deck thickness")   
   pgsPointOfInterest poi(span,girder,0.00);
   (*pTable)(row,0) << _T("Slab Thickness for Design");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( poi ));

   (*pTable)(row,0) << _T("Slab Thickness for Construction");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetGrossSlabDepth( poi ));

   (*pTable)(row,0) << _T("\"A\" Dimension at Start");
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metStart));

   (*pTable)(row,0) << _T("\"A\" Dimension at End");
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metEnd));

   (*pTable)(row,0) << _T("Overlay");
   (*pTable)(row++,1) << olay.SetValue(pDeck->OverlayWeight);

#pragma Reminder("#*#*#*#*#*# WSDOT girder summary - Diaphragms #*#*#*#*#*#")
//   (*pTable)(row,0) << _T("Intermediate Diaphragm (H x W)");
//   (*pTable)(row,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmHeight() );
//   (*pTable)(row,1) << _T(" x ");
//   (*pTable)(row++,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmWidth() );

#pragma Reminder("UPDATE - update for new railing/sidewalk model")
   (*pTable)(row,0) << _T("Left Traffic Barrier");
   (*pTable)(row++,1) << pBridgeDesc->GetLeftRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << _T("Right Traffic Barrier");
   (*pTable)(row++,1) << pBridgeDesc->GetRightRailingSystem()->strExteriorRailing;

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

   (*pTable)(row,0) << _T("Connection Boundary Condition at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(span);
   (*pTable)(row++,1) << CPierData::AsString(pPrevPier->GetConnectionType());

   pPier = pBridgeDesc->GetPier(span+1);
   strPierLabel = (pPier->IsAbutment() ? _T("Abutment") : _T("Pier"));
   pPier->GetBearingOffset(pgsTypes::Back,&brgOffset,&brgOffsetMeasure);
   pPier->GetGirderEndDistance(pgsTypes::Back,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(span+1);
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pPier->IsAbutment()) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pPier->IsAbutment());
   row++;

   (*pTable)(row,0) << _T("Connection Boundary Condition at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(span+1);
   (*pTable)(row++,1) << CPierData::AsString(pNextPier->GetConnectionType());

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      (*pTable)(row,0) << _T("Left Lifting Point Location");
      (*pTable)(row++,1) << length.SetValue(pGirderLifting->GetLeftLiftingLoopLocation(span,girder));

      (*pTable)(row,0) << _T("Right Lifting Point Location");
      (*pTable)(row++,1) << length.SetValue(pGirderLifting->GetRightLiftingLoopLocation(span,girder));
   }

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      (*pTable)(row,0) << _T("Leading Truck Support Location");
      (*pTable)(row++,1) << length.SetValue(pGirderHauling->GetLeadingOverhang(span,girder));

      (*pTable)(row,0) << _T("Trailing Truck Support Location");
      (*pTable)(row++,1) << length.SetValue(pGirderHauling->GetTrailingOverhang(span,girder));
   }
}

void concrete(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Concrete"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
   
   // Get the interfaces
   GET_IFACE2( pBroker, IBridgeMaterial, pMat );

   // Populate the table
   (*pTable)(0,0) << _T("Girder : 28 day strength");
   (*pTable)(0,1) << stress.SetValue( pMat->GetFcGdr(span,girder) );

   (*pTable)(1,0) << _T("Girder : release strength");
   (*pTable)(1,1) << stress.SetValue( pMat->GetFciGdr(span,girder) );

   (*pTable)(2,0) << _T("Slab");
   (*pTable)(2,1) << stress.SetValue( pMat->GetFcSlab() );
}

void prestressing(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Prestressing"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLength2UnitValue, area, pDisplayUnits->GetAreaUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true );
   
   // Get the interfaces
   GET_IFACE2( pBroker, IBridgeMaterial, pMat );
   const matPsStrand* pStrand = pMat->GetStrand(span,girder,pgsTypes::Permanent);
   const matPsStrand* pTempStrand = pMat->GetStrand(span,girder,pgsTypes::Temporary);

   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeom );
   GET_IFACE2( pBroker, ISectProp2, pSectProp2 );

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(girder);

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,girder);

   GET_IFACE2(pBroker,IPointOfInterest, pIPOI);
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_MIDSPAN);
   pgsPointOfInterest poi = *vPoi.begin();

   StrandIndexType Nh = pStrandGeom->GetNumStrands( span, girder, pgsTypes::Harped);

   Float64 es; // eccentricity of straight strands
   Float64 eh; // eccentricity of harped strands at mid-span
   Float64 eh2; // eccentricity of 1 harped strand at mid-span
   Float64 ehe; // eccentricty of harped strands at end of girder

   Float64 EndOffset, HpOffset;
   pStrandGeom->GetHarpStrandOffsets(span,girder,&EndOffset,&HpOffset);

   Float64 nEff;
   es = pStrandGeom->GetSsEccentricity( poi, &nEff );
   eh = pStrandGeom->GetHsEccentricity( poi, &nEff );

   if ( 0 < Nh  )
   {
      GET_IFACE2(pBroker,IBridge, pBridge);
      GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

      StrandIndexType ns1 = pStrandGeometry->GetNextNumStrands(span, girder, pgsTypes::Harped, 0);
      ConfigStrandFillVector hfill = pStrandGeom->ComputeStrandFill(span, girder, pgsTypes::Harped, ns1);

      GDRCONFIG config = pBridge->GetGirderConfiguration(span,girder);
      config.PrestressConfig.SetStrandFill(pgsTypes::Harped, hfill);

      eh2 = pStrandGeom->GetHsEccentricity( poi, config.PrestressConfig, &nEff ); //** See Note Below
   }
   else
   {
      eh2 = eh;
   }

   ehe = pStrandGeom->GetHsEccentricity( pgsPointOfInterest(span,girder,0.00), &nEff );

   // ** eh2 is the eccentricity of a harped strand, at the midspan. We use this value to compute Fb.
   //    Fb is the distance from the bottom of the girder to the "lower bundle". This is a WSDOT
   //    specific calculation. There is an assumption here that the first strand is in the "lower
   //    bundle" and the actual definition of the harped strands at the harping point represents
   //    a bundle. In general, this might not be true.

   Float64 Fo  = pSectProp2->GetYtGirder(pgsTypes::CastingYard,poi) + ehe;
   Float64 Fcl = pSectProp2->GetYb(pgsTypes::CastingYard,poi) - eh;
   Float64 Fb  = pSectProp2->GetYb(pgsTypes::CastingYard,poi) - eh2;
   Float64 E   = pSectProp2->GetYb(pgsTypes::CastingYard,poi) - es;

   // Populate the table
   Int16 row = 0;
   (*pTable)(row,0) << _T("Permanent Strands");
   (*pTable)(row,1) << _T("");
   row++;

   (*pTable)(row,0) << _T("Nominal Strand Diameter");
   (*pTable)(row,1) << component.SetValue( pStrand->GetNominalDiameter() );
   row++;

   (*pTable)(row,0) << _T("Nominal Strand Area");
   (*pTable)(row,1) << area.SetValue( pStrand->GetNominalArea() );
   row++;

   GirderLibraryEntry::MeasurementLocation hpRef = pGdrEntry->GetHarpingPointReference();
   (*pTable)(row,0) << _T("Harping Point");
   if ( hpRef == GirderLibraryEntry::mlEndOfGirder )
      (*pTable)(row,0) << _T(" (measured from end of girder)");
   else
      (*pTable)(row,0) << _T(" (measured from bearing point)");

   Float64 hpLoc = pGdrEntry->GetHarpingPointLocation();
   GirderLibraryEntry::MeasurementType hpMeasure = pGdrEntry->GetHarpingPointMeasure();

   switch ( hpMeasure )
   {
   case GirderLibraryEntry::mtFractionOfSpanLength:
      (*pTable)(row,1) << hpLoc << Sub2(_T("L"),_T("span"));
      break;

   case GirderLibraryEntry::mtFractionOfGirderLength:
      (*pTable)(row,1) << hpLoc << Sub2(_T("L"),_T("girder"));
      break;

   case GirderLibraryEntry::mtAbsoluteDistance:
      (*pTable)(row,1) << length.SetValue(hpLoc);
      break;
   }
   row++;

   bool are_harped_straight = pStrandGeom->GetAreHarpedStrandsForcedStraight(span,girder);

   (*pTable)(row,0) << _T("Number of ") << LABEL_HARP_TYPE(are_harped_straight)<< _T(" Strands");
   (*pTable)(row,1) << pStrandGeom->GetNumStrands( span, girder, pgsTypes::Harped );
   row++;

   (*pTable)(row,0) << LABEL_HARP_TYPE(are_harped_straight)<<_T(" Strand ") << Sub2(_T("P"),_T("jack"));
   (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(span,girder,pgsTypes::Harped) );
   row++;

   (*pTable)(row,0) << _T("Number of Straight Strands");
   (*pTable)(row,1) << pStrandGeom->GetNumStrands( span, girder, pgsTypes::Straight );
   StrandIndexType nDebonded = pStrandGeom->GetNumDebondedStrands(span,girder,pgsTypes::Straight);
   if ( nDebonded != 0 )
      (*pTable)(row,1) << _T(" (") << nDebonded << _T(" debonded)");
   row++;

   (*pTable)(row,0) << _T("Straight Strand ") << Sub2(_T("P"),_T("jack"));
   (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(span,girder,pgsTypes::Straight) );
   row++;

   if (0 <  pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary) )
   {
      (*pTable)(row,0) << _T("Temporary Strands");
      (*pTable)(row,1) << _T("");
      row++;

      (*pTable)(row,0) << _T("Nominal Strand Diameter");
      (*pTable)(row,1) << component.SetValue( pTempStrand->GetNominalDiameter() );
      row++;

      (*pTable)(row,0) << _T("Nominal Strand Area");
      (*pTable)(row,1) << area.SetValue( pTempStrand->GetNominalArea() );
      row++;

      (*pTable)(row,0) << _T("Number of Temporary Strands");
      switch ( pgirderData->PrestressData.TempStrandUsage )
      {
      case pgsTypes::ttsPTAfterLifting:
         (*pTable)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately after lifting");
         break;

      case pgsTypes::ttsPTBeforeShipping:
         (*pTable)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately before shipping");
         break;
      }

      (*pTable)(row,1) << pStrandGeom->GetNumStrands( span, girder, pgsTypes::Temporary );
      row++;

      (*pTable)(row,0) << _T("Temporary Strand ") << Sub2(_T("P"),_T("jack"));
      (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(span,girder,pgsTypes::Temporary) );
      row++;
   }

   if (are_harped_straight)
   {
      (*pTable)(row,0) << _T("C.G. of ") << LABEL_HARP_TYPE(are_harped_straight)<< _T(" Strands from top");
      (*pTable)(row,1) << component.SetValue( Fo );
      row++;
   }
   else
   {
      (*pTable)(row,0) << _T("C.G. of Harped Strands at end, ") << Sub2(_T("F"),_T("o"));
      (*pTable)(row,1) << component.SetValue( Fo );
      row++;

      (*pTable)(row,0) << _T("C.G. of Harped Strands at centerline, ") << Sub2(_T("F"),_T("cl"));
      (*pTable)(row,1) << component.SetValue( Fcl );
      row++;

      (*pTable)(row,0) << _T("C.G. of Lower Harped Strand Bundle, ") << Sub2(_T("F"),_T("b"));
      (*pTable)(row,1) << component.SetValue( Fb );
      row++;
   }
   
   (*pTable)(row,0) << _T("C.G. of Straight Strands, E");
   (*pTable)(row,1) << component.SetValue( E );
   row++;
}
