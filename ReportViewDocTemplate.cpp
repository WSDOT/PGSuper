///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include "stdafx.h"
#include "ReportViewDocTemplate.h"

CReportViewDocTemplate::CReportViewDocTemplate(UINT nIDResource,
                            CRuntimeClass* pDocClass,
                            CRuntimeClass* pFrameClass,
                            CRuntimeClass* pViewClass,
                            int maxViewCount) :
CCountedMultiDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass,maxViewCount)
{
   m_RptIdx = INVALID_INDEX;
   m_bPrompt = true;
}

CReportViewDocTemplate::~CReportViewDocTemplate(void)
{
}

CollectionIndexType CReportViewDocTemplate::GetReportIndex()
{
   return m_RptIdx;
}

void CReportViewDocTemplate::SetReportIndex(CollectionIndexType rptIdx)
{
   m_RptIdx = rptIdx;
}

bool CReportViewDocTemplate::PromptForReportSpecification()
{
   return m_bPrompt;
}

void CReportViewDocTemplate::PromptForReportSpecification(bool bPrompt)
{
   m_bPrompt = bPrompt;
}
