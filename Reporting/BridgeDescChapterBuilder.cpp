///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Reporting\BridgeDescChapterBuilder.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Alignment.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\BeamFactory.h>

#include <PsgLib\ConnectionLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\TrafficBarrierEntry.h>


#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\GirderLabel.h>

#include <Material\Material.h>
#include <LRFD\LRFD.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void write_alignment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_profile_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_crown_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_bridge_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_pier_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_ts_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_framing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_span_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_girder_spacing(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,const CGirderSpacing2* pGirderSpacing,RowIndexType row,ColumnIndexType col);
static void write_bearing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const CGirderKey& girderKey);
static void write_ps_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const CGirderKey& girderKey);
static void write_segment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,GroupIndexType grpIdx,GirderIndexType gdrIdx,Uint16 level);
static void write_slab_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level);
static void write_lrfd_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level);
static void write_lrfd_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fci,Float64 fc,Float64 Eci,Float64 Ec,Float64 lambda,const CConcreteMaterial& concrete,RowIndexType row);
static void write_aci209_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level,bool bAASHTOParameters);
static void write_aci209_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row,bool bAASHTOParameters);
static void write_cebfip_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level);
static void write_cebfip_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row);
static void write_deck_reinforcing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);

/****************************************************************************
CLASS
   CBridgeDescChapterBuilder
****************************************************************************/

CBridgeDescChapterBuilder::CBridgeDescChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CBridgeDescChapterBuilder::GetName() const
{
   return TEXT("Bridge Description");
}

rptChapter* CBridgeDescChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else if ( pGdrLineRptSpec)
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
      girderKey.groupIndex = 0;
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

#pragma Reminder("UPDATE: Bridge Description Chapter - need to include temporary supports, erection sequence, post-tensioning, etc")

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   //write_alignment_data( pBroker, pDisplayUnits, pChapter, level);
   //write_profile_data( pBroker, pDisplayUnits, pChapter, level);
   //write_crown_data( pBroker, pDisplayUnits, pChapter, level);
   write_bridge_data( pBroker, pDisplayUnits, pChapter, level);
   write_concrete_details(pBroker,pDisplayUnits,pChapter,girderKey,level);
   write_pier_data( pBroker, pDisplayUnits, pChapter, level);
   write_bearing_data( pBroker, pDisplayUnits, pChapter, level,girderKey );
   write_span_data( pBroker, pDisplayUnits, pChapter, level);
   write_ps_data( pBroker, pDisplayUnits, pChapter, level,girderKey );
   write_slab_data( pBroker, pDisplayUnits, pChapter, level );
   write_deck_reinforcing_data( pBroker, pDisplayUnits, pChapter, level );

   return pChapter;
}

CChapterBuilder* CBridgeDescChapterBuilder::Clone() const
{
   return new CBridgeDescChapterBuilder(m_bSelect);
}

void CBridgeDescChapterBuilder::WriteAlignmentData(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter,Uint16 level)
{
   write_alignment_data( pBroker, pDisplayUnits, pChapter, level);
}

void CBridgeDescChapterBuilder::WriteProfileData(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter,Uint16 level)
{
   write_profile_data( pBroker, pDisplayUnits, pChapter, level);
}

