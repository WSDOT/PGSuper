///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation 
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
#include <PgsExt\Helpers.h>

#include <Materials/Materials.h>
#include <LRFD\LRFD.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline CString GetHaunchIncrementString(IndexType inc,IndexType numVals)
{
   CString strLabel;
   if (numVals > 1)
   {
      strLabel.Format(_T("%.2f"),(float)(inc) / (numVals - 1));
      if (strLabel.GetAt(3) == '0') // trim excess zeroes
      {
         strLabel.Truncate(3);
      }
   }
   else
   {
      strLabel = _T("Uniform Depth");
   }

   return strLabel;
}


static void write_alignment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_profile_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_crown_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_bridge_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_connection_abbrevation_footnotes(rptChapter* pChapter);
static void write_tempsupport_connection_abbrevation_footnotes(rptChapter* pChapter);
static void write_pier_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_ts_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_framing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_span_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_girder_spacing(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,const CGirderSpacing2* pGirderSpacing,RowIndexType row,ColumnIndexType col);
static void write_bearing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const std::vector<CGirderKey>& girderKeys);
static void write_ps_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const std::vector<CGirderKey>& girderKeys);
static void write_pt_data(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter, Uint16 level, const std::vector<CGirderKey>& girderKeys);
static void write_segment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,GroupIndexType grpIdx,GirderIndexType gdrIdx,Uint16 level);
static void write_slab_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_haunch_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level);
static void write_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level);
static void write_lrfd_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level);
static void write_lrfd_concrete_row(IEAFDisplayUnits* pDisplayUnits, rptRcTable* pTable, Float64 fci, Float64 fc, Float64 Eci, Float64 Ec, bool bHas90dayStrengthColumns, Float64 lambda, const CConcreteMaterial& concrete, RowIndexType row);
static void write_lrfd_concrete_row(IEAFDisplayUnits* pDisplayUnits, rptRcTable* pTable, Float64 fci, Float64 fc, Float64 Eci, Float64 Ec, bool bHas90dayStrengthColumns, bool bUse90dayStrength, Float64 fc90, Float64 Ec90, Float64 lambda, const CConcreteMaterial& concrete, RowIndexType row);
static void write_aci209_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level,bool bAASHTOParameters);
static void write_aci209_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row,bool bAASHTOParameters,Float64 lambda);
static void write_cebfip_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level);
static void write_cebfip_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row);
static void write_uhpc_concrete_row(IEAFDisplayUnits* pDisplayUnits, std::unique_ptr<rptRcTable>& pUHPCTable, const CConcreteMaterial& concrete, RowIndexType row);
static void write_deck_reinforcing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_friction_loss_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);

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

