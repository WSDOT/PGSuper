///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <Reporting\HaulingCheckDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHaulingCheckDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHaulingCheckDetailsChapterBuilder::CHaulingCheckDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CHaulingCheckDetailsChapterBuilder::GetName() const
{
   return TEXT("Hauling Check Details");
}

rptChapter* CHaulingCheckDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (!pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      rptParagraph* p = new rptParagraph;
      *pChapter << p;
      *p <<color(Red)<<_T("Hauling analysis disabled in Project Criteria library entry. No analysis performed.")<<color(Black)<<rptNewLine;
   }
   else
   {
      // Artifact does heavy lifting
      GET_IFACE2(pBroker,IArtifact,pArtifacts);
      const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);
      const pgsHaulingAnalysisArtifact* pHaulArtifact = pArtifact->GetHaulingAnalysisArtifact();

      pHaulArtifact->BuildHaulingDetailsReport(span, girder, pChapter, pBroker, pDisplayUnits);
   }

   return pChapter;
}



CChapterBuilder* CHaulingCheckDetailsChapterBuilder::Clone() const
{
   return new CHaulingCheckDetailsChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