void CBridgeDescChapterBuilder::WriteCrownData(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter,Uint16 level)
{
   write_crown_data( pBroker, pDisplayUnits, pChapter, level);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void write_alignment_data(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter, Uint16 level)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, IRoadwayData, pAlignment);
   rptParagraph* pPara;

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), false);

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);

   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Alignment Details") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   AlignmentData2 alignment = pAlignment->GetAlignmentData2();

   CComBSTR bstrBearing;
   direction_formatter->Format(alignment.Direction, CComBSTR("°,\',\""), &bstrBearing);
   *pPara << _T("Direction: ") << RPT_BEARING(OLE2T(bstrBearing)) << rptNewLine;

   *pPara << _T("Ref. Point: ") << rptRcStation(alignment.RefStation, &pDisplayUnits->GetStationFormat())
      << _T(" ")
      << _T("(E (X) ") << length.SetValue(alignment.xRefPoint);
   *pPara << _T(", ")
      << _T("N (Y) ") << length.SetValue(alignment.yRefPoint) << _T(")") << rptNewLine;

   if (alignment.HorzCurves.size() == 0)
   {
      return;
   }

   bool bHasEntrySpirals = false;
   bool bHasExitSpirals = false;
   bool bHasCircularCurves = false;
   for (const auto& hc : alignment.HorzCurves)
   {
      if (!IsZero(hc.EntrySpiral))
      {
         bHasEntrySpirals = true;
      }

      if (!IsZero(hc.ExitSpiral))
      {
         bHasExitSpirals = true;
      }

      if (!IsZero(hc.Radius))
      {
         bHasCircularCurves = true;
      }
   }

   GET_IFACE2(pBroker, IRoadway, pRoadway);

   pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns = alignment.HorzCurves.size() + 1;
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns, _T("Horizontal Curve Data"));
   *pPara << pTable << rptNewLine;

   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   RowIndexType row = 0;

   (*pTable)(row++, 0) << _T("Curve Parameters");
   (*pTable)(row++, 0) << _T("Back Tangent Bearing");
   if (bHasEntrySpirals || bHasExitSpirals)
   {
      (*pTable)(row++, 0) << _T("Back Tangent Length");
   }
   (*pTable)(row++, 0) << _T("Forward Tangent Bearing");
   if (bHasEntrySpirals || bHasExitSpirals)
   {
      (*pTable)(row++, 0) << _T("Forward Tangent Length");
   }

   if (bHasEntrySpirals)
   {
      (*pTable)(row++, 0) << _T("TS Station");
      (*pTable)(row++, 0) << _T("SPI Station");
      (*pTable)(row++, 0) << _T("SC Station");
   }

   if (bHasCircularCurves)
   {
      (*pTable)(row++, 0) << _T("PC Station");
   }
   
   (*pTable)(row++, 0) << _T("PI Station");

   if (bHasCircularCurves)
   {
      (*pTable)(row++, 0) << _T("PT Station");
   }

   if (bHasExitSpirals)
   {
      (*pTable)(row++, 0) << _T("CS Station");
      (*pTable)(row++, 0) << _T("SPI Station");
      (*pTable)(row++, 0) << _T("ST Station");
   }

   if (bHasEntrySpirals || bHasExitSpirals)
   {
      (*pTable)(row++, 0) << _T("Total Delta (") << Sub2(symbol(DELTA), _T("c")) << _T(")");
   }

   if (bHasEntrySpirals)
   {
      pTable->SetColumnSpan(row, 0, nColumns);
      for (ColumnIndexType colIdx = 1; colIdx < nColumns; colIdx++)
      {
         pTable->SetColumnSpan(row, colIdx, SKIP_CELL);
      }
      (*pTable)(row++, 0) << Bold(_T("Entry Spiral"));

      (*pTable)(row++, 0) << _T("Length");
      (*pTable)(row++, 0) << _T("Delta (") << Sub2(symbol(DELTA), _T("s")) << _T(")");
      (*pTable)(row++, 0) << _T("Deviation Angle (DE)");
      (*pTable)(row++, 0) << _T("Deflection Angle (DF)");
      (*pTable)(row++, 0) << _T("Deflection Angle (DH = DE - DF)");
      (*pTable)(row++, 0) << _T("Long Tangent (u)");
      (*pTable)(row++, 0) << _T("Short Tangent (v)");
      (*pTable)(row++, 0) << _T("Long Chord (") << Sub2(_T("C"), _T("s")) << _T(")");
      (*pTable)(row++, 0) << Sub2(_T("X"), _T("s"));
      (*pTable)(row++, 0) << Sub2(_T("Y"), _T("s"));
      (*pTable)(row++, 0) << _T("Throw (Q)");
      (*pTable)(row++, 0) << _T("t");
   }

   if (bHasEntrySpirals || bHasExitSpirals)
   {
      pTable->SetColumnSpan(row, 0, nColumns);
      for (ColumnIndexType colIdx = 1; colIdx < nColumns; colIdx++)
      {
         pTable->SetColumnSpan(row, colIdx, SKIP_CELL);
      }
      (*pTable)(row++, 0) << Bold(_T("Circular Curve"));
   }

   if (bHasCircularCurves)
   {
      (*pTable)(row++, 0) << _T("Delta (") << Sub2(symbol(DELTA), _T("cc")) << _T(")");
      (*pTable)(row++, 0) << _T("Degree Curvature (DC)");
      (*pTable)(row++, 0) << _T("Radius (R)");
      (*pTable)(row++, 0) << _T("Tangent (T)");
      (*pTable)(row++, 0) << _T("Length (L)");
      (*pTable)(row++, 0) << _T("Chord (C)");
      (*pTable)(row++, 0) << _T("External (E)");
      (*pTable)(row++, 0) << _T("Mid Ordinate (MO)");
   }

   if (bHasExitSpirals)
   {
      pTable->SetColumnSpan(row, 0, nColumns);
      for (ColumnIndexType colIdx = 1; colIdx < nColumns; colIdx++)
      {
         pTable->SetColumnSpan(row, colIdx, SKIP_CELL);
      }
      (*pTable)(row++, 0) << Bold(_T("Exit Spiral"));

      (*pTable)(row++, 0) << _T("Length");
      (*pTable)(row++, 0) << _T("Delta (") << Sub2(symbol(DELTA), _T("s")) << _T(")");
      (*pTable)(row++, 0) << _T("Deviation Angle (DE)");
      (*pTable)(row++, 0) << _T("Deflection Angle (DF)");
      (*pTable)(row++, 0) << _T("Deflection Angle (DH = DE - DF)");
      (*pTable)(row++, 0) << _T("Long Tangent (u)");
      (*pTable)(row++, 0) << _T("Short Tangent (v)");
      (*pTable)(row++, 0) << _T("Long Chord (") << Sub2(_T("C"), _T("s")) << _T(")");
      (*pTable)(row++, 0) << Sub2(_T("X"), _T("s"));
      (*pTable)(row++, 0) << Sub2(_T("Y"), _T("s"));
      (*pTable)(row++, 0) << _T("Throw (Q)");
      (*pTable)(row++, 0) << _T("t");
   }

   pTable->SetColumnSpan(row, 0, nColumns);
   for (ColumnIndexType colIdx = 1; colIdx < nColumns; colIdx++)
   {
      pTable->SetColumnSpan(row, colIdx, SKIP_CELL);
   }
   (*pTable)(row++, 0) << Bold(_T("Curve Points (x,y)"));

   if (bHasEntrySpirals)
   {
      (*pTable)(row++, 0) << _T("TS");
      (*pTable)(row++, 0) << _T("SPI");
      (*pTable)(row++, 0) << _T("SC");
   }

   if (bHasCircularCurves)
   {
      (*pTable)(row++, 0) << _T("PC");
   }

   (*pTable)(row++, 0) << _T("PI");

   if (bHasCircularCurves)
   {
      (*pTable)(row++, 0) << _T("PT");
   }

   if (bHasExitSpirals)
   {
      (*pTable)(row++, 0) << _T("CS");
      (*pTable)(row++, 0) << _T("SPI");
      (*pTable)(row++, 0) << _T("ST");
   }

   if (bHasCircularCurves)
   {
      (*pTable)(row++, 0) << _T("Center of Circular Curve (CCC)");
   }

   if (bHasEntrySpirals || bHasExitSpirals)
   {
      (*pTable)(row++, 0) << _T("Center of Curve (CC)");
   }

   length.ShowUnitTag(true);

   ColumnIndexType col = 1;
   IndexType hcIdx = 0; // keeps tracks of the actual curves in the model (curves with zero radius input are not curves in the alignment model)
   std::vector<HorzCurveData>::iterator iter;
   for (iter = alignment.HorzCurves.begin(); iter != alignment.HorzCurves.end(); iter++, col++)
   {
      HorzCurveData& hc_data = *iter;
      row = 0;

      (*pTable)(row++, col) << _T("Curve ") << col;

      CComPtr<IHorzCurve> hc;

      CComPtr<IDirection> bkTangent;
      CComPtr<IDirection> fwdTangent;
      if (IsZero(hc_data.Radius))
      {
         pRoadway->GetBearing(hc_data.PIStation - ::ConvertToSysUnits(1.0, unitMeasure::Feet), &bkTangent);
         pRoadway->GetBearing(hc_data.PIStation + ::ConvertToSysUnits(1.0, unitMeasure::Feet), &fwdTangent);
      }
      else
      {
         pRoadway->GetCurve(hcIdx, &hc);
         hc->get_BkTangentBrg(&bkTangent);
         hc->get_FwdTangentBrg(&fwdTangent);
      }

      Float64 bk_tangent_value;
      bkTangent->get_Value(&bk_tangent_value);

      CComBSTR bstrBkTangent;
      direction_formatter->Format(bk_tangent_value, CComBSTR("°,\',\""), &bstrBkTangent);
      (*pTable)(row++, col) << RPT_BEARING(OLE2T(bstrBkTangent));

      if (bHasEntrySpirals || bHasExitSpirals)
      {
         Float64 back_tangent_length;
         hc->get_BkTangentLength(&back_tangent_length);
         (*pTable)(row++, col) << length.SetValue(back_tangent_length);
      }

      Float64 fwd_tangent_value;
      fwdTangent->get_Value(&fwd_tangent_value);

      CComBSTR bstrFwdTangent;
      direction_formatter->Format(fwd_tangent_value, CComBSTR("°,\',\""), &bstrFwdTangent);
      (*pTable)(row++, col) << RPT_BEARING(OLE2T(bstrFwdTangent));

      if (bHasEntrySpirals || bHasExitSpirals)
      {
         Float64 fwd_tangent_length;
         hc->get_FwdTangentLength(&fwd_tangent_length);
         (*pTable)(row++, col) << length.SetValue(fwd_tangent_length);
      }

      // Curve stations
      HCURVESTATIONS stations;

      if (!IsZero(hc_data.Radius))
      {
         stations = pRoadway->GetCurveStations(hcIdx);
      }

      if (IsZero(hc_data.Radius))
      {
         if (bHasEntrySpirals)
         {
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
         }

         if (bHasCircularCurves)
         {
            (*pTable)(row++, col) << _T("-");
         }
      }
      else
      {
         if (IsEqual(stations.TSStation, stations.SCStation))
         {
            if (bHasEntrySpirals)
            {
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
            }

            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << rptRcStation(stations.TSStation, &pDisplayUnits->GetStationFormat());
            }
         }
         else
         {
            if (bHasEntrySpirals)
            {
               (*pTable)(row++, col) << rptRcStation(stations.TSStation, &pDisplayUnits->GetStationFormat());
               (*pTable)(row++, col) << rptRcStation(stations.SPI1Station, &pDisplayUnits->GetStationFormat());
               (*pTable)(row++, col) << rptRcStation(stations.SCStation, &pDisplayUnits->GetStationFormat());
            }

            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << _T("-");
            }
         }
      }

      (*pTable)(row++, col) << rptRcStation(hc_data.PIStation, &pDisplayUnits->GetStationFormat());

      if (IsZero(hc_data.Radius))
      {
         if (bHasCircularCurves)
         {
            (*pTable)(row++, col) << _T("-");
         }

         if (bHasExitSpirals)
         {
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
         }
      }
      else
      {
         if (IsEqual(stations.CSStation, stations.STStation))
         {
            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << rptRcStation(stations.CSStation, &pDisplayUnits->GetStationFormat());
            }

            if (bHasExitSpirals)
            {
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
            }
         }
         else
         {
            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << _T("-");
            }

            if (bHasExitSpirals)
            {
               (*pTable)(row++, col) << rptRcStation(stations.CSStation, &pDisplayUnits->GetStationFormat());
               (*pTable)(row++, col) << rptRcStation(stations.SPI2Station, &pDisplayUnits->GetStationFormat());
               (*pTable)(row++, col) << rptRcStation(stations.STStation, &pDisplayUnits->GetStationFormat());
            }
         }
      }

      CComPtr<IAngle> delta;
      Float64 delta_value;
      CComBSTR bstrDelta;


      if (bHasEntrySpirals || bHasExitSpirals)
      {
         delta.Release();
         hc->get_CurveAngle(&delta);
         delta->get_Value(&delta_value);

         CurveDirectionType direction;
         hc->get_Direction(&direction);
         delta_value *= (direction == cdRight ? -1 : 1);

         angle_formatter->put_Signed(VARIANT_FALSE);
         angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
         (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));
      }

      if (bHasEntrySpirals)
      {
         // Entry Spiral Data
         row++; // entry spiral heading row
         if (IsZero(hc_data.EntrySpiral))
         {
            (*pTable)(row++, col) << _T("-"); // Length
            (*pTable)(row++, col) << _T("-"); // Delta
            (*pTable)(row++, col) << _T("-"); // DE
            (*pTable)(row++, col) << _T("-"); // DF
            (*pTable)(row++, col) << _T("-"); // DH
            (*pTable)(row++, col) << _T("-"); // Long tangent
            (*pTable)(row++, col) << _T("-"); // Short tangent
            (*pTable)(row++, col) << _T("-"); // Long Chord
            (*pTable)(row++, col) << _T("-"); // Xs
            (*pTable)(row++, col) << _T("-"); // Ys
            (*pTable)(row++, col) << _T("-"); // o
            (*pTable)(row++, col) << _T("-"); // Xo
         }
         else
         {
            (*pTable)(row++, col) << length.SetValue(hc_data.EntrySpiral);
            delta.Release();
            SpiralType spiralType = spEntry;
            hc->get_SpiralAngle(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_FALSE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            delta.Release();
            hc->get_DE(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            delta.Release();
            hc->get_DF(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            delta.Release();
            hc->get_DH(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            Float64 longTangent, shortTangent;
            hc->get_LongTangent(spiralType, &longTangent);
            hc->get_ShortTangent(spiralType, &shortTangent);
            (*pTable)(row++, col) << length.SetValue(longTangent);
            (*pTable)(row++, col) << length.SetValue(shortTangent);
            Float64 long_chord;
            hc->get_SpiralChord(spiralType, &long_chord);
            (*pTable)(row++, col) << length.SetValue(long_chord);
            Float64 X, Y;
            hc->get_X(spiralType, &X);
            hc->get_Y(spiralType, &Y);
            (*pTable)(row++, col) << length.SetValue(X);
            (*pTable)(row++, col) << length.SetValue(Y);
            Float64 Q, T;
            hc->get_Q(spiralType, &Q);
            hc->get_T(spiralType, &T);
            (*pTable)(row++, col) << length.SetValue(Q);
            (*pTable)(row++, col) << length.SetValue(T);
         }
      }


      // Circular curve data
      if (bHasEntrySpirals || bHasExitSpirals)
      {
         row++; // circular curve heading row
      }

      if (bHasCircularCurves)
      {
         if (IsZero(hc_data.Radius))
         {
            (*pTable)(row++, col) << _T("-"); // Delta
            (*pTable)(row++, col) << _T("-"); // Degree curvature
            (*pTable)(row++, col) << _T("-"); // Radius
            (*pTable)(row++, col) << _T("-"); // Tangent
            (*pTable)(row++, col) << _T("-"); // Length
            (*pTable)(row++, col) << _T("-"); // Chord
            (*pTable)(row++, col) << _T("-"); // External
            (*pTable)(row++, col) << _T("-"); // Mid Ordinate
         }
         else
         {
            delta.Release();
            hc->get_CircularCurveAngle(&delta);
            delta->get_Value(&delta_value);

            CurveDirectionType direction;
            hc->get_Direction(&direction);

            delta_value *= (direction == cdRight ? -1 : 1);
            bstrDelta.Empty();
            angle_formatter->put_Signed(VARIANT_FALSE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);

            CComBSTR bstrDC;
            delta.Release();
            hc->get_DegreeCurvature(::ConvertToSysUnits(100.0, unitMeasure::Feet), dcHighway, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDC);

            Float64 tangent;
            hc->get_Tangent(&tangent);

            Float64 curve_length;
            hc->get_CurveLength(&curve_length);

            Float64 external;
            hc->get_External(&external);

            Float64 chord;
            hc->get_Chord(&chord);

            Float64 mid_ordinate;
            hc->get_MidOrdinate(&mid_ordinate);

            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDC));
            (*pTable)(row++, col) << length.SetValue(hc_data.Radius);
            (*pTable)(row++, col) << length.SetValue(tangent);
            (*pTable)(row++, col) << length.SetValue(curve_length);
            (*pTable)(row++, col) << length.SetValue(chord);
            (*pTable)(row++, col) << length.SetValue(external);
            (*pTable)(row++, col) << length.SetValue(mid_ordinate);
         }
      }


      if (bHasExitSpirals)
      {
         // Exit Spiral Data
         row++; // exit spiral heading row
         if (IsZero(hc_data.ExitSpiral))
         {
            (*pTable)(row++, col) << _T("-"); // Length
            (*pTable)(row++, col) << _T("-"); // Delta
            (*pTable)(row++, col) << _T("-"); // DE
            (*pTable)(row++, col) << _T("-"); // DF
            (*pTable)(row++, col) << _T("-"); // DH
            (*pTable)(row++, col) << _T("-"); // Long tangent
            (*pTable)(row++, col) << _T("-"); // Short tangent
            (*pTable)(row++, col) << _T("-"); // Long Chord
            (*pTable)(row++, col) << _T("-"); // Xs
            (*pTable)(row++, col) << _T("-"); // Ys
            (*pTable)(row++, col) << _T("-"); // o
            (*pTable)(row++, col) << _T("-"); // Xo
         }
         else
         {
            (*pTable)(row++, col) << length.SetValue(hc_data.ExitSpiral);
            delta.Release();
            SpiralType spiralType = spExit;
            hc->get_SpiralAngle(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_FALSE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            delta.Release();
            hc->get_DE(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            delta.Release();
            hc->get_DF(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            delta.Release();
            hc->get_DH(spiralType, &delta);
            delta->get_Value(&delta_value);
            angle_formatter->put_Signed(VARIANT_TRUE);
            angle_formatter->Format(delta_value, CComBSTR("°,\',\""), &bstrDelta);
            (*pTable)(row++, col) << RPT_ANGLE(OLE2T(bstrDelta));

            Float64 longTangent, shortTangent;
            hc->get_LongTangent(spiralType, &longTangent);
            hc->get_ShortTangent(spiralType, &shortTangent);
            (*pTable)(row++, col) << length.SetValue(longTangent);
            (*pTable)(row++, col) << length.SetValue(shortTangent);

            Float64 long_chord;
            hc->get_SpiralChord(spiralType, &long_chord);
            (*pTable)(row++, col) << length.SetValue(long_chord);

            Float64 X, Y;
            hc->get_X(spiralType, &X);
            hc->get_Y(spiralType, &Y);
            (*pTable)(row++, col) << length.SetValue(X);
            (*pTable)(row++, col) << length.SetValue(Y);

            Float64 Q, T;
            hc->get_Q(spiralType, &Q);
            hc->get_T(spiralType, &T);
            (*pTable)(row++, col) << length.SetValue(Q);
            (*pTable)(row++, col) << length.SetValue(T);
         }
      }

      // Key Points
      row++; // heading row
      CComPtr<IPoint2d> pnt;
      Float64 x, y;

      if (IsZero(hc_data.Radius))
      {
         if (bHasEntrySpirals)
         {
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
         }

         if (bHasCircularCurves)
         {
            (*pTable)(row++, col) << _T("-");
         }
      }
      else
      {
         if (IsEqual(stations.TSStation, stations.SCStation))
         {
            if (bHasEntrySpirals)
            {
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
            }

            if (bHasCircularCurves)
            {
               pRoadway->GetCurvePoint(hcIdx, cptTS, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);
            }
         }
         else
         {
            if (bHasEntrySpirals)
            {
               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptTS, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);

               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptSPI1, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);

               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptSC, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);
            }

            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << _T("-");
            }
         }
      }

      pnt.Release();
      pRoadway->GetCurvePoint(hcIdx, cptPI, pgsTypes::pcGlobal, &pnt);
      pnt->Location(&x, &y);
      (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);

      if (IsZero(hc_data.Radius))
      {
         if (bHasCircularCurves)
         {
            (*pTable)(row++, col) << _T("-");
         }

         if (bHasExitSpirals)
         {
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
            (*pTable)(row++, col) << _T("-");
         }

      }
      else
      {
         if (IsEqual(stations.CSStation, stations.STStation))
         {
            if (bHasCircularCurves)
            {
               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptST, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);
            }

            if (bHasExitSpirals)
            {
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
               (*pTable)(row++, col) << _T("-");
            }
         }
         else
         {
            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << _T("-");
            }

            if (bHasExitSpirals)
            {
               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptCS, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);

               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptSPI2, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);

               pnt.Release();
               pRoadway->GetCurvePoint(hcIdx, cptST, pgsTypes::pcGlobal, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);
            }
         }
      }

      if (bHasCircularCurves)
      {
         if (IsZero(hc_data.Radius))
         {
            (*pTable)(row++, col) << _T("-");
         }
         else
         {
            pnt.Release();
            pRoadway->GetCurvePoint(hcIdx, cptCCC, pgsTypes::pcGlobal, &pnt);
            pnt->Location(&x, &y);
            (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);
         }
      }

      if (bHasEntrySpirals || bHasExitSpirals)
      {
         if (IsZero(hc_data.EntrySpiral) && IsZero(hc_data.ExitSpiral))
         {
            (*pTable)(row++, col) << _T("-");
         }
         else
         {
            pnt.Release();
            pRoadway->GetCurvePoint(hcIdx, cptCC, pgsTypes::pcGlobal, &pnt);
            pnt->Location(&x, &y);
            (*pTable)(row, col) << length.SetValue(y) << _T(", "); (*pTable)(row++, col) << length.SetValue(x);
         }
      }

      if (hc != nullptr)
      {
         // we reported a real curve
         hcIdx++;
      }
   }
}

