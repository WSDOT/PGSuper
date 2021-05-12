///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <Reporting\LRFDTimeDependentShrinkageStrainChapterBuilder.h>
#include <Reporting\ACI209ShrinkageStrainChapterBuilder.h>
#include <Reporting\CEBFIPShrinkageStrainChapterBuilder.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CShrinkageStrainChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CShrinkageStrainChapterBuilder::CShrinkageStrainChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CShrinkageStrainChapterBuilder::GetName() const
{
   return TEXT("Shrinkage Strain Details");
}

rptChapter* CShrinkageStrainChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter;
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   ATLASSERT( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP );
   if ( pLossParams->GetTimeDependentModel() == pgsTypes::tdmAASHTO )
   {
      pChapter = CLRFDTimeDependentShrinkageStrainChapterBuilder().Build(pRptSpec,level);
   }
   else if ( pLossParams->GetTimeDependentModel() == pgsTypes::tdmACI209 )
   {
      pChapter = CACI209ShrinkageStrainChapterBuilder().Build(pRptSpec,level);
   }
   else if ( pLossParams->GetTimeDependentModel() == pgsTypes::tdmCEBFIP )
   {
      pChapter = CCEBFIPShrinkageStrainChapterBuilder().Build(pRptSpec,level);
   }
   else
   {
      ATLASSERT(false);
   }

   return pChapter;
}

CChapterBuilder* CShrinkageStrainChapterBuilder::Clone() const
{
   return new CShrinkageStrainChapterBuilder;
}
