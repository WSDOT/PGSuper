///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

/****************************************************************************
CLASS
   CDeckElevationsChapterBuilder
****************************************************************************/

#include "DeckElevationsChapterBuilder.h"
#include <PgsExt\ReportStyleHolder.h>

#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>


#include <WBFLCogo.h>

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDeckElevationsChapterBuilder::CDeckElevationsChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDeckElevationsChapterBuilder::GetName() const
{
   return TEXT("Deck Elevations");
}

Uint32 CDeckElevationsChapterBuilder::GetMaxLevel() const
{
   return 1;
}

rptChapter* CDeckElevationsChapterBuilder::Build(IBroker* pBroker,Int16 /*span*/,Int16 /*girder*/,
                                       IDisplayUnits* pDispUnit,
                                       Int16 level) const
{
   USES_CONVERSION;

   // This report does not use the passd span and girder parameters
   rptChapter* pChapter = CChapterBuilder::Build(pBroker,0/*span*/,0/*girder*/,pDispUnit,level);

   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   //
   // Bridge Elevation Table
   //

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(12,"Elevations along Girder Lines");
   for ( int i = 0; i < 12; i++ )
   {
      pTable->SetColumnStyle(i, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
      pTable->SetStripeRowColumnStyle(i, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT ) );
   }

   (*pPara) << pTable;


   (*pTable)(0,0)  << "Girder";
   (*pTable)(0,1)  << "Intersection of CL Brg/CL Girder at Start of Span";
   (*pTable)(0,2)  << Sub2("0.1L","s");
   (*pTable)(0,3)  << Sub2("0.2L","s");
   (*pTable)(0,4)  << Sub2("0.3L","s");
   (*pTable)(0,5)  << Sub2("0.4L","s");
   (*pTable)(0,6)  << Sub2("0.5L","s");
   (*pTable)(0,7)  << Sub2("0.6L","s");
   (*pTable)(0,8)  << Sub2("0.7L","s");
   (*pTable)(0,9)  << Sub2("0.8L","s");
   (*pTable)(0,10) << Sub2("0.9L","s");
   (*pTable)(0,11) << "Intersection of CL Brg/CL Girder at End of Span";

   INIT_UV_PROTOTYPE( rptLengthSectionValue, dist, pDispUnit->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, haunch, pDispUnit->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, cogoPoint, pDispUnit->GetAlignmentLengthUnit(), true );


   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   GET_IFACE2(pBroker, IBridge,pBridge);
   GET_IFACE2(pBroker, IRoadway, pAlignment);
   GET_IFACE2(pBroker, IPointOfInterest, pPOI);
   GET_IFACE2(pBroker,IGirder,pGdr);

   long row = pTable->GetNumberOfHeaderRows();

   Uint16 nSpans = pBridge->GetSpanCount();
   for ( Uint16 span = 0; span < nSpans; span++ )
   {
      Uint16 nGdrs = pBridge->GetGirderCount(span);
      for ( Uint16 gdr = 0; gdr < nGdrs; gdr++ )
      {
         Float64 length = pBridge->GetSpanLength(span,gdr);
         Float64 brgStart = pBridge->GetGirderStartBearingOffset(span,gdr);
         Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

         CComPtr<IDirection> gdr_bearing;
         pBridge->GetGirderBearing(span,gdr,&gdr_bearing);
         double gdr_bearing_value;
         gdr_bearing->get_Value(&gdr_bearing_value);

         CComBSTR bstrBearing;
         direction_formatter->Format(gdr_bearing_value,CComBSTR("°,\',\""),&bstrBearing);

         std::vector<pgsPointOfInterest> vPoi = pPOI->GetTenthPointPOIs( pgsTypes::BridgeSite3, span, gdr );
//         ATLASSERT(vPoi.size() == 11); // 

         (*pTable)(row,0) << bold(ON) << "Span " << (Int16)(span+1) << " "
                          << "Girder " << (char)(gdr+'A') << bold(OFF) << rptNewLine;
         (*pTable)(row,0) << "Bearing: " << OLE2A(bstrBearing) << rptNewLine;
         (*pTable)(row,0) << "Brg-Brg: " << dist.SetValue(length) << rptNewLine;

         GET_IFACE2(pBroker, IGirder, pGirder );
         Int32 nWebs = pGirder->GetNumberOfMatingSurfaces(span,gdr);
         for ( Int32 web = 0; web < nWebs; web++ )
         {
            // offset to centerline of web, measured from centerline of girder
            // < 0 = left of centerline
#pragma Reminder("UPDATE: Assuming prismatic section")
            // in a nonprismatic member, web offset can  very with location
            Float64 webOffset = pGirder->GetMatingSurfaceLocation(pgsPointOfInterest(span,gdr,0.00),web);
            char* dirWeb;
            dirWeb = (IsZero(webOffset) ? "" : ((webOffset < 0) ? "(L)" : "(R)"));

            (*pTable)(row,0) << "Web: " << long(web+1) << rptNewLine;
            (*pTable)(row,0) << "CLGdr Offset: " << dist.SetValue(fabs(webOffset)) << dirWeb << rptNewLine;

            long k = 0;
            for ( std::vector<pgsPointOfInterest>::iterator iter = vPoi.begin(); iter != vPoi.end(); iter++, k++ )
            {
               pgsPointOfInterest& poi = *iter;

               Float64 location = poi.GetDistFromStart() + brgStart - end_size;
               Float64 sta, offset;

               pBridge->GetStationAndOffset(span,gdr,location,&sta,&offset);

               Float64 total_offset = offset + webOffset;
               Float64 elev;
               elev = pAlignment->GetElevation(sta,total_offset);

               Float64 top_gdr_elev = pGdr->GetTopGirderElevation(poi,web);

               char* dirGdr;
               dirGdr = (IsZero(total_offset) ? "" : ((total_offset < 0) ? "(L)" : "(R)"));
               total_offset = fabs(total_offset);

               (*pTable)(row,k+1) << "Station: " << rptRcStation(sta, &pDispUnit->GetStationFormat() ) << rptNewLine;
               (*pTable)(row,k+1) << "Offset: " << dist.SetValue(total_offset) << dirGdr << rptNewLine;
               (*pTable)(row,k+1) << "Deck Elev: " << dist.SetValue(elev) << rptNewLine;
               (*pTable)(row,k+1) << "Top Gdr Elev: " << dist.SetValue( top_gdr_elev ) << rptNewLine;
            }

            row++;
         }
      }
   }

   return pChapter;
}


CChapterBuilder* CDeckElevationsChapterBuilder::Clone() const
{
   return new CDeckElevationsChapterBuilder;
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
