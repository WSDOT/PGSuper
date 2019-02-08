///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <Reporting\InitialStrainAnalysisChapterBuilder.h>
#include <Reporting\InitialStrainAnalysisReportSpecification.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInitialStrainAnalysisChapterBuilder::CInitialStrainAnalysisChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CInitialStrainAnalysisChapterBuilder::GetName() const
{
   return TEXT("Initial Strain Analysis");
}

rptChapter* CInitialStrainAnalysisChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CInitialStrainAnalysisReportSpecification* pGdrRptSpec = dynamic_cast<CInitialStrainAnalysisReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);

   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());
   IntervalIndexType intervalIdx = pGdrRptSpec->GetInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << rptNewLine;

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), &vPoi);

   GET_IFACE2(pBroker,ILosses,pLosses);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    dist,       pDisplayUnits->GetSpanLengthUnit(),      false);

   for ( int i = 0; i < 3; i++ )
   {
      CString strLabel;
      strLabel.Format(_T("%s - Initial Strian Analysis"),i == 0 ? _T("Creep") : i == 1 ? _T("Shrinkage") : _T("Relaxation"));
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(12,strLabel);
      *pPara << pTable << rptNewLine;

      ColumnIndexType colIdx = 0;
      (*pTable)(0,colIdx++) << _T("ID");
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("X"),_T("g")),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("P"),_T("r")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("M"),_T("r")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("F"),_T("x")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("F"),_T("y")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("M"),_T("z")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("F"),_T("x")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("F"),_T("y")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("M"),_T("z")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("P"),_T("re")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(0,colIdx++) << COLHDR(Sub2(_T("M"),_T("re")),rptMomentUnitTag,pDisplayUnits->GetSmallMomentUnit());

      RowIndexType rowIdx = pTable->GetNumberOfHeaderRows();
      for (const pgsPointOfInterest& poi : vPoi)
      {
         colIdx = 0;
         (*pTable)(rowIdx,colIdx++) << poi.GetID();

         Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
         (*pTable)(rowIdx,colIdx++) << dist.SetValue(Xg);

         const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
         const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Pr[i]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mr[i]);

         //(*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Fx[i][pgsTypes::Back]);
         //(*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Fy[i][pgsTypes::Back]);
         //(*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mz[i][pgsTypes::Back]);

         //(*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Fx[i][pgsTypes::Ahead]);
         //(*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Fy[i][pgsTypes::Ahead]);
         //(*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mz[i][pgsTypes::Ahead]);

         //(*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Pre[i]);
         //(*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Mre[i]);

         rowIdx++;
      } // next POI
   } // next i

   return pChapter;
}

CChapterBuilder* CInitialStrainAnalysisChapterBuilder::Clone() const
{
   return new CInitialStrainAnalysisChapterBuilder;
}
