///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <Reporting\InternalForceChapterBuilder.h>

#include <Reporting\CombinedMomentsTable.h>

#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\PrestressForce.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInternalForceChapterBuilder::CInternalForceChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CInternalForceChapterBuilder::GetName() const
{
   return TEXT("Internal Time-Dependent Forces");
}

rptChapter* CInternalForceChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

#if defined _DEBUG
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   ATLASSERT( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP );
#endif 

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ILosses,pLosses);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType firstReleaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);

   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << _T("Use net section properties to compute girder and deck stresses") << rptNewLine;

   for ( IntervalIndexType intervalIdx = firstReleaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      if ( pIntervals->GetDuration(intervalIdx) == 0 )
      {
         continue; // skip all zero duration intervals
      }

      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      CString strName;
      strName.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      pPara->SetName(strName);
      *pPara << pPara->GetName() << rptNewLine;

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(13);
      *pPara << pTable << rptNewLine;

      pTable->SetNumberOfHeaderRows(3);
      pTable->SetRowSpan(0,0,3);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      pTable->SetRowSpan(1,0,SKIP_CELL);
      pTable->SetRowSpan(2,0,SKIP_CELL);

      pTable->SetColumnSpan(0,1,6);
      (*pTable)(0,1) << _T("Girder");
      pTable->SetColumnSpan(0,2,SKIP_CELL);
      pTable->SetColumnSpan(0,3,SKIP_CELL);
      pTable->SetColumnSpan(0,4,SKIP_CELL);
      pTable->SetColumnSpan(0,5,SKIP_CELL);
      pTable->SetColumnSpan(0,6,SKIP_CELL);

      pTable->SetColumnSpan(0,7,6);
      (*pTable)(0,7) << _T("Deck");
      pTable->SetColumnSpan(0,8,SKIP_CELL);
      pTable->SetColumnSpan(0,9,SKIP_CELL);
      pTable->SetColumnSpan(0,10,SKIP_CELL);
      pTable->SetColumnSpan(0,11,SKIP_CELL);
      pTable->SetColumnSpan(0,12,SKIP_CELL);

      pTable->SetColumnSpan(1,1,2);
      (*pTable)(1,1) << _T("Creep");
      pTable->SetColumnSpan(1,2,SKIP_CELL);

      pTable->SetColumnSpan(1,3,2);
      (*pTable)(1,3) << _T("Shrinkage");
      pTable->SetColumnSpan(1,4,SKIP_CELL);

      pTable->SetColumnSpan(1,5,2);
      (*pTable)(1,5) << _T("Relaxation");
      pTable->SetColumnSpan(1,6,SKIP_CELL);

      pTable->SetColumnSpan(1,7,2);
      (*pTable)(1,7) << _T("Creep");
      pTable->SetColumnSpan(1,8,SKIP_CELL);

      pTable->SetColumnSpan(1,9,2);
      (*pTable)(1,9) << _T("Shrinkage");
      pTable->SetColumnSpan(1,10,SKIP_CELL);

      pTable->SetColumnSpan(1,11,2);
      (*pTable)(1,11) << _T("Relaxation");
      pTable->SetColumnSpan(1,12,SKIP_CELL);

      ColumnIndexType colIdx = 1;
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("P"),_T("g")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("M"),_T("g")),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("P"),_T("g")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("M"),_T("g")),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("P"),_T("g")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("M"),_T("g")),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("P"),_T("d")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("M"),_T("d")),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("P"),_T("d")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("M"),_T("d")),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("P"),_T("d")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      (*pTable)(2,colIdx++) << COLHDR(Sub2(symbol(DELTA) << _T("M"),_T("d")),rptMomentUnitTag,pDisplayUnits->GetMomentUnit());

      std::vector<pgsPointOfInterest> vPoi;
      PoiAttributeType refAttribute;
      GetCombinedResultsPoi(pBroker,girderKey,intervalIdx,&vPoi,&refAttribute);

      RowIndexType rowIdx = pTable->GetNumberOfHeaderRows();
      BOOST_FOREACH(const pgsPointOfInterest& poi,vPoi)
      {
         colIdx = 0;

         const LOSSDETAILS* pLossDetails = pLosses->GetLossDetails(poi,intervalIdx);
         const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

         (*pTable)(rowIdx,colIdx++) << location.SetValue(refAttribute, poi);

         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pgsTypes::pftCreep]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pgsTypes::pftCreep]);
         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pgsTypes::pftShrinkage]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pgsTypes::pftShrinkage]);
         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pgsTypes::pftRelaxation]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pgsTypes::pftRelaxation]);

         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftCreep]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftCreep]);
         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftShrinkage]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftShrinkage]);
         (*pTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pgsTypes::pftRelaxation]);
         (*pTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pgsTypes::pftRelaxation]);

         rowIdx++;
      } // next poi
   } // next interval

   return pChapter;
}

CChapterBuilder* CInternalForceChapterBuilder::Clone() const
{
   return new CInternalForceChapterBuilder;
}
