///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <Reporting\TimelineChapterBuilder.h>
#include <Reporting\BrokerReportSpecification.h>
#include <Reporting\TimelineManagerReportSpecification.h>

#include <IFace\Project.h>
#include <IFace\DocumentType.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimelineChapterBuilder::CTimelineChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTimelineChapterBuilder::GetName() const
{
   return TEXT("Construction Timeline");
}

rptChapter* CTimelineChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   auto pTimelineMgrRptSpec = std::dynamic_pointer_cast<const CTimelineManagerReportSpecification>(pRptSpec);
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   CGirderKey girderKey;
   if (pGirderRptSpec)
   {
      girderKey = pGirderRptSpec->GetGirderKey();
   }

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2_NOCHECK(pBroker, IUserDefinedLoadData, pUserDefinedLoads);
   GET_IFACE2_NOCHECK(pBroker, IDocumentType, pDocType);

   const auto* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTimelineManager* pTimelineMgr;
   if (pTimelineMgrRptSpec)
   {
      pTimelineMgr = pTimelineMgrRptSpec->GetTimelineManager();
   }
   else
   {
      pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pEventTable = rptStyleManager::CreateDefaultTable(7);
   *pPara << pEventTable << rptNewLine;

   pEventTable->EnableRowStriping(false);

   ColumnIndexType col = 0;
   (*pEventTable)(0,col++) << _T("Event");
   (*pEventTable)(0, col++) << _T("Occurance") << rptNewLine << _T("(Days)");
   (*pEventTable)(0, col++) << _T("Elapsed Time") << rptNewLine << _T("(Days)");

   pEventTable->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pEventTable->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   (*pEventTable)(0,col++) << _T("Description");

   pEventTable->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pEventTable->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   (*pEventTable)(0, col++) << _T("Activity");

   pEventTable->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pEventTable->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   (*pEventTable)(0, col++) << _T("Activity Details");

   pEventTable->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pEventTable->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   (*pEventTable)(0, col++) << _T("Items");

   RowIndexType row  = pEventTable->GetNumberOfHeaderRows();
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for (EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++)
   {
      col = 0;

      const auto* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      (*pEventTable)(row, col++) << LABEL_EVENT(eventIdx);
      (*pEventTable)(row, col++) << pTimelineEvent->GetDay();

      const auto* pNextTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx + 1);
      if(pNextTimelineEvent == nullptr)
      {
         (*pEventTable)(row, col++) << _T("");
      }
      else
      {
         (*pEventTable)(row, col++) << pNextTimelineEvent->GetDay() - pTimelineEvent->GetDay();
      }

      (*pEventTable)(row,col++) << pTimelineEvent->GetDescription();

      int nActivities = 0;
      const auto& constructSegmentActivity = pTimelineEvent->GetConstructSegmentsActivity();
      if (constructSegmentActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << (pDocType->IsPGSuperDocument() ? _T("Construct Girders") : _T("Construct Segments"));
         (*pEventTable)(row, col + 1) << _T("Time from strand stressing to release: ") << constructSegmentActivity.GetRelaxationTime() << _T(" days") << rptNewLine;
         (*pEventTable)(row, col + 1) << _T("Total curing duration: ") << constructSegmentActivity.GetTotalCuringDuration() << _T(" days") << rptNewLine;

         const auto& segments = constructSegmentActivity.GetSegments();
         for (const auto& segmentID : segments)
         {
            const auto* pSegment = pBridgeDesc->FindSegment(segmentID);
            ATLASSERT(pSegment);
            if (girderKey == pSegment->GetSegmentKey())
            {
               (*pEventTable)(row, col + 2) << SEGMENT_LABEL(pSegment->GetSegmentKey()) << rptNewLine;
            }
         }
         nActivities++;
      }

      const auto& erectPierActivity = pTimelineEvent->GetErectPiersActivity();
      if (erectPierActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Erect Piers and Temporary Supports");
         const auto& piers = erectPierActivity.GetPiers();
         for (const auto& pierID : piers)
         {
            const auto* pPier = pBridgeDesc->FindPier(pierID);
            ATLASSERT(pPier);
            (*pEventTable)(row, col + 1) << _T("") << rptNewLine;
            (*pEventTable)(row, col + 2) << GetLabel(pPier, pDisplayUnits) << rptNewLine;
         }

         const auto& ts = erectPierActivity.GetTempSupports();
         for (const auto& tsID : ts)
         {
            const auto* pTS = pBridgeDesc->FindTemporarySupport(tsID);
            ATLASSERT(pTS);
            (*pEventTable)(row, col + 1) << _T("") << rptNewLine;
            (*pEventTable)(row, col + 2) << GetLabel(pTS, pDisplayUnits) << rptNewLine;
         }

         nActivities++;
      }

      const auto& erectSegmentsActivity = pTimelineEvent->GetErectSegmentsActivity();
      if (erectSegmentsActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << (pDocType->IsPGSuperDocument() ? _T("Erect Girders") : _T("Erect Segments"));
         const auto& segments = erectSegmentsActivity.GetSegments();
         for (const auto& segmentID : segments)
         {
            const auto* pSegment = pBridgeDesc->FindSegment(segmentID);
            ATLASSERT(pSegment);
            if (girderKey == pSegment->GetSegmentKey())
            {
               (*pEventTable)(row, col + 1) << _T("") << rptNewLine;
               (*pEventTable)(row, col + 2) << SEGMENT_LABEL(pSegment->GetSegmentKey()) << rptNewLine;
            }
         }

         nActivities++;
      }

      const auto& removeTemporarySupportsActivity = pTimelineEvent->GetRemoveTempSupportsActivity();
      if (removeTemporarySupportsActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Remove Temporary Supports");
         const auto& ts = removeTemporarySupportsActivity.GetTempSupports();
         for (const auto& tsID : ts)
         {
            const auto* pTS = pBridgeDesc->FindTemporarySupport(tsID);
            ATLASSERT(pTS);
            (*pEventTable)(row, col + 1) << _T("") << rptNewLine;
            (*pEventTable)(row, col + 2) << GetLabel(pTS, pDisplayUnits) << rptNewLine;
         }

         nActivities++;
      }

      const auto& castClosureJointActivity = pTimelineEvent->GetCastClosureJointActivity();
      if (castClosureJointActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Cast Closure Joints");

         (*pEventTable)(row, col + 1) << _T("Total curing duration: ") << castClosureJointActivity.GetTotalCuringDuration() << _T(" days") << rptNewLine;
         (*pEventTable)(row, col + 1) << _T("Active curing duration: ") << castClosureJointActivity.GetActiveCuringDuration() << _T(" days") << rptNewLine;

         const auto& piers = castClosureJointActivity.GetPiers();
         for (const auto& pierID : piers)
         {
            const auto* pPier = pBridgeDesc->FindPier(pierID);
            ATLASSERT(pPier);
            (*pEventTable)(row, col + 2) << GetLabel(pPier, pDisplayUnits) << rptNewLine;
         }

         const auto& ts = castClosureJointActivity.GetTempSupports();
         for (const auto& tsID : ts)
         {
            const auto* pTS = pBridgeDesc->FindTemporarySupport(tsID);
            ATLASSERT(pTS);
            (*pEventTable)(row, col + 2) << GetLabel(pTS, pDisplayUnits) << rptNewLine;
         }

         nActivities++;
      }

      const auto& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
      if (castDeckActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Cast Deck");

         (*pEventTable)(row, col + 1) << _T("Total curing duration: ") << castDeckActivity.GetTotalCuringDuration() << _T(" days") << rptNewLine;
         (*pEventTable)(row, col + 1) << _T("Active curing duration: ") << castDeckActivity.GetActiveCuringDuration() << _T(" days") << rptNewLine;

         if (castDeckActivity.GetCastingType() == CCastDeckActivity::Continuous)
         {
            (*pEventTable)(row, col + 2) << _T("Continuous casting");
         }
         else
         {
            (*pEventTable)(row, col + 1) << _T("Time between castings: ") << castDeckActivity.GetTimeBetweenCasting() << _T(" days") << rptNewLine;
            (*pEventTable)(row, col + 1) << _T("Number of casting regions: ") << castDeckActivity.GetCastingRegionCount() << rptNewLine;

            if (pTimelineEvent->GetCastClosureJointActivity().IsEnabled())
            {
               (*pEventTable)(row, col + 1) << _T("Closure joint cast with region: ") << LABEL_INDEX(castDeckActivity.GetClosureJointCastingRegion()) << rptNewLine;
            }

            (*pEventTable)(row, col + 2) << _T("Staged casting");
         }

         nActivities++;
      }

      const auto& castLongitudinalJointActivity = pTimelineEvent->GetCastLongitudinalJointActivity();
      if (castLongitudinalJointActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Cast Longitudinal Joints") << rptNewLine;

         (*pEventTable)(row, col + 1) << _T("Total curing duration: ") << castLongitudinalJointActivity.GetTotalCuringDuration() << _T(" days") << rptNewLine;
         (*pEventTable)(row, col + 1) << _T("Active curing duration: ") << castLongitudinalJointActivity.GetActiveCuringDuration() << _T(" days") << rptNewLine;

         (*pEventTable)(row, col + 2) << _T("") << rptNewLine;

         nActivities++;
      }

      const auto& stressTendonActivity = pTimelineEvent->GetStressTendonActivity();
      if (stressTendonActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Stress Tendons") << rptNewLine;
         (*pEventTable)(row, col + 1) << _T("") << rptNewLine;

         const auto& tendons = stressTendonActivity.GetTendons();
         for (const auto& tendonKey : tendons)
         {
            const auto* pGirder = pBridgeDesc->FindGirder(tendonKey.girderID);
            ATLASSERT(pGirder);
            if (girderKey == pGirder->GetGirderKey())
            {
               (*pEventTable)(row, col + 2) << GIRDER_LABEL(pGirder->GetGirderKey()) << _T(", Duct ") << LABEL_DUCT(tendonKey.ductIdx) << rptNewLine;
            }
         }

         nActivities++;
      }

      const auto& applyLoadActivity = pTimelineEvent->GetApplyLoadActivity();
      if (applyLoadActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         (*pEventTable)(row, col) << _T("Apply Loads") << rptNewLine;
         (*pEventTable)(row, col + 1) << _T("") << rptNewLine;

         if (applyLoadActivity.IsIntermediateDiaphragmLoadApplied())
         {
            (*pEventTable)(row, col + 2) << _T("Intermediate Diaphragms") << rptNewLine;
         }

         if (applyLoadActivity.IsRailingSystemLoadApplied())
         {
            (*pEventTable)(row, col + 2) << _T("Traffic Barriers\\Railing System") << rptNewLine;
         }

         if (applyLoadActivity.IsOverlayLoadApplied())
         {
            (*pEventTable)(row, col + 2) << _T("Overlay") << rptNewLine;
         }

         if (applyLoadActivity.IsLiveLoadApplied())
         {
            (*pEventTable)(row, col + 2) << _T("Open to Traffic, Live Load") << rptNewLine;
         }

         if (applyLoadActivity.IsRatingLiveLoadApplied())
         {
            (*pEventTable)(row, col + 2) << _T("Load Rating Live Loads") << rptNewLine;
         }

         if (applyLoadActivity.IsUserLoadApplied())
         {
            (*pEventTable)(row, col + 2) << _T("User Defined Loads") << rptNewLine;
            const auto& userLoadIDs = applyLoadActivity.GetUserLoads();
            for (const auto& loadID : userLoadIDs)
            {
               const auto* pLoad = pUserDefinedLoads->FindPointLoad(loadID);
               if (pLoad && (girderKey.girderIndex == ALL_GIRDERS || pLoad->m_SpanKey.girderIndex == girderKey.girderIndex))
               {
                  (*pEventTable)(row, col + 2) << GetLoadDescription(pLoad) << rptNewLine;
               }
            }

            for (const auto& loadID : userLoadIDs)
            {
               const auto* pLoad = pUserDefinedLoads->FindDistributedLoad(loadID);
               if (pLoad && (girderKey.girderIndex == ALL_GIRDERS || pLoad->m_SpanKey.girderIndex == girderKey.girderIndex))
               {
                  (*pEventTable)(row, col + 2) << GetLoadDescription(pLoad) << rptNewLine;
               }
            }

            for (const auto& loadID : userLoadIDs)
            {
               const auto* pLoad = pUserDefinedLoads->FindMomentLoad(loadID);
               if (pLoad && (girderKey.girderIndex == ALL_GIRDERS || pLoad->m_SpanKey.girderIndex == girderKey.girderIndex))
               {
                  (*pEventTable)(row, col + 2) << GetLoadDescription(pLoad) << rptNewLine;
               }
            }
         }

         nActivities++;
      }

      const auto& geometryControlActivity = pTimelineEvent->GetGeometryControlActivity();
      if (geometryControlActivity.IsEnabled())
      {
         if (0 < nActivities) row++;

         CString name;
         pgsTypes::GeometryControlActivityType type = geometryControlActivity.GetGeometryControlEventType();
         if (type == pgsTypes::gcaGeometryControlEvent)
         {
            name = _T("Roadway Geometry Control Event");
         }
         else if (type == pgsTypes::gcaSpecCheckEvent)
         {
            name = _T("Roadway Geometry Control Spec Check Only");
         }
         else if (type == pgsTypes::gcaGeometryReportingEvent)
         {
            name = _T("Roadway Geometry Control Report Only");
         }
         else
         {
            ATLASSERT(0);
         }

         (*pEventTable)(row,col) << name << rptNewLine;
         (*pEventTable)(row,col + 1) << _T("") << rptNewLine;
         (*pEventTable)(row,col + 2) << _T("") << rptNewLine;

         nActivities++;
      }


      if (nActivities == 0)
      {
         (*pEventTable)(row, col++) << _T("") << rptNewLine;
         (*pEventTable)(row, col++) << _T("") << rptNewLine;
         (*pEventTable)(row, col++) << _T("") << rptNewLine;
      }
      else if ( 1 < nActivities)
      {
         for (int c = 0; c < col; c++)
         {
            pEventTable->SetRowSpan(row - nActivities + 1, c, nActivities, true);
         }
      }

      row++;
   }

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CTimelineChapterBuilder::Clone() const
{
   return std::make_unique<CTimelineChapterBuilder>();
}