void write_profile_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   GET_IFACE2(pBroker, IRoadwayData, pAlignment ); 
   rptParagraph* pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), true );

   pPara = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pPara;
   *pPara << _T("Profile Details") << rptNewLine;

   ProfileData2 profile = pAlignment->GetProfileData2();

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Station: ") << rptRcStation(profile.Station, &pDisplayUnits->GetStationFormat()) << rptNewLine;
   *pPara << _T("Elevation: ") << length.SetValue(profile.Elevation) << rptNewLine;
   *pPara << _T("Grade: ") << profile.Grade*100 << _T("%") << rptNewLine;

   if ( profile.VertCurves.size() == 0 )
   {
      return;
   }

   GET_IFACE2(pBroker, IRoadway, pRoadway);

   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(profile.VertCurves.size()+1,_T("Vertical Curve Data"));

   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << pTable << rptNewLine;

   RowIndexType row = 0;
   ColumnIndexType col = 0;
   (*pTable)(row++,col) << _T("Curve Parameters");
   (*pTable)(row++,col) << _T("BVC Station");
   (*pTable)(row++,col) << _T("BVC Elevation");
   (*pTable)(row++,col) << _T("PVI Station");
   (*pTable)(row++,col) << _T("PVI Elevation");
   (*pTable)(row++,col) << _T("EVC Station");
   (*pTable)(row++,col) << _T("EVC Elevation");
   (*pTable)(row++,col) << _T("Entry Grade");
   (*pTable)(row++,col) << _T("Exit Grade");
   (*pTable)(row++,col) << _T("L1");
   (*pTable)(row++,col) << _T("L2");
   (*pTable)(row++,col) << _T("Length");
   (*pTable)(row++,col) << _T("High Pt Station");
   (*pTable)(row++,col) << _T("High Pt Elevation");
   (*pTable)(row++,col) << _T("Low Pt Station");
   (*pTable)(row++,col) << _T("Low Pt Elevation");

   col++;

   std::vector<VertCurveData>::iterator iter;
   for ( iter = profile.VertCurves.begin(); iter != profile.VertCurves.end(); iter++ )
   {
      row = 0;

      VertCurveData& vcd = *iter;

      (*pTable)(row++,col) << _T("Curve ") << col;
      if ( IsZero(vcd.L1) && IsZero(vcd.L2) )
      {
         Float64 pvi_elevation = pRoadway->GetElevation(vcd.PVIStation,0.0);
         Float64 g1;
         if ( iter == profile.VertCurves.begin() )
         {
            g1 = profile.Grade;
         }
         else
         {
            g1 = (*(iter-1)).ExitGrade;
         }

         Float64 g2 = vcd.ExitGrade;

         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << rptRcStation(vcd.PVIStation, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(pvi_elevation);
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << g1*100 << _T("%");
         (*pTable)(row++,col) << g2*100 << _T("%");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
      }
      else
      {
         CComPtr<IVertCurve> vc;
         pRoadway->GetVertCurve(col-1,&vc);

         CComPtr<IProfilePoint> bvc;
         vc->get_BVC(&bvc);

         CComPtr<IStation> bvc_station;
         bvc->get_Station(&bvc_station);

         Float64 bvc_station_value;
         bvc_station->get_Value(&bvc_station_value);

         Float64 bvc_elevation;
         bvc->get_Elevation(&bvc_elevation);


         CComPtr<IProfilePoint> pvi;
         vc->get_PVI(&pvi);

         CComPtr<IStation> pvi_station;
         pvi->get_Station(&pvi_station);

         Float64 pvi_station_value;
         pvi_station->get_Value(&pvi_station_value);

         Float64 pvi_elevation;
         pvi->get_Elevation(&pvi_elevation);


         CComPtr<IProfilePoint> evc;
         vc->get_EVC(&evc);

         CComPtr<IStation> evc_station;
         evc->get_Station(&evc_station);

         Float64 evc_station_value;
         evc_station->get_Value(&evc_station_value);

         Float64 evc_elevation;
         evc->get_Elevation(&evc_elevation);

         Float64 g1,g2;
         vc->get_EntryGrade(&g1);
         vc->get_ExitGrade(&g2);

         Float64 L1,L2, Length;
         vc->get_L1(&L1);
         vc->get_L2(&L2);
         vc->get_Length(&Length);

         CComPtr<IProfilePoint> high;
         vc->get_HighPoint(&high);

         CComPtr<IStation> high_station;
         high->get_Station(&high_station);

         Float64 high_station_value;
         high_station->get_Value(&high_station_value);

         Float64 high_elevation;
         high->get_Elevation(&high_elevation);

         CComPtr<IProfilePoint> low;
         vc->get_LowPoint(&low);

         CComPtr<IStation> low_station;
         low->get_Station(&low_station);

         Float64 low_station_value;
         low_station->get_Value(&low_station_value);

         Float64 low_elevation;
         low->get_Elevation(&low_elevation);

         (*pTable)(row++,col) << rptRcStation(bvc_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(bvc_elevation);
         (*pTable)(row++,col) << rptRcStation(pvi_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(pvi_elevation);
         (*pTable)(row++,col) << rptRcStation(evc_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(evc_elevation);
         (*pTable)(row++,col) << g1*100 << _T("%");
         (*pTable)(row++,col) << g2*100 << _T("%");
         (*pTable)(row++,col) << length.SetValue(L1);
         (*pTable)(row++,col) << length.SetValue(L2);
         (*pTable)(row++,col) << length.SetValue(Length);
         (*pTable)(row++,col) << rptRcStation(high_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(high_elevation);
         (*pTable)(row++,col) << rptRcStation(low_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(low_elevation);
      }

      col++;
   }
}

void write_crown_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   GET_IFACE2(pBroker, IRoadwayData, pAlignment ); 
   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,_T("Superelevation Details"));
   *pPara << pTable << rptNewLine;

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   (*pTable)(0,0) << _T("Section");
   (*pTable)(0,1) << _T("Station");
   (*pTable)(0,2) << _T("Left Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,3) << _T("Right Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,4) << COLHDR(_T("Crown Point Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), false );

   RoadwaySectionData section = pAlignment->GetRoadwaySectionData();

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   std::vector<CrownData2>::iterator iter;
   for ( iter = section.Superelevations.begin(); iter != section.Superelevations.end(); iter++ )
   {
      CrownData2& crown = *iter;
      (*pTable)(row,0) << row;
      (*pTable)(row,1) << rptRcStation(crown.Station,&pDisplayUnits->GetStationFormat());
      (*pTable)(row,2) << crown.Left;
      (*pTable)(row,3) << crown.Right;
      (*pTable)(row,4) << RPT_OFFSET(crown.CrownPointOffset,length);

      row++;
   }
}

void write_bridge_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;



   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("General Bridge Information"));
   *pPara << pTable << rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   RowIndexType row = 0;

   (*pTable)(row,0) << _T("Number of Spans");
   (*pTable)(row,1) << nSpans;
   row++;


   // collect all the girder types in use and report the unique names
   std::set<std::_tstring> strGirderNames;

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();

      for ( GroupIndexType gdrTypeGroupIdx = 0; gdrTypeGroupIdx < nGirderTypeGroups; gdrTypeGroupIdx++ )
      {
         GirderIndexType firstGdrIdx,lastGdrIdx;
         std::_tstring strGirderName;
         pGroup->GetGirderTypeGroup(gdrTypeGroupIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);
         strGirderNames.insert(strGirderName);
      }
   }

   (*pTable)(row,0) << (1 < strGirderNames.size() ? _T("Girder Types") : _T("Girder Type"));
   std::set<std::_tstring>::iterator iter;
   for ( iter = strGirderNames.begin(); iter != strGirderNames.end(); iter++ )
   {
      if ( iter != strGirderNames.begin() )
      {
         (*pTable)(row,1) << rptNewLine;
      }

      (*pTable)(row,1) << (*iter);
   }

   row++;
}

void write_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level)
{
   GET_IFACE2(pBroker,ILibrary, pLib );
   GET_IFACE2(pBroker,ISpecification, pSpec );
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   int loss_method = pSpecEntry->GetLossMethod();
   if ( loss_method == LOSSES_TIME_STEP )
   {
      switch( pSpecEntry->GetTimeDependentModel() )
      {
      case TDM_AASHTO:
         write_aci209_concrete_details(pBroker,pDisplayUnits,pChapter,girderKey,level,true/*AASHTO parameters*/);
         break;

      case TDM_ACI209:
         write_aci209_concrete_details(pBroker,pDisplayUnits,pChapter,girderKey,level,false/*AASHTO parameters*/);
         break;

      case TDM_CEBFIP:
         write_cebfip_concrete_details(pBroker,pDisplayUnits,pChapter,girderKey,level);
         break;
      }
   }
   else
   {
      write_lrfd_concrete_details(pBroker,pDisplayUnits,pChapter,girderKey,level);
   }
}

void write_lrfd_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bK1 = (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion());

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion());

   ColumnIndexType nColumns = 10;
   if ( bK1 )
   {
      nColumns += 6;
   }
   if ( bLambda )
   {
      nColumns++;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns,_T("Concrete Properties"));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pPara << pTable << rptNewLine;

   ColumnIndexType col = 0;
   RowIndexType row = 0;
   (*pTable)(row,col++) << _T("Element");
   (*pTable)(row,col++) << _T("Type");
   (*pTable)(row,col++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << COLHDR(RPT_ECI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << COLHDR(RPT_FC,  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << COLHDR(RPT_EC,  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(symbol(gamma),_T("w")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(symbol(gamma),_T("s")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(_T("D"),_T("agg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(row,col++) << COLHDR(RPT_STRESS(_T("ct")),  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( bK1 )
   {
      pTable->SetNumberOfHeaderRows(2);
      for ( ColumnIndexType i = 0; i < col; i++ )
      {
         pTable->SetRowSpan(0,i,2); 
         pTable->SetRowSpan(1,i,SKIP_CELL);
      }

      pTable->SetColumnSpan(0,col,2);
      pTable->SetColumnSpan(0,col+1,SKIP_CELL);
      (*pTable)(0,col) << Sub2(_T("E"),_T("c"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      pTable->SetColumnSpan(0,col+1,SKIP_CELL);
      (*pTable)(0,col) << _T("Creep");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      pTable->SetColumnSpan(0,col+1,SKIP_CELL);
      (*pTable)(0,col) << _T("Shrinkage");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));
   }

   if ( bLambda )
   {
      if ( bK1 )
      {
         pTable->SetRowSpan(0,col,2);
         pTable->SetRowSpan(1,col,SKIP_CELL);
         (*pTable)(0,col++) << symbol(lambda);
      }
      else
      {
         (*pTable)(row,col++) << symbol(lambda);
      }
   }


   row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType firstGirderIdx = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex));
      GirderIndexType lastGirderIdx  = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx));
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            EventIndexType erectEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());

            IntervalIndexType initialIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
            IntervalIndexType finalIntervalIdx   = pIntervals->GetInterval(erectEventIdx);

            Float64 fci = pMaterials->GetSegmentFc(thisSegmentKey,initialIntervalIdx);
            Float64 fc  = pMaterials->GetSegmentFc(thisSegmentKey,finalIntervalIdx);

            Float64 Eci = pMaterials->GetSegmentEc(thisSegmentKey,initialIntervalIdx);
            Float64 Ec  = pMaterials->GetSegmentEc(thisSegmentKey,finalIntervalIdx);

            Float64 lambda = pMaterials->GetSegmentLambda(thisSegmentKey);

            if ( nSegments == 1 )
            {
               (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(gdrIdx);
            }
            else
            {
               (*pTable)(row,0) << _T("Segment ") << LABEL_SEGMENT(segIdx);
            }

            write_lrfd_concrete_row(pDisplayUnits,pTable,fci,fc,Eci,Ec,lambda,pSegment->Material.Concrete,row);
            row++;

            const CClosureJointData* pClosure = pSegment->GetEndClosure();
            if ( pClosure )
            {
               ATLASSERT(false); // this should never happen because the basic concrete model
                                 // can't be used with PGSplice (PGSuper doesn't use closure joints)
            }
         } // segIdx
      } // gdrIdx
   } // spanIdx

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->GetDeckType() != pgsTypes::sdtNone )
   {
      IntervalIndexType intervalIdx = pIntervals->GetCompositeDeckInterval();

      Float64 fc = pMaterials->GetDeckFc(intervalIdx);
      Float64 Ec = pMaterials->GetDeckEc(intervalIdx);
      Float64 lambda = pMaterials->GetDeckLambda();

      (*pTable)(row,0) << _T("Deck");
      write_lrfd_concrete_row(pDisplayUnits,pTable,-1.0,fc,-1.0,Ec,lambda,pDeck->Concrete,row);
      row++;
   }

   if (pBridgeDesc->HasStructuralLongitudinalJoints())
   {
      IntervalIndexType intervalIdx = pIntervals->GetCompositeLongitudinalJointInterval();

      Float64 fc = pMaterials->GetLongitudinalJointFc(intervalIdx);
      Float64 Ec = pMaterials->GetLongitudinalJointEc(intervalIdx);
      Float64 lambda = pMaterials->GetLongitudinalJointLambda();

      (*pTable)(row, 0) << _T("Longitudinal Joint");
      write_lrfd_concrete_row(pDisplayUnits, pTable, -1.0, fc, -1.0, Ec, lambda, pBridgeDesc->GetLongitudinalJointMaterial(), row);
      row++;
   }
}

void write_lrfd_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fci,Float64 fc,Float64 Eci,Float64 Ec,Float64 lambda,const CConcreteMaterial& concrete,RowIndexType row)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         false );

   bool bK1 = (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion());
   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion());

   ColumnIndexType col = 1;

   (*pTable)(row,col++) << lrfdConcreteUtil::GetTypeName( (matConcrete::Type)concrete.Type, true );
   if ( !concrete.bHasInitial )
   {
      (*pTable)(row,col++) << _T("-");
   }
   else
   {
      (*pTable)(row,col++) << stress.SetValue( fci );
   }

   if ( !concrete.bHasInitial )
   {
      (*pTable)(row,col++) << _T("-");
   }
   else
   {
      (*pTable)(row,col++) << modE.SetValue( Eci );
   }

   (*pTable)(row,col++) << stress.SetValue( fc );
   (*pTable)(row,col++) << modE.SetValue( Ec );
   (*pTable)(row,col++) << density.SetValue( concrete.WeightDensity );

   if ( concrete.bUserEc )
   {
      (*pTable)(row,col++) << RPT_NA;
   }
   else
   {
      (*pTable)(row,col++) << density.SetValue( concrete.StrengthDensity );
   }

   (*pTable)(row,col++) << cmpdim.SetValue( concrete.MaxAggregateSize );

   if ( concrete.bHasFct )
   {
      (*pTable)(row,col++) << stress.SetValue( concrete.Fct );
   }
   else
   {
      (*pTable)(row,col++) << RPT_NA;
   }


   if (bK1)
   {
      (*pTable)(row,col++) << concrete.EcK1;
      (*pTable)(row,col++) << concrete.EcK2;
      (*pTable)(row,col++) << concrete.CreepK1;
      (*pTable)(row,col++) << concrete.CreepK2;
      (*pTable)(row,col++) << concrete.ShrinkageK1;
      (*pTable)(row,col++) << concrete.ShrinkageK2;
   }

   if ( bLambda )
   {
      (*pTable)(row,col++) << lambda;
   }
}

void write_aci209_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level,bool bAASHTOParameters)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(11 + (bAASHTOParameters ? 6 : 0),_T("Concrete Properties"));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pPara << pTable << rptNewLine;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ACI209_TimeDependentProperties.png")) << rptNewLine;

   ColumnIndexType col = 0;
   RowIndexType row = 0;
   (*pTable)(row,col++) << _T("Element");
   (*pTable)(row,col++) << _T("Type");
   (*pTable)(row,col++) << COLHDR(_T("(") << RPT_FC << _T(")") << Sub(_T("28")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << COLHDR(_T("(") << RPT_EC << _T(")") << Sub(_T("28")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << _T("Cure Method");
   (*pTable)(row,col++) << _T("Cement Type");
   (*pTable)(row,col++) << COLHDR(_T("a"), rptTimeUnitTag, pDisplayUnits->GetWholeDaysUnit() );
   (*pTable)(row,col++) << symbol(beta);
   (*pTable)(row,col++) << COLHDR(Sub2(symbol(gamma),_T("w")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(symbol(gamma),_T("s")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(_T("D"),_T("agg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( bAASHTOParameters )
   {
      pTable->SetNumberOfHeaderRows(2);
      for ( ColumnIndexType i = 0; i < col; i++ )
      {
         pTable->SetRowSpan(0,i,2); 
         pTable->SetRowSpan(1,i,SKIP_CELL);
      }

      pTable->SetColumnSpan(0,col,2);
      pTable->SetColumnSpan(0,col+1,SKIP_CELL);
      (*pTable)(0,col)   << Sub2(_T("E"),_T("c"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      pTable->SetColumnSpan(0,col+1,SKIP_CELL);
      (*pTable)(0,col)   << _T("Creep");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      pTable->SetColumnSpan(0,col+1,SKIP_CELL);
      (*pTable)(0,col)   << _T("Shrinkage");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));
   }

   row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IMaterials,pMaterials);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType firstGirderIdx = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex));
      GirderIndexType lastGirderIdx  = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx));
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            Float64 fc28 = pMaterials->GetSegmentFc28(thisSegmentKey);
            Float64 Ec28 = pMaterials->GetSegmentEc28(thisSegmentKey);

            if ( nSegments == 1 )
            {
               (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(gdrIdx);
            }
            else
            {
               (*pTable)(row,0) << _T("Segment ") << LABEL_SEGMENT(segIdx);
            }

            write_aci209_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pSegment->Material.Concrete,row,bAASHTOParameters);
            row++;

            const CClosureJointData* pClosure = pSegment->GetEndClosure();
            if ( pClosure )
            {
               Float64 fc28 = pMaterials->GetClosureJointFc28(thisSegmentKey);
               Float64 Ec28 = pMaterials->GetClosureJointEc28(thisSegmentKey);

               (*pTable)(row,0) << _T("Closure Joint ") << LABEL_SEGMENT(segIdx);
               write_aci209_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pClosure->GetConcrete(),row,bAASHTOParameters);
               row++;
            }
         } // segIdx
      } // gdrIdx
   } // spanIdx

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->GetDeckType() != pgsTypes::sdtNone )
   {
      Float64 fc28 = pMaterials->GetDeckFc28();
      Float64 Ec28 = pMaterials->GetDeckEc28();

      (*pTable)(row,0) << _T("Deck");
      write_aci209_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pDeck->Concrete,row,bAASHTOParameters);
      row++;
   }

   (*pPara) << Sub2(symbol(gamma),_T("w")) << _T(" =  Unit weight including reinforcement (used for dead load calculations)") << rptNewLine;
   (*pPara) << Sub2(symbol(gamma),_T("s")) << _T(" =  Unit weight (used to compute ") << Sub2(_T("E"),_T("c")) << _T(")") << rptNewLine;
   (*pPara) << Sub2(_T("D"),_T("agg")) << _T(" =  Maximum aggregate size") << rptNewLine;
}

