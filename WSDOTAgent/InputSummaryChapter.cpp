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
CInputSummaryChapter::CInputSummaryChapter()
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
   *p << color(Red) << "NOTE: Several details have been omitted from this report" << color(Black) << rptNewLine;

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

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,"Girder Line Geometry");
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

   // Populate the table
   RowIndexType row = 0;

   (*pTable)(row,0) << "Girder Type";
   (*pTable)(row++,1) << pSpan->GetGirderTypes()->GetGirderName(girder);

   (*pTable)(row,0) << "Span Length, CL Bearing to CL Bearing" ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSpanLength(span,girder));

   (*pTable)(row,0) << "Girder Length" ;
   (*pTable)(row++,1) << glength.SetValue(pBridge->GetGirderLength(span,girder));

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
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing( SpacingIndexType(nGirders-2) ));
         else
            (*pTable)(row++,1) << " - ";

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << "Left Girder Spacing\nEnd of Span";
         else
            (*pTable)(row,0) << "Left Joint Spacing\nEnd of Span";

         if ( 1 < pBridge->GetGirderCount(span) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pSpan->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing( SpacingIndexType(nGirders-2) ));
         else
            (*pTable)(row++,1) << " - ";
      }
   }

#pragma Reminder("UPDATE: Assumes constant deck thickness")   
   pgsPointOfInterest poi(span,girder,0.00);
   (*pTable)(row,0) << "Slab Thickness for Design";
   (*pTable)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( poi ));

   (*pTable)(row,0) << "Slab Thickness for Construction";
   (*pTable)(row++,1) << component.SetValue(pBridge->GetGrossSlabDepth( poi ));

   (*pTable)(row,0) << "\"A\" Dimension at Start";
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metStart));

   (*pTable)(row,0) << "\"A\" Dimension at End";
   (*pTable)(row++,1) << component.SetValue(pGirderTypes->GetSlabOffset(girder,pgsTypes::metEnd));

   (*pTable)(row,0) << "Overlay";
   (*pTable)(row++,1) << olay.SetValue(pDeck->OverlayWeight);

#pragma Reminder("#*#*#*#*#*# WSDOT girder summary - Diaphragms #*#*#*#*#*#")
//   (*pTable)(row,0) << "Intermediate Diaphragm (H x W)";
//   (*pTable)(row,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmHeight() );
//   (*pTable)(row,1) << " x ";
//   (*pTable)(row++,1) << component.SetValue( pXSectData->pGirderEntry->GetDiaphragmWidth() );

#pragma Reminder("UPDATE - update for new railing/sidewalk model")
   (*pTable)(row,0) << "Left Traffic Barrier";
   (*pTable)(row++,1) << pBridgeDesc->GetLeftRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << "Right Traffic Barrier";
   (*pTable)(row++,1) << pBridgeDesc->GetRightRailingSystem()->strExteriorRailing;

   (*pTable)(row,0) << "Connection Geometry at Pier " << LABEL_PIER(span);
   (*pTable)(row++,1) << pPrevPier->GetConnection(pgsTypes::Ahead);
   (*pTable)(row,0) << "Connection Boundary Condition at Pier " << LABEL_PIER(span);
   (*pTable)(row++,1) << CPierData::AsString(pPrevPier->GetConnectionType());

   (*pTable)(row,0) << "Connection Geometry at Pier " << LABEL_PIER(span+1);
   (*pTable)(row++,1) << pNextPier->GetConnection(pgsTypes::Back);

   (*pTable)(row,0) << "Connection Boundary Condition at Pier " << LABEL_PIER(span+1);
   (*pTable)(row++,1) << CPierData::AsString(pNextPier->GetConnectionType());

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      (*pTable)(row,0) << "Left Lifting Point Location";
      (*pTable)(row++,1) << length.SetValue(pGirderLifting->GetLeftLiftingLoopLocation(span,girder));

      (*pTable)(row,0) << "Right Lifting Point Location";
      (*pTable)(row++,1) << length.SetValue(pGirderLifting->GetRightLiftingLoopLocation(span,girder));
   }

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      (*pTable)(row,0) << "Leading Truck Support Location";
      (*pTable)(row++,1) << length.SetValue(pGirderHauling->GetLeadingOverhang(span,girder));

      (*pTable)(row,0) << "Trailing Truck Support Location";
      (*pTable)(row++,1) << length.SetValue(pGirderHauling->GetTrailingOverhang(span,girder));
   }
}

void concrete(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,"Concrete");
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );
   
   // Get the interfaces
   GET_IFACE2( pBroker, IBridgeMaterial, pMat );

   // Populate the table
   (*pTable)(0,0) << "Girder : 28 day strength";
   (*pTable)(0,1) << stress.SetValue( pMat->GetFcGdr(span,girder) );

   (*pTable)(1,0) << "Girder : release strength";
   (*pTable)(1,1) << stress.SetValue( pMat->GetFciGdr(span,girder) );

   (*pTable)(2,0) << "Slab";
   (*pTable)(2,1) << stress.SetValue( pMat->GetFcSlab() );
}

