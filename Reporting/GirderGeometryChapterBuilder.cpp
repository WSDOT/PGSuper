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
#include <Reporting\GirderGeometryChapterBuilder.h>
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

void girder_points(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter);
void girder_offsets(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter);
void girder_spacing(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter);
void girder_ends(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter);
void girder_lengths(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter);

/****************************************************************************
CLASS
   CGirderGeometryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderGeometryChapterBuilder::CGirderGeometryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CGirderGeometryChapterBuilder::GetName() const
{
   return TEXT("Girder Geometry");
}

rptChapter* CGirderGeometryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CBrokerReportSpecification* pSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   girder_points(pBroker,pDisplayUnits,pChapter);
   girder_offsets(pBroker,pDisplayUnits,pChapter);
   girder_spacing(pBroker,pDisplayUnits,pChapter);
   girder_ends(pBroker,pDisplayUnits,pChapter);
   girder_lengths(pBroker,pDisplayUnits,pChapter);

   return pChapter;
}


CChapterBuilder* CGirderGeometryChapterBuilder::Clone() const
{
   return new CGirderGeometryChapterBuilder;
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

void girder_points(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pPara << _T("Girder Points");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), false );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);
   GET_IFACE2(pBroker, IRoadway,pAlignment);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType span = 0; span < nSpans; span++ )
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(span) << std::endl;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(19,os.str().c_str());

      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);

      (*pTable)(0,0)  << _T("Girder");
      pTable->SetRowSpan(0,0,3);  // span 3 rows
      pTable->SetRowSpan(1,0,SKIP_CELL);
      pTable->SetRowSpan(2,0,SKIP_CELL);

      (*pTable)(0,1)  << _T("Start of Girder");
      pTable->SetColumnSpan(0,1,9);

      (*pTable)(0,2)  << _T("End of Girder");
      pTable->SetColumnSpan(0,2,9);

      ColumnIndexType i;
      for ( i = 3; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(0,i,SKIP_CELL);


      if ( span == 0 )
         (*pTable)(1,1) << _T("Abutment Line");
      else
         (*pTable)(1,1) << _T("Pier Line");
      pTable->SetColumnSpan(1,1,3);

      (*pTable)(1,2) << _T("Girder End");
      pTable->SetColumnSpan(1,2,3);

      (*pTable)(1,3) << _T("CL Bearing");
      pTable->SetColumnSpan(1,3,3);

      (*pTable)(1,4) << _T("CL Bearing");
      pTable->SetColumnSpan(1,4,3);

      (*pTable)(1,5) << _T("Girder End");
      pTable->SetColumnSpan(1,5,3);

      if ( span == nSpans-1 )
         (*pTable)(1,6) << _T("Abutment Line");
      else
         (*pTable)(1,6) << _T("Pier Line");
      pTable->SetColumnSpan(1,6,3);

      for ( i = 7; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(1,i,SKIP_CELL);

      (*pTable)(2,1)  << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2,2)  << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2,3)  << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,4)  << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2,5)  << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2,6)  << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,7)  << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2,8)  << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2,9)  << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,10) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2,11) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2,12) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,13) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2,14) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2,15) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,16) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2,17) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2,18) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGdrs = pBridge->GetGirderCount(span);
      for ( GirderIndexType gdr = 0; gdr < nGdrs; gdr++ )
      {
         CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
         pGdr->GetGirderEndPoints(span,gdr,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

         (*pTable)(row,0) << LABEL_GIRDER(gdr);

         Float64 x,y;

         pntPier1->get_X(&x);
         pntPier1->get_Y(&y);
         (*pTable)(row,1) << cogoPoint.SetValue(x);
         (*pTable)(row,2) << cogoPoint.SetValue(y);

         Float64 station, offset, elev;
         pAlignment->GetStationAndOffset(pntPier1,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);
         (*pTable)(row,3) << cogoPoint.SetValue(elev);

         pntEnd1->get_X(&x);
         pntEnd1->get_Y(&y);
         (*pTable)(row,4) << cogoPoint.SetValue(x);
         (*pTable)(row,5) << cogoPoint.SetValue(y);

         pAlignment->GetStationAndOffset(pntEnd1,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);
         (*pTable)(row,6) << cogoPoint.SetValue(elev);

         pntBrg1->get_X(&x);
         pntBrg1->get_Y(&y);
         (*pTable)(row,7) << cogoPoint.SetValue(x);
         (*pTable)(row,8) << cogoPoint.SetValue(y);

         pAlignment->GetStationAndOffset(pntBrg1,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);
         (*pTable)(row,9) << cogoPoint.SetValue(elev);

         pntBrg2->get_X(&x);
         pntBrg2->get_Y(&y);
         (*pTable)(row,10) << cogoPoint.SetValue(x);
         (*pTable)(row,11) << cogoPoint.SetValue(y);

         pAlignment->GetStationAndOffset(pntBrg2,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);
         (*pTable)(row,12) << cogoPoint.SetValue(elev);

         pntEnd2->get_X(&x);
         pntEnd2->get_Y(&y);
         (*pTable)(row,13) << cogoPoint.SetValue(x);
         (*pTable)(row,14) << cogoPoint.SetValue(y);

         pAlignment->GetStationAndOffset(pntEnd2,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);
         (*pTable)(row,15) << cogoPoint.SetValue(elev);

         pntPier2->get_X(&x);
         pntPier2->get_Y(&y);
         (*pTable)(row,16) << cogoPoint.SetValue(x);
         (*pTable)(row,17) << cogoPoint.SetValue(y);

         pAlignment->GetStationAndOffset(pntPier2,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);
         (*pTable)(row,18) << cogoPoint.SetValue(elev);

         row++;
      }
   }
}

void girder_offsets(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pPara << _T("Girder Offsets");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), false );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);
   GET_IFACE2(pBroker, IRoadway,pAlignment);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType span = 0; span < nSpans; span++ )
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(span) << std::endl;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(19,os.str().c_str());

      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);

      (*pTable)(0,0)  << _T("Girder");
      pTable->SetRowSpan(0,0,3);  // span 3 rows
      pTable->SetRowSpan(1,0,SKIP_CELL);
      pTable->SetRowSpan(2,0,SKIP_CELL);

      (*pTable)(0,1)  << _T("Start of Girder");
      pTable->SetColumnSpan(0,1,9);

      (*pTable)(0,2)  << _T("End of Girder");
      pTable->SetColumnSpan(0,2,9);

      ColumnIndexType i;
      for ( i = 3; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(0,i,SKIP_CELL);


      if ( span == 0 )
         (*pTable)(1,1) << _T("Abutment Line");
      else
         (*pTable)(1,1) << _T("Pier Line");
      pTable->SetColumnSpan(1,1,3);

      (*pTable)(1,2) << _T("Girder End");
      pTable->SetColumnSpan(1,2,3);

      (*pTable)(1,3) << _T("CL Bearing");
      pTable->SetColumnSpan(1,3,3);

      (*pTable)(1,4) << _T("CL Bearing");
      pTable->SetColumnSpan(1,4,3);

      (*pTable)(1,5) << _T("Girder End");
      pTable->SetColumnSpan(1,5,3);

      if ( span == nSpans-1 )
         (*pTable)(1,6) << _T("Abutment Line");
      else
         (*pTable)(1,6) << _T("Pier Line");
      pTable->SetColumnSpan(1,6,3);

      for ( i = 7; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(1,i,SKIP_CELL);

      (*pTable)(2,1)  << _T("Station");
      (*pTable)(2,2)  << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,3)  << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,4)  << _T("Station");
      (*pTable)(2,5)  << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,6)  << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,7)  << _T("Station");
      (*pTable)(2,8)  << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,9)  << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,10) << _T("Station");
      (*pTable)(2,11) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,12) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,13) << _T("Station");
      (*pTable)(2,14) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,15) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,16) << _T("Station");
      (*pTable)(2,17) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );
      (*pTable)(2,18) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit() );

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGdrs = pBridge->GetGirderCount(span);
      for ( GirderIndexType gdr = 0; gdr < nGdrs; gdr++ )
      {
         CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
         pGdr->GetGirderEndPoints(span,gdr,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

         (*pTable)(row,0) << LABEL_GIRDER(gdr);

         Float64 x,y;
         Float64 station, offset, elev;

         pAlignment->GetStationAndOffset(pntPier1,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);

         pntPier1->get_X(&x);
         pntPier1->get_Y(&y);
         (*pTable)(row,1) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
         (*pTable)(row,2) << RPT_OFFSET(offset,cogoPoint);
         (*pTable)(row,3) << cogoPoint.SetValue(elev);

         pAlignment->GetStationAndOffset(pntEnd1,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);

         pntEnd1->get_X(&x);
         pntEnd1->get_Y(&y);
         (*pTable)(row,4) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
         (*pTable)(row,5) << RPT_OFFSET(offset,cogoPoint);
         (*pTable)(row,6) << cogoPoint.SetValue(elev);

         pAlignment->GetStationAndOffset(pntBrg1,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);

         pntBrg1->get_X(&x);
         pntBrg1->get_Y(&y);
         (*pTable)(row,7) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
         (*pTable)(row,8) << RPT_OFFSET(offset,cogoPoint);
         (*pTable)(row,9) << cogoPoint.SetValue(elev);

         pAlignment->GetStationAndOffset(pntBrg2,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);

         pntBrg2->get_X(&x);
         pntBrg2->get_Y(&y);
         (*pTable)(row,10) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
         (*pTable)(row,11) << RPT_OFFSET(offset,cogoPoint);
         (*pTable)(row,12) << cogoPoint.SetValue(elev);

         pAlignment->GetStationAndOffset(pntEnd2,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);

         pntEnd2->get_X(&x);
         pntEnd2->get_Y(&y);
         (*pTable)(row,13) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
         (*pTable)(row,14) << RPT_OFFSET(offset,cogoPoint);
         (*pTable)(row,15) << cogoPoint.SetValue(elev);

         pAlignment->GetStationAndOffset(pntPier2,&station,&offset);
         elev = pAlignment->GetElevation(station,offset);

         pntPier2->get_X(&x);
         pntPier2->get_Y(&y);
         (*pTable)(row,16) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
         (*pTable)(row,17) << RPT_OFFSET(offset,cogoPoint);
         (*pTable)(row,18) << cogoPoint.SetValue(elev);

         row++;
      }
   }
}

void girder_lengths(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   USES_CONVERSION;

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pPara << _T("Girder Lengths");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   *pPara << _T("C-C Pier = Abutment/Pier Line to Abutment/Pier Line length measured along the girder") << rptNewLine;
   *pPara << _T("C-C Bearing = Centerline bearing to centerline bearing length measured along the girder") << rptNewLine;
   *pPara << _T("Girder Length, Horizontal = End to end length of the girder projected into a horizontal plane") << rptNewLine;
   *pPara << _T("Girder Length, Along Grade = End to end length of girder measured along grade of the girder (slope adjusted) = ") << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("SlopeAdjustedGirderLength.png"),rptRcImage::Middle) << rptNewLine;
   *pPara << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,  pDisplayUnits->GetSpanLengthUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(7);
   scalar.SetPrecision(4);
   scalar.SetTolerance(1.0e-6);

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType span = 0; span < nSpans; span++ )
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(span) << std::endl;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(7,os.str().c_str());
      pTable->SetNumberOfHeaderRows(2);

      (*pPara) << pTable << rptNewLine;

      pTable->SetRowSpan(0,0,2);
      pTable->SetRowSpan(1,0,SKIP_CELL);
      (*pTable)(0,0) << _T("Girder");

      pTable->SetRowSpan(0,1,2);
      pTable->SetRowSpan(1,1,SKIP_CELL);
      (*pTable)(0,1) << COLHDR(_T("C-C Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(0,2,2);
      pTable->SetRowSpan(1,2,SKIP_CELL);
      (*pTable)(0,2) << COLHDR(_T("C-C Bearing") << rptNewLine << Sub2(_T("L"),_T("s")), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetColumnSpan(0,3,2);
      pTable->SetColumnSpan(0,4,SKIP_CELL);
      (*pTable)(0,3) << _T("Girder Length");
      (*pTable)(1,3) << COLHDR(_T("Horizontal") << rptNewLine << Sub2(_T("L"),_T("g")), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(1,4) << COLHDR(_T("Along")    << rptNewLine << _T("Grade"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(0,5,2);
      pTable->SetRowSpan(1,5,SKIP_CELL);
      (*pTable)(0,5) << _T("Girder") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

      pTable->SetRowSpan(0,6,2);
      pTable->SetRowSpan(1,6,SKIP_CELL);
      (*pTable)(0,6) << _T("Direction");

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGdrs = pBridge->GetGirderCount(span);
      for ( GirderIndexType gdr = 0; gdr < nGdrs; gdr++ )
      {
         (*pTable)(row,0) << LABEL_GIRDER(gdr);

         Float64 L = pBridge->GetCCPierLength(span,gdr);
         (*pTable)(row,1) << length.SetValue(L);

         L = pBridge->GetSpanLength(span,gdr);
         (*pTable)(row,2) << length.SetValue(L);

         L = pBridge->GetGirderLength(span,gdr);
         (*pTable)(row,3) << length.SetValue(L);

         L = pBridge->GetGirderPlanLength(span,gdr);
         (*pTable)(row,4) << length.SetValue(L);

         Float64 slope = pBridge->GetGirderSlope(span,gdr);
         (*pTable)(row,5) << scalar.SetValue(slope);

         CComPtr<IDirection> gdr_bearing;
         pBridge->GetGirderBearing(span,gdr,&gdr_bearing);
         Float64 gdr_bearing_value;
         gdr_bearing->get_Value(&gdr_bearing_value);

         CComBSTR bstrBearing;
         direction_formatter->Format(gdr_bearing_value,CComBSTR("°,\',\""),&bstrBearing);

         (*pTable)(row,6) << RPT_BEARING(OLE2T(bstrBearing));

         row++;
      }
   }
}

void girder_spacing(IBroker*pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   USES_CONVERSION;

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pPara << _T("Girder Spacing");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, spacingValue, pDisplayUnits->GetXSectionDimUnit(), false );

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
   angle_formatter->put_Signed(VARIANT_TRUE);

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);
   GET_IFACE2(pBroker, IRoadway,pAlignment);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType span = 0; span < nSpans; span++ )
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(span) << std::endl;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(11,os.str().c_str());

      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);

      (*pTable)(0,0)  << _T("Girder");
      pTable->SetRowSpan(0,0,3);  // span 3 rows
      pTable->SetRowSpan(1,0,SKIP_CELL);
      pTable->SetRowSpan(2,0,SKIP_CELL);

      (*pTable)(0,1)  << _T("Start of Girder");
      pTable->SetColumnSpan(0,1,5);

      (*pTable)(0,2)  << _T("End of Girder");
      pTable->SetColumnSpan(0,2,5);

      ColumnIndexType i;
      for ( i = 3; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(0,i,SKIP_CELL);


      if ( span == 0 )
         (*pTable)(1,1) << _T("Spacing at Abutment Line");
      else
         (*pTable)(1,1) << _T("Spacing at Pier Line");
      pTable->SetColumnSpan(1,1,2);

      (*pTable)(1,2) << _T("Spacing at CL Brg");
      pTable->SetColumnSpan(1,2,2);

      (*pTable)(1,3) << _T("Angle") << rptNewLine << _T("with") << rptNewLine << _T("CL Pier");
      pTable->SetRowSpan(1,3,2);

      (*pTable)(1,4) << _T("Spacing at CL Brg");
      pTable->SetColumnSpan(1,4,2);

      if ( span == nSpans-1 )
         (*pTable)(1,5) << _T("Spacing at Abutment Line");
      else
         (*pTable)(1,5) << _T("Spacing at Pier Line");
      pTable->SetColumnSpan(1,5,2);

      (*pTable)(1,6) << _T("Angle") << rptNewLine << _T("with") << rptNewLine << _T("CL Pier");
      pTable->SetRowSpan(1,6,2);

      for ( i = 7; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(1,i,SKIP_CELL);

      (*pTable)(2,1)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      if ( span == 0 )
         (*pTable)(2,2)  << COLHDR(_T("Along Abutment Line"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      else
         (*pTable)(2,2)  << COLHDR(_T("Along Pier Line"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      (*pTable)(2,3)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      (*pTable)(2,4)  << COLHDR(_T("Along CL Brg"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      pTable->SetRowSpan(2,5,SKIP_CELL);

      (*pTable)(2,6)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      (*pTable)(2,7)  << COLHDR(_T("Along CL Brg"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      (*pTable)(2,8)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      if ( span == nSpans-1 )
         (*pTable)(2,9)  << COLHDR(_T("Along Abutment Line"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      else
         (*pTable)(2,9)  << COLHDR(_T("Along Pier Line"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
      pTable->SetRowSpan(2,10,SKIP_CELL);

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      PierIndexType prevPierIdx = span;
      PierIndexType nextPierIdx = span+1;

      std::vector<Float64> spacing[8];
      spacing[0] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtPierLine,    pgsTypes::NormalToItem);
      spacing[1] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtPierLine,    pgsTypes::AlongItem);
      spacing[2] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtCenterlineBearing, pgsTypes::NormalToItem);
      spacing[3] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtCenterlineBearing, pgsTypes::AlongItem);
      spacing[4] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back,  pgsTypes::AtCenterlineBearing, pgsTypes::NormalToItem);
      spacing[5] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back,  pgsTypes::AtCenterlineBearing, pgsTypes::AlongItem);
      spacing[6] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back,  pgsTypes::AtPierLine,    pgsTypes::NormalToItem);
      spacing[7] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back,  pgsTypes::AtPierLine,    pgsTypes::AlongItem);


      GirderIndexType nGdrs = pBridge->GetGirderCount(span);
      for ( GirderIndexType gdr = 0; gdr < nGdrs; gdr++ )
      {
         (*pTable)(row,0) << LABEL_GIRDER(gdr);

         // girder/pier angle
         (*pTable)(row,1) << _T("");
         (*pTable)(row,2) << _T("");
         (*pTable)(row,3) << _T("");
         (*pTable)(row,4) << _T("");

         CComPtr<IAngle> startAngle, endAngle;
         pBridge->GetGirderAngle(span,gdr,pgsTypes::Ahead,&startAngle);
         pBridge->GetGirderAngle(span,gdr,pgsTypes::Back, &endAngle);

         Float64 angle;
         startAngle->get_Value(&angle);
         if ( M_PI <= angle )
            angle -= M_PI;

         CComBSTR bstrStartAngle;
         angle_formatter->Format(angle,CComBSTR("°,\',\""),&bstrStartAngle);
         (*pTable)(row,5) << RPT_ANGLE(OLE2T(bstrStartAngle));

         (*pTable)(row,6) << _T("");
         (*pTable)(row,7) << _T("");
         (*pTable)(row,8) << _T("");
         (*pTable)(row,9) << _T("");

         CComBSTR bstrEndAngle;
         endAngle->get_Value(&angle);
         if ( M_PI <= angle )
            angle -= M_PI;

         angle_formatter->Format(angle,CComBSTR("°,\',\""),&bstrEndAngle);
         (*pTable)(row,10) << RPT_ANGLE(OLE2T(bstrEndAngle));

         // girder spacing (between girders)
         if ( gdr < nGdrs-1 )
         {
            row++;

            (*pTable)(row,0) << _T("");

            (*pTable)(row,1) << spacingValue.SetValue(spacing[0][gdr]);
            (*pTable)(row,2) << spacingValue.SetValue(spacing[1][gdr]);
            (*pTable)(row,3) << spacingValue.SetValue(spacing[2][gdr]);
            (*pTable)(row,4) << spacingValue.SetValue(spacing[3][gdr]);

            (*pTable)(row,5) << _T("");

            (*pTable)(row,6) << spacingValue.SetValue(spacing[4][gdr]);
            (*pTable)(row,7) << spacingValue.SetValue(spacing[5][gdr]);
            (*pTable)(row,8) << spacingValue.SetValue(spacing[6][gdr]);
            (*pTable)(row,9) << spacingValue.SetValue(spacing[7][gdr]);

            (*pTable)(row,10) << _T("");
         }

         row++;
      }
   }

   (*pPara) << symbol(NORMAL) << _T(" to Alignment: spacing is measured along a line that is normal to the alignment and passes through the point where the Abutment/Pier Line or CL Brg intersect the alignment.") << rptNewLine;
}


void girder_ends(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   USES_CONVERSION;

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pPara << _T("Girder Ends");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,  pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType span = 0; span < nSpans; span++ )
   {
      std::_tostringstream os;
      os << _T("Span ") << LABEL_SPAN(span) << std::endl;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(11,os.str().c_str());
      pTable->SetNumberOfHeaderRows(3);

      (*pPara) << pTable << rptNewLine;

      ColumnIndexType col0 = 0;
      ColumnIndexType col1 = 0;
      ColumnIndexType col2 = 0;

      pTable->SetRowSpan(0,col0,3);
      pTable->SetRowSpan(1,col1,SKIP_CELL);
      pTable->SetRowSpan(2,col2,SKIP_CELL);
      (*pTable)(0,col0++) << _T("Girder");

      pTable->SetColumnSpan(0,col0,5);
      (*pTable)(0,col0++) << _T("Start of Girder");

      pTable->SetColumnSpan(0,col0,5);
      (*pTable)(0,col0++) << _T("End of Girder");

      ColumnIndexType i;
      for ( i = col0; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(0,i,SKIP_CELL);

      col1++;
      col2++;

      pTable->SetColumnSpan(1,col1,2);
      if ( span == 0 ) 
         (*pTable)(1,col1++) << _T("Abutment Line to CL Brg");
      else
         (*pTable)(1,col1++) << _T("Pier Line to CL Brg");
      (*pTable)(2,col2++) << COLHDR(symbol(NORMAL) << _T(" to Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(2,col2++) << COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetColumnSpan(1,col1,2);
      if ( span == 0 ) 
         (*pTable)(1,col1++) << _T("Abutment Line to Girder End");
      else
         (*pTable)(1,col1++) << _T("Pier Line to Girder End");
      (*pTable)(2,col2++) << COLHDR(symbol(NORMAL) << _T(" to Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(2,col2++) << COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(1,col1,2);
      pTable->SetRowSpan(2,col2++,SKIP_CELL);
      (*pTable)(1,col1++)<< COLHDR(_T("CL Brg to Girder End") << rptNewLine << _T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );


      pTable->SetColumnSpan(1,col1,2);
      if ( span == nSpans-1 ) 
         (*pTable)(1,col1++) << _T("Abutment Line to CL Brg");
      else
         (*pTable)(1,col1++) << _T("Pier Line to CL Brg");
      (*pTable)(2,col2++) << COLHDR(symbol(NORMAL) << _T(" to Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(2,col2++) << COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetColumnSpan(1,col1,2);
      if ( span == nSpans-1 ) 
         (*pTable)(1,col1++) << _T("Abutment Line to Girder End");
      else
         (*pTable)(1,col1++) << _T("Pier Line to Girder End");
      (*pTable)(2,col2++) << COLHDR(symbol(NORMAL) << _T(" to Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(2,col2++)<< COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(1,col1,2);
      pTable->SetRowSpan(2,col2++,SKIP_CELL);
      (*pTable)(1,col1++)<< COLHDR(_T("CL Brg to Girder End") << rptNewLine << _T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      for ( i = col1; i < pTable->GetNumberOfColumns(); i++ )
         pTable->SetColumnSpan(1,i,SKIP_CELL);


      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGdrs = pBridge->GetGirderCount(span);
      for ( GirderIndexType gdr = 0; gdr < nGdrs; gdr++ )
      {
         ColumnIndexType col = 0;
         (*pTable)(row,col++) << LABEL_GIRDER(gdr);

         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(span,gdr,pgsTypes::Ahead,pgsTypes::NormalToItem) );
         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(span,gdr,pgsTypes::Ahead,pgsTypes::AlongItem) );

         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToGirderEndDistance(span,gdr,pgsTypes::Ahead,pgsTypes::NormalToItem) );
         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToGirderEndDistance(span,gdr,pgsTypes::Ahead,pgsTypes::AlongItem) );

         (*pTable)(row,col++) << length.SetValue( pBridge->GetGirderStartConnectionLength(span,gdr) );

         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(span,gdr,pgsTypes::Back,pgsTypes::NormalToItem) );
         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(span,gdr,pgsTypes::Back,pgsTypes::AlongItem) );

         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToGirderEndDistance(span,gdr,pgsTypes::Back,pgsTypes::NormalToItem) );
         (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToGirderEndDistance(span,gdr,pgsTypes::Back,pgsTypes::AlongItem) );

         (*pTable)(row,col++) << length.SetValue( pBridge->GetGirderEndConnectionLength(span,gdr) );

         row++;
      }
   }
}
