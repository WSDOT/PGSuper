///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Pier SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\CopyPierPropertiesReportSpecificationBuilder.h>
#include <Reporting\CopyPierPropertiesReportSpecification.h>


CCopyPierPropertiesReportSpecificationBuilder::CCopyPierPropertiesReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
   CBrokerReportSpecificationBuilder(pBroker)
{
}

CCopyPierPropertiesReportSpecificationBuilder::~CCopyPierPropertiesReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CCopyPierPropertiesReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
   std::shared_ptr<CCopyPierPropertiesReportSpecification> pOldTLMRptSpec(std::dynamic_pointer_cast<CCopyPierPropertiesReportSpecification>(pOldRptSpec));

   std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
   if (pOldTLMRptSpec)
   {
      std::shared_ptr<CCopyPierPropertiesReportSpecification> pNewTLMRptSpec(std::make_shared<CCopyPierPropertiesReportSpecification>(*pOldTLMRptSpec));
//      pNewTLMRptSpec->SetCopyPierProperties(pOldTLMRptSpec->GetCopyPierProperties());

     pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewTLMRptSpec);
   }
   else
   {
      pNewRptSpec = std::make_shared<CCopyPierPropertiesReportSpecification>(rptDesc.GetReportName(), m_pBroker);
   }

   return pNewRptSpec;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CCopyPierPropertiesReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // Use all chapters at the maximum level
   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CCopyPierPropertiesReportSpecification>(rptDesc.GetReportName(), m_pBroker));

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}
