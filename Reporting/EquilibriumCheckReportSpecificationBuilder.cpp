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

boost::shared_ptr<CReportSpecification> CEquilibriumCheckReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey;
   if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }
   else
   {
      girderKey.groupIndex   = 0;
      girderKey.girderIndex  = 0;
   }

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi( pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS),POI_SPAN | POI_5L) );
   pgsPointOfInterest initial_poi = vPoi.front();

   // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
   boost::shared_ptr<CEquilibriumCheckReportSpecification> pOldGRptSpec = boost::dynamic_pointer_cast<CEquilibriumCheckReportSpecification>(pOldRptSpec);

   CEquilibriumCheckDlg dlg(m_pBroker,pOldGRptSpec,initial_poi,0);

   if ( dlg.DoModal() == IDOK )
   {     

      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         boost::shared_ptr<CEquilibriumCheckReportSpecification> pNewGRptSpec = boost::shared_ptr<CEquilibriumCheckReportSpecification>( new CEquilibriumCheckReportSpecification(*pOldGRptSpec) );

         pNewGRptSpec->SetOptions(dlg.GetPOI(),dlg.GetInterval());

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CEquilibriumCheckReportSpecification>( new CEquilibriumCheckReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.GetPOI(),dlg.GetInterval()) );
      }

      rptDesc.ConfigureReportSpecification(pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CEquilibriumCheckReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // always prompt
   boost::shared_ptr<CReportSpecification> nullSpec;
   return CreateReportSpec(rptDesc,nullSpec);
}
