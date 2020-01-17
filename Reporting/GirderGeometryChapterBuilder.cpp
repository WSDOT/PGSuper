///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\DocumentType.h>

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
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Girder Points");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), false );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);
   GET_IFACE2(pBroker, IRoadway,pAlignment);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, 0);
      ColumnIndexType nColumns;
      std::_tstring strSegmentLabel;
      std::_tostringstream os;
      if ( nSegments == 1 )
      {
         os << _T("Span ") << LABEL_SPAN(grpIdx) << std::endl;
         strSegmentLabel = _T("Girder");
         nColumns = 19;
      }
      else
      {
         os << _T("Group ") << LABEL_GROUP(grpIdx) << std::endl;
         strSegmentLabel = _T("Segment");
         nColumns = 20;
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns,os.str().c_str());

      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);

      ColumnIndexType col = 0;

      // header row 1 and first one (or two) columns
      if ( 1 < nSegments )
      {
         pTable->SetRowSpan(0, col, 3);  // span 3 rows
         (*pTable)(0,col++) << _T("Girder");
      }

      pTable->SetRowSpan(0, col, 3);  // span 3 rows
      (*pTable)(0,col++)  << strSegmentLabel;


      pTable->SetColumnSpan(0,col,9);
      (*pTable)(0, col) << _T("Start of ") << strSegmentLabel;

      pTable->SetColumnSpan(1, col, 3);
      if (1 < nSegments)
      {
         (*pTable)(1, col) << _T("Support Line");
      }
      else if (grpIdx == 0)
      {
         (*pTable)(1, col) << _T("Abutment Line");
      }
      else
      {
         (*pTable)(1, col) << _T("Pier Line");
      }

      (*pTable)(2, col++) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2, col++) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1, col, 3);
      (*pTable)(1, col) << strSegmentLabel << _T(" End");
      (*pTable)(2, col++) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2, col++) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1, col, 3);
      (*pTable)(1, col) << _T("CL Bearing");
      (*pTable)(2, col++) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2, col++) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());



      pTable->SetColumnSpan(0, col, 9);
      (*pTable)(0,col)  << _T("End of ") << strSegmentLabel;

      pTable->SetColumnSpan(1, col, 3);
      (*pTable)(1,col) << _T("CL Bearing");
      (*pTable)(2, col++) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2, col++) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1,col,3);
      (*pTable)(1, col) << strSegmentLabel << _T(" End");
      (*pTable)(2, col++) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2, col++) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1, col, 3);
      if (1 < nSegments)
      {
         (*pTable)(1, col) << _T("Reference Line");
      }
      else if (grpIdx == nGroups-1)
      {
         (*pTable)(1,col) << _T("Abutment Line");
      }
      else
      {
         (*pTable)(1,col) << _T("Pier Line");
      }
      (*pTable)(2, col++) << _T("East") << rptNewLine << _T("(X)");
      (*pTable)(2, col++) << _T("North") << rptNewLine << _T("(Y)");
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());


      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         ColumnIndexType col = 0;
         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         if (nSegments == 1)
         {
            (*pTable)(row,col++) << LABEL_GIRDER(gdrIdx);
         }
         else
         {
            (*pTable)(row,col) << LABEL_GIRDER(gdrIdx);
            pTable->SetRowSpan(row,col,nSegments);
            col++;
         }

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            if ( segIdx != 0 )
            {
               col = 1;
            }

            CSegmentKey segmentKey(girderKey,segIdx);

            CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
            pGdr->GetSegmentEndPoints(segmentKey,pgsTypes::pcGlobal,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

            if ( 1 < nSegments)
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            Float64 x,y;

            pntPier1->get_X(&x);
            pntPier1->get_Y(&y);
            (*pTable)(row,col++) << cogoPoint.SetValue(x);
            (*pTable)(row,col++) << cogoPoint.SetValue(y);

            Float64 station, offset, elev;
            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntPier1,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pntEnd1->get_X(&x);
            pntEnd1->get_Y(&y);
            (*pTable)(row,col++) << cogoPoint.SetValue(x);
            (*pTable)(row,col++) << cogoPoint.SetValue(y);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntEnd1,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pntBrg1->get_X(&x);
            pntBrg1->get_Y(&y);
            (*pTable)(row,col++) << cogoPoint.SetValue(x);
            (*pTable)(row,col++) << cogoPoint.SetValue(y);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntBrg1,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pntBrg2->get_X(&x);
            pntBrg2->get_Y(&y);
            (*pTable)(row,col++) << cogoPoint.SetValue(x);
            (*pTable)(row,col++) << cogoPoint.SetValue(y);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntBrg2,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pntEnd2->get_X(&x);
            pntEnd2->get_Y(&y);
            (*pTable)(row,col++) << cogoPoint.SetValue(x);
            (*pTable)(row,col++) << cogoPoint.SetValue(y);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntEnd2,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pntPier2->get_X(&x);
            pntPier2->get_Y(&y);
            (*pTable)(row,col++) << cogoPoint.SetValue(x);
            (*pTable)(row,col++) << cogoPoint.SetValue(y);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntPier2,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            row++;
         } // next segment
      } // next girder
   }// next group
}

