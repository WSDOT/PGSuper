///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporting\ReportStyleHolder.h>
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
CTexasIBNSChapterBuilder::CTexasIBNSChapterBuilder()
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
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

	/* For broker passed in, get interface information */
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);

   if( pGdrArtifact->Passed() )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << "The Specification Check was Successful" << rptNewLine;
   }
   else
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << "The Specification Check Was Not Successful" << color(Black);
   }

#if defined IGNORE_2007_CHANGES
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   // let the paragraph builder to all the work here...
   CTexasIBNSParagraphBuilder parabuilder;
   rptParagraph* pcontent = parabuilder.Build(pBroker,span,girder,pDisplayUnits,level);

   *pChapter << pcontent;

   return pChapter;
}

/*--------------------------------------------------------------------*/
CChapterBuilder* CTexasIBNSChapterBuilder::Clone() const
{
   return new CTexasIBNSChapterBuilder;
}

