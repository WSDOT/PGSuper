///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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
#include "MyReportSpecificationBuilder.h"
#include "MyReportSpecification.h"
#include <MFCTools\Prompts.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMyReportSpecificationBuilder::CMyReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CMyReportSpecificationBuilder::~CMyReportSpecificationBuilder(void)
{
}

std::shared_ptr<CReportSpecification> CMyReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,std::shared_ptr<CReportSpecification>& pRptSpec)
{
   CString strAnswer;
   AfxQuestion(_T("My Report Specification"),_T("Enter some text to put into the report"),_T(""),strAnswer);
   std::shared_ptr<CMyReportSpecification> pSpec(std::make_shared<CMyReportSpecification>(rptDesc.GetReportName(),m_pBroker));
   pSpec->SetMessage(strAnswer);
   std::shared_ptr<CReportSpecification> pNewRptSpec(std::dynamic_pointer_cast<CReportSpecification>(pSpec));

   rptDesc.ConfigureReportSpecification(pNewRptSpec);
   return pNewRptSpec;
}

std::shared_ptr<CReportSpecification> CMyReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // there is no default configuration for this report. The user must be prompted every time
   std::shared_ptr<CReportSpecification> nullSpec;
   return CreateReportSpec(rptDesc,nullSpec);
}
