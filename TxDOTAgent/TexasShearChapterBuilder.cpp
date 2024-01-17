///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ShearCheckTable.h>
#include <Reporting\InterfaceShearTable.h>
#include <Reporting\ConfinementCheckTable.h>
#include <Reporting\OptionalDeflectionCheck.h>
#include <Reporting\LongReinfShearCheck.h>
#include <Reporting\GirderDetailingCheck.h>
#include <Reporting\DebondCheckTable.h>
#include "TexasShearChapterBuilder.h"

#include <PgsExt\ReportPointOfInterest.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

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

rptChapter* CTexasShearChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   const pgsGirderArtifact* pGirderArtifact = pArtifacts->GetGirderArtifact(girderKey);

   // Vertical Shear check
   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   bool bStrutAndTieRequired;
   *p << CShearCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired) << rptNewLine;
   CShearCheckTable().BuildNotes(pChapter,pBroker,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthI,bStrutAndTieRequired);

   if ( bPermit )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CShearCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired) << rptNewLine;
      CShearCheckTable().BuildNotes(pChapter,pBroker,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthII,bStrutAndTieRequired);
   }

   // Interface Shear check
   if ( pBridge->IsCompositeDeck() )
   {
      CInterfaceShearTable().Build(pBroker,pChapter,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthI);

      if ( bPermit )
         CInterfaceShearTable().Build(pBroker,pChapter,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthII);
   }

   // Longitudinal reinforcement for shear
   CLongReinfShearCheck().Build(pChapter,pBroker,pGirderArtifact,liveLoadIntervalIdx,pgsTypes::StrengthI,pDisplayUnits);

   if ( bPermit )
   {
      CLongReinfShearCheck().Build(pChapter,pBroker,pGirderArtifact,liveLoadIntervalIdx,pgsTypes::StrengthII,pDisplayUnits);
   }

   // Girder Detailing
   CGirderDetailingCheck(true).Build(pChapter,pBroker,pGirderArtifact,pDisplayUnits);

   // Debonding check if applicable
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   if ( pStrandGeometry->GetNumDebondedStrands(CSegmentKey(girderKey,0),pgsTypes::Straight,pgsTypes::dbetEither) )
   {
      CDebondCheckTable().Build(pChapter, pBroker, pGirderArtifact, pDisplayUnits);
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTexasShearChapterBuilder::Clone() const
{
   return std::make_unique<CTexasShearChapterBuilder>();
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
