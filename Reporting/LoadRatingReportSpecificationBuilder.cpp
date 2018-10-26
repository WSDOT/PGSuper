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

#include "stdafx.h"
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <IFace\RatingSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLoadRatingReportSpecificationBuilder::CLoadRatingReportSpecificationBuilder(IBroker* pBroker) :
CGirderReportSpecificationBuilder(pBroker)
{
}

CLoadRatingReportSpecificationBuilder::~CLoadRatingReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CLoadRatingReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      return CGirderReportSpecificationBuilder::CreateReportSpec(rptDesc,pRptSpec);
   }
   else
   {
      AfxMessageBox(_T("No rating types defined. Select Project | Load Rating Options to select rating types"));
      return boost::shared_ptr<CReportSpecification>();
   }
}

boost::shared_ptr<CReportSpecification> CLoadRatingReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      return CGirderReportSpecificationBuilder::CreateDefaultReportSpec(rptDesc);
   }
   else
   {
      AfxMessageBox(_T("No rating types defined. Select Project | Load Rating Options to select rating types"));
      return boost::shared_ptr<CReportSpecification>();
   }
}