void write_aci209_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row,bool bAASHTOParameters)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue,    time,    pDisplayUnits->GetFractionalDaysUnit(),     false );

   ColumnIndexType col = 1;
   (*pTable)(row,col++) << lrfdConcreteUtil::GetTypeName( (matConcrete::Type)concrete.Type, true );
   (*pTable)(row,col++) << stress.SetValue( fc28 );
   (*pTable)(row,col++) << modE.SetValue( Ec28 );

   std::_tstring strCure(concrete.CureMethod == pgsTypes::Steam ? _T("Steam") : _T("Moist"));
   (*pTable)(row,col++) << strCure;

   if ( concrete.bACIUserParameters )
   {
      (*pTable)(row,col++) << RPT_NA;
   }
   else
   {
      std::_tstring strCement(concrete.ACI209CementType == pgsTypes::TypeI ? _T("Type I") : _T("Type II"));
      (*pTable)(row,col++) << strCement;
   }
   (*pTable)(row,col++) << time.SetValue(concrete.A);
   (*pTable)(row,col++) << concrete.B;

   (*pTable)(row,col++) << density.SetValue( concrete.WeightDensity );

   if (concrete.bUserEc )
   {
      (*pTable)(row,col++) << RPT_NA;
   }
   else
   {
      (*pTable)(row,col++) << density.SetValue( concrete.StrengthDensity );
   }

   (*pTable)(row,col++) << cmpdim.SetValue( concrete.MaxAggregateSize );

   if ( bAASHTOParameters )
   {
      (*pTable)(row,col++) << concrete.EcK1;
      (*pTable)(row,col++) << concrete.EcK2;
      (*pTable)(row,col++) << concrete.CreepK1;
      (*pTable)(row,col++) << concrete.CreepK2;
      (*pTable)(row,col++) << concrete.ShrinkageK1;
      (*pTable)(row,col++) << concrete.ShrinkageK2;
   }
}

void write_cebfip_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(10,_T("Concrete Properties"));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pPara << pTable << rptNewLine;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("CEBFIP_TimeDependentProperties.png")) << rptNewLine;

   ColumnIndexType col = 0;
   RowIndexType row = 0;
   (*pTable)(row,col++) << _T("Element");
   (*pTable)(row,col++) << _T("Type");
   (*pTable)(row,col++) << COLHDR(_T("(") << RPT_FC << _T(")") << Sub(_T("28")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << COLHDR(_T("(") << RPT_EC << _T(")") << Sub(_T("28")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row,col++) << _T("Cement Type");
   (*pTable)(row,col++) << _T("s");
   (*pTable)(row,col++) << Sub2(symbol(beta),_T("SC"));
   (*pTable)(row,col++) << COLHDR(Sub2(symbol(gamma),_T("w")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(symbol(gamma),_T("s")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row,col++) << COLHDR(Sub2(_T("D"),_T("agg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IMaterials,pMaterials);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType firstGirderIdx = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex));
      GirderIndexType lastGirderIdx  = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx));
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            Float64 fc28 = pMaterials->GetSegmentFc28(thisSegmentKey);
            Float64 Ec28 = pMaterials->GetSegmentEc28(thisSegmentKey);

            if ( nSegments == 1 )
            {
               (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(gdrIdx);
            }
            else
            {
               (*pTable)(row,0) << _T("Segment ") << LABEL_SEGMENT(segIdx);
            }

            write_cebfip_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pSegment->Material.Concrete,row);
            row++;

            const CClosureJointData* pClosure = pSegment->GetEndClosure();
            if ( pClosure )
            {
               Float64 fc28 = pMaterials->GetClosureJointFc28(thisSegmentKey);
               Float64 Ec28 = pMaterials->GetClosureJointEc28(thisSegmentKey);

               (*pTable)(row,0) << _T("Closure Joint ") << LABEL_SEGMENT(segIdx);
               write_cebfip_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pClosure->GetConcrete(),row);
               row++;
            }
         } // segIdx
      } // gdrIdx
   } // spanIdx

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->GetDeckType() != pgsTypes::sdtNone )
   {
      Float64 fc28 = pMaterials->GetDeckFc28();
      Float64 Ec28 = pMaterials->GetDeckEc28();

      (*pTable)(row,0) << _T("Deck");
      write_cebfip_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pDeck->Concrete,row);
      row++;
   }

   (*pPara) << Sub2(symbol(beta),_T("SC")) << _T(" is used in CEB-FIP Eq'n. 2.1-76") << rptNewLine;
   (*pPara) << Sub2(symbol(gamma),_T("w")) << _T(" =  Unit weight including reinforcement (used for dead load calculations)") << rptNewLine;
   (*pPara) << Sub2(symbol(gamma),_T("s")) << _T(" =  Unit weight (used to compute ") << Sub2(_T("E"),_T("c")) << _T(")") << rptNewLine;
   (*pPara) << Sub2(_T("D"),_T("agg")) << _T(" =  Maximum aggregate size") << rptNewLine;
}

void write_cebfip_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         false );

   ColumnIndexType col = 1;
   (*pTable)(row,col++) << lrfdConcreteUtil::GetTypeName( (matConcrete::Type)concrete.Type, true );
   (*pTable)(row,col++) << stress.SetValue( fc28 );
   (*pTable)(row,col++) << modE.SetValue( Ec28 );

   if ( concrete.bCEBFIPUserParameters )
   {
      (*pTable)(row,col++) << RPT_NA;
   }
   else
   {
      (*pTable)(row,col++) << matCEBFIPConcrete::GetCementType((matCEBFIPConcrete::CementType)concrete.CEBFIPCementType);
   }

   (*pTable)(row,col++) << concrete.S;
   (*pTable)(row,col++) << concrete.BetaSc;

   (*pTable)(row,col++) << density.SetValue( concrete.WeightDensity );

   if (concrete.bUserEc )
   {
      (*pTable)(row,col++) << RPT_NA;
   }
   else
   {
      (*pTable)(row,col++) << density.SetValue( concrete.StrengthDensity );
   }

   (*pTable)(row,col++) << cmpdim.SetValue( concrete.MaxAggregateSize );
}