rptChapter* CBridgeDescChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);
   auto pMultiGirderRptSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   std::vector<CGirderKey> girderKeys;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKeys.push_back(pGdrRptSpec->GetGirderKey());
   }
   else if ( pGdrLineRptSpec)
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridge,pBridge);

      CGirderKey girderKey = pGdrLineRptSpec->GetGirderKey();
      pBridge->GetGirderline(girderKey, &girderKeys);
   }
   else if ( pMultiGirderRptSpec)
   {
      pMultiGirderRptSpec->GetBroker(&pBroker);
      girderKeys = pMultiGirderRptSpec->GetGirderKeys();
   }
   else
   {
      ATLASSERT(false); // not expecting a different kind of report spec
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, IBridge, pBridge);
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   //write_alignment_data( pBroker, pDisplayUnits, pChapter, level); // now written as its own chapter
   //write_profile_data( pBroker, pDisplayUnits, pChapter, level); // now written as its own chapter
   //write_crown_data( pBroker, pDisplayUnits, pChapter, level); // now written as its own chapter
   write_bridge_data( pBroker, pDisplayUnits, pChapter, level);
   write_concrete_details(pBroker,pDisplayUnits,pChapter,girderKeys,level);
   write_friction_loss_data( pBroker, pDisplayUnits, pChapter, level);
   write_pier_data( pBroker, pDisplayUnits, pChapter, level);
   if (0 < nTS)
   {
      write_ts_data(pBroker, pDisplayUnits, pChapter, level);
      write_framing_data(pBroker, pDisplayUnits, pChapter, level);
   }
   else
   {
      write_span_data(pBroker, pDisplayUnits, pChapter, level);
   }
   write_bearing_data( pBroker, pDisplayUnits, pChapter, level, girderKeys );
   write_ps_data( pBroker, pDisplayUnits, pChapter, level, girderKeys );
   write_pt_data(pBroker, pDisplayUnits, pChapter, level, girderKeys);

   if (pBridge->GetHaunchInputDepthType() != pgsTypes::hidACamber)
   {
      write_haunch_data(pBroker, pDisplayUnits, pChapter, girderKeys, level);
   }

   write_slab_data( pBroker, pDisplayUnits, pChapter, level );
   write_deck_reinforcing_data( pBroker, pDisplayUnits, pChapter, level );

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CBridgeDescChapterBuilder::Clone() const
{
   return std::make_unique<CBridgeDescChapterBuilder>(m_bSelect);
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

   const AlignmentData2& alignment = pAlignment->GetAlignmentData2();

   *pPara << _T("Name: ") << alignment.Name << rptNewLine;

   CComBSTR bstrBearing;
   direction_formatter->Format(alignment.Direction, CComBSTR("°,\',\""), &bstrBearing);
   *pPara << _T("Direction: ") << RPT_BEARING(OLE2T(bstrBearing)) << rptNewLine;

   *pPara << _T("Ref. Point: ") << rptRcStation(alignment.RefStation, &pDisplayUnits->GetStationFormat())
      << _T(" ")
      << _T("(E (X) ") << length.SetValue(alignment.xRefPoint);
   *pPara << _T(", ")
      << _T("N (Y) ") << length.SetValue(alignment.yRefPoint) << _T(")") << rptNewLine;

   if (alignment.CompoundCurves.size() == 0)
   {
      return;
   }

   bool bHasEntrySpirals = false;
   bool bHasExitSpirals = false;
   bool bHasCircularCurves = false;
   for (const auto& hc : alignment.CompoundCurves)
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

   ColumnIndexType nColumns = alignment.CompoundCurves.size() + 1;
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
   (*pTable)(row++, 0) << Bold(_T("Curve Points (E(x),N(y))"));

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
   auto iter = std::cbegin(alignment.CompoundCurves);
   auto end = std::cend(alignment.CompoundCurves);
   for ( ; iter != end; iter++, col++)
   {
      const auto& hc_data = *iter;
      row = 0;

      (*pTable)(row++, col) << _T("Curve ") << col;

      CComPtr<ICompoundCurve> hc;

      CComPtr<IDirection> bkTangent;
      CComPtr<IDirection> fwdTangent;
      if (IsZero(hc_data.Radius))
      {
         pRoadway->GetBearing(hc_data.PIStation - WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Feet), &bkTangent);
         pRoadway->GetBearing(hc_data.PIStation + WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Feet), &fwdTangent);
      }
      else
      {
         pRoadway->GetCurve(hcIdx, pgsTypes::pcGlobal, &hc);
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
            hc->get_DegreeCurvature(WBFL::Units::ConvertToSysUnits(100.0, WBFL::Units::Measure::Feet), dcHighway, &delta);
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
               hc->get_TS(&pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
            }
         }
         else
         {
            if (bHasEntrySpirals)
            {
               pnt.Release();
               hc->get_TS(&pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);

               pnt.Release();
               hc->get_SPI(spEntry, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);

               pnt.Release();
               hc->get_SC(&pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
            }

            if (bHasCircularCurves)
            {
               (*pTable)(row++, col) << _T("-");
            }
         }
      }

      if (!IsZero(hc_data.Radius))
      {
         pnt.Release();
         hc->get_PI(&pnt);
         pnt->Location(&x, &y);
         (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
      }

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
               hc->get_ST(&pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
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
               hc->get_CS(&pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);

               pnt.Release();
               hc->get_SPI(spExit, &pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);

               pnt.Release();
               hc->get_ST(&pnt);
               pnt->Location(&x, &y);
               (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
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
            hc->get_CCC(&pnt);
            pnt->Location(&x, &y);
            (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
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
            hc->get_CC(&pnt);
            pnt->Location(&x, &y);
            (*pTable)(row, col) << length.SetValue(x) << _T(", "); (*pTable)(row++, col) << length.SetValue(y);
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
   GET_IFACE2(pBroker, IRoadwayData, pRoadwayData ); 
   rptParagraph* pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), true );

   pPara = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pPara;
   *pPara << _T("Profile Details") << rptNewLine;

   const ProfileData2& profile = pRoadwayData->GetProfileData2();

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

   auto begin = std::cbegin(profile.VertCurves);
   auto iter = begin;
   auto end = std::cend(profile.VertCurves);
   for ( ; iter != end; iter++)
   {
      row = 0;

      const auto& vcd(*iter);

      (*pTable)(row++,col) << _T("Curve ") << col;
      if ( IsZero(vcd.L1) && IsZero(vcd.L2) )
      {
         Float64 pgl_offset = 0;
         if (pRoadwayData->GetRoadwaySectionData().AlignmentPointIdx != pRoadwayData->GetRoadwaySectionData().ProfileGradePointIdx)
         {
            IndexType pglIdx = pRoadway->GetProfileGradeLineIndex(vcd.PVIStation);
            pgl_offset = pRoadway->GetAlignmentOffset(pglIdx, vcd.PVIStation);
         }

         Float64 pvi_elevation = pRoadway->GetElevation(vcd.PVIStation,pgl_offset);
         Float64 g1;
         if ( iter == begin )
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
         CComPtr<IVerticalCurve> vc;
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
         CComQIPtr<IProfileElement> element(vc);
         element->GetLength(&Length);

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
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Roadway Cross Sections") << rptNewLine;
   pPara = new rptParagraph;
   *pChapter << pPara;

   const RoadwaySectionData& section = pAlignment->GetRoadwaySectionData();

   if (section.RoadwaySectionTemplates.empty())
   {
      *pPara << _T("No roadway templates are defined. The roadway is flat with no superelevations") << rptNewLine;
   }
   else
   {
      *pPara << _T("Each Roadway Cross Section Template contains ") << section.NumberOfSegmentsPerSection << _T(" segments per section.") << rptNewLine;
      *pPara << _T("The horizontal alignment is coincident with Ridge Point #") << section.AlignmentPointIdx << _T(" between Segments ") << section.AlignmentPointIdx << _T(" and ") << (section.AlignmentPointIdx+1) << _T(".") << rptNewLine;
      *pPara << _T("The profile grade line (PGL) is coincident with Ridge Point #") << section.ProfileGradePointIdx << _T(" between Segments ") << section.ProfileGradePointIdx << _T(" and ") << (section.ProfileGradePointIdx+1) << _T(".") << rptNewLine;
      *pPara << _T("Slopes are measured ") << (section.slopeMeasure == RoadwaySectionData::RelativeToAlignmentPoint ? _T("relative to the alignment point.") : _T("left to right.")) << rptNewLine;

      ColumnIndexType numcols = 2 + section.NumberOfSegmentsPerSection * 2 - 2;

      // Setup the table
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(numcols, _T(""));
      *pPara << pTable << rptNewLine;

      pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      pTable->SetNumberOfHeaderRows(2);

      std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

      ColumnIndexType col = 0;
      pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << rptNewLine << _T("Template");
      pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << rptNewLine << _T("Station");

      (*pTable)(0, col) << _T("Segment 1");
      (*pTable)(1, col++) << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

      for (IndexType ns = 2; ns <= section.NumberOfSegmentsPerSection - 1; ns++)
      {
         pTable->SetColumnSpan(0, col, 2);
         (*pTable)(0, col) << _T("Segment ") << ns;
         (*pTable)(1, col++) << COLHDR(_T("Length"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
         (*pTable)(1, col++) << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      }

      (*pTable)(0, col) << _T("Segment ") << section.NumberOfSegmentsPerSection;
      (*pTable)(1, col) << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");


      INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), false);

      RowIndexType row = pTable->GetNumberOfHeaderRows() + 1;
      IndexType ntempl = 1;
      for (const auto& crown : section.RoadwaySectionTemplates)
      {
         col = 0;
         (*pTable)(row, col++) << ntempl++;
         (*pTable)(row, col++) << rptRcStation(crown.Station, &pDisplayUnits->GetStationFormat());
         (*pTable)(row, col++) << crown.LeftSlope;
         for (const auto& segm : crown.SegmentDataVec)
         {
            (*pTable)(row, col++) << length.SetValue(segm.Length);
            (*pTable)(row, col++) << segm.Slope;
         }

         (*pTable)(row, col++) << crown.RightSlope;

         row++;
      }
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

void write_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level)
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
         write_aci209_concrete_details(pBroker,pDisplayUnits,pChapter,girderKeys,level,true/*AASHTO parameters*/);
         break;

      case TDM_ACI209:
         write_aci209_concrete_details(pBroker,pDisplayUnits,pChapter,girderKeys,level,false/*AASHTO parameters*/);
         break;

      case TDM_CEBFIP:
         write_cebfip_concrete_details(pBroker,pDisplayUnits,pChapter,girderKeys,level);
         break;
      }
   }
   else
   {
      write_lrfd_concrete_details(pBroker,pDisplayUnits,pChapter,girderKeys,level);
   }
}

void write_lrfd_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level)
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

   GET_IFACE2(pBroker,ILibrary, pLib);
   GET_IFACE2(pBroker,ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   bool bUse90dayStrength;
   Float64 factor;
   pSpecEntry->Use90DayStrengthForSlowCuringConcrete(&bUse90dayStrength, &factor);

   if (bUse90dayStrength)
   {
      nColumns += 2;
   }



   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns,_T("Concrete Properties"));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pPara << pTable << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << _T("* = Modulus of elasticity was input") << rptNewLine;
   *pPara << Sub2(_T("w"), _T("c")) << _T(" = unit weight of plain concrete") << rptNewLine;
   *pPara << Sub2(_T("w"), _T("w")) << _T(" = unit weight of concrete with reinforcement") << rptNewLine;
   *pPara << Sub2(_T("D"), _T("agg")) << _T(" = maximum aggregate diameter") << rptNewLine;
   *pPara << RPT_STRESS(_T("ct")) << _T(" = average splitting strength of lightweight concrete") << rptNewLine;
   *pPara << Sub2(_T("K"), _T("1")) << _T(" = averaging factor for course of aggregate") << rptNewLine;
   *pPara << Sub2(_T("K"), _T("2")) << _T(" = bounding factor for course of aggregate") << rptNewLine;
   *pPara << symbol(lambda) << _T(" = concrete density modification factor") << rptNewLine;

   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims)
   {
      // Ec with square root, no K values
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? _T("Ec_2004_SI.png") : _T("Ec_2004_US.png"))) << rptNewLine;
   }
   else if (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() && lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2015Interims)
   {
      // Ec with square root, with K values
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + (lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? _T("Ec_2005_SI.png") : _T("Ec_2005_US.png"))) << rptNewLine;
   }
   else
   {
      // Ec with 0.33 exponent and K values
      ATLASSERT(lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US);
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ec_2016.png")) << rptNewLine;
   }

   GET_IFACE2(pBroker, IMaterials, pMaterials);
   if (pMaterials->HasUHPC())
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ec_PCI_UHPC.png")) << _T(" for PCI-UHPC and AASHTO UHPC GS") << rptNewLine;
   }


   ColumnIndexType col = 0;
   RowIndexType row = 0;
   (*pTable)(row, col++) << _T("Element");
   (*pTable)(row, col++) << _T("Type");
   (*pTable)(row, col++) << COLHDR(RPT_FCI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row, col++) << COLHDR(RPT_ECI, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row, col++) << COLHDR(RPT_FC,  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*pTable)(row, col++) << COLHDR(RPT_EC,  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if (bUse90dayStrength)
   {
      (*pTable)(row, col++) << COLHDR(RPT_FC << Sub(_T("90")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      (*pTable)(row, col++) << COLHDR(RPT_EC << Sub(_T("90")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   }
   (*pTable)(row, col++) << COLHDR(Sub2(_T("w"),_T("c")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*pTable)(row, col++) << COLHDR(Sub2(_T("w"), _T("w")), rptDensityUnitTag, pDisplayUnits->GetDensityUnit());
   (*pTable)(row, col++) << COLHDR(Sub2(_T("D"),_T("agg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(row, col++) << COLHDR(RPT_STRESS(_T("ct")),  rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   if ( bK1 )
   {
      pTable->SetNumberOfHeaderRows(2);
      for ( ColumnIndexType i = 0; i < col; i++ )
      {
         pTable->SetRowSpan(0,i,2); 
      }

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col) << _T("MOE");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col) << _T("Creep");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col) << _T("Shrinkage");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));
   }

   if ( bLambda )
   {
      if ( bK1 )
      {
         pTable->SetRowSpan(0,col,2);
         (*pTable)(0,col++) << symbol(lambda);
      }
      else
      {
         (*pTable)(row,col++) << symbol(lambda);
      }
   }


   row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IIntervals,pIntervals);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   for(const auto& thisGirderKey : girderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         EventIndexType erectEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());

         IntervalIndexType initialIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

         Float64 fci = pMaterials->GetSegmentFc(thisSegmentKey,initialIntervalIdx);
         Float64 fc  = pMaterials->GetSegmentFc28(thisSegmentKey);

         Float64 Eci = pMaterials->GetSegmentEc(thisSegmentKey,initialIntervalIdx);
         Float64 Ec  = pMaterials->GetSegmentEc28(thisSegmentKey);

         Float64 fc90(-99999), Ec90(-99999);
         if (bUse90dayStrength)
         {
            IntervalIndexType finalIntervalIdx = pIntervals->GetIntervalCount() - 1;
            fc90 = pMaterials->GetSegmentFc(thisSegmentKey, finalIntervalIdx);
            Ec90 = pMaterials->GetSegmentEc(thisSegmentKey, finalIntervalIdx);
         }

         Float64 lambda = pMaterials->GetSegmentLambda(thisSegmentKey);

         (*pTable)(row, 0) << pgsGirderLabel::GetGirderLabel(thisGirderKey);

         write_lrfd_concrete_row(pDisplayUnits,pTable,fci,fc,Eci,Ec,bUse90dayStrength,bUse90dayStrength,fc90,Ec90,lambda,pSegment->Material.Concrete,row);
         row++;

         const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
         if ( pClosure )
         {
            ATLASSERT(false); // this should never happen because the basic concrete model
                              // can't be used with PGSplice (PGSuper doesn't use closure joints)
         }
      } // segIdx
   } // girderKey

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->GetDeckType() != pgsTypes::sdtNone )
   {
      Float64 fc = pMaterials->GetDeckFc28();
      Float64 Ec = pMaterials->GetDeckEc28();
      Float64 lambda = pMaterials->GetDeckLambda();

      (*pTable)(row,0) << _T("Deck");
      write_lrfd_concrete_row(pDisplayUnits,pTable,-1.0,fc,-1.0,Ec,bUse90dayStrength,lambda,pDeck->Concrete,row);
      row++;
   }

   if (pBridgeDesc->HasStructuralLongitudinalJoints())
   {
      Float64 fc = pMaterials->GetLongitudinalJointFc28();
      Float64 Ec = pMaterials->GetLongitudinalJointEc28();
      Float64 lambda = pMaterials->GetLongitudinalJointLambda();

      (*pTable)(row, 0) << _T("Longitudinal Joint");
      write_lrfd_concrete_row(pDisplayUnits, pTable, -1.0, fc, -1.0, Ec, bUse90dayStrength, lambda, pBridgeDesc->GetLongitudinalJointMaterial(), row);
      row++;
   }

   // UHPC Concrete Properties
   std::unique_ptr<rptRcTable> pUHPCTable;
   pUHPCTable.reset(rptStyleManager::CreateDefaultTable(9, _T("UHPC Concrete Properties")));
   // don't add to paragraph here. if we don't have UHPC, then this table will just get tossed out


   pUHPCTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pUHPCTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pUHPCTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pUHPCTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));


   col = 0;
   (*pUHPCTable)(0, col++) << _T("Element");
   (*pUHPCTable)(0, col++) << COLHDR(RPT_STRESS(_T("t,cri")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pUHPCTable)(0, col++) << COLHDR(RPT_STRESS(_T("t,cr")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pUHPCTable)(0, col++) << COLHDR(RPT_STRESS(_T("t,loc")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pUHPCTable)(0, col++) << Sub2(symbol(epsilon),_T("t,loc")) << _T(" x 1000");
   (*pUHPCTable)(0, col++) << Sub2(symbol(alpha), _T("u"));
   (*pUHPCTable)(0, col++) << Sub2(symbol(epsilon), _T("cu")) << _T(" x 1000");
   (*pUHPCTable)(0, col++) << Sub2(symbol(gamma), _T("u"));
   (*pUHPCTable)(0, col++) << COLHDR(_T("Fiber Length"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());


   IndexType nUHPC = 0; // keep track of number of UHPC elements reported. Add table to paragraph if 1 or more
   // otherwise table will automatically delete because it is a unique_ptr
   row = pUHPCTable->GetNumberOfHeaderRows();
   for (const auto& thisGirderKey : girderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey thisSegmentKey(thisGirderKey, segIdx);

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         if (pSegment->Material.Concrete.Type == pgsTypes::UHPC)
         {
            nUHPC++;
            (*pUHPCTable)(row, 0) << pgsGirderLabel::GetGirderLabel(thisSegmentKey);

            write_uhpc_concrete_row(pDisplayUnits, pUHPCTable, pSegment->Material.Concrete, row);
            row++;
         }
      } // segIdx
   } // girderKey

   if (0 < nUHPC)
   {
      *pPara << pUHPCTable.release() << rptNewLine;
   }
}

void write_uhpc_concrete_row(IEAFDisplayUnits* pDisplayUnits, std::unique_ptr<rptRcTable>& pUHPCTable, const CConcreteMaterial& concrete, RowIndexType row)
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   ColumnIndexType col = 1;
   (*pUHPCTable)(row, col++) << stress.SetValue(concrete.ftcri);
   (*pUHPCTable)(row, col++) << stress.SetValue(concrete.ftcr);
   (*pUHPCTable)(row, col++) << stress.SetValue(concrete.ftloc);
   (*pUHPCTable)(row, col++) << concrete.etloc * 1000;
   (*pUHPCTable)(row, col++) << concrete.alpha_u;
   if(concrete.bExperimental_ecu)
      (*pUHPCTable)(row, col++) << concrete.ecu * 1000;
   else
      (*pUHPCTable)(row, col++) << _T("");

   (*pUHPCTable)(row, col++) << concrete.gamma_u;
   (*pUHPCTable)(row, col++) << cmpdim.SetValue(concrete.FiberLength);
}

void write_lrfd_concrete_row(IEAFDisplayUnits* pDisplayUnits, rptRcTable* pTable, Float64 fci, Float64 fc, Float64 Eci, Float64 Ec, bool bHas90dayStrengthColumns, Float64 lambda, const CConcreteMaterial& concrete, RowIndexType row)
{
   write_lrfd_concrete_row(pDisplayUnits, pTable, fci, fc, Eci, Ec, bHas90dayStrengthColumns, false, -1, -1, lambda, concrete, row);
}

void write_lrfd_concrete_row(IEAFDisplayUnits* pDisplayUnits, rptRcTable* pTable, Float64 fci, Float64 fc, Float64 Eci, Float64 Ec, bool bHas90dayStrengthColumns, bool bUse90dayStrength, Float64 fc90, Float64 Ec90, Float64 lambda, const CConcreteMaterial& concrete, RowIndexType row)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         false );

   bool bK1 = (lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion());
   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion());

   ColumnIndexType col = 1;

   (*pTable)(row,col++) << lrfdConcreteUtil::GetTypeName( (WBFL::Materials::ConcreteType)concrete.Type, true );
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
      (*pTable)(row,col) << modE.SetValue( Eci );
      if (concrete.bUserEci)
      {
         (*pTable)(row, col) << _T(" *");
      }
      col++;
   }

   (*pTable)(row,col++) << stress.SetValue( fc );
   
   (*pTable)(row,col) << modE.SetValue( Ec );
   if (concrete.bUserEc)
   {
      (*pTable)(row, col) << _T(" *");
   }
   col++;

   if (bHas90dayStrengthColumns)
   {
      if (bUse90dayStrength && concrete.Type == pgsTypes::Normal)
      {
         (*pTable)(row, col++) << stress.SetValue(fc90);
         (*pTable)(row, col++) << modE.SetValue(Ec90);
      }
      else
      {
         (*pTable)(row, col++) << _T("-");
         (*pTable)(row, col++) << _T("-");
      }
   }

   (*pTable)(row, col++) << density.SetValue( concrete.StrengthDensity );
   (*pTable)(row, col++) << density.SetValue( concrete.WeightDensity );

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

