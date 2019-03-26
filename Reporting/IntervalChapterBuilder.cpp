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
#include <Reporting\IntervalChapterBuilder.h>

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CIntervalChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CIntervalChapterBuilder::CIntervalChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CIntervalChapterBuilder::GetName() const
{
   return TEXT("Analysis Intervals");
}

rptChapter* CIntervalChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pIntervalTable = rptStyleManager::CreateDefaultTable(8);
   *pPara << pIntervalTable << rptNewLine;

   pIntervalTable->SetColumnStyle(1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pIntervalTable->SetStripeRowColumnStyle(1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   (*pIntervalTable)(0,col++) << _T("Interval");
   (*pIntervalTable)(0,col++) << _T("Description");
   (*pIntervalTable)(0,col++) << _T("Start") << rptNewLine << _T("Event");
   (*pIntervalTable)(0,col++) << _T("End") << rptNewLine << _T("Event");
   (*pIntervalTable)(0,col++) << COLHDR(_T("Start"),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pIntervalTable)(0,col++) << COLHDR(_T("Middle"),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pIntervalTable)(0,col++) << COLHDR(_T("End"),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());
   (*pIntervalTable)(0,col++) << COLHDR(_T("Duration"),rptTimeUnitTag,pDisplayUnits->GetWholeDaysUnit());

   
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   RowIndexType row  = pIntervalTable->GetNumberOfHeaderRows();
   for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      col = 0;
      (*pIntervalTable)(row,col++) << LABEL_INTERVAL(intervalIdx);
      (*pIntervalTable)(row,col++) << pIntervals->GetDescription(intervalIdx);
      (*pIntervalTable)(row,col++) << LABEL_EVENT(pIntervals->GetStartEvent(intervalIdx));
      (*pIntervalTable)(row,col++) << LABEL_EVENT(pIntervals->GetEndEvent(intervalIdx));
      (*pIntervalTable)(row,col++) << pIntervals->GetTime(intervalIdx,pgsTypes::Start);
      (*pIntervalTable)(row,col++) << pIntervals->GetTime(intervalIdx,pgsTypes::Middle);
      (*pIntervalTable)(row,col++) << pIntervals->GetTime(intervalIdx,pgsTypes::End);
      (*pIntervalTable)(row,col++) << pIntervals->GetDuration(intervalIdx);
      row++;
   }

   return pChapter;
}

CChapterBuilder* CIntervalChapterBuilder::Clone() const
{
   return new CIntervalChapterBuilder;
}
