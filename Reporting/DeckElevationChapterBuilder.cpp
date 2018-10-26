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
#include <Reporting\DeckElevationChapterBuilder.h>

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

/****************************************************************************
CLASS
   CDeckElevationChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDeckElevationChapterBuilder::CDeckElevationChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDeckElevationChapterBuilder::GetName() const
{
   return TEXT("Deck Elevations");
}

rptChapter* CDeckElevationChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CBrokerReportSpecification* pSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pPara << "Deck Elevations over Girder Webs" << rptNewLine;
   (*pChapter) << pPara;

   pPara = new rptParagraph();
   (*pChapter) << pPara;
   *pPara << Bold("Notes") << rptNewLine;
   *pPara << "Web Offsets are measured from and normal to the centerline girder" << rptNewLine;
   *pPara << "Station, normal offset, and deck elevations are given for 10th points between bearings" << rptNewLine;

   //
   // Bridge Elevation Table
   //

   INIT_UV_PROTOTYPE( rptLengthSectionValue, webDim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, cogoPoint, pDisplayUnits->GetAlignmentLengthUnit(), true );

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IPointOfInterest, pPOI);
   GET_IFACE2(pBroker, IGirder, pGirder );

   RowIndexType row_step = 4; // number of rows reported for each web
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for ( SpanIndexType span = 0; span < nSpans; span++ )
   {
      std::ostringstream os;
      os << "Span " << LABEL_SPAN(span) << std::endl;
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(14,os.str().c_str());
      (*pPara) << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(1);
      pTable->SetNumberOfStripedRows(row_step); // stripe in bands of row_step

      ColumnIndexType col = 0;
      for ( col = 0; col < pTable->GetNumberOfColumns(); col++ )
      {
         pTable->SetColumnStyle(col,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
         pTable->SetStripeRowColumnStyle(col,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }

      col = 0;
      (*pTable)(0,col++) << "Girder";
      (*pTable)(0,col++) << "Web";
      (*pTable)(0,col++) << "";
      (*pTable)(0,col++) << "CL Brg";
      (*pTable)(0,col++) << Sub2("0.1L","s");
      (*pTable)(0,col++) << Sub2("0.2L","s");
      (*pTable)(0,col++) << Sub2("0.3L","s");
      (*pTable)(0,col++) << Sub2("0.4L","s");
      (*pTable)(0,col++) << Sub2("0.5L","s");
      (*pTable)(0,col++) << Sub2("0.6L","s");
      (*pTable)(0,col++) << Sub2("0.7L","s");
      (*pTable)(0,col++) << Sub2("0.8L","s");
      (*pTable)(0,col++) << Sub2("0.9L","s");
      (*pTable)(0,col++) << "CL Brg";

      RowIndexType row = pTable->GetNumberOfHeaderRows();
      col = 0;

      GirderIndexType nGdrs = pBridge->GetGirderCount(span);
      for ( GirderIndexType gdr = 0; gdr < nGdrs; gdr++ )
      {
         Float64 length = pBridge->GetSpanLength(span,gdr);
         Float64 brgStart = pBridge->GetGirderStartBearingOffset(span,gdr);
         Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

         std::vector<pgsPointOfInterest> vPoi = pPOI->GetTenthPointPOIs( pgsTypes::BridgeSite3, span, gdr );

         MatingSurfaceIndexType nWebs = pGirder->GetNumberOfMatingSurfaces(span,gdr);
         pTable->SetRowSpan(row,0,Int16(row_step*nWebs));

         RowIndexType r;
         for ( r = row+1; r < row+row_step*nWebs; r++ )
         {
            pTable->SetRowSpan(r,0,-1);
            pTable->SetRowSpan(r,1,-1);
         }

         (*pTable)(row,0) << LABEL_GIRDER(gdr);

         for ( MatingSurfaceIndexType web = 0; web < nWebs; web++ )
         {
            col = 1;

            pTable->SetRowSpan(row,col,row_step);
            for ( r = row+1; r < row+row_step; r++ )
            {
               pTable->SetRowSpan(r,col,-1);
            }

            (*pTable)(row,col++) << (web+1) << rptNewLine;

            (*pTable)(row,col)   << Bold("Web Offset (") << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag() ) << Bold(")");
            (*pTable)(row+1,col) << Bold("Station");
            (*pTable)(row+2,col) << Bold("Offset (") << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag() ) << Bold(")");
            (*pTable)(row+3,col) << Bold("Elev (") << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag() ) << Bold(")");
            
            col++;

            for ( std::vector<pgsPointOfInterest>::iterator iter = vPoi.begin(); iter != vPoi.end(); iter++ )
            {
               pgsPointOfInterest& poi = *iter;

               // offset to centerline of web, measured from centerline of girder
               // < 0 = left of centerline
               // in a nonprismatic member, web offset can  very with location
               Float64 webOffset = pGirder->GetMatingSurfaceLocation(poi,web);

               Float64 location = poi.GetDistFromStart() + brgStart - end_size;

               Float64 sta, offset;
               pBridge->GetStationAndOffset(span,gdr,location,&sta,&offset);

               Float64 total_offset = offset + webOffset;
               Float64 elev = pAlignment->GetElevation(sta,total_offset);

               (*pTable)(row,col)   << RPT_OFFSET(webOffset,dist);
               (*pTable)(row+1,col) << rptRcStation(sta, &pDisplayUnits->GetStationFormat() );
               (*pTable)(row+2,col) << RPT_OFFSET(total_offset,dist);
               (*pTable)(row+3,col) << dist.SetValue(elev);

               col++;
            }

            row += row_step;
         }
      }
   }

   return pChapter;
}


CChapterBuilder* CDeckElevationChapterBuilder::Clone() const
{
   return new CDeckElevationChapterBuilder;
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
