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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#include "StdAfx.h"
#include "resource.h"
#include <Reporting\CrackedSectionReportSpecificationBuilder.h>
#include <Reporting\CrackedSectionReportSpecification.h>
#include "SelectCrackedSectionDlg.h"

#include <IFace\Selection.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCrackedSectionReportSpecificationBuilder::CCrackedSectionReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CCrackedSectionReportSpecificationBuilder::~CCrackedSectionReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CCrackedSectionReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   pgsPointOfInterest current_poi;

   // initialize dialog for the current cut location
   std::shared_ptr<CCrackedSectionReportSpecification> pInitRptSpec( std::dynamic_pointer_cast<CCrackedSectionReportSpecification>(pOldRptSpec) );
   if (!pInitRptSpec)
   {
      // Fist time through get CL of selected segement
      CSegmentKey segmentKey;
      GET_IFACE(ISelection, pSelection);
      CSelection selection = pSelection->GetSelection();

      if (selection.Type == CSelection::Girder)
      {
         segmentKey.groupIndex = selection.GroupIdx;
         segmentKey.girderIndex = selection.GirderIdx;
      }
      else if (selection.Type == CSelection::Segment)
      {
         segmentKey.groupIndex = selection.GroupIdx;
         segmentKey.girderIndex = selection.GirderIdx;
         segmentKey.segmentIndex = selection.SegmentIdx;
      }
      else
      {
         segmentKey.groupIndex = 0;
         segmentKey.girderIndex = 0;
      }

      if (segmentKey.groupIndex == ALL_GROUPS)
      {
         segmentKey.groupIndex = 0;
      }

      if (segmentKey.girderIndex == ALL_GIRDERS)
      {
         segmentKey.girderIndex = 0;
      }

      if (segmentKey.segmentIndex == ALL_SEGMENTS)
      {
         segmentKey.segmentIndex = 0;
      }

      if (selection.Type != CSelection::Segment)
      {
         GET_IFACE(IBridge, pBridge);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
         segmentKey.segmentIndex = (nSegments - 1) / 2;
      }

      GET_IFACE(IPointOfInterest, pPOI);
      PoiList vPoi;
      pPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      current_poi = vPoi.front();
   }
   else
   {
      // Need to check if POI is still on the bridge. It's possible that girder could have been deleted or shortened
      if (pInitRptSpec->IsValid())
      {
         current_poi = pInitRptSpec->GetPOI();
      }
      else
      {
         // Not on the bridge. Default to something safe
         ::AfxMessageBox(_T("The selected location for this report no longer exists. The report has been moved to a valid location."), MB_OK | MB_ICONEXCLAMATION);
         GET_IFACE(IPointOfInterest, pPOI);
         PoiList vPoi;
         pPOI->GetPointsOfInterest(CSegmentKey(0, 0, 0), POI_5L | POI_ERECTED_SEGMENT, &vPoi);
         ATLASSERT(vPoi.size() == 1);

         current_poi = vPoi.front();
         pInitRptSpec->SetPOI(current_poi);
      }
   }

   CSelectCrackedSectionDlg dlg(m_pBroker,pInitRptSpec);
   dlg.m_InitialPOI = current_poi;

   if ( dlg.DoModal() == IDOK )
   {
      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if (pInitRptSpec)
      {
         // copy settings from existing spec
         std::shared_ptr<CCrackedSectionReportSpecification> pNewGRptSpec(std::make_shared<CCrackedSectionReportSpecification>(*pInitRptSpec) );
         pNewGRptSpec->SetOptions(dlg.GetPOI(),dlg.m_MomentType == 0 ? true : false);

         pNewRptSpec = pNewGRptSpec;
      }
      else
      {
         pNewRptSpec = std::make_shared<CCrackedSectionReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.GetPOI(),dlg.m_MomentType == 0 ? true : false);
      }

      rptDesc.ConfigureReportSpecification(pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CCrackedSectionReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // there is no default configuration for this report. The user must be prompted every time for
   // the station information.

   // a future improvement might be to cache the last station range used and use it again ???
   return CreateReportSpec(rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>());
}