void write_pier_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, IBridge,      pBridge ); 

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false );

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
   angle_formatter->put_Signed(VARIANT_FALSE);

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   // Table for pier layout
   rptRcTable* pLayoutTable = rptStyleManager::CreateDefaultTable(5,_T("Pier Layout"));
   *pPara << pLayoutTable << rptNewLine;

   pLayoutTable->SetNumberOfHeaderRows(2);

   pLayoutTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pLayoutTable->SetRowSpan(0,0,2);
   pLayoutTable->SetRowSpan(1,0,SKIP_CELL);
   (*pLayoutTable)(0,0) << _T("");

   pLayoutTable->SetRowSpan(0,1,2);
   pLayoutTable->SetRowSpan(1,1,SKIP_CELL);
   (*pLayoutTable)(0,1) << _T("Station");

   pLayoutTable->SetRowSpan(0,2,2);
   pLayoutTable->SetRowSpan(1,2,SKIP_CELL);
   (*pLayoutTable)(0,2) << _T("Bearing");

   pLayoutTable->SetRowSpan(0,3,2);
   pLayoutTable->SetRowSpan(1,3,SKIP_CELL);
   (*pLayoutTable)(0,3) << _T("Skew Angle");

   pLayoutTable->SetRowSpan(0,4,2);
   pLayoutTable->SetRowSpan(1,4,SKIP_CELL);
   (*pLayoutTable)(0,4) << _T("Boundary") << rptNewLine << _T("Condition");

   // Table for pier diaphragms
   rptRcTable* pDiaphragmTable = rptStyleManager::CreateDefaultTable(9,_T("Pier Diaphragms"));
   *pPara << pDiaphragmTable << rptNewLine;

   pDiaphragmTable->SetNumberOfHeaderRows(2);

   pDiaphragmTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pDiaphragmTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pDiaphragmTable->SetRowSpan(0,0,2);
   pDiaphragmTable->SetRowSpan(1,0,SKIP_CELL);
   (*pDiaphragmTable)(0,0) << _T("");

   pDiaphragmTable->SetColumnSpan(0,1,4);
   pDiaphragmTable->SetColumnSpan(0,2,SKIP_CELL);
   pDiaphragmTable->SetColumnSpan(0,3,SKIP_CELL);
   pDiaphragmTable->SetColumnSpan(0,4,SKIP_CELL);
   (*pDiaphragmTable)(0,1) << _T("Diaphragm - Back");
   (*pDiaphragmTable)(1,1) << COLHDR(_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pDiaphragmTable)(1,2) << COLHDR(_T("Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pDiaphragmTable)(1,3) << _T("Loading");
   (*pDiaphragmTable)(1,4) << COLHDR(_T("Location(*)"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   pDiaphragmTable->SetColumnSpan(0,5,4);
   pDiaphragmTable->SetColumnSpan(0,6,SKIP_CELL);
   pDiaphragmTable->SetColumnSpan(0,7,SKIP_CELL);
   pDiaphragmTable->SetColumnSpan(0,8,SKIP_CELL);
   (*pDiaphragmTable)(0,5) << _T("Diaphragm - Ahead");
   (*pDiaphragmTable)(1,5) << COLHDR(_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pDiaphragmTable)(1,6) << COLHDR(_T("Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pDiaphragmTable)(1,7) << _T("Loading");
   (*pDiaphragmTable)(1,8) << COLHDR(_T("Location(*)"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   (*pPara) << _T("(*) Distance from Abutment/Pier Line to Centroid of Diaphragm") << rptNewLine;

   // table for connection data
   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pConnectionTable = rptStyleManager::CreateDefaultTable(9,_T("Pier Connections"));
   *pPara << pConnectionTable << rptNewLine;

   pConnectionTable->SetNumberOfHeaderRows(2);

   pConnectionTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pConnectionTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pConnectionTable->SetRowSpan(0,0,2);
   pConnectionTable->SetRowSpan(1,0,SKIP_CELL);
   (*pConnectionTable)(0,0) << _T("");

   pConnectionTable->SetColumnSpan(0,1,4);
   pConnectionTable->SetColumnSpan(0,2,SKIP_CELL);
   pConnectionTable->SetColumnSpan(0,3,SKIP_CELL);
   pConnectionTable->SetColumnSpan(0,4,SKIP_CELL);
   (*pConnectionTable)(0,1) << _T("Back");
   (*pConnectionTable)(1,1) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,2) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*pConnectionTable)(1,3) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,4) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   pConnectionTable->SetColumnSpan(0,5,4);
   pConnectionTable->SetColumnSpan(0,6,SKIP_CELL);
   pConnectionTable->SetColumnSpan(0,7,SKIP_CELL);
   pConnectionTable->SetColumnSpan(0,8,SKIP_CELL);
   (*pConnectionTable)(0,5) << _T("Ahead");
   (*pConnectionTable)(1,5) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,6) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*pConnectionTable)(1,7) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,8) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << _T("Bearing Offset Measure") << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder,  true,  true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder,  true,  false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder,  false, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder,  false, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true,  true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true,  false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false, false) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("End Distance Measure") << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder,  true,  true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder,  true,  false) << rptNewLine;
   //*pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder,  false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder,  false, false) << rptNewLine; // produces the same result at the line above
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true,  true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true,  false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder,     true,  true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder,     true,  false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder,     false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder,     false, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier,    true,  true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier,    true,  false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier,    false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier,    false, false) << rptNewLine;

   bool bNoDeck = IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType());

   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   RowIndexType row1 = pLayoutTable->GetNumberOfHeaderRows();
   RowIndexType row2 = pDiaphragmTable->GetNumberOfHeaderRows();
   RowIndexType row3 = pConnectionTable->GetNumberOfHeaderRows();
   while ( pPier != nullptr )
   {
      PierIndexType pierIdx = pPier->GetIndex();

      CComPtr<IDirection> bearing;
      CComPtr<IAngle> skew;

      pBridge->GetPierDirection( pierIdx, &bearing );
      pBridge->GetPierSkew( pierIdx, &skew );

      Float64 skew_value;
      skew->get_Value(&skew_value);

      CComBSTR bstrAngle;
      angle_formatter->Format(skew_value,CComBSTR("°,\',\""),&bstrAngle);

      Float64 bearing_value;
      bearing->get_Value(&bearing_value);

      CComBSTR bstrBearing;
      direction_formatter->Format(bearing_value,CComBSTR("°,\',\""),&bstrBearing);

      bool bAbutment = pPier->IsAbutment();
      if ( bAbutment )
      {
         (*pLayoutTable)(row1,0) << _T("Abutment ") << LABEL_PIER(pierIdx);
         (*pDiaphragmTable)(row2,0) << _T("Abutment ") << LABEL_PIER(pierIdx);
         (*pConnectionTable)(row3,0) << _T("Abutment ") << LABEL_PIER(pierIdx);
      }
      else
      {
         (*pLayoutTable)(row1,0) << _T("Pier ") << LABEL_PIER(pierIdx);
         (*pDiaphragmTable)(row2,0) << _T("Pier ") << LABEL_PIER(pierIdx);
         (*pConnectionTable)(row3,0) << _T("Pier ") << LABEL_PIER(pierIdx);
      }

      (*pLayoutTable)(row1,1) << rptRcStation(pPier->GetStation(), &pDisplayUnits->GetStationFormat() );
      (*pLayoutTable)(row1,2) << RPT_BEARING(OLE2T(bstrBearing));
      (*pLayoutTable)(row1,3) << RPT_ANGLE(OLE2T(bstrAngle));

      if ( pPier->IsInteriorPier() )
      {
         (*pLayoutTable)(row1,4) << CPierData2::AsString(pPier->GetSegmentConnectionType());
      }
      else
      {
         (*pLayoutTable)(row1,4) << CPierData2::AsString(pPier->GetBoundaryConditionType(),bNoDeck);
      }

      row1++;

      // diaphragm table
      if ( pPier->GetPrevSpan() )
      {
         if ( pPier->GetDiaphragmHeight(pgsTypes::Back) < 0 )
         {
            (*pDiaphragmTable)(row2,1) << _T("Compute");
         }
         else
         {
            (*pDiaphragmTable)(row2,1) << cmpdim.SetValue(pPier->GetDiaphragmHeight(pgsTypes::Back));
         }

         if ( pPier->GetDiaphragmWidth(pgsTypes::Back) < 0 )
         {
            (*pDiaphragmTable)(row2,2) << _T("Compute");
         }
         else
         {
            (*pDiaphragmTable)(row2,2) << cmpdim.SetValue(pPier->GetDiaphragmWidth(pgsTypes::Back));
         }
         switch( pPier->GetDiaphragmLoadType(pgsTypes::Back) )
         {
         case ConnectionLibraryEntry::ApplyAtBearingCenterline:
            (*pDiaphragmTable)(row2,3) << _T("Apply load at centerline bearing");
            (*pDiaphragmTable)(row2,4) << RPT_NA;
            break;
         case ConnectionLibraryEntry::ApplyAtSpecifiedLocation:
            (*pDiaphragmTable)(row2,3) << _T("Apply load to girder");
            (*pDiaphragmTable)(row2,4) << cmpdim.SetValue(pPier->GetDiaphragmLoadLocation(pgsTypes::Back));
            break;
         case ConnectionLibraryEntry::DontApply:
            (*pDiaphragmTable)(row2,3) << _T("Ignore weight");
            (*pDiaphragmTable)(row2,4) << RPT_NA;
            break;
         default:
            ATLASSERT(false); // is there a new type????
         }
      }
      else
      {
         (*pDiaphragmTable)(row2,1) << _T("");
         (*pDiaphragmTable)(row2,2) << _T("");
         (*pDiaphragmTable)(row2,3) << _T("");
         (*pDiaphragmTable)(row2,4) << _T("");
      }

      if ( pPier->GetNextSpan() )
      {
         if ( pPier->GetDiaphragmHeight(pgsTypes::Ahead) < 0 )
         {
            (*pDiaphragmTable)(row2,5) << _T("Compute");
         }
         else
         {
            (*pDiaphragmTable)(row2,5) << cmpdim.SetValue(pPier->GetDiaphragmHeight(pgsTypes::Ahead));
         }

         if ( pPier->GetDiaphragmWidth(pgsTypes::Ahead) < 0 )
         {
            (*pDiaphragmTable)(row2,6) << _T("Compute");
         }
         else
         {
            (*pDiaphragmTable)(row2,6) << cmpdim.SetValue(pPier->GetDiaphragmWidth(pgsTypes::Ahead));
         }

         switch( pPier->GetDiaphragmLoadType(pgsTypes::Ahead) )
         {
         case ConnectionLibraryEntry::ApplyAtBearingCenterline:
            (*pDiaphragmTable)(row2,7) << _T("Apply load at centerline bearing");
            (*pDiaphragmTable)(row2,8) << RPT_NA;
            break;
         case ConnectionLibraryEntry::ApplyAtSpecifiedLocation:
            (*pDiaphragmTable)(row2,7) << _T("Apply load to girder");
            (*pDiaphragmTable)(row2,8) << cmpdim.SetValue(pPier->GetDiaphragmLoadLocation(pgsTypes::Ahead));
            break;
         case ConnectionLibraryEntry::DontApply:
            (*pDiaphragmTable)(row2,7) << _T("Ignore weight");
            (*pDiaphragmTable)(row2,8) << RPT_NA;
            break;
         default:
            ATLASSERT(false); // is there a new type????
         }
      }
      else
      {
         (*pDiaphragmTable)(row2,5) << _T("");
         (*pDiaphragmTable)(row2,6) << _T("");
         (*pDiaphragmTable)(row2,7) << _T("");
         (*pDiaphragmTable)(row2,8) << _T("");
      }

      row2++;

      //
      // Connection table
      //

      // back side
      if ( pPier->GetPrevSpan() )
      {
         Float64 brgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
         pPier->GetBearingOffset(pgsTypes::Back,&brgOffset,&brgOffsetMeasure);
         (*pConnectionTable)(row3,1) << cmpdim.SetValue(brgOffset);

         (*pConnectionTable)(row3,2) << GetBearingOffsetMeasureString(brgOffsetMeasure,bAbutment,true);

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
         pPier->GetGirderEndDistance(pgsTypes::Back,&endDist,&endDistMeasure);
         (*pConnectionTable)(row3,3) << cmpdim.SetValue(endDist);
         (*pConnectionTable)(row3,4) << GetEndDistanceMeasureString(endDistMeasure,bAbutment,true);
      }
      else
      {
         (*pConnectionTable)(row3,1) << _T("");
         (*pConnectionTable)(row3,2) << _T("");
         (*pConnectionTable)(row3,3) << _T("");
         (*pConnectionTable)(row3,4) << _T("");
      }

      // Ahead side
      if ( pPier->GetNextSpan() )
      {
         Float64 brgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
         pPier->GetBearingOffset(pgsTypes::Ahead,&brgOffset,&brgOffsetMeasure);
         (*pConnectionTable)(row3,5) << cmpdim.SetValue(brgOffset);
         (*pConnectionTable)(row3,6) << GetBearingOffsetMeasureString(brgOffsetMeasure,bAbutment,true);

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
         pPier->GetGirderEndDistance(pgsTypes::Ahead,&endDist,&endDistMeasure);
         (*pConnectionTable)(row3,7) << cmpdim.SetValue(endDist);
         (*pConnectionTable)(row3,8) << GetEndDistanceMeasureString(endDistMeasure,bAbutment,true);
      }
      else
      {
         (*pConnectionTable)(row3,5) << _T("");
         (*pConnectionTable)(row3,6) << _T("");
         (*pConnectionTable)(row3,7) << _T("");
         (*pConnectionTable)(row3,8) << _T("");
      }

      if ( pPier->GetNextSpan() )
      {
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }

      row3++;
   }
}

void write_bearing_data(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter, Uint16 level, const CGirderKey& girderKey)
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false);

   rptParagraph* pPara1 = new rptParagraph;
   pPara1->SetStyleName(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara1;
   *pPara1 << _T("Bearing Details");

   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("BearingDetails.png")) << rptNewLine;

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2_NOCHECK(pBroker, IBridgeDescription, pIBridgeDesc);

   PierIndexType nPiers = pBridge->GetPierCount();

   // don't want temp supports so use last interval
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   rptRcTable* ptable = rptStyleManager::CreateDefaultTable(10, _T(""));
   *pPara << ptable;
   (*ptable)(0,0) << _T("Location");
   (*ptable)(0,1) << _T("Shape");
   (*ptable)(0,2) << _T("# of") << rptNewLine << _T("Bearings");
   (*ptable)(0,3) << COLHDR(_T("Spacing"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*ptable)(0,4) << COLHDR(_T("Length or")<<rptNewLine<<_T("Diameter"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*ptable)(0,5) << COLHDR(_T("Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*ptable)(0,6) << COLHDR(_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*ptable)(0,7) << COLHDR(_T("Recess")<<rptNewLine<<_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*ptable)(0,8) << COLHDR(_T("Recess")<<rptNewLine<<_T("Length"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*ptable)(0,9) << COLHDR(_T("Sole Plate")<<rptNewLine<<_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   ptable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   ptable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   // TRICKY: use adapter class to get bearing lines over piers
   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);
   std::unique_ptr<IProductReactionAdapter> pBearingLines =  std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign,lastIntervalIdx,girderKey);

   RowIndexType row = 1;
   ReactionLocationIter iter = pBearingLines->GetReactionLocations(pBridge);
   for (iter.First(); !iter.IsDone(); iter.Next())
   {
      ColumnIndexType col = 0;
      const ReactionLocation& brgLoc(iter.CurrentItem());

      pgsTypes::PierFaceType pf = brgLoc.Face == rftBack ? pgsTypes::Back : pgsTypes::Ahead;

      const CBearingData2* pbd = pIBridgeDesc->GetBearingData(brgLoc.PierIdx, pf, girderKey.girderIndex);

      if (pbd != nullptr)
      {
         bool is_rect = pbd->Shape == bsRectangular;

         (*ptable)(row, col++) << brgLoc.PierLabel;
         (*ptable)(row, col++) << (is_rect ? _T("Rectangular") : _T("Round"));
         (*ptable)(row, col++) << pbd->BearingCount;
         if (pbd->BearingCount > 1)
         {
            (*ptable)(row, col++) << cmpdim.SetValue(pbd->Spacing);
         }
         else
         {
            (*ptable)(row, col++) << RPT_NA;;
         }

         (*ptable)(row, col++) << cmpdim.SetValue(pbd->Length);
         if (is_rect)
         {
            (*ptable)(row, col++) << cmpdim.SetValue(pbd->Width);
         }
         else
         {
            (*ptable)(row, col++) << RPT_NA;;
         }

         (*ptable)(row, col++) << cmpdim.SetValue(pbd->Height);
         (*ptable)(row, col++) << cmpdim.SetValue(pbd->RecessHeight);
         (*ptable)(row, col++) << cmpdim.SetValue(pbd->RecessLength);
         (*ptable)(row, col++) << cmpdim.SetValue(pbd->SolePlateHeight);

         row++;
      }
      else
      {
         ATLASSERT(0); // should never happen
      }
   }
}

void write_ts_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, ILibrary,     pLib );
   GET_IFACE2(pBroker, ITempSupport,  pTemporarySupport ); 
   GET_IFACE2(pBroker, IRoadwayData, pAlignment);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   INIT_UV_PROTOTYPE( rptLengthUnitValue, xdim,   pDisplayUnits->GetXSectionDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, offset, pDisplayUnits->GetAlignmentLengthUnit(), true );

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
   angle_formatter->put_Signed(VARIANT_FALSE);

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pLayoutTable = rptStyleManager::CreateDefaultTable(6,_T("Temporary Supports"));
   *pPara << pLayoutTable << rptNewLine;

   pLayoutTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*pLayoutTable)(0,0) << _T("");
   (*pLayoutTable)(0,1) << _T("Station");
   (*pLayoutTable)(0,2) << _T("Bearing");
   (*pLayoutTable)(0,3) << _T("Skew Angle");
   (*pLayoutTable)(0,4) << _T("Type");
   (*pLayoutTable)(0,5) << _T("Erection") << rptNewLine << _T("Stage");
   (*pLayoutTable)(0,6) << _T("Removal") << rptNewLine << _T("Stage");

   // connections
   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pConnectionsTable = rptStyleManager::CreateDefaultTable(8,_T("Temporary Support Connections"));
   *pPara << pConnectionsTable << rptNewLine;

   pConnectionsTable->SetNumberOfHeaderRows(1);

   pConnectionsTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pConnectionsTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*pConnectionsTable)(0,0) << _T("");
   (*pConnectionsTable)(0,1) << _T("Boundary") << rptNewLine << _T("Condition");
   (*pConnectionsTable)(0,2) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionsTable)(0,3) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*pConnectionsTable)(0,4) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionsTable)(0,5) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");
   (*pConnectionsTable)(0,6) << _T("Closure") << rptNewLine << _T("Stage");

   RowIndexType row1 = pLayoutTable->GetNumberOfHeaderRows();
   RowIndexType row2 = pConnectionsTable->GetNumberOfHeaderRows();
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();

      CComPtr<IDirection> bearing;
      CComPtr<IAngle> skew;

      pTemporarySupport->GetDirection( tsIdx, &bearing );
      pTemporarySupport->GetSkew( tsIdx, &skew );
      
      Float64 skew_value;
      skew->get_Value(&skew_value);

      CComBSTR bstrAngle;
      angle_formatter->Format(skew_value,CComBSTR("°,\',\""),&bstrAngle);

      Float64 bearing_value;
      bearing->get_Value(&bearing_value);

      CComBSTR bstrBearing;
      direction_formatter->Format(bearing_value,CComBSTR("°,\',\""),&bstrBearing);

      (*pLayoutTable)(row1,0) << _T("TS ") << LABEL_TEMPORARY_SUPPORT(tsIdx);

      (*pLayoutTable)(row1,1) << rptRcStation(pTS->GetStation(), &pDisplayUnits->GetStationFormat() );
      (*pLayoutTable)(row1,2) << RPT_BEARING(OLE2T(bstrBearing));
      (*pLayoutTable)(row1,3) << RPT_ANGLE(OLE2T(bstrAngle));
      (*pLayoutTable)(row1,4) << CTemporarySupportData::AsString(pTS->GetSupportType());

      EventIndexType erectionEventIdx, removalEventIdx;
      pTimelineMgr->GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);

      const CTimelineEvent* pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);
      const CTimelineEvent* pRemovalEvent  = pTimelineMgr->GetEventByIndex(removalEventIdx);

      (*pLayoutTable)(row1,5) << _T("Event ") << LABEL_EVENT(erectionEventIdx) << _T(": ") << pErectionEvent->GetDescription();
      (*pLayoutTable)(row1,6) << _T("Event ") << LABEL_EVENT(removalEventIdx)  << _T(": ") << pRemovalEvent->GetDescription();

      row1++;

      (*pConnectionsTable)(row2,0) << _T("TS ") << LABEL_TEMPORARY_SUPPORT(tsIdx);
      (*pConnectionsTable)(row2,1) << CTemporarySupportData::AsString(pTS->GetConnectionType());

      if ( pTS->GetConnectionType() == pgsTypes::tsctClosureJoint )
      {
         Float64 brgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
         pTS->GetBearingOffset(&brgOffset,&brgOffsetMeasure);
         (*pConnectionsTable)(row2,2) << cmpdim.SetValue(brgOffset);
         (*pConnectionsTable)(row2,3) << GetBearingOffsetMeasureString(brgOffsetMeasure,false,false);

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
         pTS->GetGirderEndDistance(&endDist,&endDistMeasure);
         (*pConnectionsTable)(row2,4) << cmpdim.SetValue(endDist);
         (*pConnectionsTable)(row2,5) << GetEndDistanceMeasureString(endDistMeasure,false,false);

         const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(0);
         IDType cpID = pClosureJoint->GetID();
         EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(cpID);
         const CTimelineEvent* pCastClosureEvent = pTimelineMgr->GetEventByIndex(castClosureEventIdx);
         (*pConnectionsTable)(row2,6) << _T("Event ") << LABEL_EVENT(castClosureEventIdx) << _T(": ") << pCastClosureEvent->GetDescription();
      }
      else
      {
         (*pConnectionsTable)(row2,2) << RPT_NA;
         (*pConnectionsTable)(row2,3) << RPT_NA;
         (*pConnectionsTable)(row2,4) << RPT_NA;
         (*pConnectionsTable)(row2,5) << RPT_NA;
         (*pConnectionsTable)(row2,6) << RPT_NA;
      }

      row2++;
   }
}