void write_aci209_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level,bool bAASHTOParameters)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool bLambda = bAASHTOParameters && (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion());

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(11 + (bAASHTOParameters ? (bLambda ? 7 : 6) : 0),_T("Concrete Properties"));
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
      }

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col)   << Sub2(_T("E"),_T("c"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col)   << _T("Creep");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col)   << _T("Shrinkage");
      (*pTable)(1,col++) << Sub2(_T("K"),_T("1"));
      (*pTable)(1,col++) << Sub2(_T("K"),_T("2"));

      if (bLambda)
      {
         pTable->SetRowSpan(0, col, 2);
         (*pTable)(0, col++) << symbol(lambda);
      }
   }

   row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IMaterials,pMaterials);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   for(const auto& thisGirderKey : girderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         Float64 fc28 = pMaterials->GetSegmentFc28(thisSegmentKey);
         Float64 Ec28 = pMaterials->GetSegmentEc28(thisSegmentKey);

         (*pTable)(row, 0) << pgsGirderLabel::GetGirderLabel(thisGirderKey);

         write_aci209_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pSegment->Material.Concrete,row,bAASHTOParameters,pMaterials->GetSegmentLambda(thisSegmentKey));
         row++;

         const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
         if ( pClosure )
         {
            Float64 fc28 = pMaterials->GetClosureJointFc28(thisSegmentKey);
            Float64 Ec28 = pMaterials->GetClosureJointEc28(thisSegmentKey);

            (*pTable)(row,0) << _T("Closure Joint ") << LABEL_SEGMENT(segIdx);
            write_aci209_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pClosure->GetConcrete(),row,bAASHTOParameters,pMaterials->GetClosureJointLambda(thisSegmentKey));
            row++;
         }
      } // segIdx
   } // gdrIdx

   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->GetDeckType() != pgsTypes::sdtNone )
   {
      Float64 fc28 = pMaterials->GetDeckFc28();
      Float64 Ec28 = pMaterials->GetDeckEc28();

      (*pTable)(row,0) << _T("Deck");
      write_aci209_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pDeck->Concrete,row,bAASHTOParameters,pMaterials->GetDeckLambda());
      row++;
   }

   (*pPara) << Sub2(symbol(gamma),_T("w")) << _T(" =  Unit weight including reinforcement (used for dead load calculations)") << rptNewLine;
   (*pPara) << Sub2(symbol(gamma),_T("s")) << _T(" =  Unit weight (used to compute ") << Sub2(_T("E"),_T("c")) << _T(")") << rptNewLine;
   (*pPara) << Sub2(_T("D"),_T("agg")) << _T(" =  Maximum aggregate size") << rptNewLine;
}

void write_aci209_concrete_row(IEAFDisplayUnits* pDisplayUnits,rptRcTable* pTable,Float64 fc28,Float64 Ec28,const CConcreteMaterial& concrete,RowIndexType row,bool bAASHTOParameters,Float64 lambda)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         false );
   INIT_UV_PROTOTYPE( rptTimeUnitValue,    time,    pDisplayUnits->GetFractionalDaysUnit(),     false );

   bool bLambda = bAASHTOParameters && (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion());

   ColumnIndexType col = 1;
   (*pTable)(row,col++) << lrfdConcreteUtil::GetTypeName( (WBFL::Materials::ConcreteType)concrete.Type, true );
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
      std::_tstring strCement(concrete.ACI209CementType == pgsTypes::TypeI ? _T("Type I") : _T("Type III"));
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

      if (bLambda)
      {
         (*pTable)(row, col++) << lambda;
      }
   }
}