void prestressing(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,"Prestressing");
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLength2UnitValue, area, pDisplayUnits->GetAreaUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true );
   
   // Get the interfaces
   GET_IFACE2( pBroker, IBridgeMaterial, pMat );
   const matPsStrand* pStrand = pMat->GetStrand(span,girder);

   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeom );
   GET_IFACE2( pBroker, ISectProp2, pSectProp2 );

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(girder);

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,girder);

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

      GDRCONFIG config = pBridge->GetGirderConfiguration(span,girder);
      config.Nstrands[pgsTypes::Harped] = pStrandGeometry->GetNextNumStrands(span, girder, pgsTypes::Harped, 0);

      eh2 = pStrandGeom->GetHsEccentricity( poi, config, &nEff ); //** See Note Below
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
   (*pTable)(row,0) << "Nominal Strand Diameter";
   (*pTable)(row,1) << component.SetValue( pStrand->GetNominalDiameter() );
   row++;

   (*pTable)(row,0) << "Nominal Strand Area";
   (*pTable)(row,1) << area.SetValue( pStrand->GetNominalArea() );
   row++;

   GirderLibraryEntry::MeasurementLocation hpRef = pGdrEntry->GetHarpingPointReference();
   (*pTable)(row,0) << "Harping Point";
   if ( hpRef == GirderLibraryEntry::mlEndOfGirder )
      (*pTable)(row,0) << " (measured from end of girder)";
   else
      (*pTable)(row,0) << " (measured from bearing point)";

   Float64 hpLoc = pGdrEntry->GetHarpingPointLocation();
   GirderLibraryEntry::MeasurementType hpMeasure = pGdrEntry->GetHarpingPointMeasure();

   switch ( hpMeasure )
   {
   case GirderLibraryEntry::mtFractionOfSpanLength:
      (*pTable)(row,1) << hpLoc << Sub2("L","span");
      break;

   case GirderLibraryEntry::mtFractionOfGirderLength:
      (*pTable)(row,1) << hpLoc << Sub2("L","girder");
      break;

   case GirderLibraryEntry::mtAbsoluteDistance:
      (*pTable)(row,1) << length.SetValue(hpLoc);
      break;
   }
   row++;

   (*pTable)(row,0) << "Number of Harped Strands";
   (*pTable)(row,1) << pStrandGeom->GetNumStrands( span, girder, pgsTypes::Harped );
   row++;

   (*pTable)(row,0) << "Harped Strand " << Sub2("P","jack");
   (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(span,girder,pgsTypes::Harped) );
   row++;

   (*pTable)(row,0) << "Number of Straight Strands";
   (*pTable)(row,1) << pStrandGeom->GetNumStrands( span, girder, pgsTypes::Straight );
   StrandIndexType nDebonded = pStrandGeom->GetNumDebondedStrands(span,girder,pgsTypes::Straight);
   if ( nDebonded != 0 )
      (*pTable)(row,1) << " (" << nDebonded << " debonded)";
   row++;

   (*pTable)(row,0) << "Straight Strand " << Sub2("P","jack");
   (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(span,girder,pgsTypes::Straight) );
   row++;

   if (0 <  pStrandGeom->GetMaxStrands(span,girder,pgsTypes::Temporary) )
   {
      (*pTable)(row,0) << "Number of Temporary Strands";
      switch ( girderData.TempStrandUsage )
      {
      case pgsTypes::ttsPTAfterLifting:
         (*pTable)(row,0) << rptNewLine << "Temporary strands post-tensioned immediately after lifting";
         break;

      case pgsTypes::ttsPTBeforeShipping:
         (*pTable)(row,0) << rptNewLine << "Temporary strands post-tensioned immediately before shipping";
         break;
      }

      (*pTable)(row,1) << pStrandGeom->GetNumStrands( span, girder, pgsTypes::Temporary );
      row++;

      (*pTable)(row,0) << "Temporary Strand " << Sub2("P","jack");
      (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(span,girder,pgsTypes::Temporary) );
      row++;
   }

   (*pTable)(row,0) << "C.G. of Harped Strands at end, " << Sub2("F","o");
   (*pTable)(row,1) << component.SetValue( Fo );
   row++;

   (*pTable)(row,0) << "C.G. of Harped Strands at centerline, " << Sub2("F","cl");
   (*pTable)(row,1) << component.SetValue( Fcl );
   row++;

   (*pTable)(row,0) << "C.G. of Lower Harped Strand Bundle, " << Sub2("F","b");
   (*pTable)(row,1) << component.SetValue( Fb );
   row++;
   
   (*pTable)(row,0) << "C.G. of Straight Strands, E";
   (*pTable)(row,1) << component.SetValue( E );
   row++;
}