void write_framing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3,_T("Framing"));
   *pPara << pTable << rptNewLine;

   (*pTable)(0,0) << _T("");

   if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      (*pTable)(0,1) << _T("Girder Spacing");
   }
   else
   {
      (*pTable)(0,1) << _T("Joint Spacing");
   }

   (*pTable)(0,2) << _T("Datum");

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CPierData2* pPier = pBridgeDesc->GetGirderGroup(grpIdx)->GetPier(pgsTypes::metStart);
      const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(0);
      const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
      (*pTable)(row,0) << _T("Pier ") << LABEL_PIER(pPier->GetIndex());
      write_girder_spacing(pBroker,pDisplayUnits,pTable,pSpacing,row,1);

      row++;

      const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
      while ( pSegment )
      {
         const CPierData2* pPier = nullptr;
         const CTemporarySupportData* pTS = nullptr;
         const CClosureJointData* pClosure = pSegment->GetEndClosure();
         if ( pClosure == nullptr )
         {
            pPier = pGirder->GetPier(pgsTypes::metEnd);
         }
         else
         {
            pPier = pClosure->GetPier();
            pTS = pClosure->GetTemporarySupport();
         }

         if ( pPier )
         {
            const CGirderSpacing2* pSpacing;
            if ( pPier->GetNextSpan() )
            {
               pSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
            }
            else
            {
               pSpacing = pPier->GetGirderSpacing(pgsTypes::Back);
            }

            (*pTable)(row,0) << _T("Pier ") << LABEL_PIER(pPier->GetIndex());
            write_girder_spacing(pBroker,pDisplayUnits,pTable,pSpacing,row,1);
         }
         else if ( pTS )
         {
            (*pTable)(row,0) << _T("TS ") << LABEL_TEMPORARY_SUPPORT(pTS->GetIndex());
            const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();
            write_girder_spacing(pBroker,pDisplayUnits,pTable,pSpacing,row,1);
         }
         else
         {
            ATLASSERT(false);
         }

         if ( pClosure )
         {
            pSegment = pClosure->GetRightSegment();
         }
         else
         {
            pSegment = nullptr;
         }

         row++;
      }
      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      
      if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
      {
         *pPara << _T("Girder Spacing Datum") << rptNewLine;
      }
      else
      {
         *pPara << _T("Joint Spacing Datum") << rptNewLine;
      }

      *pPara << _T("(1) Measured normal to the alignment at the centerline of pier") << rptNewLine;
      *pPara << _T("(2) Measured normal to the alignment at the centerline of bearing") << rptNewLine;
      *pPara << _T("(3) Measured at and along the centerline of pier") << rptNewLine;
      *pPara << _T("(4) Measured at and along the centerline of bearing") << rptNewLine;
   } // next group
}

void write_span_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(6,_T("Spans"));
   *pPara << pTable << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);
   pTable->SetRowSpan(0,0,2);
   (*pTable)(0,0) << _T("Span");

   pTable->SetRowSpan(0,1,2);
   (*pTable)(0,1) << _T("#") << rptNewLine << _T("Girders");

   pTable->SetRowSpan(1,4,SKIP_CELL);
   pTable->SetRowSpan(1,5,SKIP_CELL);

   pTable->SetColumnSpan(0,2,2);
   (*pTable)(0,2) << _T("Start of Span");

   if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      (*pTable)(1,0) << _T("Girder Spacing");
   }
   else
   {
      (*pTable)(1,0) << _T("Joint Spacing");
   }

   (*pTable)(1,1) << _T("Datum");

   pTable->SetColumnSpan(0,3,2);
   pTable->SetColumnSpan(0,4,SKIP_CELL);
   pTable->SetColumnSpan(0,5,SKIP_CELL);
   (*pTable)(0,3) << _T("End of Span");

   if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      (*pTable)(1,2) << _T("Girder Spacing");
   }
   else
   {
      (*pTable)(1,2) << _T("Joint Spacing");
   }

   (*pTable)(1,3) << _T("Datum");

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();

   RowIndexType row = 2; // data starts on row to because we have 2 heading rows

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();

      (*pTable)(row,0) << LABEL_GROUP(grpIdx);
      (*pTable)(row,1) << nGirders;

      if ( 1 < nGirders )
      {
         const CGirderSpacing2* pStartGirderSpacing = pGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
         write_girder_spacing(pBroker,pDisplayUnits,pTable,pStartGirderSpacing,row,2);

         const CGirderSpacing2* pEndGirderSpacing = pGroup->GetPier(pgsTypes::metEnd)->GetGirderSpacing(pgsTypes::Back);
         write_girder_spacing(pBroker,pDisplayUnits,pTable,pEndGirderSpacing,row,4);
      }
      else
      {
         (*pTable)(row,2) << _T("-");
         (*pTable)(row,3) << _T("-");
         (*pTable)(row,4) << _T("-");
         (*pTable)(row,5) << _T("-");
      }

      row++;
   }

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   
   if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      *pPara << _T("Girder Spacing Datum") << rptNewLine;
   }
   else
   {
      *pPara << _T("Joint Spacing Datum") << rptNewLine;
   }

   *pPara << _T("(1) Measured normal to the alignment at the abutment/pier line") << rptNewLine;
   *pPara << _T("(2) Measured normal to the alignment at the centerline of bearing") << rptNewLine;
   *pPara << _T("(3) Measured at and along the abutment/pier line") << rptNewLine;
   *pPara << _T("(4) Measured at and along the centerline of bearing") << rptNewLine;
}

void write_girder_spacing(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,const CGirderSpacing2* pGirderSpacing,RowIndexType row,ColumnIndexType col)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool bIsGirderSpacing = IsGirderSpacing( pBridgeDesc->GetGirderSpacingType() );

   INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdim, (bIsGirderSpacing ? pDisplayUnits->GetXSectionDimUnit() :  pDisplayUnits->GetComponentDimUnit()),  true );

   GroupIndexType nSpacingGroups = pGirderSpacing->GetSpacingGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nSpacingGroups; grpIdx++ )
   {
      if ( grpIdx != 0 )
      {
         (*pTable)(row,col) << rptNewLine;
      }

      GirderIndexType firstGdrIdx, lastGdrIdx;
      Float64 spacing;
      pGirderSpacing->GetSpacingGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

      ATLASSERT( firstGdrIdx != lastGdrIdx );
      (*pTable)(row,col) << _T("Between ") << LABEL_GIRDER(firstGdrIdx) << _T(" & ") << LABEL_GIRDER(lastGdrIdx) << _T(" : ") << xdim.SetValue(spacing);
   }

   if ( pGirderSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      if ( pGirderSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine )
      {
         (*pTable)(row,col+1) << _T("1");
      }
      else
      {
         (*pTable)(row,col+1) << _T("2");
      }
   }
   else
   {
      if ( pGirderSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine )
      {
         (*pTable)(row,col+1) << _T("3");
      }
      else
      {
         (*pTable)(row,col+1) << _T("4");
      }
   }
}

