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
#include <Reporting\BearingTimeStepDetailsReportSpecificationBuilder.h>
#include <Reporting\BearingTimeStepDetailsReportSpecification.h>
#include "BearingTimeStepDetailsDlg.h"

#include <IFace/Tools.h>
#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <Reporting/ReactionInterfaceAdapters.h>
#include <IFace/BearingDesignParameters.h>



CBearingTimeStepDetailsReportSpecificationBuilder::CBearingTimeStepDetailsReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CBearingTimeStepDetailsReportSpecificationBuilder::~CBearingTimeStepDetailsReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CBearingTimeStepDetailsReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   std::shared_ptr<CBearingTimeStepDetailsReportSpecification> pInitRptSpec( std::dynamic_pointer_cast<CBearingTimeStepDetailsReportSpecification>(pOldRptSpec) );

   GET_IFACE2(GetBroker(),IBridge, pBridge);
   GET_IFACE2(GetBroker(),IPointOfInterest, pPOI);
   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   GET_IFACE2(GetBroker(),IBearingDesignParameters, pBearingDesignParameters);
   SHEARDEFORMATIONDETAILS details;
   CGirderKey girderKey;

   ReactionLocation initial_location;
   if (pInitRptSpec)
   {
       initial_location = pInitRptSpec->GetReactionLocation();
   }
   else
   {
       GET_IFACE2(GetBroker(),ISelection, pSelection);
       CSelection selection = pSelection->GetSelection();
       if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment)
       {
           girderKey.groupIndex = 0;
           girderKey.girderIndex = selection.GirderIdx;
       }
       else
       {
           girderKey.groupIndex = 0;
           girderKey.girderIndex = 0;
       }

       GET_IFACE2(GetBroker(),IBearingDesign, pBearingDesign);
       GET_IFACE2(GetBroker(), IPointOfInterest, pPoi);

       IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
       std::unique_ptr<CmbLsBearingDesignReactionAdapter> pForces(std::make_unique<CmbLsBearingDesignReactionAdapter>(pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));
       ReactionLocationContainer vReactionLocations = pForces->GetBearingReactionLocations(lastCompositeDeckIntervalIdx, girderKey, pBridge, pPoi, pBearingDesign);
       initial_location = vReactionLocations.front();   //get first location based on girder
   }
   
   pBearingDesignParameters->GetBearingParameters(girderKey, &details);

   PoiList vPoi;
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey.girderIndex, details.startGroup, details.endGroup, &vGirderKeys);
   for (const auto& thisGirderKey : vGirderKeys)
   {
       PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(thisGirderKey.groupIndex);
       PierIndexType endPierIdx = pBridge->GetGirderGroupEndPier(thisGirderKey.groupIndex);
       for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
       {
           if (pierIdx == startPierIdx)
           {
               CSegmentKey segmentKey(thisGirderKey, 0);
               PoiList segPoi;
               pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &segPoi);
               vPoi.push_back(segPoi.front());
           }
           else if (pierIdx == endPierIdx)
           {
               SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
               CSegmentKey segmentKey(thisGirderKey, nSegments - 1);
               PoiList segPoi;
               pPOI->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &segPoi);
               vPoi.push_back(segPoi.front());
           }
           else
           {
               Float64 Xgp;
               VERIFY(pBridge->GetPierLocation(thisGirderKey, pierIdx, &Xgp));
               pgsPointOfInterest poi = pPOI->ConvertGirderPathCoordinateToPoi(thisGirderKey, Xgp);
               vPoi.push_back(poi);
           }
       }
   }



   CBearingTimeStepDetailsDlg dlg(GetBroker(), pInitRptSpec, initial_location, INVALID_INDEX);

   if ( dlg.DoModal() == IDOK )
   {
      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pInitRptSpec)
      {
         std::shared_ptr<CBearingTimeStepDetailsReportSpecification> pNewGRptSpec(std::make_shared<CBearingTimeStepDetailsReportSpecification>(*pInitRptSpec) );

         pNewGRptSpec->SetOptions(dlg.UseAllLocations(),dlg.GetReactionLocation(),dlg.GetInterval());

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CBearingTimeStepDetailsReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.UseAllLocations(),dlg.GetReactionLocation(),dlg.GetInterval());
      }

      rptDesc.ConfigureReportSpecification(pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CBearingTimeStepDetailsReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // always prompt
   return CreateReportSpec(rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>());
}
