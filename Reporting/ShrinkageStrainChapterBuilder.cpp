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
#include <Reporting\ShrinkageStrainChapterBuilder.h>

#include <IFace/Tools.h>
#include <IFace\Project.h>


CShrinkageStrainChapterBuilder::CShrinkageStrainChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CShrinkageStrainChapterBuilder::GetName() const
{
   return TEXT("Shrinkage Strain Details");
}

rptChapter* CShrinkageStrainChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pBroker = pGirderRptSpec->GetBroker();

   //rptChapter* pChapter;
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   ATLASSERT( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP );

   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;
   *pPara << _T("Shrinkage strain details are listed in the Time Step Details Report.") << rptNewLine;

   return pChapter;
}

