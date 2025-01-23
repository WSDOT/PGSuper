///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\GirderTendonGeometryChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
#include <IFace\ReportOptions.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderTendonGeometryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderTendonGeometryChapterBuilder::CGirderTendonGeometryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CGirderTendonGeometryChapterBuilder::GetName() const
{
   return TEXT("Girder Tendon Geometry");
}

rptChapter* CGirderTendonGeometryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   CGirderKey girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pTendonGeom->GetDuctCount(girderKey) == 0 )
   {
      *pPara << _T("No tendons defined") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        dist,     pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        ecc,      pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,        stress,   pDisplayUnits->GetStressUnit(), false);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));
   
   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), &vPoi);

   // critical sections haven't been computed when time-step analysis happens
   // so don't attempt to report them here.
   pPoi->RemovePointsOfInterest(vPoi, POI_CRITSECTSHEAR1);
   pPoi->RemovePointsOfInterest(vPoi, POI_CRITSECTSHEAR2);

   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CString strTitle;
      strTitle.Format(_T("Tendon %d"),LABEL_DUCT(ductIdx));
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(10,strTitle);
      *pPara << pTable << rptNewLine;

      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);

      ColumnIndexType col = 0;
      (*pTable)(0,col++) << _T("POI");
      (*pTable)(0,col++) << COLHDR(_T("X"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable)(0,col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,col++) << _T("Slope X");
      (*pTable)(0,col++) << _T("Slope Y");
      (*pTable)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,col++) << symbol(alpha) << rptNewLine << _T("(from Start)");
      (*pTable)(0,col++) << symbol(alpha) << rptNewLine << _T("(from End)");
      (*pTable)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pTable)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      RowIndexType row = 1;
      for (const pgsPointOfInterest& poi : vPoi)
      {
         col = 0;

         if ( pPoi->IsOnGirder(poi) )
         {
            (*pTable)(row,col++) << location.SetValue(POI_SPAN,poi);

            const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,stressTendonIntervalIdx);
            const FRICTIONLOSSDETAILS& frDetails(pDetails->GirderFrictionLossDetails[ductIdx]);

			   ATLASSERT(IsEqual(frDetails.X, pPoi->ConvertPoiToGirderCoordinate(poi)));

			   Float64 ductOffset(0), slopeX(0), slopeY(0), angle_start(0), angle_end(0);
            WBFL::Geometry::Point2d tendon_ecc;
			   if (pTendonGeom->IsOnDuct(poi, ductIdx))
			   {
				   ductOffset = pTendonGeom->GetGirderDuctOffset(stressTendonIntervalIdx, poi, ductIdx);

				   CComPtr<IVector3d> slope;
				   pTendonGeom->GetGirderTendonSlope(poi, ductIdx, &slope);
				   slope->get_X(&slopeX);
				   slope->get_Y(&slopeY);

               tendon_ecc = pTendonGeom->GetGirderTendonEccentricity(stressTendonIntervalIdx, poi, ductIdx);

				   angle_start = pTendonGeom->GetGirderTendonAngularChange(poi, ductIdx, pgsTypes::metStart);
				   angle_end   = pTendonGeom->GetGirderTendonAngularChange(poi, ductIdx, pgsTypes::metEnd);
			   }

            (*pTable)(row,col++) << dist.SetValue( frDetails.X );
            (*pTable)(row,col++) << ecc.SetValue(ductOffset);
            (*pTable)(row,col++) << slopeX;
            (*pTable)(row,col++) << slopeY;
            (*pTable)(row,col++) << ecc.SetValue(tendon_ecc.Y());
            (*pTable)(row,col++) << angle_start;
            (*pTable)(row,col++) << angle_end;
            (*pTable)(row,col++) << stress.SetValue( frDetails.dfpF ); // friction
            (*pTable)(row,col++) << stress.SetValue( frDetails.dfpA ); // anchor set
         }
         row++;
      } // next poi

      dist.ShowUnitTag(true);
      ecc.ShowUnitTag(true);
      stress.ShowUnitTag(true);

      Float64 Lduct = pTendonGeom->GetDuctLength(girderKey,ductIdx);
      *pPara << _T("Duct Length = ") << dist.SetValue(Lduct) << rptNewLine;
      *pPara << rptNewLine;

      Float64 pfpF = pLosses->GetGirderTendonAverageFrictionLoss(girderKey,ductIdx);
      *pPara << _T("Avg. Friction Loss = ") << stress.SetValue(pfpF) << rptNewLine;

      Float64 pfpA = pLosses->GetGirderTendonAverageAnchorSetLoss(girderKey,ductIdx);
      *pPara << _T("Avg. Anchor Set Loss = ") << stress.SetValue(pfpA) << rptNewLine;
      *pPara << rptNewLine;

      Float64 Lset = pLosses->GetGirderTendonAnchorSetZoneLength(girderKey,ductIdx,pgsTypes::metStart);
      *pPara << _T("Left End, ") << Sub2(_T("L"),_T("set")) << _T(" = ") << dist.SetValue(Lset) << rptNewLine;

      Lset = pLosses->GetGirderTendonAnchorSetZoneLength(girderKey,ductIdx,pgsTypes::metEnd);
      *pPara << _T("Right End, ") << Sub2(_T("L"),_T("set")) << _T(" = ") << dist.SetValue(Lset) << rptNewLine;

      *pPara << rptNewLine;

      Float64 elongation = pLosses->GetGirderTendonElongation(girderKey,ductIdx,pgsTypes::metStart);
      *pPara << _T("Left End, Elongation = ") << ecc.SetValue(elongation) << rptNewLine;

      elongation = pLosses->GetGirderTendonElongation(girderKey,ductIdx,pgsTypes::metEnd);
      *pPara << _T("Right End, Elongation = ") << ecc.SetValue(elongation) << rptNewLine;

      *pPara << rptNewLine;


      dist.ShowUnitTag(false);
      ecc.ShowUnitTag(false);
      stress.ShowUnitTag(false);
   }


   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CGirderTendonGeometryChapterBuilder::Clone() const
{
   return std::make_unique<CGirderTendonGeometryChapterBuilder>();
}
