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
#include <Reporting\SplittingCheckDetailsChapterBuilder.h>

#include <Reporting\SpanGirderReportSpecification.h>

#include <PgsExt\GirderArtifact.h>

#include <IFace/Tools.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\SplittingChecks.h>


CSplittingCheckDetailsChapterBuilder::CSplittingCheckDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CSplittingCheckDetailsChapterBuilder::GetKey() const
{
   return TEXT("Splitting Resistance Details");
}

LPCTSTR CSplittingCheckDetailsChapterBuilder::GetName() const
{
   if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      return TEXT("Splitting Resistance Details");
   }
   else
   {
      return TEXT("Bursting Resistance Details");
   }
}

rptChapter* CSplittingCheckDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pBroker = pGirderRptSpec->GetBroker();
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker, IArtifact, pIArtifact);

   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   GET_IFACE2(pBroker, ISplittingChecks, pSplittingChecks);
   pSplittingChecks->ReportSplittingCheckDetails(pGirderArtifact, pChapter);

   return pChapter;
}
