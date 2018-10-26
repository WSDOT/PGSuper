///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <Reporting\LongitudinalReinforcementForShearLoadRatingChapterBuilder.h>
#include <Reporting\RatingSummaryTable.h>

#include <IFace\Artifact.h>

#include <IFace\RatingSpecification.h>
#include <IFace\Bridge.h>

#include <Reporting\LongReinfShearCheck.h>

#include <PgsExt\RatingArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CLongitudinalReinforcementForShearLoadRatingChapterBuilder
****************************************************************************/

CLongitudinalReinforcementForShearLoadRatingChapterBuilder::CLongitudinalReinforcementForShearLoadRatingChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLongitudinalReinforcementForShearLoadRatingChapterBuilder::GetName() const
{
   return TEXT("Longitudinal Reinforcement for Shear Load Rating Checks");
}

rptChapter* CLongitudinalReinforcementForShearLoadRatingChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara;

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Design Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
      {
         if ( pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) )
         {
            CLongReinfShearCheck().Build(pChapter,pBroker,girderKey,pgsTypes::StrengthI_Inventory,pDisplayUnits);
         }
         else
         {
            (*pPara) << _T("Shear rating not computed") << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      {
         if ( pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) )
         {
            CLongReinfShearCheck().Build(pChapter,pBroker,girderKey,pgsTypes::StrengthI_Operating,pDisplayUnits);
         }
         else
         {
            (*pPara) << _T("Shear rating not computed") << rptNewLine;
         }
      }
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Legal Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         if ( pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) )
         {
            CLongReinfShearCheck().Build(pChapter,pBroker,girderKey,pgsTypes::StrengthI_LegalRoutine,pDisplayUnits);
         }
         else
         {
            (*pPara) << _T("Shear rating not computed") << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         if ( pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) )
         {
            CLongReinfShearCheck().Build(pChapter,pBroker,girderKey,pgsTypes::StrengthI_LegalSpecial,pDisplayUnits);
         }
         else
         {
            (*pPara) << _T("Shear rating not computed") << rptNewLine;
         }
      }
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Permit Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << Super(_T("*")) << _T("MBE 6A.4.5.2 Permit load rating should only be used if the bridge has a rating factor greater than 1.0 when evaluated for AASHTO legal loads.") << rptNewLine;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         if ( pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) )
         {
            CLongReinfShearCheck().Build(pChapter,pBroker,girderKey,pgsTypes::StrengthII_PermitRoutine,pDisplayUnits);
         }
         else
         {
            (*pPara) << _T("Shear rating not computed") << rptNewLine;
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         if ( pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) )
         {
            CLongReinfShearCheck().Build(pChapter,pBroker,girderKey,pgsTypes::StrengthII_PermitSpecial,pDisplayUnits);
         }
         else
         {
            (*pPara) << _T("Shear rating not computed") << rptNewLine;
         }
      }
   }
   return pChapter;
}

CChapterBuilder* CLongitudinalReinforcementForShearLoadRatingChapterBuilder::Clone() const
{
   return new CLongitudinalReinforcementForShearLoadRatingChapterBuilder;
}
