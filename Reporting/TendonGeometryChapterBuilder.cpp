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
#include <Reporting\TendonGeometryChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTendonGeometryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTendonGeometryChapterBuilder::CTendonGeometryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTendonGeometryChapterBuilder::GetName() const
{
   return TEXT("Tendon Geometry");
}

rptChapter* CTendonGeometryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   CGirderKey girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   GET_IFACE2(pBroker,IPointOfInterest,pSegmentPOI);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( pTendonGeom->GetDuctCount(girderKey) == 0 )
      return pChapter;

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        X,        pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        ecc,      pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,        stress,   pDisplayUnits->GetStressUnit(), false);

   location.IncludeSpanAndGirder(true);


   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CString strTitle;
      strTitle.Format(_T("Tendon Geometry - Tendon %d"),LABEL_DUCT(ductIdx));
      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(9,strTitle);
      *pPara << pTable << rptNewLine;

      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

      ColumnIndexType col = 0;
      (*pTable)(0,col++) << _T("POI");
      (*pTable)(0,col++) << COLHDR(_T("X"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable)(0,col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,col++) << _T("Slope");
      (*pTable)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,col++) << symbol(alpha) << rptNewLine << _T("(from Start)");
      (*pTable)(0,col++) << symbol(alpha) << rptNewLine << _T("(from End)");
      (*pTable)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pTable)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      std::vector<pgsPointOfInterest> vPOI(pSegmentPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

      RowIndexType row = 1;
      std::vector<pgsPointOfInterest>::iterator iter(vPOI.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPOI.end());
      for ( ; iter != end; iter++, row++ )
      {
         col = 0;

         pgsPointOfInterest& poi = *iter;
         (*pTable)(row,col++) << location.SetValue(POI_GIRDER,poi,0.0);

         const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);
         const FRICTIONLOSSDETAILS& frDetails(pDetails->FrictionLossDetails[ductIdx]);

         (*pTable)(row,col++) << X.SetValue( frDetails.X );
         (*pTable)(row,col++) << ecc.SetValue(pTendonGeom->GetDuctOffset(stressTendonIntervalIdx,poi,ductIdx));

         CComPtr<IVector3d> slope;
         pTendonGeom->GetTendonSlope(poi,ductIdx,&slope);
         Float64 Y;
         slope->get_Y(&Y);
         (*pTable)(row,col++) << Y;

         (*pTable)(row,col++) << ecc.SetValue(pTendonGeom->GetEccentricity(stressTendonIntervalIdx,poi,ductIdx));
         (*pTable)(row,col++) << pTendonGeom->GetAngularChange(poi,ductIdx,pgsTypes::metStart);
         (*pTable)(row,col++) << pTendonGeom->GetAngularChange(poi,ductIdx,pgsTypes::metEnd);
         (*pTable)(row,col++) << stress.SetValue( frDetails.dfpF ); // friction
         (*pTable)(row,col++) << stress.SetValue( frDetails.dfpA ); // anchor set
      }

      X.ShowUnitTag(true);
      Float64 Lset = pLosses->GetAnchorSetZoneLength(girderKey,ductIdx,pgsTypes::metStart);
      *pPara << _T("Left End, ") << Sub2(_T("L"),_T("set")) << _T(" = ") << X.SetValue(Lset) << rptNewLine;

      Lset = pLosses->GetAnchorSetZoneLength(girderKey,ductIdx,pgsTypes::metEnd);
      *pPara << _T("Right End, ") << Sub2(_T("L"),_T("set")) << _T(" = ") << X.SetValue(Lset) << rptNewLine;
      X.ShowUnitTag(false);
   }


   return pChapter;
}

CChapterBuilder* CTendonGeometryChapterBuilder::Clone() const
{
   return new CTendonGeometryChapterBuilder;
}
