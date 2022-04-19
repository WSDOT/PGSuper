///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\CopyPierPropertiesChapterBuilder.h>
#include <Reporting\BrokerReportSpecification.h>
#include <Reporting\CopyPierPropertiesReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\DocumentType.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCopyPierPropertiesChapterBuilder::CCopyPierPropertiesChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect),
m_FromPierIdx(INVALID_INDEX)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCopyPierPropertiesChapterBuilder::GetName() const
{
   return TEXT("Pier Properties Comparison");
}

rptChapter* CCopyPierPropertiesChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pBrokerRptSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CCopyPierPropertiesReportSpecification* pCopyPierPropertiesMgrRptSpec = dynamic_cast<CCopyPierPropertiesReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter = new rptChapter(GetName());
   if (!m_CallBacks.empty())
   {
      for (auto callback : m_CallBacks)
      {
         *pChapter << callback->BuildComparisonReportParagraph(m_FromPierIdx, m_ToPiers);
      }

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("- Note that this report may not show all properties. Refer to the Details report for the pier in question to see a complete listing of properties.");
   }
   else
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Nothing to report. Please select a property type");
   }
   return pChapter;
}

CChapterBuilder* CCopyPierPropertiesChapterBuilder::Clone() const
{
   return new CCopyPierPropertiesChapterBuilder;
}

void CCopyPierPropertiesChapterBuilder::SetCopyPierProperties(std::vector<ICopyPierPropertiesCallback*>& rCallbacks, PierIndexType fromPierIdx, const std::vector<PierIndexType>& toPiers)
{
   m_CallBacks = rCallbacks;
   m_FromPierIdx = fromPierIdx;
   m_ToPiers = toPiers;
}