void girder_offsets(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Girder Offsets");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), false );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IGirder,pGdr);
   GET_IFACE2(pBroker, IRoadway,pAlignment);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, 0);
      ColumnIndexType nColumns;
      std::_tstring strSegmentLabel;
      std::_tostringstream os;
      if ( nSegments == 1)
      {
         os << _T("Span ") << LABEL_SPAN(grpIdx) << std::endl;
         strSegmentLabel = _T("Girder");
         nColumns = 19;
      }
      else
      {
         os << _T("Group ") << LABEL_GROUP(grpIdx) << std::endl;
         strSegmentLabel = _T("Segment");
         nColumns = 20;
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns,os.str().c_str());

      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);

      ColumnIndexType col = 0;

      // header row 1 and first one (or two) columns
      if ( 1 < nSegments )
      {
         pTable->SetRowSpan(0, col, 3);  // span 3 rows
         (*pTable)(0,col++) << _T("Girder");
      }

      pTable->SetRowSpan(0, col, 3);  // span 3 rows
      (*pTable)(0,col++)  << strSegmentLabel;

      pTable->SetColumnSpan(0, col, 9);
      (*pTable)(0,col)  << _T("Start of ") << strSegmentLabel;

      pTable->SetColumnSpan(1, col, 3);
      if (1 < nSegments)
      {
         (*pTable)(1, col) << _T("Reference Line");
      }
      else if (grpIdx == 0)
      {
         (*pTable)(1, col) << _T("Abutment Line");
      }
      else
      {
         (*pTable)(1, col) << _T("Pier Line");
      }
      (*pTable)(2, col++) << _T("Station");
      (*pTable)(2, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());


      pTable->SetColumnSpan(1,col, 3);
      (*pTable)(1, col) << strSegmentLabel << _T(" End");
      (*pTable)(2, col++) << _T("Station");
      (*pTable)(2, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1, col, 3);
      (*pTable)(1, col) << _T("CL Bearing");
      (*pTable)(2, col++) << _T("Station");
      (*pTable)(2, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());



      pTable->SetColumnSpan(0, col, 9);
      (*pTable)(0,col)  << _T("End of ") << strSegmentLabel;

      pTable->SetColumnSpan(1, col, 3);
      (*pTable)(1, col) << _T("CL Bearing");
      (*pTable)(2, col++) << _T("Station");
      (*pTable)(2, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1, col, 3);
      (*pTable)(1, col) << strSegmentLabel << _T(" End");
      (*pTable)(2, col++) << _T("Station");
      (*pTable)(2, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      pTable->SetColumnSpan(1, col, 3);
      if (1 < nSegments)
      {
         (*pTable)(1, col) << _T("Reference Line");
      }
      else if (grpIdx == nGroups-1)
      {
         (*pTable)(1, col) << _T("Abutment Line");
      }
      else
      {
         (*pTable)(1, col) << _T("Pier Line");
      }
      (*pTable)(2, col++) << _T("Station");
      (*pTable)(2, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(2, col++) << COLHDR(_T("Deck Elev"), rptLengthUnitTag, pDisplayUnits->GetAlignmentLengthUnit());

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         ColumnIndexType col = 0;

         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         if ( nSegments == 1 )
         {
            (*pTable)(row,col++) << LABEL_GIRDER(gdrIdx);
         }
         else
         {
            (*pTable)(row,col) << LABEL_GIRDER(gdrIdx);
            pTable->SetRowSpan(row,col++,nSegments);
         }

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            if ( segIdx != 0 )
            {
               col = 1;
            }

            CSegmentKey segmentKey(girderKey,segIdx);

            CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
            pGdr->GetSegmentEndPoints(segmentKey,pgsTypes::pcGlobal,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

            if ( 1 < nSegments )
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            Float64 x,y;
            Float64 station, offset, elev;

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntPier1,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);

            pntPier1->get_X(&x);
            pntPier1->get_Y(&y);
            (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
            (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntEnd1,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);

            pntEnd1->get_X(&x);
            pntEnd1->get_Y(&y);
            (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
            (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntBrg1,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);

            pntBrg1->get_X(&x);
            pntBrg1->get_Y(&y);
            (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
            (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntBrg2,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);

            pntBrg2->get_X(&x);
            pntBrg2->get_Y(&y);
            (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
            (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntEnd2,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);

            pntEnd2->get_X(&x);
            pntEnd2->get_Y(&y);
            (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
            (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            pAlignment->GetStationAndOffset(pgsTypes::pcGlobal,pntPier2,&station,&offset);
            elev = pAlignment->GetElevation(station,offset);

            pntPier2->get_X(&x);
            pntPier2->get_Y(&y);
            (*pTable)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );
            (*pTable)(row,col++) << RPT_OFFSET(offset,cogoPoint);
            (*pTable)(row,col++) << cogoPoint.SetValue(elev);

            row++;
         } // next segment
      } // next girder
   } // next group
}

void girder_lengths(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, IDocumentType, pDocType);

   GET_IFACE2(pBroker, IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   if ( pDocType->IsPGSuperDocument() )
   {
      *pPara << _T("Girder Lengths");
   }
   else
   {
      *pPara << _T("Segment Lengths");
   }
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   if (pDocType->IsPGSuperDocument())
   {
      *pPara << _T("C-C Pier = Abutment/Pier Line to Abutment/Pier Line length measured along the girder") << rptNewLine;
      *pPara << _T("C-C Bearing = Centerline bearing to centerline bearing length measured along the girder centerline") << rptNewLine;
      *pPara << _T("Girder Length, Plan = End to end length of the girder projected into a horizontal plane") << rptNewLine;
      *pPara << _T("Girder Length, Along Grade = End to end length of girder measured along grade of the girder (slope adjusted) = ") << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("SlopeAdjustedGirderLength.png"),rptRcImage::Middle) << rptNewLine;
   }
   else
   {
      *pPara << _T("C-C Support = Abutment/Pier/Temp Support Line to Abutment/Pier/Temp Support Line length measured along the segment") << rptNewLine;
      *pPara << _T("C-C Bearing = Centerline bearing to centerline bearing length measured along the segment") << rptNewLine;
      *pPara << _T("Segment Length, Plan = End to end length of the segment projected into a horizontal plane") << rptNewLine;
      *pPara << _T("Segment Length, Along Grade = End to end length of segment measured along grade of the segment (slope adjusted) = ") << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("SlopeAdjustedGirderLength.png"),rptRcImage::Middle) << rptNewLine;
   }
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

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      ColumnIndexType nColumns;
      std::_tstring strSegmentLabel;
      std::_tostringstream os;
      if (pDocType->IsPGSuperDocument())
      {
         os << _T("Span ") << LABEL_SPAN(grpIdx) << std::endl;
         strSegmentLabel = _T("Girder");
         nColumns = 7;
      }
      else
      {
         os << _T("Group ") << LABEL_GROUP(grpIdx) << std::endl;
         strSegmentLabel = _T("Segment");
         nColumns = 8;
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns,os.str().c_str());
      pTable->SetNumberOfHeaderRows(2);

      (*pPara) << pTable << rptNewLine;

      ColumnIndexType col = 0;

      if (pDocType->IsPGSpliceDocument())
      {
         pTable->SetRowSpan(0, col, 2);  // span 2 rows
         (*pTable)(0,col++) << _T("Girder");
      }

      pTable->SetRowSpan(0,col,2);
      (*pTable)(0,col++) << strSegmentLabel;

      pTable->SetRowSpan(0,col,2);
      if (pDocType->IsPGSuperDocument())
      {
         (*pTable)(0, col++) << COLHDR(_T("C-C Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      }
      else
      {
         (*pTable)(0, col++) << COLHDR(_T("C-C Support"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      }

      pTable->SetRowSpan(0,col,2);
      (*pTable)(0,col++) << COLHDR(_T("C-C Bearing"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col) << strSegmentLabel << _T(" Length");
      (*pTable)(1,col++) << COLHDR(_T("Plan"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(1,col++) << COLHDR(_T("Along") << rptNewLine << _T("Grade"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(0,col,2);
      (*pTable)(0,col++) << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

      pTable->SetRowSpan(0,col,2);
      (*pTable)(0,col++) << _T("Direction");

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         ColumnIndexType col = 0;

         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         if (pDocType->IsPGSuperDocument())
         {
            (*pTable)(row,col++) << LABEL_GIRDER(gdrIdx);
         }
         else
         {
            pTable->SetRowSpan(row, col, nSegments);
            (*pTable)(row,col++) << LABEL_GIRDER(gdrIdx);
         }

         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            if ( segIdx != 0 )
            {
               col = 1;
            }
            CSegmentKey segmentKey(girderKey,segIdx);

            if ( pDocType->IsPGSpliceDocument() )
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            Float64 L = pBridge->GetSegmentLayoutLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            L = pBridge->GetSegmentSpanLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            L = pBridge->GetSegmentLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            L = pBridge->GetSegmentPlanLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            Float64 slope = pBridge->GetSegmentSlope(segmentKey);
            (*pTable)(row,col++) << scalar.SetValue(slope);

            CComPtr<IDirection> gdr_bearing;
            pBridge->GetSegmentBearing(segmentKey,&gdr_bearing);
            Float64 gdr_bearing_value;
            gdr_bearing->get_Value(&gdr_bearing_value);

            CComBSTR bstrBearing;
            direction_formatter->Format(gdr_bearing_value,CComBSTR("°,\',\""),&bstrBearing);

            (*pTable)(row,col++) << RPT_BEARING(OLE2T(bstrBearing));

            row++;
         } // next segment
      } // next girder
   } // next group
}

void girder_spacing(IBroker*pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   USES_CONVERSION;

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Girder Spacing");
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, spacingValue, pDisplayUnits->GetXSectionDimUnit(), false );

   CComPtr<IAngleDisplayUnitFormatter> angle_formatter;
   angle_formatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
   angle_formatter->put_Signed(VARIANT_TRUE);

   GET_IFACE2(pBroker, IBridge,pBridge);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();
   std::_tstring strSegmentLabel(nSpans == nGroups ? _T("Girder") : _T("Segment"));
   std::_tstring strSupportLabel(nSpans == nGroups ? _T("Pier") : _T("Support"));

   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);

      SegmentIndexType nSegments = pBridge->GetSegmentCount( CGirderKey(grpIdx,0) );
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         std::_tostringstream os;

         if ( nSpans == nGroups )
         {
            os << _T("Span ") << LABEL_SPAN(grpIdx) << std::endl;
         }
         else
         {
            os << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx) << std::endl;
         }
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(11,os.str().c_str());

         (*pPara) << pTable << rptNewLine;

         pTable->SetNumberOfHeaderRows(3);

         pTable->SetRowSpan(0, 0, 3);  // span 3 rows
         (*pTable)(0,0)  << _T("Girder");

         pTable->SetColumnSpan(0,1,5);
         (*pTable)(0, 1) << _T("Start of ") << strSegmentLabel;

         pTable->SetColumnSpan(0,6,5);
         (*pTable)(0, 6) << _T("End of ") << strSegmentLabel;

         pTable->SetColumnSpan(1,1,2);
         (*pTable)(1, 1) << _T("Spacing at CL ") << strSupportLabel;

         pTable->SetColumnSpan(1,3,2);
         (*pTable)(1, 3) << _T("Spacing at CL Brg");

         pTable->SetRowSpan(1,5,2);
         (*pTable)(1, 5) << _T("Angle") << rptNewLine << _T("with") << rptNewLine << _T("CL ") << strSupportLabel;

         pTable->SetColumnSpan(1,6,2);
         (*pTable)(1, 6) << _T("Spacing at CL Brg");

         pTable->SetColumnSpan(1,8,2);
         (*pTable)(1, 8) << _T("Spacing at CL ") << strSupportLabel;

         pTable->SetRowSpan(1,10,2);
         (*pTable)(1, 10) << _T("Angle") << rptNewLine << _T("with") << rptNewLine << _T("CL ") << strSupportLabel;

         (*pTable)(2,1)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
         (*pTable)(2,2)  << COLHDR(_T("Along CL ") << strSupportLabel,rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
         (*pTable)(2,3)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
         (*pTable)(2,4)  << COLHDR(_T("Along CL Brg"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());

         (*pTable)(2,6)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
         (*pTable)(2,7)  << COLHDR(_T("Along CL Brg"),rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
         (*pTable)(2,8)  << COLHDR(symbol(NORMAL) << _T(" to Alignment"),  rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());
         (*pTable)(2,9)  << COLHDR(_T("Along CL ") << strSupportLabel,rptLengthUnitTag,pDisplayUnits->GetXSectionDimUnit());

         RowIndexType row = pTable->GetNumberOfHeaderRows();

         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            CSegmentKey segmentKey(grpIdx, gdrIdx, segIdx);

#pragma Reminder("UPDATE: spacing is measured at ends of segments")
            // need to get piers at the ends of the segmetns
            // could be a pier or temp support
            // then need to get spacing at pier or TS

            PierIndexType prevPierIdx, nextPierIdx;
            pBridge->GetGirderGroupPiers(grpIdx, &prevPierIdx, &nextPierIdx);

            std::vector<Float64> spacing[8];
            if (1 < nGirders)
            {
               spacing[0] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtPierLine, pgsTypes::NormalToItem);
               spacing[1] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtPierLine, pgsTypes::AlongItem);
               spacing[2] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtCenterlineBearing, pgsTypes::NormalToItem);
               spacing[3] = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtCenterlineBearing, pgsTypes::AlongItem);
               spacing[4] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back, pgsTypes::AtCenterlineBearing, pgsTypes::NormalToItem);
               spacing[5] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back, pgsTypes::AtCenterlineBearing, pgsTypes::AlongItem);
               spacing[6] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back, pgsTypes::AtPierLine, pgsTypes::NormalToItem);
               spacing[7] = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back, pgsTypes::AtPierLine, pgsTypes::AlongItem);
            }

            CComPtr<IAngle> objStartAngle, objEndAngle;
            pBridge->GetSegmentAngle(segmentKey, pgsTypes::metStart, &objStartAngle);
            pBridge->GetSegmentAngle(segmentKey, pgsTypes::metEnd, &objEndAngle);

            Float64 startAngle;
            objStartAngle->get_Value(&startAngle);
            if (M_PI <= startAngle)
            {
               startAngle -= M_PI;
            }

            Float64 endAngle;
            objEndAngle->get_Value(&endAngle);
            if (M_PI <= endAngle)
            {
               endAngle -= M_PI;
            }

            (*pTable)(row,0) << LABEL_GIRDER(gdrIdx);

            // girder/pier angle
            (*pTable)(row,1) << _T("");
            (*pTable)(row,2) << _T("");
            (*pTable)(row,3) << _T("");
            (*pTable)(row,4) << _T("");


            CComBSTR bstrStartAngle;
            angle_formatter->Format(startAngle,CComBSTR("°,\',\""),&bstrStartAngle);
            (*pTable)(row,5) << RPT_ANGLE(OLE2T(bstrStartAngle));

            (*pTable)(row,6) << _T("");
            (*pTable)(row,7) << _T("");
            (*pTable)(row,8) << _T("");
            (*pTable)(row,9) << _T("");

            CComBSTR bstrEndAngle;
            angle_formatter->Format(endAngle,CComBSTR("°,\',\""),&bstrEndAngle);
            (*pTable)(row,10) << RPT_ANGLE(OLE2T(bstrEndAngle));

            // girder spacing (between girders)
            if ( gdrIdx < nGirders-1 )
            {
               row++;

               (*pTable)(row,0) << _T("");

               (*pTable)(row,1) << spacingValue.SetValue(spacing[0][gdrIdx]);
               (*pTable)(row,2) << spacingValue.SetValue(spacing[1][gdrIdx]);
               (*pTable)(row,3) << spacingValue.SetValue(spacing[2][gdrIdx]);
               (*pTable)(row,4) << spacingValue.SetValue(spacing[3][gdrIdx]);

               (*pTable)(row,5) << _T("");

               (*pTable)(row,6) << spacingValue.SetValue(spacing[4][gdrIdx]);
               (*pTable)(row,7) << spacingValue.SetValue(spacing[5][gdrIdx]);
               (*pTable)(row,8) << spacingValue.SetValue(spacing[6][gdrIdx]);
               (*pTable)(row,9) << spacingValue.SetValue(spacing[7][gdrIdx]);

               (*pTable)(row,10) << _T("");
            }

            row++;
         }
      }
   }

   (*pPara) << symbol(NORMAL) << _T(" to Alignment: spacing is measured along a line that is normal to the alignment at the CL ") << strSupportLabel << _T(" and passes through the point where the CL ") << strSupportLabel << _T(" or CL Brg intersect the alignment.") << rptNewLine;
}


