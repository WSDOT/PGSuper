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
#include "InputSummaryChapter.h"
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PierData2.h>
#include <PsgLib\GirderLibraryEntry.h>

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


void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void concrete(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);
void prestressing(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits);

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

   CGirderReportSpecification* pSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey( pSpec->GetGirderKey() );

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << color(Red) << _T("NOTE: Several details have been omitted from this report") << color(Black) << rptNewLine;

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      if ( 1 < nSegments )
      {
         p = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << p;
         *p << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      p = new rptParagraph;
      *pChapter << p;

      GET_IFACE2( pBroker, IStrandGeometry, pStrandGeometry );
      if (pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey))
      {
         *p << color(Red) << Bold(_T("Warning: This is a non-standard girder because it utilizes straight web strands. WSDOT Standard Girders utilize harped strands.")) << color(Black) << rptNewLine;
      }

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

      if (pSegment->Strands.NumPermStrandsType == CStrandData::npsDirectSelection)
      {
         *p << color(Red) << Bold(_T("Warning: This is a non-standard girder because it utilizes Direct Strand Fill. WSDOT Standard Girders utilize sequentially filled strands.")) << color(Black) << rptNewLine;
      }

      girder_line_geometry( pChapter, pBroker, segmentKey, pDisplayUnits );
      concrete( pChapter, pBroker, segmentKey, pDisplayUnits );
      prestressing( pChapter, pBroker, segmentKey, pDisplayUnits );
   }

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
void girder_line_geometry(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
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
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CPierData2* pPrevPier = pGroup->GetPier(pgsTypes::metStart);
   const CPierData2* pNextPier = pGroup->GetPier(pgsTypes::metEnd);

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
         hash = HashGirderSpacing(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetMeasurementLocation(),pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetMeasurementType());
      }
      else
      {
         pStr = &strGirderSpacingMeasureAtEndOfSpan;
         hash = HashGirderSpacing(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetMeasurementLocation(),pNextPier->GetGirderSpacing(pgsTypes::Back)->GetMeasurementType());
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
   (*pTable)(row++,1) << pGroup->GetGirder(segmentKey.girderIndex)->GetGirderName();

   (*pTable)(row,0) << _T("Span Length, CL Bearing to CL Bearing") ;
   (*pTable)(row++,1) << length.SetValue(pBridge->GetSegmentSpanLength(segmentKey));

   (*pTable)(row,0) << _T("Girder Length") ;
   (*pTable)(row++,1) << glength.SetValue(pBridge->GetSegmentLength(segmentKey));

   (*pTable)(row,0) << _T("Number of Girders");
   (*pTable)(row++,1) << pBridge->GetGirderCount(segmentKey.groupIndex);


   if ( pBridge->IsInteriorGirder(segmentKey) )
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
            (*pTable)(row++,1) << pUnitValue->SetValue(pPrevPier->GetGirderSpacing(pgsTypes::Ahead)->GetGirderSpacing( SpacingIndexType(nGirders-2) ));
         else
            (*pTable)(row++,1) << _T(" - ");

         if ( IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
            (*pTable)(row,0) << _T("Left Girder Spacing\nEnd of Span");
         else
            (*pTable)(row,0) << _T("Left Joint Spacing\nEnd of Span");

         if ( 1 < pBridge->GetGirderCount(segmentKey.groupIndex) )
            (*pTable)(row++,1) << pUnitValue->SetValue(pNextPier->GetGirderSpacing(pgsTypes::Back)->GetGirderSpacing( SpacingIndexType(nGirders-2) ));
         else
            (*pTable)(row++,1) << _T(" - ");
      }
   }

