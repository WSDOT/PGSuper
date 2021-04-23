///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\CopyTempSupportPropertiesChapterBuilder.h>
#include <Reporting\BrokerReportSpecification.h>
#include <Reporting\CopyTempSupportPropertiesReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\DocumentType.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCopyTempSupportPropertiesChapterBuilder::CCopyTempSupportPropertiesChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect),
m_pCallBack(nullptr),
m_FromTempSupportIdx(INVALID_INDEX)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCopyTempSupportPropertiesChapterBuilder::GetName() const
{
   return TEXT("CopyTempSupportProperties");
}

rptChapter* CCopyTempSupportPropertiesChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pBrokerRptSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CCopyTempSupportPropertiesReportSpecification* pCopyTempSupportPropertiesMgrRptSpec = dynamic_cast<CCopyTempSupportPropertiesReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter = new rptChapter(GetName());
   if (m_pCallBack)
   {
      *pChapter << m_pCallBack->BuildComparisonReportParagraph(m_FromTempSupportIdx, m_ToTempSupports);
   }
   else
   {
      ATLASSERT(0);
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Error, no call back for ") << GetName();
   }

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("- Note that this report may not show all properties. Refer to the Details report for the girder in question to see a complete listing of properties.");

   return pChapter;
}

CChapterBuilder* CCopyTempSupportPropertiesChapterBuilder::Clone() const
{
   return new CCopyTempSupportPropertiesChapterBuilder;
}

void CCopyTempSupportPropertiesChapterBuilder::SetCopyTempSupportProperties(ICopyTemporarySupportPropertiesCallback* pCallBack, PierIndexType fromTempSupportIdx, const std::vector<PierIndexType>& toTempSupports)
{
   m_pCallBack = pCallBack;
   m_FromTempSupportIdx = fromTempSupportIdx;
   m_ToTempSupports = toTempSupports;
}