void girder_ends(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker, IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GET_IFACE2(pBroker, IDocumentType, pDocType);

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   if ( pDocType->IsPGSuperDocument() )
   {
      *pPara << _T("Girder End Distances");
   }
   else
   {
      *pPara << _T("Segment End Distances");
   }
   (*pChapter) << pPara;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,  pDisplayUnits->GetSpanLengthUnit(), false );

   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(grpIdx, 0);
      ColumnIndexType nColumns;
      std::_tstring strSegmentLabel;
      std::_tstring strSupportLabel;
      std::_tostringstream os;
      if (nSegments == 1)
      {
         os << _T("Span ") << LABEL_SPAN(grpIdx) << std::endl;
         strSegmentLabel = _T("Girder");
         strSupportLabel = _T("Pier");
         nColumns = 11;
      }
      else
      {
         os << _T("Group ") << LABEL_GROUP(grpIdx) << std::endl;
         strSegmentLabel = _T("Segment");
         strSupportLabel = _T("Support");
         nColumns = 12;
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nColumns,os.str().c_str());
      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);

      ColumnIndexType col = 0;
      // header row 1 and first one (or two) columns
      if (1 < nSegments)
      {
         pTable->SetRowSpan(0, col, 3);  // span 3 rows
         (*pTable)(0,col++) << _T("Girder");
      }

      pTable->SetRowSpan(0, col, 3);  // span 3 rows
      (*pTable)(0,col++)  << strSegmentLabel;

      pTable->SetColumnSpan(0,col,5);
      (*pTable)(0,col) << _T("Start of ") << strSegmentLabel;
      col += 5;

      pTable->SetColumnSpan(0,col,5);
      (*pTable)(0,col) << _T("End of ") << strSegmentLabel;
      col += 5;

      // header rows 2 and 3
      col = 1;
      if (1 < nSegments)
      {
         col++;
      }
      pTable->SetColumnSpan(1,col,2);
      (*pTable)(1,col) << _T("CL ") << strSupportLabel << _T(" to CL Brg");
      (*pTable)(2,col++) << COLHDR(symbol(NORMAL) << _T(" to ") << strSupportLabel, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      if ( nSegments == 1 )
      {
         (*pTable)(2,col++) << COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*pTable)(2,col++) << COLHDR(_T("Along Segment"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      pTable->SetColumnSpan(1,col,2);
      if ( nSegments == 1 )
      {
         (*pTable)(1,col) << _T("CL ") << strSupportLabel << _T(" to Girder End");
      }
      else
      {
         (*pTable)(1,col) << _T("CL ") << strSupportLabel << _T(" to Segment End");
      }

      (*pTable)(2,col++) << COLHDR(symbol(NORMAL) << _T(" to ") << strSupportLabel, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      if ( nSegments == 1 )
      {
         (*pTable)(2,col++) << COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*pTable)(2,col++) << COLHDR(_T("Along Segment"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      pTable->SetRowSpan(1,col,2);
      if (nSegments == 1)
      {
         (*pTable)(1,col++)<< COLHDR(_T("CL Brg to Girder End") << rptNewLine << _T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*pTable)(1,col++)<< COLHDR(_T("CL Brg to Segment End") << rptNewLine << _T("Along Segment"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }


      pTable->SetColumnSpan(1,col,2);
      (*pTable)(1,col) << _T("CL ") << strSupportLabel << _T(" to CL Brg");
      (*pTable)(2,col++) << COLHDR(symbol(NORMAL) << _T(" to ") << strSupportLabel, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      if ( nSegments == 1 )
      {
         (*pTable)(2,col++) << COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*pTable)(2,col++) << COLHDR(_T("Along Segment"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      pTable->SetColumnSpan(1,col,2);
      (*pTable)(1,col) << _T("CL ") << strSupportLabel << _T(" to ") << strSegmentLabel << _T(" End");
      (*pTable)(2,col++) << COLHDR(symbol(NORMAL) << _T(" to ") << strSupportLabel, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      if ( nSegments == 1 )
      {
         (*pTable)(2,col++)<< COLHDR(_T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*pTable)(2,col++)<< COLHDR(_T("Along Segment"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      pTable->SetRowSpan(1,col,2);
      if ( nSegments == 1 )
      {
         (*pTable)(1,col)<< COLHDR(_T("CL Brg to Girder End") << rptNewLine << _T("Along Girder"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }
      else
      {
         (*pTable)(1,col)<< COLHDR(_T("CL Brg to Segment End") << rptNewLine << _T("Along Segment"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         ColumnIndexType col = 0;

         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         if ( nSegments == 1 )
         {
            (*pTable)(row,col++) << LABEL_GIRDER(gdrIdx);
         }
         else
         {
            (*pTable)(row,col) << LABEL_GIRDER(gdrIdx);
            pTable->SetRowSpan(row,col++,nSegments);
         }

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            if ( segIdx != 0 )
            {
               col = 1;
            }
            CSegmentKey segmentKey(girderKey,segIdx);

            if ( 1 < nSegments )
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(segmentKey,pgsTypes::metStart,pgsTypes::NormalToItem) );
            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(segmentKey,pgsTypes::metStart,pgsTypes::AlongItem) );

            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToSegmentEndDistance(segmentKey,pgsTypes::metStart,pgsTypes::NormalToItem) );
            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToSegmentEndDistance(segmentKey,pgsTypes::metStart,pgsTypes::AlongItem) );

            (*pTable)(row,col++) << length.SetValue( pBridge->GetSegmentStartEndDistance(segmentKey) );

            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(segmentKey,pgsTypes::metEnd,pgsTypes::NormalToItem) );
            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToCLBearingDistance(segmentKey,pgsTypes::metEnd,pgsTypes::AlongItem) );

            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToSegmentEndDistance(segmentKey,pgsTypes::metEnd,pgsTypes::NormalToItem) );
            (*pTable)(row,col++) << length.SetValue( pBridge->GetCLPierToSegmentEndDistance(segmentKey,pgsTypes::metEnd,pgsTypes::AlongItem) );

            (*pTable)(row,col++) << length.SetValue( pBridge->GetSegmentEndEndDistance(segmentKey) );

            row++;
         } // next segment
      } // next girder
   } // next group
}