#pragma Reminder("UPDATE: Assumes constant deck thickness")   
   pgsPointOfInterest poi(segmentKey,0.00);
   (*pTable)(row,0) << _T("Slab Thickness for Design");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( poi ));

   (*pTable)(row,0) << _T("Slab Thickness for Construction");
   (*pTable)(row++,1) << component.SetValue(pBridge->GetGrossSlabDepth( poi ));

   (*pTable)(row,0) << _T("\"A\" Dimension at Start");
   (*pTable)(row++,1) << component.SetValue(pGroup->GetSlabOffset(segmentKey.girderIndex,pgsTypes::metStart));

   (*pTable)(row,0) << _T("\"A\" Dimension at End");
   (*pTable)(row++,1) << component.SetValue(pGroup->GetSlabOffset(segmentKey.girderIndex,pgsTypes::metEnd));

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

   std::_tstring strPierLabel(pPrevPier->IsAbutment() ? _T("Abutment") : _T("Pier"));
   Float64 brgOffset, endDistance;
   ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
   ConnectionLibraryEntry::EndDistanceMeasurementType endDistanceMeasure;
   pPrevPier->GetBearingOffset(pgsTypes::Ahead,&brgOffset,&brgOffsetMeasure);
   pPrevPier->GetGirderEndDistance(pgsTypes::Ahead,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(pPrevPier->GetIndex());
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pPrevPier->IsAbutment()) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pPrevPier->IsAbutment());
   row++;

   (*pTable)(row,0) << _T("Connection Boundary Condition at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(pPrevPier->GetIndex());
   (*pTable)(row++,1) << CPierData2::AsString(pPrevPier->GetPierConnectionType());

   strPierLabel = (pNextPier->IsAbutment() ? _T("Abutment") : _T("Pier"));
   pNextPier->GetBearingOffset(pgsTypes::Back,&brgOffset,&brgOffsetMeasure);
   pNextPier->GetGirderEndDistance(pgsTypes::Back,&endDistance,&endDistanceMeasure);
   (*pTable)(row,0) << _T("Connection Geometry at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(pNextPier->GetIndex());
   (*pTable)(row,1) << _T("Bearing Offset: ") << length.SetValue(brgOffset) << _T(" ") << GetBearingOffsetMeasureString(brgOffsetMeasure,pNextPier->IsAbutment()) << rptNewLine;
   (*pTable)(row,1) << _T("End Distance: ") << length.SetValue(endDistance) << _T(" ") << GetEndDistanceMeasureString(endDistanceMeasure,pNextPier->IsAbutment());
   row++;

   (*pTable)(row,0) << _T("Connection Boundary Condition at ") << strPierLabel.c_str() << _T(" ") << LABEL_PIER(pNextPier->GetIndex());
   (*pTable)(row++,1) << CPierData2::AsString(pNextPier->GetPierConnectionType());

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (pGirderLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      (*pTable)(row,0) << _T("Left Lifting Point Location");
      (*pTable)(row++,1) << length.SetValue(pGirderLifting->GetLeftLiftingLoopLocation(segmentKey));

      (*pTable)(row,0) << _T("Right Lifting Point Location");
      (*pTable)(row++,1) << length.SetValue(pGirderLifting->GetRightLiftingLoopLocation(segmentKey));
   }

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      (*pTable)(row,0) << _T("Leading Truck Support Location");
      (*pTable)(row++,1) << length.SetValue(pGirderHauling->GetLeadingOverhang(segmentKey));

      (*pTable)(row,0) << _T("Trailing Truck Support Location");
      (*pTable)(row++,1) << length.SetValue(pGirderHauling->GetTrailingOverhang(segmentKey));
   }
}

void concrete(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
{
   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Concrete"));
   *p << pTable << rptNewLine;

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true );

   GET_IFACE2( pBroker, IIntervals, pIntervals );
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   
   // Get the interfaces
   GET_IFACE2( pBroker, IMaterials, pMaterial );

   // Populate the table
   (*pTable)(0,0) << _T("Girder : 28 day strength");
   (*pTable)(0,1) << stress.SetValue( pMaterial->GetSegmentFc(segmentKey,liveLoadIntervalIdx) );

   (*pTable)(1,0) << _T("Girder : release strength");
   (*pTable)(1,1) << stress.SetValue( pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx) );

   (*pTable)(2,0) << _T("Slab");
   (*pTable)(2,1) << stress.SetValue( pMaterial->GetDeckFc(liveLoadIntervalIdx) );
}