void write_ps_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const CGirderKey& girderKey)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdim,    pDisplayUnits->GetXSectionDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,   pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dia,     pDisplayUnits->GetComponentDimUnit(), true );

   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, ISegmentData,      pSegmentData);
   GET_IFACE2(pBroker, IBridge,           pBridge ); 
   GET_IFACE2(pBroker, IStrandGeometry,   pStrand);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, ISpecification,    pSpec );

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      GirderIndexType firstGirderIdx = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex ));
      GirderIndexType lastGirderIdx  = Min(nGirders,  (girderKey.girderIndex == ALL_GIRDERS ? nGirders : firstGirderIdx));
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            // Setup the table
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,SEGMENT_LABEL(thisSegmentKey));
            pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
            pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
            *pPara << pTable << rptNewLine;

            RowIndexType row = 0;

            const CStrandData* pStrands = pSegmentData->GetStrandData(thisSegmentKey);
            bool harpedAreStraight = pStrand->GetAreHarpedStrandsForcedStraight(thisSegmentKey);

            (*pTable)(row,0) << _T("Girder Type");
            (*pTable)(row,1) << pGroup->GetGirder(gdrIdx)->GetGirderName();
            row++;

            if (pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone)
            {
               PierIndexType startPierIdx, endPierIdx;
               pBridge->GetGirderGroupPiers(grpIdx, &startPierIdx, &endPierIdx);
               for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
               {
                  if (pBridge->IsAbutment(pierIdx))
                  {
                     (*pTable)(row, 0) << _T("Slab Offset (\"A\" Dimension) at Abutment ") << LABEL_PIER(pierIdx);
                  }
                  else
                  {
                     (*pTable)(row, 0) << _T("Slab Offset (\"A\" Dimension) at Pier ") << LABEL_PIER(pierIdx);
                  }
                  (*pTable)(row, 1) << cmpdim.SetValue(pBridge->GetSlabOffset(thisSegmentKey.groupIndex, pierIdx, thisSegmentKey.girderIndex));
                  row++;
               }
            }

            if (IsTopWidthSpacing(pBridgeDesc->GetGirderSpacingType()))
            {
               (*pTable)(row, 0) << _T("Top Flange Width");
               Float64 wLeft, wRight;
               Float64 wStart = pGirder->GetTopWidth(pgsTypes::metStart, &wLeft, &wRight);
               Float64 wEnd = pGirder->GetTopWidth(pgsTypes::metEnd, &wLeft, &wRight);
               if (IsEqual(wStart, wEnd))
               {
                  (*pTable)(row, 1) << xdim.SetValue(wStart);
               }
               else
               {
                  (*pTable)(row, 1) << _T("Start: ") << xdim.SetValue(wStart) << rptNewLine;
                  (*pTable)(row, 1) << _T("End: ") << xdim.SetValue(wEnd);
               }
               row++;
            }
            
            if (pSpec->IsAssExcessCamberInputEnabled())
            {
               (*pTable)(row, 0) << _T("Assumed Excess Camber");
               (*pTable)(row, 1) << cmpdim.SetValue(pBridge->GetAssExcessCamber(thisSegmentKey.groupIndex, thisSegmentKey.girderIndex));
               row++;
            }

            CString strFillType;
            if (pStrands->GetStrandDefinitionType() == CStrandData::sdtTotal)
            {
               strFillType = "Sequence of Permanent Strands";
            }
            else if (pStrands->GetStrandDefinitionType() == CStrandData::sdtStraightHarped)
            {
               strFillType = "Sequence of Straight/Harped Strands";
            }
            else if (pStrands->GetStrandDefinitionType() == CStrandData::sdtDirectSelection)
            {
               strFillType = "Non-Sequential, Direct Fill";
            }
            else if (pStrands->GetStrandDefinitionType() == CStrandData::sdtDirectInput)
            {
               strFillType = "User Defined Strands";
            }
            else
            {
               ATLASSERT(false);
            }

            (*pTable)(row,0) << _T("Strand Fill Type");
            (*pTable)(row,1) << strFillType;
            row++;

            (*pTable)(row,0) << _T("Type of Adjustable Web Strands");
            (*pTable)(row,1) << LABEL_HARP_TYPE(harpedAreStraight);
            row++;

            (*pTable)(row,0) << _T("Number of Straight Strands");
            (*pTable)(row,1) << pStrand->GetStrandCount(thisSegmentKey,pgsTypes::Straight);
            StrandIndexType nDebonded = pStrand->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Straight,pgsTypes::dbetEither);
            if ( nDebonded != 0 )
            {
               (*pTable)(row,1) << rptNewLine << nDebonded << _T(" debonded");
            }

            StrandIndexType nExtendedLeft  = pStrand->GetNumExtendedStrands(thisSegmentKey,pgsTypes::metStart,pgsTypes::Straight);
            StrandIndexType nExtendedRight = pStrand->GetNumExtendedStrands(thisSegmentKey,pgsTypes::metEnd,pgsTypes::Straight);
            if ( nExtendedLeft != 0 || nExtendedRight != 0 )
            {
               (*pTable)(row,1) << rptNewLine << nExtendedLeft  << _T(" extended at left end");
               (*pTable)(row,1) << rptNewLine << nExtendedRight << _T(" extended at right end");
            }
            row++;

            (*pTable)(row,0) << _T("Straight Strand P") << Sub(_T("jack"));
            (*pTable)(row,1) << force.SetValue(pStrands->GetPjack(pgsTypes::Straight));
            row++;

            (*pTable)(row,0) << _T("Number of ")<< LABEL_HARP_TYPE(harpedAreStraight) <<_T(" Strands");
            (*pTable)(row,1) << pStrand->GetStrandCount(thisSegmentKey,pgsTypes::Harped);
            nDebonded = pStrand->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Harped,pgsTypes::dbetEither);
            if ( nDebonded != 0 )
            {
               (*pTable)(row,1) << _T(" (") << nDebonded << _T(" debonded)");
            }
            row++;

            (*pTable)(row,0) << LABEL_HARP_TYPE(harpedAreStraight) << _T(" Strand P") << Sub(_T("jack"));
            (*pTable)(row,1) << force.SetValue(pStrands->GetPjack(pgsTypes::Harped));
            row++;

            (*pTable)(row,0) << _T("Total Number of Permanent Strands");
            (*pTable)(row,1) << pStrand->GetStrandCount(thisSegmentKey,pgsTypes::Straight) + pStrand->GetStrandCount(thisSegmentKey,pgsTypes::Harped);
            row++;

            if ( 0 < pStrand->GetMaxStrands(thisSegmentKey,pgsTypes::Temporary) )
            {
               (*pTable)(row,0) << _T("Number of Temporary Strands");
               (*pTable)(row,1) << pStrand->GetStrandCount(thisSegmentKey,pgsTypes::Temporary);
               nDebonded = pStrand->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Temporary,pgsTypes::dbetEither);
               if ( nDebonded != 0 )
               {
                  (*pTable)(row,1) << _T(" (") << nDebonded << _T(" debonded)");
               }
               row++;

               (*pTable)(row,0) << _T("Temporary Strand P") << Sub(_T("jack"));
               (*pTable)(row,1) << force.SetValue(pStrands->GetPjack(pgsTypes::Temporary));
               row++;
            }

            GET_IFACE2(pBroker,IGirder,pGirder);
            bool bSymmetric = pGirder->IsSymmetricSegment(thisSegmentKey);

            GET_IFACE2(pBroker,IPointOfInterest,pPoi);
            PoiList vPoi;
            int nHarpedAdjustments;
            if ( harpedAreStraight )
            {
               if ( bSymmetric )
               {
                  nHarpedAdjustments = 1;
                  pPoi->GetPointsOfInterest(thisSegmentKey, POI_START_FACE,&vPoi);
                  ATLASSERT(vPoi.size() == 1);
               }
               else
               {
                  nHarpedAdjustments = 2;
                  pPoi->GetPointsOfInterest(thisSegmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
                  ATLASSERT(vPoi.size() == 2);
               }
            }
            else
            {
               if ( bSymmetric )
               {
                  nHarpedAdjustments = 2;
                  pPoi->GetPointsOfInterest(thisSegmentKey,POI_END_FACE,&vPoi);
                  ATLASSERT(vPoi.size() == 1);
                  PoiList vHPPoi;
                  pPoi->GetPointsOfInterest(thisSegmentKey, POI_HARPINGPOINT,&vHPPoi);
                  if ( vHPPoi.size() != 0 )
                  {
                     vPoi.push_back(vHPPoi.front());
                  }
               }
               else
               {
                  nHarpedAdjustments = 4;
                  pPoi->GetPointsOfInterest(thisSegmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
                  ATLASSERT(vPoi.size() == 2);
                  PoiList vHpPoi;
                  pPoi->GetPointsOfInterest(thisSegmentKey, POI_HARPINGPOINT, &vHpPoi);
                  pPoi->MergePoiLists(vPoi, vHpPoi,&vPoi);
               }
            }

            for ( int i = 0; i < nHarpedAdjustments; i++ )
            {
               std::_tstring endoff;
               if ( harpedAreStraight )
               {
                  std::_tstring side;
                  if ( bSymmetric )
                  {
                     side = _T("");
                  }
                  else
                  {
                     side = (i == 0 ? _T("left ") : _T("right "));
                  }

                  if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoLEGACY)
                  {    // Method used pre-version 6.0
                     endoff = _T("Distance from top-most location in harped strand grid to top-most harped strand at ") + side + _T("end of girder");
                  }
                  else if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoCGFROMTOP)
                  {
                     endoff = _T("Distance from top of girder to CG of harped strands at ") + side + _T("end of girder");
                  }
                  else if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoCGFROMBOTTOM)
                  {
                     endoff = _T("Distance from bottom of girder to CG of harped strands at ") + side + _T("end of girder");
                  }
                  else if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoTOP2TOP)
                  {
                     endoff = _T("Distance from top of girder to top-most harped strand at ") + side + _T("end of girder");
                  }
                  else if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoTOP2BOTTOM)
                  {
                     endoff = _T("Distance from bottom of girder to top-most harped strand at ") + side + _T("end of girder");
                  }
                  else if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoBOTTOM2BOTTOM)
                  {
                     endoff = _T("Distance from bottom of girder to lowest harped strand at ") + side + _T("end of girder");
                  }
                  else if( pStrands->GetHarpStrandOffsetMeasurementAtEnd()==hsoECCENTRICITY)
                  {
                     endoff = _T("Eccentricity of harped strand group at ") + side + _T("end of girder");
                  }
                  else
                  {
                     ATLASSERT(false);
                  }

                  (*pTable)(row,0) << endoff;
                  if ( bSymmetric )
                  {
                     (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                  }
                  else
                  {
                     if ( i == 0 )
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                     }
                     else
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metEnd));
                     }
                  }
                  row++;
               }
               else
               {
                  std::_tstring location;
                  if ( bSymmetric )
                  {
                     location = (i == 0 ? _T("ends of girder") : _T("harp point"));
                  }
                  else
                  {
                     if ( i == 0 )
                     {
                        location = _T("left end of girder");
                     }
                     else if ( i == 1 )
                     {
                        location = _T("left harp point");
                     }
                     else if ( i == 2 )
                     {
                        location = _T("right harp point");
                     }
                     else
                     {
                        location = _T("right end of girder");
                     }
                  }

                  if (i == 1 || i == 2)
                  {
                     if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoLEGACY)
                     {    // Method used pre-version 6.0
                        endoff = _T("Distance from top-most location in harped strand grid to top-most harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoCGFROMTOP)
                     {
                        endoff = _T("Distance from top of girder to CG of harped strands at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoCGFROMBOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to CG of harped strands at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoTOP2TOP)
                     {
                        endoff = _T("Distance from top of girder to top-most harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoTOP2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to top-most harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoBOTTOM2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to lowest harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoECCENTRICITY)
                     {
                        endoff = _T("Eccentricity of harped strand group at ") + location;
                     }
                     else
                     {
                        ATLASSERT(false);
                     }
                  }
                  else
                  {
                     if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoLEGACY)
                     {    // Method used pre-version 6.0
                        endoff = _T("Distance from top-most location in harped strand grid to top-most harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoCGFROMTOP)
                     {
                        endoff = _T("Distance from top of girder to CG of harped strands at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoCGFROMBOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to CG of harped strands at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoTOP2TOP)
                     {
                        endoff = _T("Distance from top of girder to top-most harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoTOP2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to top-most harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoBOTTOM2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to lowest harped strand at ") + location;
                     }
                     else if (pStrands->GetHarpStrandOffsetMeasurementAtEnd() == hsoECCENTRICITY)
                     {
                        endoff = _T("Eccentricity of harped strand group at ") + location;
                     }
                     else
                     {
                        ATLASSERT(false);
                     }
                  }

                  (*pTable)(row,0) << endoff;
                  if ( bSymmetric )
                  {
                     if ( i == 0 )
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                     }
                     else
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart));
                     }
                  }
                  else
                  {
                     if ( i == 0 )
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                     }
                     else if ( i == 1 )
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart));
                     }
                     else if ( i == 2 )
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd));
                     }
                     else
                     {
                        (*pTable)(row,1) << cmpdim.SetValue(pStrands->GetHarpStrandOffsetAtEnd(pgsTypes::metEnd));
                     }
                  }
                  row++;
               }
            }
            (*pTable)(row,0) << _T("Specified Release Strength ") << RPT_FCI;
            (*pTable)(row,1) << stress.SetValue( pSegmentData->GetSegmentMaterial(thisSegmentKey)->Concrete.Fci );
            row++;

            (*pTable)(row,0) << _T("Specified 28 day Strength ") << RPT_FC;
            (*pTable)(row,1) << stress.SetValue( pSegmentData->GetSegmentMaterial(thisSegmentKey)->Concrete.Fc );
            row++;

            const matPsStrand* pstrand = pSegmentData->GetStrandMaterial(thisSegmentKey,pgsTypes::Straight);
            ATLASSERT(pstrand!=0);

            (*pTable)(row,0) << _T("Prestressing Strand (Permanent)");
            if ( IS_SI_UNITS(pDisplayUnits) )
            {
               (*pTable)(row,1) << dia.SetValue(pstrand->GetNominalDiameter()) << _T(" Dia.");
               std::_tstring strData;

               strData += _T(" ");
               strData += (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("Grade 1725") : _T("Grade 1860"));
               strData += _T(" ");
               strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));

               (*pTable)(row,1) << strData;
            }
            else
            {
               Float64 diam = pstrand->GetNominalDiameter();

               // special designator for 1/2" special (as per High concrete)
               if (IsEqual(diam,0.013208))
               {
                  (*pTable)(row,1) << _T(" 1/2\" Special, ");
               }

               (*pTable)(row,1) << dia.SetValue(diam) << _T(" Dia.");
               std::_tstring strData;

               strData += _T(" ");
               strData += (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("Grade 250") : _T("Grade 270"));
               strData += _T(" ");
               strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));

               (*pTable)(row,1) << strData;
            }
            row++;


            if ( 0 < pStrand->GetMaxStrands(thisSegmentKey,pgsTypes::Temporary) )
            {
               const matPsStrand* pstrand = pSegmentData->GetStrandMaterial(thisSegmentKey,pgsTypes::Temporary);
               ATLASSERT(pstrand!=0);

               (*pTable)(row,0) << _T("Prestressing Strand (Temporary)");
               if ( IS_SI_UNITS(pDisplayUnits) )
               {
                  (*pTable)(row,1) << dia.SetValue(pstrand->GetNominalDiameter()) << _T(" Dia.");
                  std::_tstring strData;

                  strData += _T(" ");
                  strData += (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("Grade 1725") : _T("Grade 1860"));
                  strData += _T(" ");
                  strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));

                  (*pTable)(row,1) << strData;
               }
               else
               {
                  Float64 diam = pstrand->GetNominalDiameter();

                  // special designator for 1/2" special (as per High concrete)
                  if (IsEqual(diam,0.013208))
                  {
                     (*pTable)(row,1) << _T(" 1/2\" Special, ");
                  }

                  (*pTable)(row,1) << dia.SetValue(diam) << _T(" Dia.");
                  std::_tstring strData;

                  strData += _T(" ");
                  strData += (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("Grade 250") : _T("Grade 270"));
                  strData += _T(" ");
                  strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));

                  (*pTable)(row,1) << strData;
               }
               row++;
            }

            switch ( pStrands->GetTemporaryStrandUsage() )
            {
            case pgsTypes::ttsPTBeforeLifting:
               *pPara << _T("NOTE: Temporary strands post-tensioned immediately before lifting") << rptNewLine;
               break;

            case pgsTypes::ttsPTAfterLifting:
               *pPara << _T("NOTE: Temporary strands post-tensioned immediately after lifting") << rptNewLine;
               break;

            case pgsTypes::ttsPTBeforeShipping:
               *pPara << _T("NOTE: Temporary strands post-tensioned immediately before shipping") << rptNewLine;
               break;
            }
         } // segIdx
      } // gdrIdx
   } // spanIdx
}

void write_segment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,GroupIndexType grpIdx,GirderIndexType gdrIdx,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdim,    pDisplayUnits->GetXSectionDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,   pDisplayUnits->GetGeneralForceUnit(), true );

   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, IBridge,           pBridge);

   rptParagraph* pPara;
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << _T("Spliced Girder") << rptNewLine;



   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderKey girderKey(grpIdx,gdrIdx);

      pPara = new rptParagraph;
      *pChapter << pPara;
      (*pPara) << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
      (*pPara) << _T("Layout Length: ") << xdim.SetValue(pBridge->GetGirderLayoutLength(girderKey)) << rptNewLine;
      (*pPara) << _T("Span Length: ") << xdim.SetValue(pBridge->GetGirderSpanLength(girderKey)) << rptNewLine;

      const CSplicedGirderData* pGirderData = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx);

      SegmentIndexType nSegments = pGirderData->GetSegmentCount();

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

         const CPrecastSegmentData* pSegment = pGirderData->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         // Setup the table
         std::_tostringstream os;
         os << _T("Girder ") << LABEL_GIRDER(segmentKey.girderIndex) << _T(" Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) <<std::endl;
         rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,os.str().c_str());
         pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
         pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
         *pPara << pTable << rptNewLine;


         RowIndexType row = 0;

         (*pTable)(row,0) << _T("Girder Type");
         (*pTable)(row,1) << pBridgeDesc->GetGirderName();
         row++;

         EventIndexType constructionEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(segID);
         EventIndexType erectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(segID);

         const CTimelineEvent* pConstructionEvent = pTimelineMgr->GetEventByIndex(constructionEventIdx);
         const CTimelineEvent* pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);

         (*pTable)(row,0) << _T("Constuction Event");
         (*pTable)(row,1) << _T("Event ") << LABEL_EVENT(constructionEventIdx) << _T(": ") << pConstructionEvent->GetDescription();
         row++;

         (*pTable)(row,0) << _T("Erection Event");
         (*pTable)(row,1) << _T("Event ") << LABEL_EVENT(erectionEventIdx) << _T(": ") << pErectionEvent->GetDescription();
         row++;

         (*pTable)(row,0) << _T("Profile Variation Type");
         (*pTable)(row,1) << CPrecastSegmentData::GetSegmentVariation( pSegment->GetVariationType() );
         row++;

         switch ( pSegment->GetVariationType() )
         {
            case pgsTypes::svtNone:
               (*pTable)(row,0) << _T("None");
               (*pTable)(row,1) << _T("-");
               row++;
               break;

            case pgsTypes::svtLinear:
            case pgsTypes::svtParabolic:
               (*pTable)(row,0) << _T("Left Prismatic Length");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Left Prismatic Height");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Left Prismatic Bottom Flange Depth");
               (*pTable)(row,1) << cmpdim.SetValue( pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Right Prismatic Length");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationLength(pgsTypes::sztRightPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Right Prismatic Height");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationHeight(pgsTypes::sztRightPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Right Prismatic Bottom Flange Depth");
               (*pTable)(row,1) << cmpdim.SetValue( pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic) );
               row++;
               break;

            case pgsTypes::svtDoubleLinear:
            case pgsTypes::svtDoubleParabolic:
               (*pTable)(row,0) << _T("Left Prismatic Length");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Left Prismatic Height");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Left Prismatic Bottom Flange Depth");
               (*pTable)(row,1) << cmpdim.SetValue( pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Left Tapered Length");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationLength(pgsTypes::sztLeftTapered));
               row++;

               (*pTable)(row,0) << _T("Left Tapered Height");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationHeight(pgsTypes::sztLeftTapered));
               row++;

               (*pTable)(row,0) << _T("Left Tapered Bottom Flange Depth");
               (*pTable)(row,1) << cmpdim.SetValue( pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftTapered));
               row++;

               (*pTable)(row,0) << _T("Right Tapered Length");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationLength(pgsTypes::sztRightTapered));
               row++;

               (*pTable)(row,0) << _T("Right Tapered Height");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationHeight(pgsTypes::sztRightTapered));
               row++;

               (*pTable)(row,0) << _T("Right Tapered Bottom Flange Depth");
               (*pTable)(row,1) << cmpdim.SetValue( pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightTapered));
               row++;

               (*pTable)(row,0) << _T("Right Prismatic Length");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationLength(pgsTypes::sztRightPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Right Prismatic Height");
               (*pTable)(row,1) << xdim.SetValue( pSegment->GetVariationHeight(pgsTypes::sztRightPrismatic) );
               row++;

               (*pTable)(row,0) << _T("Right Prismatic Bottom Flange Depth");
               (*pTable)(row,1) << cmpdim.SetValue( pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic) );
               row++;
               break;


            //case CPrecastSegmentData::General:
            default:
               ATLASSERT(false); // not implemented yet
               break;
         }

         (*pTable)(row,0) << _T("Layout Length");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentLayoutLength(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Segment Length");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentLength(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Span Length");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentSpanLength(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Plan Length");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentPlanLength(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Start Bearing Offset");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentStartBearingOffset(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("Start End Distance");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentStartEndDistance(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("End End Distance");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentEndEndDistance(segmentKey) );
         row++;

         (*pTable)(row,0) << _T("End Bearing Offset");
         (*pTable)(row,1) << xdim.SetValue( pBridge->GetSegmentEndBearingOffset(segmentKey) );
         row++;

         if ( segIdx < nSegments-1 )
         {
            CollectionIndexType closureIdx = segIdx;

            std::_tostringstream os;
            os << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Closure Joint ") << LABEL_SEGMENT(closureIdx) <<std::endl;
            rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,os.str().c_str());
            pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
            pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
            pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
            *pPara << pTable << rptNewLine;

            RowIndexType row = 0;

            (*pTable)(row,0) << _T("Length");
            (*pTable)(row,1) << xdim.SetValue(pBridge->GetClosureJointLength(segmentKey));
            row++;

            SegmentIDType closureID = pIBridgeDesc->GetSegmentID(segmentKey);

            EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(closureID);
            const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

            (*pTable)(row,0) << _T("Closure Event");
            (*pTable)(row,1) << _T("Event ") << LABEL_EVENT(eventIdx) << _T(": ") << pTimelineEvent->GetDescription();
            row++;
         }
      } // segIdx
   } // grpIdx
}

