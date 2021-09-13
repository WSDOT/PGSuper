///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#include "StdAfx.h"
#include "resource.h"
#include <Reporting\MomentCapacityReportSpecificationBuilder.h>
#include <Reporting\MomentCapacityReportSpecification.h>
#include "SelectMomentCapacitySectionDlg.h"

#include <IFace\Selection.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMomentCapacityReportSpecificationBuilder::CMomentCapacityReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CMomentCapacityReportSpecificationBuilder::~CMomentCapacityReportSpecificationBuilder(void)
{
}

std::shared_ptr<CReportSpecification> CMomentCapacityReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,std::shared_ptr<CReportSpecification>& pOldRptSpec)
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
   }

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      segmentKey.groupIndex = 0;
   }

   if ( segmentKey.girderIndex == ALL_GIRDERS )
   {
      segmentKey.girderIndex = 0;
   }

   if ( segmentKey.segmentIndex == ALL_SEGMENTS )
   {
      segmentKey.segmentIndex = 0;
   }

   if ( selection.Type != CSelection::Segment )
   {
      GET_IFACE(IBridge,pBridge);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
      segmentKey.segmentIndex = (nSegments-1)/2;
   }

   GET_IFACE(IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT( vPoi.size() == 1 );
   const pgsPointOfInterest& current_poi = vPoi.front();

   std::shared_ptr<CMomentCapacityReportSpecification> pInitRptSpec( std::dynamic_pointer_cast<CMomentCapacityReportSpecification>(pOldRptSpec) );

   CSelectMomentCapacitySectionDlg dlg(m_pBroker,pInitRptSpec);
   dlg.m_InitialPOI = current_poi;

   if ( dlg.DoModal() == IDOK )
   {
      std::shared_ptr<CReportSpecification> pRptSpec;
      if (pInitRptSpec)
      {
         std::shared_ptr<CMomentCapacityReportSpecification>pGRptSpec(std::make_shared<CMomentCapacityReportSpecification>(*pInitRptSpec) );
         pGRptSpec->SetOptions( dlg.GetPOI(),dlg.m_MomentType == 0 ? true : false );

         pRptSpec = pGRptSpec;
      }
      else
      {
         pRptSpec = std::make_shared<CMomentCapacityReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.GetPOI(),dlg.m_MomentType == 0 ? true : false);
      }

      rptDesc.ConfigureReportSpecification(pRptSpec);

      return pRptSpec;
   }

   return nullptr;
}

std::shared_ptr<CReportSpecification> CMomentCapacityReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // there is no default configuration for this report. The user must be prompted every time for
   // the station information.

   // a future improvement might be to cache the last station range used and use it again ???
   return CreateReportSpec(rptDesc,std::shared_ptr<CReportSpecification>());
}