void write_cebfip_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level)
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

   for(const auto& thisGirderKey : girderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         Float64 fc28 = pMaterials->GetSegmentFc28(thisSegmentKey);
         Float64 Ec28 = pMaterials->GetSegmentEc28(thisSegmentKey);

         (*pTable)(row, 0) << pgsGirderLabel::GetGirderLabel(thisGirderKey);

         write_cebfip_concrete_row(pDisplayUnits,pTable,fc28,Ec28,pSegment->Material.Concrete,row);
         row++;

         const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
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
   (*pTable)(row,col++) << lrfdConcreteUtil::GetTypeName( (WBFL::Materials::ConcreteType)concrete.Type, true );
   (*pTable)(row,col++) << stress.SetValue( fc28 );
   (*pTable)(row,col++) << modE.SetValue( Ec28 );

   if ( concrete.bCEBFIPUserParameters )
   {
      (*pTable)(row,col++) << RPT_NA;
   }
   else
   {
      (*pTable)(row,col++) << WBFL::Materials::CEBFIPConcrete::GetCementType((WBFL::Materials::CEBFIPConcrete::CementType)concrete.CEBFIPCementType);
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

void write_friction_loss_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   // Only write for spliced girders
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   if (!pDocType->IsPGSpliceDocument())
   {
      return;
   }

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptPerLengthUnitValue, pldim, pDisplayUnits->GetPerLengthUnit(), false );
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4,_T("Post-Tensioning Anchor Set and Friction Loss Parameters"));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );
   *pPara << pTable << rptNewLine;

   (*pTable)(0,1) << COLHDR(symbol(DELTA) << Sub(_T("set")) << rptNewLine << _T("Anchor Set"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0,2) << COLHDR(Sub(_T("K")) << rptNewLine << _T("Wobble Friction Coefficient"),rptPerLengthUnitTag,pDisplayUnits->GetPerLengthUnit());
   (*pTable)(0,3) << symbol(mu) << rptNewLine <<_T("Coefficient of Friction");
   (*pTable)(1, 0) << _T("Temporary Strands");
   (*pTable)(2, 0) << _T("Tendons");

   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTemporaryStrandPostTensionParameters(&Dset,&wobble,&friction);
   (*pTable)(1, 1) << cmpdim.SetValue(Dset);
   (*pTable)(1, 2) << pldim.SetValue(wobble);
   (*pTable)(1, 3) << scalar.SetValue(friction);

   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);
   (*pTable)(2, 1) << cmpdim.SetValue(Dset);
   (*pTable)(2, 2) << pldim.SetValue(wobble);
   (*pTable)(2, 3) << scalar.SetValue(friction);

   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Time Step Analysis - Time Dependent Effects");

   pPara = new rptParagraph;
   *pChapter << pPara;

   *pPara << _T("Creep Effects are ") << (pLossParams->IgnoreCreepEffects()         ? _T("Ignored") : _T("Considered")) << rptNewLine;
   *pPara << _T("Shrinkage Effects are ") << (pLossParams->IgnoreShrinkageEffects() ? _T("Ignored") : _T("Considered")) << rptNewLine;
   *pPara << _T("Relaxation Effects are ") << (pLossParams->IgnoreRelaxationEffects()         ? _T("ignored") : _T("Considered")) << rptNewLine;
}

void write_connection_abbrevation_footnotes(rptChapter* pChapter)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << _T("Bearing Offset Measure") << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, true, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, true, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, false, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, false, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true, false) << rptNewLine;
   *pPara << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false, true) << _T(" = ") << GetBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false, false) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("End Distance Measure") << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder, true, false) << rptNewLine;
   //*pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder,  false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingAlongGirder,  false, false) << rptNewLine; // produces the same result at the line above
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, false, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, true, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, true, false) << rptNewLine;
   *pPara << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, false, true) << _T(" = ") << GetEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, false, false) << rptNewLine;
}

void write_tempsupport_connection_abbrevation_footnotes(rptChapter* pChapter)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << _T("Bearing Offset Measure") << rptNewLine;
   *pPara << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, true) << _T(" = ") << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::AlongGirder, false) << rptNewLine;
   *pPara << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, true) << _T(" = ") << GetTempSupportBearingOffsetMeasureString(ConnectionLibraryEntry::NormalToPier, false) << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("End Distance Measure") << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromBearingNormalToPier, false) << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierAlongGirder, false) << rptNewLine;
   *pPara << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, true) << _T(" = ") << GetTempSupportEndDistanceMeasureString(ConnectionLibraryEntry::FromPierNormalToPier, false) << rptNewLine;
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
   pLayoutTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pLayoutTable->SetRowSpan(0,0,2);
   (*pLayoutTable)(0,0) << _T("");

   pLayoutTable->SetRowSpan(0,1,2);
   (*pLayoutTable)(0,1) << _T("Station");

   pLayoutTable->SetRowSpan(0,2,2);
   (*pLayoutTable)(0,2) << _T("Bearing");

   pLayoutTable->SetRowSpan(0,3,2);
   (*pLayoutTable)(0,3) << _T("Skew Angle");

   pLayoutTable->SetRowSpan(0,4,2);
   (*pLayoutTable)(0,4) << _T("Boundary Condition");

   // Table for pier diaphragms
   rptRcTable* pDiaphragmTable = rptStyleManager::CreateDefaultTable(9,_T("Pier Diaphragms"));
   *pPara << pDiaphragmTable << rptNewLine;

   pDiaphragmTable->SetNumberOfHeaderRows(2);

   pDiaphragmTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pDiaphragmTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   pDiaphragmTable->SetRowSpan(0,0,2);
   (*pDiaphragmTable)(0,0) << _T("");

   pDiaphragmTable->SetColumnSpan(0,1,4);
   (*pDiaphragmTable)(0,1) << _T("Diaphragm - Back");
   (*pDiaphragmTable)(1,1) << COLHDR(_T("Height"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pDiaphragmTable)(1,2) << COLHDR(_T("Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pDiaphragmTable)(1,3) << _T("Loading");
   (*pDiaphragmTable)(1,4) << COLHDR(_T("Location(*)"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());

   pDiaphragmTable->SetColumnSpan(0,5,4);
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
   (*pConnectionTable)(0,0) << _T("");

   pConnectionTable->SetColumnSpan(0,1,4);
   (*pConnectionTable)(0,1) << _T("Back");
   (*pConnectionTable)(1,1) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,2) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*pConnectionTable)(1,3) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,4) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   pConnectionTable->SetColumnSpan(0,5,4);
   (*pConnectionTable)(0,5) << _T("Ahead");
   (*pConnectionTable)(1,5) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,6) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*pConnectionTable)(1,7) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionTable)(1,8) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");

   write_connection_abbrevation_footnotes(pChapter);

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
      (*pLayoutTable)(row1,0)     <<  LABEL_PIER_EX(bAbutment,pierIdx);
      (*pDiaphragmTable)(row2,0)  <<  LABEL_PIER_EX(bAbutment,pierIdx);
      (*pConnectionTable)(row3,0) <<  LABEL_PIER_EX(bAbutment, pierIdx);

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
      CPierData2::PierConnectionFlags conFlag = pPier->IsConnectionDataAvailable();

      // back side
      if (CPierData2::pcfBothFaces == conFlag || CPierData2::pcfBackOnly == conFlag)
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
         (*pConnectionTable)(row3,1) << RPT_NA;
         (*pConnectionTable)(row3,2) << RPT_NA;
         (*pConnectionTable)(row3,3) << RPT_NA;
         (*pConnectionTable)(row3,4) << RPT_NA;
      }

      // Ahead side
      if (CPierData2::pcfBothFaces == conFlag || CPierData2::pcfAheadOnly == conFlag)
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
         (*pConnectionTable)(row3,5) << RPT_NA;
         (*pConnectionTable)(row3,6) << RPT_NA;
         (*pConnectionTable)(row3,7) << RPT_NA;
         (*pConnectionTable)(row3,8) << RPT_NA;
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

