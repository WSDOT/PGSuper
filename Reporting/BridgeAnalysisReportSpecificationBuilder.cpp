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
#include <Reporting\BridgeAnalysisReportSpecificationBuilder.h>
#include <Reporting\BridgeAnalysisReportSpecification.h>
#include <Reporting\BridgeAnalysisReportDlg.h>

#include <IFace/Tools.h>
#include <IFace\Selection.h>



CBridgeAnalysisReportSpecificationBuilder::CBridgeAnalysisReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CGirderLineReportSpecificationBuilder(pBroker)
{
}

CBridgeAnalysisReportSpecificationBuilder::~CBridgeAnalysisReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CBridgeAnalysisReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for girder and chapter list
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   GirderIndexType girder = pSelection->GetSelectedGirder().girderIndex;

   CBridgeAnalysisReportDlg dlg(GetBroker(), rptDesc, pOldRptSpec); // span only mode
   dlg.m_SegmentKey.girderIndex = girder;

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      auto pOldGRptSpec = std::dynamic_pointer_cast<CBridgeAnalysisReportSpecification>(pOldRptSpec);

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         std::shared_ptr<CBridgeAnalysisReportSpecification> pNewGRptSpec(std::make_shared<CBridgeAnalysisReportSpecification>(*pOldGRptSpec) );
         pNewGRptSpec->SetGirderIndex(dlg.m_SegmentKey.girderIndex);
         pNewGRptSpec->SetOptions(dlg.m_bDesign, dlg.m_bRating);

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CBridgeAnalysisReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.m_SegmentKey.girderIndex,dlg.m_bDesign,dlg.m_bRating);
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CBridgeAnalysisReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   return CreateReportSpec(rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>());
}
