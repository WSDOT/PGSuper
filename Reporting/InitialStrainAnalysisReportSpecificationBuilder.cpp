///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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
#include <Reporting\InitialStrainAnalysisReportSpecificationBuilder.h>
#include <Reporting\InitialStrainAnalysisReportSpecification.h>
#include "InitialStrainAnalysisDlg.h"

#include <IFace/Tools.h>
#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>



CInitialStrainAnalysisReportSpecificationBuilder::CInitialStrainAnalysisReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CInitialStrainAnalysisReportSpecificationBuilder::~CInitialStrainAnalysisReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CInitialStrainAnalysisReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey;
   if ( selection.Type == CSelection::Girder )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }
   else if ( selection.Type == CSelection::Segment )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }
   else
   {
      girderKey.groupIndex   = 0;
      girderKey.girderIndex  = 0;
   }

   // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
   auto pOldGRptSpec( std::dynamic_pointer_cast<CInitialStrainAnalysisReportSpecification>(pOldRptSpec) );

   CInitialStrainAnalysisDlg dlg(GetBroker(), pOldGRptSpec, girderKey, 0);

   if ( dlg.DoModal() == IDOK )
   {
      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         std::shared_ptr<CInitialStrainAnalysisReportSpecification> pNewGRptSpec(std::make_shared<CInitialStrainAnalysisReportSpecification>(*pOldGRptSpec));

         pNewGRptSpec->SetOptions(dlg.GetGirderKey(),dlg.GetInterval());

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CInitialStrainAnalysisReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.GetGirderKey(),dlg.GetInterval());
      }

      rptDesc.ConfigureReportSpecification(pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CInitialStrainAnalysisReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // always prompt
   return CreateReportSpec(rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>());
}
