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
#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\BridgeDescChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
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
CAlignmentChapterBuilder::CAlignmentChapterBuilder()
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
   *pPara << "Alignment Details" << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   AlignmentData2 alignment = pAlignment->GetAlignmentData2();

   CComBSTR bstrBearing;
   direction_formatter->Format(alignment.Direction,CComBSTR("°,\',\""),&bstrBearing);
   *pPara << "Direction: " << RPT_BEARING(OLE2A(bstrBearing)) << rptNewLine;

   *pPara << "Ref. Point: " << rptRcStation(alignment.RefStation, &pDisplayUnits->GetStationFormat())
          << " "
          << "(E (X) " << length.SetValue(alignment.xRefPoint);
   *pPara << ", " 
          << "N (Y) " << length.SetValue(alignment.yRefPoint) << ")" << rptNewLine;

   if ( alignment.HorzCurves.size() == 0 )
      return;

   GET_IFACE2(pBroker, IRoadway, pRoadway );

   pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(alignment.HorzCurves.size()+1,"Horizontal Curve Data");
   *pPara << pTable << rptNewLine;

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   RowIndexType row = 0;

   (*pTable)(row++,0) << "Curve Parameters";
   (*pTable)(row++,0) << "Back Tangent Bearing";
   (*pTable)(row++,0) << "Forward Tangent Bearing";
   (*pTable)(row++,0) << "TS Station";
   (*pTable)(row++,0) << "SC Station";
   (*pTable)(row++,0) << "PC Station";
   (*pTable)(row++,0) << "PI Station";
   (*pTable)(row++,0) << "PT Station";
   (*pTable)(row++,0) << "CS Station";
   (*pTable)(row++,0) << "ST Station";
   (*pTable)(row++,0) << "Total Delta (" << Sub2(symbol(DELTA),"c") << ")";
   (*pTable)(row++,0) << "Entry Spiral Length";
   (*pTable)(row++,0) << "Exit Spiral Length";
   (*pTable)(row++,0) << "Delta (" << Sub2(symbol(DELTA),"cc") << ")";
   (*pTable)(row++,0) << "Degree Curvature (DC)";
   (*pTable)(row++,0) << "Radius (R)";
   (*pTable)(row++,0) << "Tangent (T)";
   (*pTable)(row++,0) << "Length (L)";
   (*pTable)(row++,0) << "Chord (C)";
   (*pTable)(row++,0) << "External (E)";
   (*pTable)(row++,0) << "Mid Ordinate (MO)";

   length.ShowUnitTag(true);

   ColumnIndexType col = 1;
   std::vector<HorzCurveData>::iterator iter;
   for ( iter = alignment.HorzCurves.begin(); iter != alignment.HorzCurves.end(); iter++, col++ )
   {
      HorzCurveData& hc_data = *iter;
      row = 0;

      (*pTable)(row++,col) << "Curve " << col;

      if ( IsZero(hc_data.Radius) )
      {
         CComPtr<IDirection> bkTangent;
         pRoadway->GetBearing(hc_data.PIStation - ::ConvertToSysUnits(1.0,unitMeasure::Feet),&bkTangent);

         CComPtr<IDirection> fwdTangent;
         pRoadway->GetBearing(hc_data.PIStation + ::ConvertToSysUnits(1.0,unitMeasure::Feet),&fwdTangent);

         double bk_tangent_value;
         bkTangent->get_Value(&bk_tangent_value);

         double fwd_tangent_value;
         fwdTangent->get_Value(&fwd_tangent_value);

         CComBSTR bstrBkTangent;
         direction_formatter->Format(bk_tangent_value,CComBSTR("°,\',\""),&bstrBkTangent);

         CComBSTR bstrFwdTangent;
         direction_formatter->Format(fwd_tangent_value,CComBSTR("°,\',\""),&bstrFwdTangent);
         (*pTable)(row++,col) << RPT_BEARING(OLE2A(bstrBkTangent));
         (*pTable)(row++,col) << RPT_BEARING(OLE2A(bstrFwdTangent));

         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";

         (*pTable)(row++,col) << rptRcStation(hc_data.PIStation, &pDisplayUnits->GetStationFormat());

         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
      }
      else
      {
         CComPtr<IHorzCurve> hc;
         pRoadway->GetCurve(col-1,&hc);

         CComPtr<IDirection> bkTangent;
         CComPtr<IDirection> fwdTangent;
         hc->get_BkTangentBrg(&bkTangent);
         hc->get_FwdTangentBrg(&fwdTangent);

         double bk_tangent_value;
         bkTangent->get_Value(&bk_tangent_value);

         double fwd_tangent_value;
         fwdTangent->get_Value(&fwd_tangent_value);

         CComBSTR bstrBkTangent;
         direction_formatter->Format(bk_tangent_value,CComBSTR("°,\',\""),&bstrBkTangent);

         CComBSTR bstrFwdTangent;
         direction_formatter->Format(fwd_tangent_value,CComBSTR("°,\',\""),&bstrFwdTangent);

         double bk_tangent_length;
         hc->get_BkTangentLength(&bk_tangent_length);

         double total_length;
         hc->get_TotalLength(&total_length);

         double ts,sc,cs,st;
         ts = hc_data.PIStation - bk_tangent_length;
         sc = hc_data.PIStation - bk_tangent_length + hc_data.EntrySpiral;
         cs = hc_data.PIStation - bk_tangent_length + total_length - hc_data.ExitSpiral;
         st = hc_data.PIStation - bk_tangent_length + total_length;
         (*pTable)(row++,col) << RPT_BEARING(OLE2A(bstrBkTangent));
         (*pTable)(row++,col) << RPT_BEARING(OLE2A(bstrFwdTangent));
         if ( IsEqual(ts,sc) )
         {
            (*pTable)(row++,col) << "-";
            (*pTable)(row++,col) << "-";
            (*pTable)(row++,col) << rptRcStation(ts, &pDisplayUnits->GetStationFormat());
         }
         else
         {
            (*pTable)(row++,col) << rptRcStation(ts, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << rptRcStation(sc, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << "-";
         }
         (*pTable)(row++,col) << rptRcStation(hc_data.PIStation, &pDisplayUnits->GetStationFormat());

         if ( IsEqual(cs,st) )
         {
            (*pTable)(row++,col) << rptRcStation(cs, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << "-";
            (*pTable)(row++,col) << "-";
         }
         else
         {
            (*pTable)(row++,col) << "-";
            (*pTable)(row++,col) << rptRcStation(cs, &pDisplayUnits->GetStationFormat());
            (*pTable)(row++,col) << rptRcStation(st, &pDisplayUnits->GetStationFormat());
         }

         CurveDirectionType direction;
         hc->get_Direction(&direction);

         CComPtr<IAngle> delta;
         hc->get_CurveAngle(&delta);
         double delta_value;
         delta->get_Value(&delta_value);
         delta_value *= (direction == cdRight ? -1 : 1);
         CComBSTR bstrDelta;
         angle_formatter->put_Signed(VARIANT_FALSE);
         angle_formatter->Format(delta_value,CComBSTR("°,\',\""),&bstrDelta);
         (*pTable)(row++,col) << RPT_ANGLE(OLE2A(bstrDelta));

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


         double tangent;
         hc->get_Tangent(&tangent);

         double curve_length;
         hc->get_CurveLength(&curve_length);

         double chord;
         hc->get_Chord(&chord);

         double external;
         hc->get_External(&external);
         
         double mid_ordinate;
         hc->get_MidOrdinate(&mid_ordinate);

         (*pTable)(row++,col) << RPT_ANGLE(OLE2A(bstrDelta));
         (*pTable)(row++,col) << RPT_ANGLE(OLE2A(bstrDC));
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
   *pPara << "Profile Details" << rptNewLine;

   ProfileData2 profile = pAlignment->GetProfileData2();

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << "Station: " << rptRcStation(profile.Station, &pDisplayUnits->GetStationFormat()) << rptNewLine;
   *pPara << "Elevation: " << length.SetValue(profile.Elevation) << rptNewLine;
   *pPara << "Grade: " << profile.Grade*100 << "%" << rptNewLine;

   if ( profile.VertCurves.size() == 0 )
      return;


   // Setup the table
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(profile.VertCurves.size()+1,"Vertical Curve Data");

   pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pPara << pTable << rptNewLine;

   RowIndexType row = 0;
   ColumnIndexType col = 0;
   (*pTable)(row++,col) << "Curve Parameters";
   (*pTable)(row++,col) << "BVC Station";
   (*pTable)(row++,col) << "BVC Elevation";
   (*pTable)(row++,col) << "PVI Station";
   (*pTable)(row++,col) << "PVI Elevation";
   (*pTable)(row++,col) << "EVC Station";
   (*pTable)(row++,col) << "EVC Elevation";
   (*pTable)(row++,col) << "Entry Grade";
   (*pTable)(row++,col) << "Exit Grade";
   (*pTable)(row++,col) << "L1";
   (*pTable)(row++,col) << "L2";
   (*pTable)(row++,col) << "Length";
   (*pTable)(row++,col) << "High Pt Station";
   (*pTable)(row++,col) << "High Pt Elevation";
   (*pTable)(row++,col) << "Low Pt Station";
   (*pTable)(row++,col) << "Low Pt Elevation";

   col++;

   std::vector<VertCurveData>::iterator iter;
   for ( iter = profile.VertCurves.begin(); iter != profile.VertCurves.end(); iter++ )
   {
      row = 0;

      VertCurveData& vcd = *iter;

      (*pTable)(row++,col) << "Curve " << col;
      if ( IsZero(vcd.L1) && IsZero(vcd.L2) )
      {
         Float64 pvi_elevation = pRoadway->GetElevation(vcd.PVIStation,0.0);
         Float64 g1;
         if ( iter == profile.VertCurves.begin() )
            g1 = profile.Grade;
         else
            g1 = (*(iter-1)).ExitGrade;

         Float64 g2 = vcd.ExitGrade;

         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << rptRcStation(vcd.PVIStation, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(pvi_elevation);
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << g1*100 << "%";
         (*pTable)(row++,col) << g2*100 << "%";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
         (*pTable)(row++,col) << "";
      }
      else
      {
         CComPtr<IVertCurve> vc;
         pRoadway->GetVertCurve(col-1,&vc);

         CComPtr<IProfilePoint> bvc;
         vc->get_BVC(&bvc);

         CComPtr<IStation> bvc_station;
         bvc->get_Station(&bvc_station);

         double bvc_station_value;
         bvc_station->get_Value(&bvc_station_value);

         double bvc_elevation;
         bvc->get_Elevation(&bvc_elevation);


         CComPtr<IProfilePoint> pvi;
         vc->get_PVI(&pvi);

         CComPtr<IStation> pvi_station;
         pvi->get_Station(&pvi_station);

         double pvi_station_value;
         pvi_station->get_Value(&pvi_station_value);

         double pvi_elevation;
         pvi->get_Elevation(&pvi_elevation);


         CComPtr<IProfilePoint> evc;
         vc->get_EVC(&evc);

         CComPtr<IStation> evc_station;
         evc->get_Station(&evc_station);

         double evc_station_value;
         evc_station->get_Value(&evc_station_value);

         double evc_elevation;
         evc->get_Elevation(&evc_elevation);

         double g1,g2;
         vc->get_EntryGrade(&g1);
         vc->get_ExitGrade(&g2);

         double L1,L2, Length;
         vc->get_L1(&L1);
         vc->get_L2(&L2);
         vc->get_Length(&Length);

         CComPtr<IProfilePoint> high;
         vc->get_HighPoint(&high);

         CComPtr<IStation> high_station;
         high->get_Station(&high_station);

         double high_station_value;
         high_station->get_Value(&high_station_value);

         double high_elevation;
         high->get_Elevation(&high_elevation);

         CComPtr<IProfilePoint> low;
         vc->get_LowPoint(&low);

         CComPtr<IStation> low_station;
         low->get_Station(&low_station);

         double low_station_value;
         low_station->get_Value(&low_station_value);

         double low_elevation;
         low->get_Elevation(&low_elevation);

         (*pTable)(row++,col) << rptRcStation(bvc_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(bvc_elevation);
         (*pTable)(row++,col) << rptRcStation(pvi_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(pvi_elevation);
         (*pTable)(row++,col) << rptRcStation(evc_station_value, &pDisplayUnits->GetStationFormat());
         (*pTable)(row++,col) << length.SetValue(evc_elevation);
         (*pTable)(row++,col) << g1*100 << "%";
         (*pTable)(row++,col) << g2*100 << "%";
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
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,"Superelevation Details");
   *pPara << pTable << rptNewLine;

   std::string strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   (*pTable)(0,0) << "Section";
   (*pTable)(0,1) << "Station";
   (*pTable)(0,2) << "Left Slope" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,3) << "Right Slope" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,4) << COLHDR("Crown Point Offset", rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );

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