void write_slab_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   rptParagraph* pPara1 = new rptParagraph;
   pPara1->SetStyleName(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara1;

   *pPara1 << _T("Deck Geometry");


   rptParagraph* pPara2 = new rptParagraph;
   *pChapter << pPara2;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, overhang, pDisplayUnits->GetXSectionDimUnit(),   true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, olay,     pDisplayUnits->GetOverlayWeightUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),        true );

   rptRcTable* table = rptStyleManager::CreateTableNoHeading(1);
   table->EnableRowStriping(false);
   *pPara2 << table;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   
   pgsTypes::SupportedDeckType deckType = pDeck->GetDeckType();

   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetGirderGroup((GroupIndexType)0)->GetGirder(0)->GetGirderLibraryEntry();
   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);
   std::_tstring strPicture = pFactory->GetSlabDimensionsImage(deckType);

   // Slab Types
   (*table)(0, 0) << Bold(_T("Deck Type")) << rptNewLine << GetDeckTypeName(deckType) << rptNewLine;

   // Slab Dimensions
   if ( deckType != pgsTypes::sdtNone )
   {
      (*table)(0,0) << Bold(_T("Cross Section")) << rptNewLine;
      if ( deckType == pgsTypes::sdtCompositeCIP )
      {
         (*table)(0,0) << _T("Gross Depth") << _T(" = ") << dim.SetValue(pDeck->GrossDepth) << rptNewLine;
      }
      else if  ( deckType == pgsTypes::sdtCompositeSIP )
      {
         (*table)(0,0) << _T("Cast Depth") << _T(" = ") << dim.SetValue(pDeck->GrossDepth) << rptNewLine;
         (*table)(0,0) << _T("Panel Depth") << _T(" = ") << dim.SetValue(pDeck->PanelDepth) << rptNewLine;
         (*table)(0,0) << _T("Panel Support") << _T(" = ") << dim.SetValue(pDeck->PanelSupport) << rptNewLine;
      }
      else if ( IsOverlayDeck(deckType) )
      {
         (*table)(0,0) << _T("Overlay Deck") << rptNewLine;
         (*table)(0,0) << _T("Gross Depth") << _T(" = ") << dim.SetValue(pDeck->GrossDepth) << rptNewLine;
      }

      if ( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP )
      {
         (*table)(0,0) << _T("Overhang Edge Depth") << _T(" = ") << dim.SetValue(pDeck->OverhangEdgeDepth) << rptNewLine;
         (*table)(0,0) << _T("Overhang Taper") << _T(" = ");
         switch(pDeck->OverhangTaper)
         {
         case pgsTypes::dotNone:
            (*table)(0,0) << _T("None") << rptNewLine;
            break;

         case pgsTypes::dotTopTopFlange:
            (*table)(0,0) << _T("Taper overhang to top of girder top flange") << rptNewLine;
            break;

         case pgsTypes::dotBottomTopFlange:
            (*table)(0,0) << _T("Taper overhang to bottom of girder top flange") << rptNewLine;
            break;
         }
      }

      if (!IsOverlayDeck(deckType))
      {
         (*table)(0, 0) << _T("Fillet = ") << dim.SetValue(pBridgeDesc->GetFillet()) << rptNewLine;
      }

      if ( pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
      {
         (*table)(0,0) << _T("Slab Offset (\"A\" Dimension) = ") << dim.SetValue(pBridgeDesc->GetSlabOffset()) << rptNewLine;
      }
      else
      {
         (*table)(0,0) << _T("Slab offset by girder") << rptNewLine;
      }
      (*table)(0,0) << rptNewLine;
   }

   // Plan (Edge of Deck)
   if ( IsAdjustableWidthDeck(pDeck->GetDeckType()) )
   {
      (*table)(0,0) << Bold(_T("Plan (Edge of Deck)")) << rptNewLine;

      rptRcTable* deckTable = rptStyleManager::CreateDefaultTable(4,_T(""));
      (*deckTable)(0,0) << _T("Station");
      (*deckTable)(0,1) << _T("Measured") << rptNewLine << _T("From");
      (*deckTable)(0,2) << COLHDR(_T("Left Offset"), rptLengthUnitTag, pDisplayUnits->GetXSectionDimUnit() );
      (*deckTable)(0,3) << COLHDR(_T("Right Offset"), rptLengthUnitTag, pDisplayUnits->GetXSectionDimUnit() );

      overhang.ShowUnitTag(false);
      (*table)(0,0) << deckTable << rptNewLine;
      std::vector<CDeckPoint>::const_iterator iter;
      RowIndexType row = deckTable->GetNumberOfHeaderRows();
      for ( iter = pDeck->DeckEdgePoints.begin(); iter != pDeck->DeckEdgePoints.end(); iter++ )
      {
         const CDeckPoint& dp = *iter;
         (*deckTable)(row,0) << rptRcStation(dp.Station, &pDisplayUnits->GetStationFormat());

         if ( dp.MeasurementType == pgsTypes::omtAlignment )
         {
            (*deckTable)(row,1) << _T("Alignment");
         }
         else
         {
            (*deckTable)(row,1) << _T("Bridge Line");
         }

         (*deckTable)(row,2) << overhang.SetValue(dp.LeftEdge);
         (*deckTable)(row,3) << overhang.SetValue(dp.RightEdge);

         row++;

         if ( iter != pDeck->DeckEdgePoints.end()-1 )
         {
            deckTable->SetColumnSpan(row,0,2);
            deckTable->SetColumnSpan(row,1,SKIP_CELL);
            (*deckTable)(row,0) << _T("Transition");

            switch( dp.LeftTransitionType )
            {
               case pgsTypes::dptLinear:
                  (*deckTable)(row,2) << _T("Linear");
                  break;

               case pgsTypes::dptSpline:
                  (*deckTable)(row,2) << _T("Spline");
                  break;

               case pgsTypes::dptParallel:
                  (*deckTable)(row,2) << _T("Parallel");
                  break;
            }

            switch( dp.RightTransitionType )
            {
               case pgsTypes::dptLinear:
                  (*deckTable)(row,3) << _T("Linear");
                  break;

               case pgsTypes::dptSpline:
                  (*deckTable)(row,3) << _T("Spline");
                  break;

               case pgsTypes::dptParallel:
                  (*deckTable)(row,3) << _T("Parallel");
                  break;
            }
         }

         row++;
      }
   } // end if

   // Roadway Surfacing
   (*table)(0,0) << Bold(_T("Wearing Surface")) << rptNewLine;
   switch( pDeck->WearingSurface )
   {
   case pgsTypes::wstSacrificialDepth:
      (*table)(0,0) << _T("Wearing Surface: Sacrificial Depth of Concrete Slab") << rptNewLine;
      break;
   case pgsTypes::wstOverlay:
      (*table)(0,0) << _T("Wearing Surface: Overlay") << rptNewLine;
      break;
   case pgsTypes::wstFutureOverlay:
      (*table)(0,0) << _T("Wearing Surface: Future Overlay") << rptNewLine;
      break;
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   }

   if ( pDeck->WearingSurface == pgsTypes::wstOverlay || pDeck->WearingSurface == pgsTypes::wstFutureOverlay )
   {
      (*table)(0,0) << _T("Overlay Weight") << _T(" = ") << olay.SetValue(pDeck->OverlayWeight) << rptNewLine;
   }

   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth || pDeck->WearingSurface == pgsTypes::wstFutureOverlay )
   {
      if ( deckType != pgsTypes::sdtNone )
      {
         (*table)(0,0) << _T("Sacrificial Depth") << _T(" = ") << dim.SetValue(pDeck->SacrificialDepth) << rptNewLine;
      }
   }

   (*table)(0,0) << rptNewLine;

   // Picture
   (*table)(1,0) << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strPicture );
}

void write_deck_reinforcing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   if (IsNonstructuralDeck(pDeck->GetDeckType()))
   {
      return; // if there is no deck, there is no deck reinforcement
   }

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cover, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spacing, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cutoff, pDisplayUnits->GetXSectionDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaPerLengthValue, As, pDisplayUnits->GetAvOverSUnit(), false );

   const CDeckRebarData& deckRebar = pDeck->DeckRebarData;

   rptParagraph* pPara;
   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   *pPara << _T("Deck Reinforcement");

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Reinforcement: ") << lrfdRebarPool::GetMaterialName(deckRebar.TopRebarType,deckRebar.TopRebarGrade).c_str() << rptNewLine;
   *pPara << _T("Top Mat Cover: ") << cover.SetValue(deckRebar.TopCover) << rptNewLine;
   *pPara << _T("Bottom Mat Cover: ") << cover.SetValue(deckRebar.BottomCover) << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Primary Reinforcement - Longitudinal reinforcement running the full length of the bridge") << rptNewLine;
   
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4,_T(""));
   *pPara << pTable << rptNewLine;

   (*pTable)(0,0) << _T("Mat");
   (*pTable)(0,1) << _T("Bars");
   (*pTable)(0,2) << _T("");
   (*pTable)(0,3) << COLHDR(_T("Lump Sum"), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   (*pTable)(1,0) << _T("Top");
   (*pTable)(1,1) << lrfdRebarPool::GetBarSize(deckRebar.TopRebarSize) << _T(" @ ") << spacing.SetValue( deckRebar.TopSpacing );

   (*pTable)(1,2) << _T("AND");
   (*pTable)(1,3) << As.SetValue( deckRebar.TopLumpSum );

   (*pTable)(2,0) << _T("Bottom");
   (*pTable)(2,1) << lrfdRebarPool::GetBarSize(deckRebar.BottomRebarSize) << _T(" @ ") << spacing.SetValue( deckRebar.BottomSpacing );

   (*pTable)(2,2) << _T("AND");
   (*pTable)(2,3) << As.SetValue( deckRebar.BottomLumpSum );

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Supplemental Reinforcement - Negative moment reinforcement at continuous piers") << rptNewLine;

   if ( deckRebar.NegMomentRebar.size() == 0 )
   {
      *pPara << rptNewLine;
      *pPara << _T("No supplemental reinforcement");
      *pPara << rptNewLine;
   }
   else
   {
      pTable = rptStyleManager::CreateDefaultTable(7,_T(""));
      *pPara << pTable << rptNewLine;
      (*pTable)(0,0) << _T("Pier");
      (*pTable)(0,1) << _T("Mat");
      (*pTable)(0,2) << COLHDR(Sub2(_T("A"),_T("s")), rptAreaPerLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
      (*pTable)(0,3) << _T("Bar");
      (*pTable)(0,4) << COLHDR(_T("Spacing"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,5) << COLHDR(_T("Left") << rptNewLine << _T("Cutoff"),rptLengthUnitTag, pDisplayUnits->GetXSectionDimUnit() );
      (*pTable)(0,6) << COLHDR(_T("Right") << rptNewLine << _T("Cutoff"),rptLengthUnitTag, pDisplayUnits->GetXSectionDimUnit() );

      RowIndexType row = pTable->GetNumberOfHeaderRows();
      std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter;
      for ( iter = deckRebar.NegMomentRebar.begin(); iter != deckRebar.NegMomentRebar.end(); iter++ )
      {
         const CDeckRebarData::NegMomentRebarData& negMomentRebar = *iter;
         (*pTable)(row,0) << LABEL_PIER(negMomentRebar.PierIdx);
         (*pTable)(row,1) << (negMomentRebar.Mat == CDeckRebarData::TopMat ? _T("Top") : _T("Bottom"));
         (*pTable)(row,2) << As.SetValue(negMomentRebar.LumpSum);
         (*pTable)(row,3) << lrfdRebarPool::GetBarSize(negMomentRebar.RebarSize);
         (*pTable)(row,4) << spacing.SetValue(negMomentRebar.Spacing);
         (*pTable)(row,5) << cutoff.SetValue(negMomentRebar.LeftCutoff);
         (*pTable)(row,6) << cutoff.SetValue(negMomentRebar.RightCutoff);

         row++;
      }
   }
}