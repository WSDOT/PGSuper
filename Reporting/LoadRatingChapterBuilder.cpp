///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <Reporting\LoadRatingChapterBuilder.h>
#include <Reporting\RatingSummaryTable.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
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
   CLoadRatingChapterBuilder
****************************************************************************/

CLoadRatingChapterBuilder::CLoadRatingChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLoadRatingChapterBuilder::GetName() const
{
   return TEXT("Load Rating");
}

rptChapter* CLoadRatingChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   GirderIndexType gdrLineIdx = pGdrRptSpec->GetGirder();

   GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara;

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Design Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << CRatingSummaryTable().BuildByLimitState(pBroker,gdrLineIdx, CRatingSummaryTable::Design ) << rptNewLine;
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Legal Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,gdrLineIdx, pgsTypes::lrLegal_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;

         pTable = CRatingSummaryTable().BuildLoadPosting(pBroker,gdrLineIdx, pgsTypes::lrLegal_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,gdrLineIdx, pgsTypes::lrLegal_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;

         pTable = CRatingSummaryTable().BuildLoadPosting(pBroker,gdrLineIdx, pgsTypes::lrLegal_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      (*pChapter) << pPara;
      pPara->SetName(_T("Permit Load Rating"));
      (*pPara) << pPara->GetName() << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << Super(_T("*")) << _T("MBE 6A.4.5.2 Permit load rating should only be used if the bridge has a rating factor greater than 1.0 when evaluated for AASHTO legal loads.") << rptNewLine;

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,gdrLineIdx, pgsTypes::lrPermit_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;

		 pTable = CRatingSummaryTable().BuildByVehicleStressTable(pBroker,gdrLineIdx,pgsTypes::lrPermit_Routine);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         rptRcTable* pTable = CRatingSummaryTable().BuildByVehicle(pBroker,gdrLineIdx, pgsTypes::lrPermit_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;

		 pTable = CRatingSummaryTable().BuildByVehicleStressTable(pBroker,gdrLineIdx,pgsTypes::lrPermit_Special);
         if ( pTable )
            (*pPara) << pTable << rptNewLine;
      }
   }

   return pChapter;
}

CChapterBuilder* CLoadRatingChapterBuilder::Clone() const
{
   return new CLoadRatingChapterBuilder;
}
