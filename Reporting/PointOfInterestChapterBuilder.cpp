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
#include <Reporting\PointOfInterestChapterBuilder.h>
#include <IFace\PointOfInterest.h>
#include <PgsExt\GirderPointOfInterest.h>

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
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);

   GirderIndexType gdrIdx = pGdrRptSpec->GetGirderKey().girderIndex;

   GET_IFACE2(pBroker,IEAFDisplayUnits, pDisplayUnits );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;

   (*pChapter) << pPara;

   (*pPara) << _T("This is a dummy report... needs to be updated for new indexing scheme") << rptNewLine;

   GET_IFACE2(pBroker,IPointOfInterest,pSegmentPOI);

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(6,_T("Segment POI"));
   (*pPara) << pTable << rptNewLine;
   (*pTable)(0,0) << _T("POI ID");
   (*pTable)(0,1) << _T("Group");
   (*pTable)(0,2) << _T("Girder");
   (*pTable)(0,3) << _T("Segment");
   (*pTable)(0,4) << COLHDR(_T("Location from start of Segment"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
   (*pTable)(0,5) << _T("Mid-Span");

   std::vector<pgsPointOfInterest> vPOI(pSegmentPOI->GetPointsOfInterest(CSegmentKey(ALL_GROUPS,gdrIdx,ALL_SEGMENTS)));

   RowIndexType row = 1;
   std::vector<pgsPointOfInterest>::iterator iter(vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPOI.end());
   for ( ; iter != end; iter++, row++ )
   {
      pgsPointOfInterest& poi = *iter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      (*pTable)(row,0) << poi.GetID();
      (*pTable)(row,1) << LABEL_GROUP(segmentKey.groupIndex);
      (*pTable)(row,2) << LABEL_GIRDER(segmentKey.girderIndex);
      (*pTable)(row,3) << LABEL_SEGMENT(segmentKey.segmentIndex);
      (*pTable)(row,4) << location.SetValue( POI_RELEASED_SEGMENT | POI_STORAGE_SEGMENT | POI_ERECTED_SEGMENT | POI_GIRDER, poi );

      if ( poi.IsMidSpan(POI_RELEASED_SEGMENT | POI_STORAGE_SEGMENT | POI_ERECTED_SEGMENT | POI_GIRDER) )
      {
         (*pTable)(row,5) << symbol(ROOT) << rptNewLine;
      }
      else
      {
         (*pTable)(row,5) << _T("");
      }


   //   pgsPointOfInterest segPoi( pSegmentPOI->GetPointOfInterest(poi.GetID()) );
   //   if ( segPoi.GetID() != INVALID_ID )
   //   {
   //      GirderIndexType gdrIdx;
   //      SegmentIndexType segIdx;
   //      ::UnhashGirderSegment(segPoi.GetSuperstructureMemberID(),&gdrIdx,&segIdx);
   //      (*pTable)(row,2) << _T("(") << segPoi.GetID() << _T(") ") << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx);
   //      (*pTable)(row,3) << loc.SetValue(segPoi.GetDistFromStart());
   //   }

   //   pgsPointOfInterest cpPoi( pClosureJointPOI->GetClosureJointPointOfInterest(poi.GetID()) );
   //   if ( cpPoi.GetID() != INVALID_ID )
   //   {
   //      GirderIndexType gdrIdx;
   //      CollectionIndexType cpIdx;
   //      ::UnhashGirderSegment(cpPoi.GetSuperstructureMemberID(),&gdrIdx,&cpIdx);

   //      (*pTable)(row,2) << _T("(") << cpPoi.GetID() << _T(") ") << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" CJ ") << LABEL_SEGMENT(cpIdx);
   //      (*pTable)(row,3) << loc.SetValue(cpPoi.GetDistFromStart());
   //   }
   }

   (*pPara) << _T("Number of Segment POI = ") << (CollectionIndexType)vPOI.size() << rptNewLine;

   return pChapter;
}

CChapterBuilder* CPointOfInterestChapterBuilder::Clone() const
{
   return new CPointOfInterestChapterBuilder;
}
