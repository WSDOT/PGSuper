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

#include "StdAfx.h"
#include <Reporting\PGSuperChapterBuilder.h>
#include <Reporting\ReportStyleHolder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPGSuperChapterBuilder
****************************************************************************/

Uint16 CPGSuperChapterBuilder::GetMaxLevel() const
{
   return 1;
}

rptChapter* CPGSuperChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   ASSERT( level <= GetMaxLevel() );

   rptChapter* pChapter = new rptChapter(GetName());
   rptParagraph* p_para = new rptParagraph;
   p_para->SetStyleName(pgsReportStyleHolder::GetChapterTitleStyle());
   *pChapter << p_para;
   *p_para << GetName() << rptNewLine;
   return pChapter;
}

