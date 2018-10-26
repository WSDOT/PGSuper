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
#include <Reporting\ProductForcesChapterBuilder.h>
#include <IFace\PointOfInterest.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <PgsExt\TimelineManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CProductForcesChapterBuilder::CProductForcesChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CProductForcesChapterBuilder::GetName() const
{
#pragma Reminder("UPDATE")
   return TEXT("Product Forces???");
}

#pragma Reminder("OBSOLETE?: this chapter builder isn't used in any reports")
// Delete this chapter builder, and the source files, if it isn't needed

rptChapter* CProductForcesChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   ATLASSERT(false);
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;
   return pChapter;
/*
   USES_CONVERSION;

#pragma Reminder("UPDATE: should only be a Girder Report Spec")

   CGirderReportSpecification* pGdrRptSpec      = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CSegmentReportSpecification* pSegmentRptSpec = dynamic_cast<CSegmentReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   if ( pGdrRptSpec )
      pGdrRptSpec->GetBroker(&pBroker);
   else
      pSegmentRptSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits, pDisplayUnits );
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IPointOfInterest,pSegmentPOI);
   GET_IFACE2(pBroker,IEventMap,pEventMap);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,   pDisplayUnits->GetAlignmentLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptMomentSectionValue, moment, pDisplayUnits->GetMomentUnit(), false );

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   ProductForceType forceType = pftGirder;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType startIntervalIdx = pIntervals->GetFirstSegmentErectionInterval();
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      ColumnIndexType nCols = 13;

      if ( intervalIdx == liveLoadIntervalIdx )
      {
         nCols += 2;
      }

      std::_tostringstream os;
      os << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(": ") << pIntervals->GetDescription(intervalIdx);
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,os.str().c_str());
      (*pPara) << pTable << rptNewLine << rptNewLine;

      ColumnIndexType colIdx = 0;

      (*pTable)(0,colIdx++) << _T("POI ID");
      (*pTable)(0,colIdx++) << _T("Group");
      (*pTable)(0,colIdx++) << _T("Girder");
      (*pTable)(0,colIdx++) << _T("Segment");
      (*pTable)(0,colIdx++) << COLHDR(_T("Location"),rptLengthUnitTag,pDisplayUnits->GetAlignmentLengthUnit());
      (*pTable)(0,colIdx++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(0,colIdx++) << COLHDR(_T("Girder"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(_T("Slab"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(_T("Diaphragm"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(_T("Traffic") << rptNewLine << _T("Barrier"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(_T("Overlay"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(_T("Total PT"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(_T("Secondary"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());

      if ( intervalIdx == liveLoadIntervalIdx )
      {
         (*pTable)(0,colIdx++) << COLHDR(_T("Min"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
         (*pTable)(0,colIdx++) << COLHDR(_T("Max"),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      }

      CSegmentKey searchKey;
      if ( pGdrRptSpec )
      {
         searchKey = CSegmentKey(pGdrRptSpec->GetGirderKey(),ALL_SEGMENTS);
      }
      else
      {
         searchKey = pSegmentRptSpec->GetSegmentKey();
      }
      searchKey.groupIndex   = ALL_GROUPS;
      searchKey.segmentIndex = ALL_SEGMENTS;
      std::vector<pgsPointOfInterest> vPOI(pSegmentPOI->GetPointsOfInterest(searchKey));

      GET_IFACE2(pBroker,IProductForces2,pProductForces);
      GET_IFACE2(pBroker,ISpecification,pSpec);
      pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
      ATLASSERT(analysisType == pgsTypes::Continuous);
      pgsTypes::BridgeAnalysisType bat = pgsTypes::ContinuousSpan;

      std::vector<Float64> Mgirder( pProductForces->GetMoment(intervalIdx,pftGirder,vPOI,bat) );
      std::vector<Float64> Mslab( pProductForces->GetMoment(intervalIdx,pftSlab,vPOI,bat) );
      std::vector<Float64> Mdia( pProductForces->GetMoment(intervalIdx,pftDiaphragm,vPOI,bat) );
      std::vector<Float64> Mbarrier( pProductForces->GetMoment(intervalIdx,pftTrafficBarrier,vPOI,bat) );
      std::vector<Float64> Moverlay( pProductForces->GetMoment(intervalIdx,pftOverlay,vPOI,bat) );
      std::vector<Float64> Mptt( pProductForces->GetMoment(intervalIdx,pftPostTensioning,vPOI,bat) );
      std::vector<Float64> Mpts( pProductForces->GetMoment(intervalIdx,pftSecondaryEffects,vPOI,bat) );

      std::vector<Float64> MMaxLL, MMinLL;
      if ( intervalIdx == liveLoadIntervalIdx )
      {
         pProductForces->GetLiveLoadMoment(pgsTypes::lltDesign,intervalIdx,vPOI,bat,true,false,&MMinLL,&MMaxLL);

      }

      RowIndexType row = 1;
      std::vector<pgsPointOfInterest>::iterator iter(vPOI.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPOI.end());
      for ( ; iter != end; iter++, row++ )
      {
         colIdx = 0;

         pgsPointOfInterest& poi = *iter;
         const CSegmentKey& segmentKey = poi.GetSegmentKey();

         (*pTable)(row,colIdx++) << poi.GetID();
         (*pTable)(row,colIdx++) << LABEL_GROUP(segmentKey.groupIndex);
         (*pTable)(row,colIdx++) << LABEL_GIRDER(segmentKey.girderIndex);
         (*pTable)(row,colIdx++) << LABEL_SEGMENT(segmentKey.segmentIndex);
         (*pTable)(row,colIdx++) << loc.SetValue( poi.GetDistFromStart() );
         (*pTable)(row,colIdx++) << location.SetValue( POI_ERECTED_SEGMENT, poi );
         (*pTable)(row,colIdx++) << moment.SetValue( Mgirder[row-1] );
         (*pTable)(row,colIdx++) << moment.SetValue( Mslab[row-1] );
         (*pTable)(row,colIdx++) << moment.SetValue( Mdia[row-1] );
         (*pTable)(row,colIdx++) << moment.SetValue( Mbarrier[row-1] );
         (*pTable)(row,colIdx++) << moment.SetValue( Moverlay[row-1] );
         (*pTable)(row,colIdx++) << moment.SetValue( Mptt[row-1] );
         (*pTable)(row,colIdx++) << moment.SetValue( Mpts[row-1] );

         if ( intervalIdx == liveLoadIntervalIdx )
         {
            (*pTable)(row,colIdx++) << moment.SetValue( MMinLL[row-1] );
            (*pTable)(row,colIdx++) << moment.SetValue( MMaxLL[row-1] );
         }
      }
   }

   return pChapter;
*/
}

CChapterBuilder* CProductForcesChapterBuilder::Clone() const
{
   return new CProductForcesChapterBuilder;
}
