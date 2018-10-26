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
#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\BridgeDescChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>


#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void write_alignment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_profile_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_crown_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);

/****************************************************************************
CLASS
   CAlignmentChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CAlignmentChapterBuilder::CAlignmentChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CAlignmentChapterBuilder::GetName() const
{
   return TEXT("Alignment");
}

rptChapter* CAlignmentChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CBrokerReportSpecification* pBrokerSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);

   // This report does not use the passd span and girder parameters
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   CComPtr<IBroker> pBroker;
   pBrokerSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   write_alignment_data(pBroker,pDisplayUnits,pChapter,level);
   write_profile_data(pBroker,pDisplayUnits,pChapter,level);
   write_crown_data(pBroker,pDisplayUnits,pChapter,level);


   return pChapter;
}


CChapterBuilder* CAlignmentChapterBuilder::Clone() const
{
   return new CAlignmentChapterBuilder;
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


void write_alignment_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, IRoadwayData, pAlignment ); 
   rptParagraph* pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), false );

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);

   pPara = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pPara;
   *pPara << _T("Alignment Details") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   AlignmentData2 alignment = pAlignment->GetAlignmentData2();

   CComBSTR bstrBearing;
   direction_formatter->Format(alignment.Direction,CComBSTR("°,\',\""),&bstrBearing);
   *pPara << _T("Direction: ") << RPT_BEARING(OLE2T(bstrBearing)) << rptNewLine;

   *pPara << _T("Ref. Point: ") << rptRcStation(alignment.RefStation, &pDisplayUnits->GetStationFormat())
          << _T(" ")
          << _T("(E (X) ") << length.SetValue(alignment.xRefPoint);
   *pPara << _T(", ") 
          << _T("N (Y) ") << length.SetValue(alignment.yRefPoint) << _T(")") << rptNewLine;

   if ( alignment.HorzCurves.size() == 0 )
      return;

   GET_IFACE2(pBroker, IRoadway, pRoadway );

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(alignment.HorzCurves.size()+1,_T("Horizontal Curve Data"));
   *pPara << pTable << rptNewLine;

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   RowIndexType row = 0;

   (*pTable)(row++,0) << _T("Curve Parameters");
   (*pTable)(row++,0) << _T("Back Tangent Bearing");
   (*pTable)(row++,0) << _T("Forward Tangent Bearing");
   (*pTable)(row++,0) << _T("TS Station");
   (*pTable)(row++,0) << _T("SC Station");
   (*pTable)(row++,0) << _T("PC Station");
   (*pTable)(row++,0) << _T("PI Station");
   (*pTable)(row++,0) << _T("PT Station");
   (*pTable)(row++,0) << _T("CS Station");
   (*pTable)(row++,0) << _T("ST Station");
   (*pTable)(row++,0) << _T("Total Delta (") << Sub2(symbol(DELTA),_T("c")) << _T(")");
   (*pTable)(row++,0) << _T("Entry Spiral Length");
   (*pTable)(row++,0) << _T("Exit Spiral Length");
   (*pTable)(row++,0) << _T("Delta (") << Sub2(symbol(DELTA),_T("cc")) << _T(")");
   (*pTable)(row++,0) << _T("Degree Curvature (DC)");
   (*pTable)(row++,0) << _T("Radius (R)");
   (*pTable)(row++,0) << _T("Tangent (T)");
   (*pTable)(row++,0) << _T("Length (L)");
   (*pTable)(row++,0) << _T("Chord (C)");
   (*pTable)(row++,0) << _T("External (E)");
   (*pTable)(row++,0) << _T("Mid Ordinate (MO)");

   length.ShowUnitTag(true);

   ColumnIndexType col = 1;
   std::vector<HorzCurveData>::iterator iter;
   for ( iter = alignment.HorzCurves.begin(); iter != alignment.HorzCurves.end(); iter++, col++ )
   {
      HorzCurveData& hc_data = *iter;
      row = 0;

      (*pTable)(row++,col) << _T("Curve ") << col;

      if ( IsZero(hc_data.Radius) )
      {
         CComPtr<IDirection> bkTangent;
         pRoadway->GetBearing(hc_data.PIStation - ::ConvertToSysUnits(1.0,unitMeasure::Feet),&bkTangent);

         CComPtr<IDirection> fwdTangent;
         pRoadway->GetBearing(hc_data.PIStation + ::ConvertToSysUnits(1.0,unitMeasure::Feet),&fwdTangent);

         Float64 bk_tangent_value;
         bkTangent->get_Value(&bk_tangent_value);

         Float64 fwd_tangent_value;
         fwdTangent->get_Value(&fwd_tangent_value);

         CComBSTR bstrBkTangent;
         direction_formatter->Format(bk_tangent_value,CComBSTR("°,\',\""),&bstrBkTangent);

         CComBSTR bstrFwdTangent;
         direction_formatter->Format(fwd_tangent_value,CComBSTR("°,\',\""),&bstrFwdTangent);
         (*pTable)(row++,col) << RPT_BEARING(OLE2T(bstrBkTangent));
         (*pTable)(row++,col) << RPT_BEARING(OLE2T(bstrFwdTangent));

         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");

         (*pTable)(row++,col) << rptRcStation(hc_data.PIStation, &pDisplayUnits->GetStationFormat());

         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
         (*pTable)(row++,col) << _T("");
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
         CComPtr<IHorzCurve> hc;
         pRoadway->GetCurve(col-1,&hc);

         CComPtr<IDirection> bkTangent;
         CComPtr<IDirection> fwdTangent;
         hc->get_BkTangentBrg(&bkTangent);
         hc->get_FwdTangentBrg(&fwdTangent);

         Float64 bk_tangent_value;
         bkTangent->get_Value(&bk_tangent_value);

         Float64 fwd_tangent_value;
         fwdTangent->get_Value(&fwd_tangent_value);

         CComBSTR bstrBkTangent;
         direction_formatter->Format(bk_tangent_value,CComBSTR("°,\',\""),&bstrBkTangent);

         CComBSTR bstrFwdTangent;
         direction_formatter->Format(fwd_tangent_value,CComBSTR("°,\',\""),&bstrFwdTangent);

         Float64 bk_tangent_length;
         hc->get_BkTangentLength(&bk_tangent_length);

         Float64 total_length;
         hc->get_TotalLength(&total_length);

         Float64 ts,sc,cs,st;
         ts = hc_data.PIStation - bk_tangent_length;
         sc = hc_data.PIStation - bk_tangent_length + hc_data.EntrySpiral;
         cs = hc_data.PIStation - bk_tangent_length + total_length - hc_data.ExitSpiral;
         st = hc_data.PIStation - bk_tangent_length + total_length;
         (*pTable)(row++,col) << RPT_BEARING(OLE2T(bstrBkTangent));
         (*pTable)(row++,col) << RPT_BEARING(OLE2T(bstrFwdTangent));
         if ( IsEqual(ts,sc) )
         {
            (*pTable)(row++,col) << _T("-");
            (*pTable)(row++,col) << _T("-");
            (*pTable)(row++,col) << rptRcStation(ts, &pDisplayUnits->GetStationFormat());
         }
         else
         {
            (*pTable)(row++,col) << rptRcStation(ts, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << rptRcStation(sc, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << _T("-");
         }
         (*pTable)(row++,col) << rptRcStation(hc_data.PIStation, &pDisplayUnits->GetStationFormat());

         if ( IsEqual(cs,st) )
         {
            (*pTable)(row++,col) << rptRcStation(cs, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << _T("-");
            (*pTable)(row++,col) << _T("-");
         }
         else
         {
            (*pTable)(row++,col) << _T("-");
            (*pTable)(row++,col) << rptRcStation(cs, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << rptRcStation(st, &pDisplayUnits->GetStationFormat());
         }

         CurveDirectionType direction;
         hc->get_Direction(&direction);

         CComPtr<IAngle> delta;
         hc->get_CurveAngle(&delta);
         Float64 delta_value;
         delta->get_Value(&delta_value);
         delta_value *= (direction == cdRight ? -1 : 1);
         CComBSTR bstrDelta;
         angle_formatter->put_Signed(VARIANT_FALSE);
         angle_formatter->Format(delta_value,CComBSTR("°,\',\""),&bstrDelta);
         (*pTable)(row++,col) << RPT_ANGLE(OLE2T(bstrDelta));

         // Entry Spiral Data
         (*pTable)(row++,col) << length.SetValue(hc_data.EntrySpiral);
         (*pTable)(row++,col) << length.SetValue(hc_data.ExitSpiral);

         // Circular curve data
         delta.Release();
         hc->get_CircularCurveAngle(&delta);
         delta->get_Value(&delta_value);
         delta_value *= (direction == cdRight ? -1 : 1);
         bstrDelta.Empty();
         angle_formatter->put_Signed(VARIANT_FALSE);
         angle_formatter->Format(delta_value,CComBSTR("°,\',\""),&bstrDelta);

         CComBSTR bstrDC;
         delta.Release();
         hc->get_DegreeCurvature(::ConvertToSysUnits(100.0,unitMeasure::Feet),dcHighway,&delta);
         delta->get_Value(&delta_value);
         angle_formatter->put_Signed(VARIANT_TRUE);
         angle_formatter->Format(delta_value,CComBSTR("°,\',\""),&bstrDC);


         Float64 tangent;
         hc->get_Tangent(&tangent);

         Float64 curve_length;
         hc->get_CurveLength(&curve_length);

         Float64 chord;
         hc->get_Chord(&chord);

         Float64 external;
         hc->get_External(&external);
         
         Float64 mid_ordinate;
         hc->get_MidOrdinate(&mid_ordinate);

         (*pTable)(row++,col) << RPT_ANGLE(OLE2T(bstrDelta));
         (*pTable)(row++,col) << RPT_ANGLE(OLE2T(bstrDC));
         (*pTable)(row++,col) << length.SetValue(hc_data.Radius);
         (*pTable)(row++,col) << length.SetValue(tangent);
         (*pTable)(row++,col) << length.SetValue(curve_length);
         (*pTable)(row++,col) << length.SetValue(chord);
         (*pTable)(row++,col) << length.SetValue(external);
         (*pTable)(row++,col) << length.SetValue(mid_ordinate);
      }
   }
}

void write_profile_data(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   GET_IFACE2(pBroker, IRoadway, pRoadway);
   GET_IFACE2(pBroker, IRoadwayData, pAlignment ); 
   rptParagraph* pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetAlignmentLengthUnit(), true );

   pPara = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pPara;
   *pPara << _T("Profile Details") << rptNewLine;

   ProfileData2 profile = pAlignment->GetProfileData2();

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Station: ") << rptRcStation(profile.Station, &pDisplayUnits->GetStationFormat()) << rptNewLine;
   *pPara << _T("Elevation: ") << length.SetValue(profile.Elevation) << rptNewLine;
   *pPara << _T("Grade: ") << profile.Grade*100 << _T("%") << rptNewLine;

   if ( profile.VertCurves.size() == 0 )
      return;


   // Setup the table
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(profile.VertCurves.size()+1,_T("Vertical Curve Data"));

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

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
            g1 = profile.Grade;
         else
            g1 = (*(iter-1)).ExitGrade;

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
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T("Superelevation Details"));
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
