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
#include <Reporting\PointOfInterestChapterBuilder.h>
#include <IFace\PointOfInterest.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPointOfInterestChapterBuilder::CPointOfInterestChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CPointOfInterestChapterBuilder::GetName() const
{
   return TEXT("Points of Interest");
}

rptChapter* CPointOfInterestChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderLineReportSpecification* pGdrRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits, pDisplayUnits );
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   ReportPoi(_T("All"),     0,                    pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);
   ReportPoi(_T("Release"), POI_RELEASED_SEGMENT, pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);
   ReportPoi(_T("Lifting"), POI_LIFT_SEGMENT,     pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);
   ReportPoi(_T("Storage"), POI_STORAGE_SEGMENT,  pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);
   ReportPoi(_T("Hauling"), POI_HAUL_SEGMENT,     pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);
   ReportPoi(_T("Erected"), POI_ERECTED_SEGMENT,  pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);
   ReportPoi(_T("Span"),    POI_SPAN,             pChapter, pGdrRptSpec->GetGirderKey(), pBroker, pPoi, pDisplayUnits, level);

   return pChapter;
}

CChapterBuilder* CPointOfInterestChapterBuilder::Clone() const
{
   return new CPointOfInterestChapterBuilder;
}

void CPointOfInterestChapterBuilder::ReportPoi(LPCTSTR strName,PoiAttributeType attribute,rptChapter* pChapter,const CGirderKey& girderKey,IBroker* pBroker,IPointOfInterest* pPoi,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const
{
   GirderIndexType gdrIdx = girderKey.girderIndex;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, coordinate, pDisplayUnits->GetSpanLengthUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   std::vector<pgsPointOfInterest> vPoi;
   if ( attribute == POI_SPAN ) 
   {
      vPoi = pPoi->GetPointsOfInterest(CSpanKey(ALL_SPANS,gdrIdx),attribute);
   }
   else
   {
      vPoi = pPoi->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,gdrIdx,ALL_SEGMENTS),attribute);
   }

   (*pPara) << _T("Number of POI = ") << (CollectionIndexType)vPoi.size() << rptNewLine;

   ColumnIndexType col = 0;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(11,strName);
   (*pPara) << pTable << rptNewLine;
   (*pTable)(0,col++) << _T("POI ID");
   (*pTable)(0,col++) << _T("Group");
   (*pTable)(0,col++) << _T("Girder");
   (*pTable)(0,col++) << _T("Segment");
   (*pTable)(0,col++) << COLHDR(_T("Xs"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(0,col++) << COLHDR(_T("Xsp"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(0,col++) << COLHDR(_T("Xg"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(0,col++) << COLHDR(_T("Xgp"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(0,col++) << _T("Span");
   (*pTable)(0,col++) << COLHDR(_T("Xspan"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(0,col++) << _T("Attribute");

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++, row++ )
   {
      col = 0;

      pgsPointOfInterest& poi = *iter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      (*pTable)(row,col++) << poi.GetID();
      (*pTable)(row,col++) << LABEL_GROUP(segmentKey.groupIndex);
      (*pTable)(row,col++) << LABEL_GIRDER(segmentKey.girderIndex);
      (*pTable)(row,col++) << LABEL_SEGMENT(segmentKey.segmentIndex);
      (*pTable)(row,col++) << coordinate.SetValue( poi.GetDistFromStart() );
      (*pTable)(row,col++) << coordinate.SetValue( pPoi->ConvertPoiToSegmentPathCoordinate(poi) );
      (*pTable)(row,col++) << coordinate.SetValue( pPoi->ConvertPoiToGirderCoordinate(poi) );
      (*pTable)(row,col++) << coordinate.SetValue( pPoi->ConvertPoiToGirderPathCoordinate(poi) );

      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      (*pTable)(row,col++) << LABEL_SPAN(spanKey.spanIndex);
      (*pTable)(row,col++) << coordinate.SetValue(Xspan);

      (*pTable)(row,col++) << poi.GetAttributes(attribute,false);
   }
}
