///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <Reporting\InitialStrainAnalysisDlg.h>

#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInitialStrainAnalysisReportSpecificationBuilder::CInitialStrainAnalysisReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CInitialStrainAnalysisReportSpecificationBuilder::~CInitialStrainAnalysisReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CInitialStrainAnalysisReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   GET_IFACE(ISelection,pSelection);
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

   boost::shared_ptr<CInitialStrainAnalysisReportSpecification> pInitRptSpec( boost::dynamic_pointer_cast<CInitialStrainAnalysisReportSpecification>(pRptSpec) );

   CInitialStrainAnalysisDlg dlg(m_pBroker,pInitRptSpec,girderKey,0);

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CInitialStrainAnalysisReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.GetGirderKey(),dlg.GetInterval()) );

      rptDesc.ConfigureReportSpecification(pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CInitialStrainAnalysisReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // always prompt
   boost::shared_ptr<CReportSpecification> nullSpec;
   return CreateReportSpec(rptDesc,nullSpec);
}