void prestressing(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits)
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
   GET_IFACE2( pBroker, IMaterials, pMaterial );
   const matPsStrand* pStrand     = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   const matPsStrand* pTempStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary);

   GET_IFACE2( pBroker, IStrandGeometry, pStrandGeom );
   GET_IFACE2( pBroker, ISectionProperties, pSectProp );

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   GET_IFACE2(pBroker,IPointOfInterest, pIPOI);
   std::vector<pgsPointOfInterest> vPoi( pIPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN) );
   pgsPointOfInterest poi(*vPoi.begin());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);


   StrandIndexType Nh = pStrandGeom->GetNumStrands(segmentKey,pgsTypes::Harped);

   Float64 es; // eccentricity of straight strands
   Float64 eh; // eccentricity of harped strands at mid-span
   Float64 eh2; // eccentricity of 1 harped strand at mid-span
   Float64 ehe; // eccentricty of harped strands at end of girder

   Float64 EndOffset, HpOffset;
   pStrandGeom->GetHarpStrandOffsets(segmentKey,&EndOffset,&HpOffset);

   Float64 nEff;
   es = pStrandGeom->GetSsEccentricity( releaseIntervalIdx, poi, &nEff );
   eh = pStrandGeom->GetHsEccentricity( releaseIntervalIdx, poi, &nEff );

   if ( 0 < Nh  )
   {
      GET_IFACE2(pBroker,IBridge, pBridge);
      GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

      GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
      StrandIndexType ns1 = pStrandGeometry->GetNextNumStrands(segmentKey, pgsTypes::Harped, 0);
      ConfigStrandFillVector hfill = pStrandGeom->ComputeStrandFill(segmentKey, pgsTypes::Harped, ns1);
      config.PrestressConfig.SetStrandFill(pgsTypes::Harped, hfill);

      eh2 = pStrandGeom->GetHsEccentricity( releaseIntervalIdx, poi, config, &nEff ); //** See Note Below
   }
   else
   {
      eh2 = eh;
   }

   ehe = pStrandGeom->GetHsEccentricity( releaseIntervalIdx, pgsPointOfInterest(segmentKey,0.00), &nEff );

   // ** eh2 is the eccentricity of a harped strand, at the midspan. We use this value to compute Fb.
   //    Fb is the distance from the bottom of the girder to the "lower bundle". This is a WSDOT
   //    specific calculation. There is an assumption here that the first strand is in the "lower
   //    bundle" and the actual definition of the harped strands at the harping point represents
   //    a bundle. In general, this might not be true.

   Float64 Fo  = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::TopGirder) + ehe;
   Float64 Yb  = pSectProp->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder);
   Float64 Fcl = Yb - eh;
   Float64 Fb  = Yb - eh2;
   Float64 E   = Yb - es;

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

   bool are_harped_straight = pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey);

   (*pTable)(row,0) << _T("Number of ") << LABEL_HARP_TYPE(are_harped_straight)<< _T(" Strands");
   (*pTable)(row,1) << pStrandGeom->GetNumStrands( segmentKey, pgsTypes::Harped );
   row++;

   (*pTable)(row,0) << LABEL_HARP_TYPE(are_harped_straight)<<_T(" Strand ") << Sub2(_T("P"),_T("jack"));
   (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(segmentKey,pgsTypes::Harped) );
   row++;

   (*pTable)(row,0) << _T("Number of Straight Strands");
   (*pTable)(row,1) << pStrandGeom->GetNumStrands( segmentKey, pgsTypes::Straight );
   StrandIndexType nDebonded = pStrandGeom->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
   if ( nDebonded != 0 )
      (*pTable)(row,1) << _T(" (") << nDebonded << _T(" debonded)");
   row++;

   (*pTable)(row,0) << _T("Straight Strand ") << Sub2(_T("P"),_T("jack"));
   (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(segmentKey,pgsTypes::Straight) );
   row++;

   if (0 <  pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
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
      switch ( pStrands->TempStrandUsage )
      {
      case pgsTypes::ttsPTAfterLifting:
         (*pTable)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately after lifting");
         break;

      case pgsTypes::ttsPTBeforeShipping:
         (*pTable)(row,0) << rptNewLine << _T("Temporary strands post-tensioned immediately before shipping");
         break;
      }

      (*pTable)(row,1) << pStrandGeom->GetNumStrands( segmentKey, pgsTypes::Temporary );
      row++;

      (*pTable)(row,0) << _T("Temporary Strand ") << Sub2(_T("P"),_T("jack"));
      (*pTable)(row,1) << force.SetValue( pStrandGeom->GetPjack(segmentKey,pgsTypes::Temporary) );
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
