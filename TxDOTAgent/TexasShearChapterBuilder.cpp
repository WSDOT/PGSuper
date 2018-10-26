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
#include <Reporting\ReportNotes.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\ConfinementCheckTable.h>
#include <Reporting\OptionalDeflectionCheck.h>
#include <Reporting\LongReinfShearCheck.h>
#include <Reporting\GirderDetailingCheck.h>
#include "TexasShearChapterBuilder.h"

#include <PgsExt\PointOfInterest.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <psgLib\ConnectionLibraryEntry.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CTexasShearChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasShearChapterBuilder::CTexasShearChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTexasShearChapterBuilder::GetName() const
{
   return TEXT("Shear");
}

rptChapter* CTexasShearChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType span = pSpec->GetSpan();
   GirderIndexType girder = pSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,girder);

   // Vertical Shear check
   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   bool bStrutAndTieRequired;
   *p << CShearCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI,bStrutAndTieRequired) << rptNewLine;
   if ( bStrutAndTieRequired )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
   }
   else
   {
      p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << p;
      *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;
   }

   if ( bPermit )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CShearCheckTable().Build(pBroker,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;

      if ( bStrutAndTieRequired )
      {
         p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << p;
         *p << STRUT_AND_TIE_REQUIRED << rptNewLine << rptNewLine;
      }
      else
      {
         p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
         *pChapter << p;
         *p << SUPPORT_COMPRESSION << rptNewLine << rptNewLine;
      }
   }

   // Interface Shear check
   if ( pBridge->IsCompositeDeck() )
   {
      CInterfaceShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthI);

      if ( bPermit )
         CInterfaceShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pgsTypes::StrengthII);
   }

   // Longitudinal reinforcement for shear
   CLongReinfShearCheck().Build(pChapter,pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::StrengthI,pDisplayUnits);

   if ( bPermit )
   {
      CLongReinfShearCheck().Build(pChapter,pBroker,span,girder,pgsTypes::BridgeSite3,pgsTypes::StrengthII,pDisplayUnits);
   }

   // Girder Detailing
   CGirderDetailingCheck(true).Build(pChapter,pBroker,span,girder,pDisplayUnits);

   return pChapter;
}

CChapterBuilder* CTexasShearChapterBuilder::Clone() const
{
   return new CTexasShearChapterBuilder;
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
