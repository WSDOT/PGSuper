///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include "TexasIBNSChapterBuilder.h"
#include "TexasIBNSParagraphBuilder.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\GirderLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS	CTexasIBNSChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasIBNSChapterBuilder::CTexasIBNSChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
/*--------------------------------------------------------------------*/
LPCTSTR CTexasIBNSChapterBuilder::GetName() const
{
   return TEXT("Girder Schedule");
}

/*--------------------------------------------------------------------*/
rptChapter* CTexasIBNSChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CMultiGirderReportSpecification* pReportSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pReportSpec->GetBroker(&pBroker);

   std::vector<SpanGirderHashType> list = pReportSpec->GetGirderList();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   // Loop over all requested girders and report spec check results
   for (std::vector<SpanGirderHashType>::iterator it=list.begin(); it!=list.end(); it++)
   {
      SpanIndexType span;
      GirderIndexType gdr;
      UnhashSpanGirder(*it,&span,&gdr);

      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,gdr);

      if( pGdrArtifact->Passed() )
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << color(Green) << _T("The Specification Check for Span ")
                << LABEL_SPAN(span)<<_T(", Girder ")<<LABEL_GIRDER(gdr) 
                << _T(" was Successful") << color(Black) << rptNewLine;
      }
      else
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << color(Red) << _T("The Specification Check for Span ")
                << LABEL_SPAN(span)<<_T(", Girder ")<<LABEL_GIRDER(gdr) 
                << _T(" was Not Successful") << color(Black);
      }
   }

   // let the paragraph builder to all the work here...
   bool doEjectPage;
   CTexasIBNSParagraphBuilder parabuilder;
   rptParagraph* pcontent = parabuilder.Build(pBroker, list, pDisplayUnits, level, doEjectPage);

   *pChapter << pcontent;

   return pChapter;
}

/*--------------------------------------------------------------------*/
CChapterBuilder* CTexasIBNSChapterBuilder::Clone() const
{
   return new CTexasIBNSChapterBuilder;
}

