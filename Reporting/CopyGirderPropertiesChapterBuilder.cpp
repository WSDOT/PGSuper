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
#include <Reporting\CopyGirderPropertiesChapterBuilder.h>
#include <Reporting\BrokerReportSpecification.h>
#include <Reporting\CopyGirderPropertiesReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\DocumentType.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCopyGirderPropertiesChapterBuilder::CCopyGirderPropertiesChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CCopyGirderPropertiesChapterBuilder::GetName() const
{
   return TEXT("Girder Property Comparison");
}

rptChapter* CCopyGirderPropertiesChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   auto pCopyGirderPropertiesMgrRptSpec = std::dynamic_pointer_cast<const CCopyGirderPropertiesReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter = new rptChapter(GetName());
   if (!m_CallBacks.empty())
   {
      for (auto callback : m_CallBacks)
      {
         *pChapter << callback->BuildComparisonReportParagraph(m_FromGirderKey);
      }

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("- Note that this report may not show all properties. Refer to the Details report for the girder in question to see a complete listing of properties.");
   }
   else
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << _T("Nothing to report. Please select a property type");
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CCopyGirderPropertiesChapterBuilder::Clone() const
{
   return std::make_unique<CCopyGirderPropertiesChapterBuilder>();
}

void CCopyGirderPropertiesChapterBuilder::SetCopyGirderProperties(std::vector<ICopyGirderPropertiesCallback*>& rCallBacks, const CGirderKey& fromGirderKey)
{
   m_CallBacks = rCallBacks;
   m_FromGirderKey = fromGirderKey;
}
