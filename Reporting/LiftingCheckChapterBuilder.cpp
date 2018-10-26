///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

rptChapter* CLiftingCheckChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec   = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IArtifact,pArtifacts);

   const pgsGirderArtifact* pGirderArtifact = pArtifacts->GetGirderArtifact(girderKey);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   CLiftingCheck check;
   check.Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   return pChapter;
}


CChapterBuilder* CLiftingCheckChapterBuilder::Clone() const
{
   return new CLiftingCheckChapterBuilder;
}
