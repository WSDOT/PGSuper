///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <IFace\Selection.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBridgeAnalysisReportSpecificationBuilder::CBridgeAnalysisReportSpecificationBuilder(IBroker* pBroker) :
CGirderReportSpecificationBuilder(pBroker)
{
}

CBridgeAnalysisReportSpecificationBuilder::~CBridgeAnalysisReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CBridgeAnalysisReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for girder and chapter list
   GET_IFACE(ISelection,pSelection);
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();
   gdrIdx = (gdrIdx == INVALID_INDEX ? 0 : gdrIdx );

   CBridgeAnalysisReportDlg dlg(m_pBroker,rptDesc,pRptSpec); // span only mode
   dlg.m_Girder = gdrIdx;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CBridgeAnalysisReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Girder,dlg.m_bDesign,dlg.m_bRating) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CBridgeAnalysisReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // always prompt
   boost::shared_ptr<CReportSpecification> nullSpec;
   return CreateReportSpec(rptDesc,nullSpec);
}
