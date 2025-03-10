///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Reporting\LiftingCheckChapterBuilder.h>
#include <Reporting\LiftingCheck.h>
#include <IFace\Artifact.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLiftingCheckChapterBuilder
****************************************************************************/

CLiftingCheckChapterBuilder::CLiftingCheckChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLiftingCheckChapterBuilder::GetName() const
{
   return TEXT("Lifting Check");
}

rptChapter* CLiftingCheckChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   CComPtr<IBroker> pBroker;
   rptChapter* pChapter;

   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pSegmentRptSpec = std::dynamic_pointer_cast<const CSegmentReportSpecification>(pRptSpec);
   if (pGirderRptSpec)
   {
      pGirderRptSpec->GetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());
      pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);
      CLiftingCheck().Build(pChapter, pBroker, girderKey, pDisplayUnits);
   }
   else
   {
      pSegmentRptSpec->GetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      const CSegmentKey& segmentKey(pSegmentRptSpec->GetSegmentKey());
      pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);
      CLiftingCheck().Build(pChapter, pBroker, segmentKey, pDisplayUnits);
   }

   return pChapter;
}


std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLiftingCheckChapterBuilder::Clone() const
{
   return std::make_unique<CLiftingCheckChapterBuilder>();
}