void write_bearing_data(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter, Uint16 level, const std::vector<CGirderKey>& girderKeys)
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

   RowIndexType row = 1;
   for (const auto& girderKey : girderKeys)
   {
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(girderKey.groupIndex, &startPierIdx, &endPierIdx);

      for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
      {
         // face reported depends on where we are in the loop, and if interior pier whether continuous
         Uint32 intStartFace, intEndFace;
         bool isContinous(false);
         if (pierIdx == startPierIdx)
         {
            intStartFace = 0;
            intEndFace = 0;
         }
         else if (pierIdx == endPierIdx)
         {
            intStartFace = 1;
            intEndFace = 1;
         }
         else
         {
            bool bLeft, bRight;
            pBridge->IsContinuousAtPier(pierIdx, &bLeft, &bRight);
            if (bLeft || bRight)
            {
               intStartFace = 0;
               intEndFace = 0;
               isContinous = true;
            }
            else
            {
               intStartFace = 0;
               intEndFace = 1;
            }
         }

         for (Uint32 idx = intStartFace; idx <= intEndFace; idx++)
         {
            ColumnIndexType col = 0;

            pgsTypes::PierFaceType pf = (idx == 0 ? pgsTypes::Ahead : pgsTypes::Back);
            std::_tstring strFace(isContinous ? _T("") : (idx == 0 ? _T(" - Ahead") : _T(" - Back")));

            const CBearingData2* pbd = pIBridgeDesc->GetBearingData(pierIdx, pf, girderKey.girderIndex);

            if (pbd != nullptr)
            {
               bool is_rect = pbd->Shape == bsRectangular;

               (*ptable)(row, col++) << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << _T(" - Pier ") << LABEL_PIER(pierIdx) << strFace;
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
   }
}

void write_ts_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, ITempSupport,  pTemporarySupport ); 

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

   rptRcTable* pLayoutTable = rptStyleManager::CreateDefaultTable(7,_T("Temporary Supports"));
   *pPara << pLayoutTable << rptNewLine;

   pLayoutTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetColumnStyle(5, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(5, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetColumnStyle(6, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pLayoutTable->SetStripeRowColumnStyle(6, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType layout_col = 0;
   (*pLayoutTable)(0, layout_col++) << _T("");
   (*pLayoutTable)(0, layout_col++) << _T("Station");
   (*pLayoutTable)(0, layout_col++) << _T("Bearing");
   (*pLayoutTable)(0, layout_col++) << _T("Skew Angle");
   (*pLayoutTable)(0, layout_col++) << _T("Type");
   (*pLayoutTable)(0, layout_col++) << _T("Erection  Event");
   (*pLayoutTable)(0, layout_col++) << _T("Removal Event");

   // connections
   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pConnectionsTable = rptStyleManager::CreateDefaultTable(7,_T("Temporary Support Connections"));
   *pPara << pConnectionsTable << rptNewLine;
   write_tempsupport_connection_abbrevation_footnotes(pChapter);

   pConnectionsTable->SetNumberOfHeaderRows(1);

   pConnectionsTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pConnectionsTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pConnectionsTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pConnectionsTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pConnectionsTable->SetColumnStyle(6, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pConnectionsTable->SetStripeRowColumnStyle(6, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType connections_col = 0;
   (*pConnectionsTable)(0, connections_col++) << _T("");
   (*pConnectionsTable)(0, connections_col++) << _T("Boundary") << rptNewLine << _T("Condition");
   (*pConnectionsTable)(0, connections_col++) << COLHDR(_T("Bearing") << rptNewLine << _T("Offset"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionsTable)(0, connections_col++) << _T("Bearing") << rptNewLine << _T("Offset") << rptNewLine << _T("Measure");
   (*pConnectionsTable)(0, connections_col++) << COLHDR(_T("End") << rptNewLine << _T("Distance"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pConnectionsTable)(0, connections_col++) << _T("End") << rptNewLine << _T("Distance") << rptNewLine << _T("Measure");
   (*pConnectionsTable)(0, connections_col++) << _T("Cast Closure Event");

   RowIndexType layout_row = pLayoutTable->GetNumberOfHeaderRows();
   RowIndexType connections_row = pConnectionsTable->GetNumberOfHeaderRows();
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      layout_col = 0;
      connections_col = 0;

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

      (*pLayoutTable)(layout_row, layout_col++) << _T("TS ") << LABEL_TEMPORARY_SUPPORT(tsIdx);

      (*pLayoutTable)(layout_row, layout_col++) << rptRcStation(pTS->GetStation(), &pDisplayUnits->GetStationFormat() );
      (*pLayoutTable)(layout_row, layout_col++) << RPT_BEARING(OLE2T(bstrBearing));
      (*pLayoutTable)(layout_row, layout_col++) << RPT_ANGLE(OLE2T(bstrAngle));
      (*pLayoutTable)(layout_row, layout_col++) << CTemporarySupportData::AsString(pTS->GetSupportType());

      EventIndexType erectionEventIdx, removalEventIdx;
      pTimelineMgr->GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);

      const CTimelineEvent* pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);
      const CTimelineEvent* pRemovalEvent  = pTimelineMgr->GetEventByIndex(removalEventIdx);

      (*pLayoutTable)(layout_row, layout_col++) << _T("Event ") << LABEL_EVENT(erectionEventIdx) << _T(": ") << pErectionEvent->GetDescription();
      (*pLayoutTable)(layout_row, layout_col++) << _T("Event ") << LABEL_EVENT(removalEventIdx)  << _T(": ") << pRemovalEvent->GetDescription();

      layout_row++;

      (*pConnectionsTable)(connections_row, connections_col++) << _T("TS ") << LABEL_TEMPORARY_SUPPORT(tsIdx);
      (*pConnectionsTable)(connections_row, connections_col++) << CTemporarySupportData::AsString(pTS->GetConnectionType());

      if ( pTS->GetConnectionType() == pgsTypes::tsctClosureJoint )
      {
         Float64 brgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
         pTS->GetBearingOffset(&brgOffset,&brgOffsetMeasure);
         (*pConnectionsTable)(connections_row, connections_col++) << cmpdim.SetValue(brgOffset);
         (*pConnectionsTable)(connections_row, connections_col++) << GetTempSupportBearingOffsetMeasureString(brgOffsetMeasure,true);

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
         pTS->GetGirderEndDistance(&endDist,&endDistMeasure);
         (*pConnectionsTable)(connections_row, connections_col++) << cmpdim.SetValue(endDist);
         (*pConnectionsTable)(connections_row, connections_col++) << GetTempSupportEndDistanceMeasureString(endDistMeasure,true);

         const CClosureJointData* pClosureJoint = pTS->GetClosureJoint(0);
         IDType cpID = pClosureJoint->GetID();
         EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(cpID);
         const CTimelineEvent* pCastClosureEvent = pTimelineMgr->GetEventByIndex(castClosureEventIdx);
         (*pConnectionsTable)(connections_row, connections_col++) << _T("Event ") << LABEL_EVENT(castClosureEventIdx) << _T(": ") << pCastClosureEvent->GetDescription();
      }
      else
      {
         (*pConnectionsTable)(connections_row, connections_col++) << _T("");
         (*pConnectionsTable)(connections_row, connections_col++) << _T("");
         (*pConnectionsTable)(connections_row, connections_col++) << _T("");
         (*pConnectionsTable)(connections_row, connections_col++) << _T("");
         (*pConnectionsTable)(connections_row, connections_col++) << _T("");
      }

      connections_row++;
   }
}

void write_framing_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   if (pBridgeDesc->GetGirderCount() == 1)
      return;

   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   // Setup the table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3,_T("Framing"));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

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
      (*pTable)(row,0) << LABEL_PIER_EX(pPier->IsAbutment(), pPier->GetIndex());
      write_girder_spacing(pBroker,pDisplayUnits,pTable,pSpacing,row,1);

      row++;

      const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
      while ( pSegment )
      {
         const CPierData2* pPier = nullptr;
         const CTemporarySupportData* pTS = nullptr;
         const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
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

   pTable->SetColumnSpan(0,2,2);
   (*pTable)(0,2) << _T("Start of Span");

   if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      (*pTable)(1,2) << _T("Girder Spacing");
   }
   else
   {
      (*pTable)(1,2) << _T("Joint Spacing");
   }

   (*pTable)(1,3) << _T("Datum");

   pTable->SetColumnSpan(0,4,2);
   (*pTable)(0,4) << _T("End of Span");

   if (IsGirderSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      (*pTable)(1,4) << _T("Girder Spacing");
   }
   else
   {
      (*pTable)(1,4) << _T("Joint Spacing");
   }

   (*pTable)(1,5) << _T("Datum");

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();

   RowIndexType row = 2; // data starts on row to because we have 2 heading rows

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();

      (*pTable)(row,0) << LABEL_SPAN(grpIdx);
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
      *pPara << _T("Girder Spacing Datum:") << rptNewLine;
   }
   else
   {
      *pPara << _T("Joint Spacing Datum:") << rptNewLine;
   }

   *pPara << _T("(1) Measured normal to the alignment at the abutment/pier line") << rptNewLine;
   *pPara << _T("(2) Measured normal to the alignment at the centerline of bearing") << rptNewLine;
   *pPara << _T("(3) Measured at and along the abutment/pier line") << rptNewLine;
   *pPara << _T("(4) Measured at and along the centerline of bearing") << rptNewLine << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   pgsTypes::WorkPointLocation wploc = pBridgeDesc->GetWorkPointLocation();
   *pPara << _T("Girder spacing is measured at the work point elevation, which is at ") << (wploc == pgsTypes::wplBottomGirder ? _T("bottom centerline of girder.") : _T("top centerline of girder.")) << rptNewLine;

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);
   CString strGrp = (bIsSplicedGirder ? _T("Group") : _T("Span"));

   pgsTypes::GirderOrientationType orientType = pBridgeDesc->GetGirderOrientation();
   *pPara << _T("Girder orientation is set ");
   if (pgsTypes::Plumb == orientType)
   {
      *pPara << _T("plumb") << rptNewLine;
   }
   else if (pgsTypes::StartNormal == orientType)
   {
      *pPara << _T("normal to roadway at start of ") << strGrp << rptNewLine;
   }
   else if (pgsTypes::MidspanNormal == orientType)
   {
      *pPara << _T("normal to roadway at middle of ") << strGrp << rptNewLine;
   }
   else if (pgsTypes::EndNormal == orientType)
   {
      *pPara << _T("normal to roadway at end of ") << strGrp << rptNewLine;
   }
   else if (pgsTypes::Balanced == orientType)
   {
      *pPara << _T("balanced to minimize haunch depth at both ends of ") << strGrp << rptNewLine;
   }
   else
   {
      ATLASSERT(0); // new type?
   }
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
      if ( 0 < grpIdx )
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

void write_ps_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const std::vector<CGirderKey>& girderKeys)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdim,    pDisplayUnits->GetXSectionDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,   pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dia,     pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLength2UnitValue, area,    pDisplayUnits->GetAreaUnit(),         true );

   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, ISegmentData,      pSegmentData);
   GET_IFACE2_NOCHECK(pBroker, IBridge,   pBridge ); 
   GET_IFACE2(pBroker, IStrandGeometry,   pStrandGeom);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker, ISpecification,    pSpec );
   GET_IFACE2(pBroker, IGirder, pIGirder);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::array<std::_tstring, 2> strDuctGeometryType{ _T("Straight"),_T("Parabolic") };
   std::array<std::_tstring, 3> strJackingEnd{ _T("Left"), _T("Right"), _T("Both") };
   std::array<std::_tstring, 3> strDuctType{ _T("Galvanized ferrous metal"),_T("Polyethylene"), _T("Formed in concrete with removable cores") };
   std::array<std::_tstring, 2> strInstallationMethod{ _T("Push"),_T("Pull") };


   for(const auto& thisGirderKey : girderKeys)
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(thisGirderKey.girderIndex);

      WebIndexType nWebs = pIGirder->GetWebCount(thisGirderKey);

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         // Setup the table
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,SEGMENT_LABEL(thisSegmentKey));
         pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
         pTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
         *pPara << pTable << rptNewLine;

         RowIndexType row = 0;

         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         bool harpedAreStraight = pStrandGeom->GetAreHarpedStrandsForcedStraight(thisSegmentKey);

         DuctIndexType nDucts = pSegment->Tendons.GetDuctCount();

         (*pTable)(row,0) << _T("Girder Type");
         (*pTable)(row,1) << pGirder->GetGirderName();
         row++;

         if (pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone && pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
         {
            for (int i = 0; i < 2; i++)
            {
               pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;

               Float64 slabOffset = pSegment->GetSlabOffset(end);

               const CPierData2* pPier;
               const CTemporarySupportData* pTS;
               pSegment->GetSupport(end, &pPier, &pTS);
               if (pPier)
               {
                  PierIndexType pierIdx = pPier->GetIndex();
                  (*pTable)(row, 0) << _T("Slab Offset (\"A\" Dimension) at ") << LABEL_PIER_EX(pPier->IsAbutment(), pierIdx);
               }
               else
               {
                  SupportIndexType tsIdx = pTS->GetIndex();
                  (*pTable)(row, 0) << _T("Slab Offset (\"A\" Dimension) at Temporary Support ") << LABEL_TEMPORARY_SUPPORT(tsIdx);
                  if (pTS->GetSupportType() == pgsTypes::ErectionTower)
                  {
                     (*pTable)(row, 0) << _T(" (ET)");
                  }
                  else
                  {
                     (*pTable)(row, 0) << _T(" (SB)");
                  }
               }
               (*pTable)(row, 1) << cmpdim.SetValue(slabOffset);
               row++;
            } // next segment
         } // if deck type != none

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

         (*pTable)(row, 0) << _T("Specified Release Strength ") << RPT_FCI;
         (*pTable)(row, 1) << stress.SetValue(pSegmentData->GetSegmentMaterial(thisSegmentKey)->Concrete.Fci);
         row++;

         (*pTable)(row, 0) << _T("Specified 28 day Strength ") << RPT_FC;
         (*pTable)(row, 1) << stress.SetValue(pSegmentData->GetSegmentMaterial(thisSegmentKey)->Concrete.Fc);
         row++;       

         if (pSpec->IsAssumedExcessCamberInputEnabled())
         {
            (*pTable)(row, 0) << _T("Assumed Excess Camber");
            (*pTable)(row, 1) << cmpdim.SetValue(pBridge->GetAssumedExcessCamber(thisSegmentKey.groupIndex, thisSegmentKey.girderIndex));
            row++;
         }

         CString strFillType;
         if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtTotal)
         {
            strFillType = "Sequence of Permanent Strands";
         }
         else if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtStraightHarped)
         {
            strFillType = "Sequence of Straight/Harped Strands";
         }
         else if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectSelection)
         {
            strFillType = "Non-Sequential, Direct Fill";
         }
         else if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectRowInput)
         {
            strFillType = "Direct Input Strand Rows";
         }
         else if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
         {
            strFillType = "Direct Input Strands";
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

         StrandIndexType Ns = pSegment->Strands.GetStrandCount(pgsTypes::Straight);
         Float64 Aps_s = pSegment->Strands.GetStrandMaterial(pgsTypes::Straight)->GetNominalArea()*Ns;
         (*pTable)(row,0) << _T("Number of Straight Strands");
         (*pTable)(row,1) << Ns;
         StrandIndexType nDebonded = pStrandGeom->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Straight,pgsTypes::dbetEither);
         if ( 0 < nDebonded )
         {
            (*pTable)(row,1) << rptNewLine << nDebonded << _T(" debonded");
         }

         StrandIndexType nExtendedLeft  = pStrandGeom->GetNumExtendedStrands(thisSegmentKey,pgsTypes::metStart,pgsTypes::Straight);
         StrandIndexType nExtendedRight = pStrandGeom->GetNumExtendedStrands(thisSegmentKey,pgsTypes::metEnd,pgsTypes::Straight);
         if ( 0 < nExtendedLeft || 0 < nExtendedRight )
         {
            (*pTable)(row,1) << rptNewLine << nExtendedLeft  << _T(" extended at left end");
            (*pTable)(row,1) << rptNewLine << nExtendedRight << _T(" extended at right end");
         }
         row++;

         (*pTable)(row, 0) << _T("Straight Strand ") << RPT_APS;
         (*pTable)(row, 1) << area.SetValue(Aps_s);
         row++;

         (*pTable)(row,0) << _T("Straight Strand ") << Sub2(_T("P"),_T("jack"));
         (*pTable)(row,1) << force.SetValue(pSegment->Strands.GetPjack(pgsTypes::Straight));
         row++;

         StrandIndexType Nh = pSegment->Strands.GetStrandCount(pgsTypes::Harped);
         Float64 Aps_h = pSegment->Strands.GetStrandMaterial(pgsTypes::Harped)->GetNominalArea()*Nh;
         (*pTable)(row,0) << _T("Number of ")<< LABEL_HARP_TYPE(harpedAreStraight) <<_T(" Strands");
         (*pTable)(row,1) << Nh;
         nDebonded = pStrandGeom->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Harped,pgsTypes::dbetEither);
         if ( 0 < nDebonded )
         {
            (*pTable)(row,1) << _T(" (") << nDebonded << _T(" debonded)");
         }
         row++;

         (*pTable)(row, 0) << _T("Harped Strand ") << RPT_APS;
         (*pTable)(row, 1) << area.SetValue(Aps_h);
         row++;

         (*pTable)(row,0) << LABEL_HARP_TYPE(harpedAreStraight) << _T(" Strand ") << Sub2(_T("P"),_T("jack"));
         (*pTable)(row,1) << force.SetValue(pSegment->Strands.GetPjack(pgsTypes::Harped));
         row++;

         (*pTable)(row,0) << _T("Total Number of Permanent Strands");
         (*pTable)(row,1) << pSegment->Strands.GetStrandCount(pgsTypes::Permanent);
         row++;

         (*pTable)(row, 0) << _T("Area of Permanent Strands");
         (*pTable)(row, 1) << area.SetValue(Aps_s + Aps_h);
         row++;

         if ( 0 < pStrandGeom->GetMaxStrands(thisSegmentKey,pgsTypes::Temporary) )
         {
            (*pTable)(row,0) << _T("Number of Temporary Strands");
            (*pTable)(row,1) << pSegment->Strands.GetStrandCount(pgsTypes::Temporary);
            nDebonded = pStrandGeom->GetNumDebondedStrands(thisSegmentKey,pgsTypes::Temporary,pgsTypes::dbetEither);
            if ( 0 < nDebonded )
            {
               (*pTable)(row,1) << _T(" (") << nDebonded << _T(" debonded)");
            }
            row++;

            (*pTable)(row, 0) << _T("Temporary Strand ") << RPT_APS;
            (*pTable)(row, 1) << area.SetValue((pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary)->GetNominalArea())*(pSegment->Strands.GetStrandCount(pgsTypes::Temporary)));
            row++;

            (*pTable)(row,0) << _T("Temporary Strand ") << Sub2(_T("P"),_T("jack"));
            (*pTable)(row,1) << force.SetValue(pSegment->Strands.GetPjack(pgsTypes::Temporary));
            row++;
         }

         bool bSymmetric = pIGirder->IsSymmetricSegment(thisSegmentKey);

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
               if ( 0 < vHPPoi.size() )
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

         if (pSegment->Strands.GetStrandDefinitionType() != pgsTypes::sdtDirectRowInput && pSegment->Strands.GetStrandDefinitionType() != pgsTypes::sdtDirectStrandInput)
         {
            for (int i = 0; i < nHarpedAdjustments; i++)
            {
               std::_tstring endoff;
               if (harpedAreStraight)
               {
                  std::_tstring side;
                  if (bSymmetric)
                  {
                     side = _T("");
                  }
                  else
                  {
                     side = (i == 0 ? _T("left ") : _T("right "));
                  }

                  if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoLEGACY)
                  {    // Method used pre-version 6.0
                     endoff = _T("Distance from top-most location in harped strand grid to top-most harped strand at ") + side + _T("end of girder");
                  }
                  else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoCGFROMTOP)
                  {
                     endoff = _T("Distance from top of girder to CG of harped strands at ") + side + _T("end of girder");
                  }
                  else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoCGFROMBOTTOM)
                  {
                     endoff = _T("Distance from bottom of girder to CG of harped strands at ") + side + _T("end of girder");
                  }
                  else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoTOP2TOP)
                  {
                     endoff = _T("Distance from top of girder to top-most harped strand at ") + side + _T("end of girder");
                  }
                  else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoTOP2BOTTOM)
                  {
                     endoff = _T("Distance from bottom of girder to top-most harped strand at ") + side + _T("end of girder");
                  }
                  else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoBOTTOM2BOTTOM)
                  {
                     endoff = _T("Distance from bottom of girder to lowest harped strand at ") + side + _T("end of girder");
                  }
                  else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoECCENTRICITY)
                  {
                     endoff = _T("Eccentricity of harped strand group at ") + side + _T("end of girder");
                  }
                  else
                  {
                     ATLASSERT(false);
                  }

                  (*pTable)(row, 0) << endoff;
                  if (bSymmetric)
                  {
                     (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                  }
                  else
                  {
                     if (i == 0)
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                     }
                     else
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metEnd));
                     }
                  }
                  row++;
               }
               else
               {
                  std::_tstring location;
                  if (bSymmetric)
                  {
                     location = (i == 0 ? _T("ends of girder") : _T("harp point"));
                  }
                  else
                  {
                     if (i == 0)
                     {
                        location = _T("left end of girder");
                     }
                     else if (i == 1)
                     {
                        location = _T("left harp point");
                     }
                     else if (i == 2)
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
                     if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoLEGACY)
                     {    // Method used pre-version 6.0
                        endoff = _T("Distance from top-most location in harped strand grid to top-most harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoCGFROMTOP)
                     {
                        endoff = _T("Distance from top of girder to CG of harped strands at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoCGFROMBOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to CG of harped strands at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoTOP2TOP)
                     {
                        endoff = _T("Distance from top of girder to top-most harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoTOP2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to top-most harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoBOTTOM2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to lowest harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoECCENTRICITY)
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
                     if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoLEGACY)
                     {    // Method used pre-version 6.0
                        endoff = _T("Distance from top-most location in harped strand grid to top-most harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoCGFROMTOP)
                     {
                        endoff = _T("Distance from top of girder to CG of harped strands at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoCGFROMBOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to CG of harped strands at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoTOP2TOP)
                     {
                        endoff = _T("Distance from top of girder to top-most harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoTOP2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to top-most harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoBOTTOM2BOTTOM)
                     {
                        endoff = _T("Distance from bottom of girder to lowest harped strand at ") + location;
                     }
                     else if (pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoECCENTRICITY)
                     {
                        endoff = _T("Eccentricity of harped strand group at ") + location;
                     }
                     else
                     {
                        ATLASSERT(false);
                     }
                  }

                  (*pTable)(row, 0) << endoff;
                  if (bSymmetric)
                  {
                     if (i == 0)
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                     }
                     else
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart));
                     }
                  }
                  else
                  {
                     if (i == 0)
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart));
                     }
                     else if (i == 1)
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart));
                     }
                     else if (i == 2)
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd));
                     }
                     else
                     {
                        (*pTable)(row, 1) << cmpdim.SetValue(pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metEnd));
                     }
                  }
                  row++;
               }
            }
         }

         if (0 < nDucts)
         {
            // plant installed tendon geometry
            pTable->SetColumnSpan(row, 0, 2);
            (*pTable)(row, 0) << Bold(_T("Plant installed tendons"));
            row++;
            for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
            {
               const auto* pDuct = pSegment->Tendons.GetDuct(ductIdx);;

               pTable->SetColumnSpan(row, 0, 2);
               if (nWebs == 1)
               {
                  (*pTable)(row, 0) << bold(ON) << _T("Duct ") << LABEL_DUCT(ductIdx) << bold(OFF);
               }
               else
               {
                  DuctIndexType firstDuctIdx = nWebs*ductIdx;
                  DuctIndexType lastDuctIdx = firstDuctIdx + nWebs - 1;
                  (*pTable)(row, 0) << bold(ON) << _T("Duct ") << LABEL_DUCT(firstDuctIdx) << _T(" - ") << LABEL_DUCT(lastDuctIdx) << bold(OFF);
               }
               row++;

               (*pTable)(row, 0) << _T("Type");
               (*pTable)(row, 1) << pDuct->Name;
               row++;

               //(*pTable)(row, 0) << _T("Shape");
               //(*pTable)(row, 1) << strDuctGeometryType[pDuct->DuctGeometryType];
               //row++;

               (*pTable)(row, 0) << _T("Number of strands");
               (*pTable)(row, 1) << pDuct->nStrands;
               row++;

               (*pTable)(row, 0) << Sub2(_T("P"), _T("jack"));
               (*pTable)(row, 1) << force.SetValue(pDuct->Pj);
               row++;

               //(*pTable)(row, 0) << _T("Jacking End");
               //(*pTable)(row, 1) << strJackingEnd[pDuct->JackingEnd];
               //row++;

               //(*pTable)(row, 0) << _T("Duct Material");
               //(*pTable)(row, 1) << strDuctType[pSegment->Tendons.DuctType];
               //row++;

               //(*pTable)(row, 0) << _T("Installation Method");
               //(*pTable)(row, 1) << strInstallationMethod[pSegment->Tendons.InstallationType];
               //row++;
            }
         }

         std::vector<std::pair<std::_tstring, const WBFL::Materials::PsStrand*>> vStrandData;
         vStrandData.emplace_back(_T("Prestressing Strand (Straight)"), pSegmentData->GetStrandMaterial(thisSegmentKey, pgsTypes::Straight));
         vStrandData.emplace_back(_T("Prestressing Strand (Harped)"), pSegmentData->GetStrandMaterial(thisSegmentKey, pgsTypes::Harped));
         if (0 < pStrandGeom->GetMaxStrands(thisSegmentKey, pgsTypes::Temporary))
         {
            vStrandData.emplace_back(_T("Prestressing Strand (Temporary)"), pSegmentData->GetStrandMaterial(thisSegmentKey, pgsTypes::Temporary));
         }
         if (0 < nDucts)
         {
            vStrandData.emplace_back(_T("Plant installed tendons"), pSegment->Tendons.m_pStrand);
         }

         for (auto strandData : vStrandData)
         {
            ATLASSERT(strandData.second != nullptr);

            (*pTable)(row, 0) << strandData.first;
            (*pTable)(row, 1) << strandData.second->GetName();
            row++;
         }

         switch (pSegment->Strands.GetTemporaryStrandUsage() )
         {
         case pgsTypes::ttsPretensioned:
            // do nothing
            break;

         case pgsTypes::ttsPTBeforeLifting:
            *pPara << _T("NOTE: Temporary strands post-tensioned immediately before lifting") << rptNewLine;
            break;

         case pgsTypes::ttsPTAfterLifting:
            *pPara << _T("NOTE: Temporary strands post-tensioned immediately after lifting") << rptNewLine;
            break;

         case pgsTypes::ttsPTBeforeShipping:
            *pPara << _T("NOTE: Temporary strands post-tensioned immediately before shipping") << rptNewLine;
            break;

         default:
            ATLASSERT(false); // is there a new option?
         }

         if (0 < nDucts)
         {
            switch (pSegmentData->GetSegmentPTData(thisSegmentKey)->InstallationEvent)
            {
            case pgsTypes::sptetRelease:
               *pPara << _T("NOTE: Plant installed tendons are post-tensioned immediately after release") << rptNewLine;
               break;
            case pgsTypes::sptetStorage:
               *pPara << _T("NOTE: Plant installed tendons are post-tensioned immediately at the beginning of storage") << rptNewLine;
               break;
            case pgsTypes::sptetHauling:
               *pPara << _T("NOTE: Plant installed tendons are post-tensioned immediately before hauling") << rptNewLine;
               break;
            default:
               ATLASSERT(false); // is there a new option?
            }
         }
      } // segIdx
   } // gdrIdx
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
         std::array<std::_tstring, 3> strOverhangTaper{_T("None"),_T("Taper overhang to top of girder top flange"),_T("Taper overhang to bottom of girder top flange")};
         (*table)(0,0) << _T("Left Overhang Edge Depth") << _T(" = ") << dim.SetValue(pDeck->OverhangEdgeDepth[pgsTypes::stLeft]) << rptNewLine;
         (*table)(0,0) << _T("Left Overhang Taper") << _T(" = ") << strOverhangTaper[pDeck->OverhangTaper[pgsTypes::stLeft]] << rptNewLine;

         (*table)(0, 0) << _T("Right Overhang Edge Depth") << _T(" = ") << dim.SetValue(pDeck->OverhangEdgeDepth[pgsTypes::stRight]) << rptNewLine;
         (*table)(0, 0) << _T("Right Overhang Taper") << _T(" = ") << strOverhangTaper[pDeck->OverhangTaper[pgsTypes::stRight]] << rptNewLine;
      }

      if (!IsOverlayDeck(deckType))
      {
         (*table)(0,0) << _T("Fillet = ") << dim.SetValue(pBridgeDesc->GetFillet()) << rptNewLine;
         (*table)(0,0) << _T("Fillet Shape: ") << (pBridgeDesc->GetDeckDescription()->HaunchShape==pgsTypes::hsSquare ? _T("Square") : _T("Filleted")) << _T(" Corners") << rptNewLine;
      }

      if (pBridgeDesc->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
         if (pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
         {
            (*table)(0,0) << _T("Slab Offset (\"A\" Dimension) = ") << dim.SetValue(pBridgeDesc->GetSlabOffset()) << rptNewLine;
         }
         else
         {
            (*table)(0,0) << _T("Slab offset by girder") << rptNewLine;
         }
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

void write_haunch_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const std::vector<CGirderKey>& girderKeys,Uint16 level)
{
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   rptParagraph* pPara1 = new rptParagraph;
   pPara1->SetStyleName(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara1;
   *pPara1 << _T("Haunch Depth Input");

   rptParagraph* pPara2 = new rptParagraph;
   *pChapter << pPara2;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::HaunchLayoutType haunchLayoutType = pBridgeDesc->GetHaunchLayoutType();
   pgsTypes::HaunchInputLocationType haunchInputLocationType = pBridgeDesc->GetHaunchInputLocationType();
   pgsTypes::HaunchInputDistributionType haunchInputDistributionType = pBridgeDesc->GetHaunchInputDistributionType();

   std::_tstring strLayout(haunchLayoutType == pgsTypes::hltAlongSpans ? _T("Span") : _T("Segment"));

   *pPara2 << _T("Haunch depths are distributed ");
   if (haunchInputDistributionType == pgsTypes::hidUniform)
   {
      *pPara2 << _T("Unformly");
   }
   else if (haunchInputDistributionType == pgsTypes::hidAtEnds)
   {
      *pPara2 << _T("Linearly between Ends");
   }
   else if (haunchInputDistributionType == pgsTypes::hidParabolic)
   {
      *pPara2 << _T("Parabolically");
   }
   else if (haunchInputDistributionType == pgsTypes::hidQuarterPoints)
   {
      *pPara2 << _T("Linearly between Quarter Points");
   }
   else if (haunchInputDistributionType == pgsTypes::hidTenthPoints)
   {
      *pPara2 << _T("Linearly between Tenth Points");
   }
   else
   {
      ATLASSERT(0);
   }

   *pPara2 << _T(" along ") << strLayout << _T("s, and are defined ");
   if (haunchInputLocationType == pgsTypes::hilSame4Bridge)
   {
      *pPara2 << _T("the Same for all ")<<strLayout<<_T("s in the Bridge. ");
   }
   else if (haunchInputLocationType == pgsTypes::hilSame4AllGirders)
   {
      *pPara2 << _T("the Same for all Girder Lines. ");
   }
   else if (haunchInputLocationType == pgsTypes::hilPerEach)
   {
      *pPara2 << (isPGSuper ?  _T("Uniquely per Girder.") : _T("Uniquely per Segment."));
   }
   else
   {
      ATLASSERT(0);
   }

   if (haunchInputLocationType == pgsTypes::hilSame4Bridge && haunchInputDistributionType == pgsTypes::hidUniform)
   {
      // No table needed for this one
      INIT_UV_PROTOTYPE(rptLengthUnitValue,dimtit,pDisplayUnits->GetComponentDimUnit(),true);
      std::vector<Float64> haunches = pBridgeDesc->GetDirectHaunchDepths();
      *pPara2 << rptNewLine << _T("Uniform haunch depth for all locations: ") << dimtit.SetValue(haunches.front());
   }
   else
   {
      *pPara2 << rptNewLine;
      *pPara2 << _T("Locations are fractional distance along ") << strLayout << _T("s.") << _T("Tables contain Haunch Depth Values in (") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag() << _T(")");

      INIT_UV_PROTOTYPE(rptLengthUnitValue,dim,pDisplayUnits->GetComponentDimUnit(),false);

      if (haunchInputLocationType == pgsTypes::hilSame4Bridge)
      {
         std::vector<Float64> haunches = pBridgeDesc->GetDirectHaunchDepths();

         IndexType numHaunches = haunches.size();
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(numHaunches + 1,_T(""));
         *pPara2 << pTable;

         pTable->SetNumberOfHeaderRows(2);
         pTable->SetRowSpan(0,0,2);
         pTable->SetColumnSpan(0,1,numHaunches);
         (*pTable)(2,0) << _T("All Girders");
         (*pTable)(0,1) << _T("All ") << strLayout << _T("s");

         RowIndexType row = pTable->GetNumberOfHeaderRows();
         ColumnIndexType col = 1;
         for (auto haunch : haunches)
         {
            (*pTable)(1,col) << GetHaunchIncrementString(col - 1,numHaunches);
            (*pTable)(row,col++) << dim.SetValue(haunch);
         }
      }
      else if (haunchLayoutType == pgsTypes::hltAlongSpans)
      {
         SpanIndexType numSpans = pBridgeDesc->GetSpanCount();
         for (SpanIndexType iSpan = 0; iSpan < numSpans; iSpan++)
         {
            IndexType numHaunches = (IndexType)haunchInputDistributionType;
            rptRcTable* pTable = rptStyleManager::CreateDefaultTable(numHaunches + 1,_T(""));
            *pPara2 << pTable << rptNewLine;

            pTable->SetNumberOfHeaderRows(2);
            pTable->SetRowSpan(0,0,2);
            pTable->SetColumnSpan(0,1,numHaunches);
            (*pTable)(0,1) << _T("Span ") << LABEL_SPAN(iSpan);

            if (haunchInputLocationType == pgsTypes::hilSame4AllGirders)
            {
               // Only need one girder
               (*pTable)(2,0) << _T("All Girders");
               std::vector<Float64> haunches = pBridgeDesc->GetSpan(iSpan)->GetDirectHaunchDepths(0); // All girders are same
               ATLASSERT(numHaunches == haunches.size());

               RowIndexType row = pTable->GetNumberOfHeaderRows();
               ColumnIndexType col = 1;
               for (auto haunch : haunches)
               {
                  (*pTable)(1,col) << GetHaunchIncrementString(col - 1,numHaunches);
                  (*pTable)(row,col++) << dim.SetValue(haunch);
               }
            }
            else
            {
               (*pTable)(0,0) << _T("Girder");
               RowIndexType row = pTable->GetNumberOfHeaderRows();
               GirderIndexType numGirders = pBridgeDesc->GetSpan(iSpan)->GetGirderCount();

               for (GirderIndexType iGdr = 0; iGdr < numGirders; iGdr++)
               {
                  (*pTable)(row,0) << LABEL_GIRDER(iGdr);

                  std::vector<Float64> haunches = pBridgeDesc->GetSpan(iSpan)->GetDirectHaunchDepths(iGdr);
                  ATLASSERT(numHaunches == haunches.size());

                  ColumnIndexType col = 1;
                  for (auto haunch : haunches)
                  {
                     if (iGdr==0)
                     {
                        (*pTable)(1,col) << GetHaunchIncrementString(col - 1,numHaunches);
                     }
                     (*pTable)(row,col++) << dim.SetValue(haunch);
                  }

                  row++;
               }
            }
         }
      }
      else if (haunchLayoutType == pgsTypes::hltAlongSegments)
      {
         auto* pGroup = pBridgeDesc->GetGirderGroup(girderKeys.front().groupIndex);
         auto nGirders = haunchInputLocationType == pgsTypes::hilSame4AllGirders ? 1 : pGroup->GetGirderCount();

         // assume number of segments is same for all girders in group
         auto* pGirder0 = pGroup->GetGirder(0);
         auto nSegments = pGirder0->GetSegmentCount();

         for (auto iSeg = 0; iSeg < nSegments; iSeg++)
         {
            IndexType numHaunches = (IndexType)haunchInputDistributionType;
            rptRcTable* pTable = rptStyleManager::CreateDefaultTable(numHaunches + 1,_T(""));
            *pPara2 << pTable << rptNewLine;

            pTable->SetNumberOfHeaderRows(2);
            pTable->SetRowSpan(0,0,2);
            pTable->SetColumnSpan(0,1,numHaunches);
            if (haunchInputLocationType != pgsTypes::hilSame4AllGirders)
            {
               (*pTable)(0,0) << _T("Girder");
            }
            (*pTable)(0,1) << _T("Segment ") << LABEL_SEGMENT(iSeg);

            RowIndexType row = pTable->GetNumberOfHeaderRows();

            for (auto iGdr = 0; iGdr < nGirders; iGdr++,row++)
            {
               auto* pGirder = pGroup->GetGirder(iGdr);
               std::vector<Float64> haunches = pGirder->GetDirectHaunchDepths(iSeg);

               if (haunchInputLocationType == pgsTypes::hilSame4AllGirders)
               {
                  // Only need one girder
                  (*pTable)(2,0) << _T("All Girders");
               }
               else
               {
                  (*pTable)(row,0) << LABEL_GIRDER(iGdr);
               }

               ColumnIndexType col = 1;
               for (auto haunch : haunches)
               {
                  if (iGdr==0)
                  {
                     (*pTable)(1,col) << GetHaunchIncrementString(col - 1,numHaunches);
                  }
                  (*pTable)(row,col++) << dim.SetValue(haunch);
               }
            }
         }
      }
      else
      {
         ATLASSERT(0);
      }
   }
}

void write_pt_data(IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, rptChapter* pChapter, Uint16 level, const std::vector<CGirderKey>& girderKeys)
{
   GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);

   bool bHasDucts = false;
   for (const auto& thisGirderKey : girderKeys)
   {
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(thisGirderKey);
      if (0 < nDucts)
      {
         bHasDucts = true;
         break;
      }
   }

   if (!bHasDucts)
      return;

   GET_IFACE2(pBroker, IGirder, pIGirder);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   rptParagraph* pPara;
   pPara = new rptParagraph;
   pPara->SetStyleName(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Field installed tendons") << rptNewLine;
   *pChapter << pPara;

   std::array<std::_tstring, 3> strDuctGeometryType{ _T("Linear"),_T("Parabolic"),_T("Offset") };
   std::array<std::_tstring, 3> strJackingEnd{ _T("Left"), _T("Right"), _T("Both") };
   std::array<std::_tstring, 3> strDuctType{ _T("Galvanized ferrous metal"),_T("Polyethylene"), _T("Formed in concrete with removable cores") };
   std::array<std::_tstring, 2> strInstallationMethod{ _T("Push"),_T("Pull") };

   for (const auto& thisGirderKey : girderKeys)
   {
      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2, GIRDER_LABEL(thisGirderKey));
      pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_RIGHT));
      pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT));
      *pPara << pTable << rptNewLine;

      WebIndexType nWebs = pIGirder->GetWebCount(thisGirderKey);

      const auto* pGirder = pBridgeDesc->GetGirderGroup(thisGirderKey.groupIndex)->GetGirder(thisGirderKey.girderIndex);
      const CPTData* pPT = pGirder->GetPostTensioning();

      RowIndexType row = 0;
      DuctIndexType nDucts = pGirder->GetPostTensioning()->GetDuctCount();
      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
      {
         const CDuctData* pDuct = pPT->GetDuct(ductIdx);

         pTable->SetColumnSpan(row, 0, 2);
         if (nWebs == 1)
         {
            (*pTable)(row, 0) << bold(ON) << _T("Duct ") << LABEL_DUCT(ductIdx) << bold(OFF);
         }
         else
         {
            DuctIndexType firstDuctIdx = nWebs*ductIdx;
            DuctIndexType lastDuctIdx = firstDuctIdx + nWebs - 1;
            (*pTable)(row, 0) << bold(ON) << _T("Duct ") << LABEL_DUCT(firstDuctIdx) << _T(" - ") << LABEL_DUCT(lastDuctIdx) << bold(OFF);
         }
         row++;

         (*pTable)(row, 0) << _T("Type");
         (*pTable)(row, 1) << pDuct->Name;
         row++;

         (*pTable)(row, 0) << _T("Shape");
         (*pTable)(row, 1) << strDuctGeometryType[pDuct->DuctGeometryType];
         row++;

         (*pTable)(row, 0) << _T("Number of Strands");
         (*pTable)(row, 1) << pDuct->nStrands;
         row++;

         (*pTable)(row, 0) << Sub2(_T("P"),_T("jack"));
         (*pTable)(row, 1) << force.SetValue(pDuct->Pj);
         row++;

         //(*pTable)(row, 0) << _T("Jacking End");
         //(*pTable)(row, 1) << strJackingEnd[pDuct->JackingEnd];
         //row++;

         //EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(pGirder->GetID(), ductIdx);
         //(*pTable)(row, 0) << _T("Installation");
         //(*pTable)(row, 1) << _T("Event ") << LABEL_EVENT(eventIdx) << _T(": ") << pTimelineMgr->GetEventByIndex(eventIdx)->GetDescription();
         //row++;

         //(*pTable)(row, 0) << _T("Duct Material");
         //(*pTable)(row, 1) << strDuctType[pPT->DuctType];
         //row++;

         //(*pTable)(row, 0) << _T("Tendon Material");
         //(*pTable)(row, 1) << pPT->pStrand->GetName();
         //row++;

         //(*pTable)(row, 0) << _T("Installation Method");
         //(*pTable)(row, 1) << strInstallationMethod[pPT->InstallationType];
         //row++;
      }
   }
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