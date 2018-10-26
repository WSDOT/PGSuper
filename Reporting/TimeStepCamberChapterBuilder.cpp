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
#include <Reporting\TimeStepCamberChapterBuilder.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeStepCamberChapterBuilder::CTimeStepCamberChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTimeStepCamberChapterBuilder::GetName() const
{
   return TEXT("Camber Details");
}

rptChapter* CTimeStepCamberChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Time Step Camber") << rptNewLine;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateTableNoHeading(2);
   *pPara << pLayoutTable << rptNewLine;

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

      rptRcTable* pTable = CreateTable(pBroker,segmentKey,releaseIntervalIdx);
      (*pLayoutTable)(segIdx,0) << pTable << rptNewLine;

      pTable = CreateTable(pBroker,segmentKey,erectionIntervalIdx);
      (*pLayoutTable)(segIdx,1) << pTable << rptNewLine;
   }

   return pChapter;
}

CChapterBuilder* CTimeStepCamberChapterBuilder::Clone() const
{
   return new CTimeStepCamberChapterBuilder;
}

rptRcTable* CTimeStepCamberChapterBuilder::CreateTable(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductForces2,pForces);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   GET_IFACE2(pBroker,IProductForces,pProduct);
   pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);


   INIT_UV_PROTOTYPE( rptPointOfInterest, location,     pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, deflection,   pDisplayUnits->GetDeflectionUnit(), false );

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(3);

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("girder")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );
   (*pTable)(0,col++) << COLHDR(Sub2(symbol(DELTA),_T("ps")),  rptLengthUnitTag, pDisplayUnits->GetDeflectionUnit() );

   std::vector<Float64> deltaGirder = pForces->GetDeflection(intervalIdx,pftGirder,vPoi,bat, ctIncremental);
   std::vector<Float64> deltaPS     = pForces->GetDeflection(intervalIdx,pftPrestress,vPoi,bat, ctIncremental);

   PoiAttributeType poiReference = (intervalIdx == releaseIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   IndexType idx = 0;
   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++, row++, idx++ )
   {
      col = 0;
      pgsPointOfInterest& poi(*poiIter);

      (*pTable)(row,col++) << location.SetValue( poiReference, poi, end_size );
      (*pTable)(row,col++) << deflection.SetValue( deltaGirder[idx] );
      (*pTable)(row,col++) << deflection.SetValue( deltaPS[idx] );
   }

   return pTable;
}
