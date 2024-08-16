///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <Reporting\PGSuperChapterBuilder.h>

#include <Reporting\SpanGirderReportSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPGSuperChapterBuilder
****************************************************************************/
CPGSuperChapterBuilder::CPGSuperChapterBuilder(bool bSelect)
{
   m_bSelect = bSelect;
}

Uint16 CPGSuperChapterBuilder::GetMaxLevel() const
{
   return 1;
}

rptChapter* CPGSuperChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   ASSERT( level <= GetMaxLevel() );

   rptChapter* pChapter = new rptChapter(GetName());
   rptParagraph* p_para = new rptParagraph;
   p_para->SetStyleName(rptStyleManager::GetChapterTitleStyle());
   *pChapter << p_para;
   *p_para << GetName() << rptNewLine;
   return pChapter;
}

bool CPGSuperChapterBuilder::Select() const
{
   return m_bSelect;
}

void CPGSuperChapterBuilder::SetSelect(bool bSelect)
{
   m_bSelect = bSelect;
}

bool CPGSuperChapterBuilder::NeedsUpdate(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   int result = CGirderReportHint::IsMyGirder(pHint,pRptSpec);
   if ( 0 < result ) // this is a SegmentReportHint and it is for our segment
      return true;

   if ( result == 0 )
      return false;// this is a SegmentReportHint and it is not for our segment

   result = CSpanReportHint::IsMySpan(pHint,pRptSpec);
   if ( 0 < result )
      return true;

   if ( result == 0 )
      return false;

   result = CGirderLineReportHint::IsMyGirder(pHint,pRptSpec);
   if ( 0 < result )
      return true;

   if ( result == 0 )
      return false;

   // base class always returns true
   return WBFL::Reporting::ChapterBuilder::NeedsUpdate(pHint,pRptSpec,level);
}
