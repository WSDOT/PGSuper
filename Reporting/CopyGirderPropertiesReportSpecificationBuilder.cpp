///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\CopyGirderPropertiesReportSpecificationBuilder.h>
#include <Reporting\CopyGirderPropertiesReportSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCopyGirderPropertiesReportSpecificationBuilder::CCopyGirderPropertiesReportSpecificationBuilder(IBroker* pBroker) :
   CBrokerReportSpecificationBuilder(pBroker)
{
}

CCopyGirderPropertiesReportSpecificationBuilder::~CCopyGirderPropertiesReportSpecificationBuilder(void)
{
}

std::shared_ptr<CReportSpecification> CCopyGirderPropertiesReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc, std::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
   std::shared_ptr<CCopyGirderPropertiesReportSpecification> pOldTLMRptSpec(std::dynamic_pointer_cast<CCopyGirderPropertiesReportSpecification>(pOldRptSpec));

   std::shared_ptr<CReportSpecification> pNewRptSpec;
   if (pOldTLMRptSpec)
   {
      std::shared_ptr<CCopyGirderPropertiesReportSpecification> pNewTLMRptSpec(std::make_shared<CCopyGirderPropertiesReportSpecification>(*pOldTLMRptSpec));
//      pNewTLMRptSpec->SetCopyGirderProperties(pOldTLMRptSpec->GetCopyGirderProperties());

     pNewRptSpec = std::static_pointer_cast<CReportSpecification>(pNewTLMRptSpec);
   }
   else
   {
      pNewRptSpec = std::make_shared<CCopyGirderPropertiesReportSpecification>(rptDesc.GetReportName(), m_pBroker);
   }

   return pNewRptSpec;
}

std::shared_ptr<CReportSpecification> CCopyGirderPropertiesReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Use all chapters at the maximum level
   std::shared_ptr<CReportSpecification> pRptSpec(std::make_shared<CCopyGirderPropertiesReportSpecification>(rptDesc.GetReportName(), m_pBroker));

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}
