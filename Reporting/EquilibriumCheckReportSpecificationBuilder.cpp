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
#include <Reporting\EquilibriumCheckReportSpecificationBuilder.h>
#include <Reporting\EquilibriumCheckReportSpecification.h>
#include <Reporting\EquilibriumCheckDlg.h>

#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEquilibriumCheckReportSpecificationBuilder::CEquilibriumCheckReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CEquilibriumCheckReportSpecificationBuilder::~CEquilibriumCheckReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CEquilibriumCheckReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CSegmentKey segmentKey;
   if ( selection.Type == CSelection::Girder )
   {
      segmentKey.groupIndex   = selection.GroupIdx;
      segmentKey.girderIndex  = selection.GirderIdx;
      segmentKey.segmentIndex = 0;
   }
   else if ( selection.Type == CSelection::Segment )
   {
      segmentKey.groupIndex   = selection.GroupIdx;
      segmentKey.girderIndex  = selection.GirderIdx;
      segmentKey.segmentIndex = selection.SegmentIdx;
   }
   else
   {
      segmentKey.groupIndex   = 0;
      segmentKey.girderIndex  = 0;
      segmentKey.segmentIndex = 0;
   }

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi( pPOI->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L) );
   ATLASSERT( vPoi.size() == 1 );
   pgsPointOfInterest initial_poi = vPoi.front();

   boost::shared_ptr<CEquilibriumCheckReportSpecification> pInitRptSpec( boost::dynamic_pointer_cast<CEquilibriumCheckReportSpecification>(pRptSpec) );

   CEquilibriumCheckDlg dlg(m_pBroker,pInitRptSpec,initial_poi,0);

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CEquilibriumCheckReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.GetPOI(),dlg.GetInterval()) );

      rptDesc.ConfigureReportSpecification(pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CEquilibriumCheckReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // always prompt
   boost::shared_ptr<CReportSpecification> nullSpec;
   return CreateReportSpec(rptDesc,nullSpec);
}
