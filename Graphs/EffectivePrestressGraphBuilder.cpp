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
#include "resource.h"
#include <Graphs/EffectivePrestressGraphBuilder.h>
#include <Graphs/DrawBeamTool.h>
#include <Graphs/ExportGraphXYTool.h>
#include "EffectivePrestressGraphController.h"
#include "EffectivePrestressGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF/AutoProgress.h>
#include <Units\UnitValueNumericalFormatTools.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\DocumentType.h>
#include <IFace/PointOfInterest.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <MFCTools\MFCTools.h>

#include <algorithm>


BEGIN_MESSAGE_MAP(CEffectivePrestressGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CEffectivePrestressGraphBuilder::CEffectivePrestressGraphBuilder() :
CGirderGraphBuilderBase()
{
   SetName(_T("Effective Prestress"));
   
   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_EFFECTIVE_PRESTRESS);
}

CEffectivePrestressGraphBuilder::CEffectivePrestressGraphBuilder(const CEffectivePrestressGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CEffectivePrestressGraphBuilder::~CEffectivePrestressGraphBuilder()
{
}

BOOL CEffectivePrestressGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != nullptr);
   return m_pGraphController->Create(pParent,IDD_EFFECTIVEPRESTRESS_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

void CEffectivePrestressGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CEffectivePrestressGraphViewController>* pController;
   CComObject<CEffectivePrestressGraphViewController>::CreateInstance(&pController);
   pController->Init((CEffectivePrestressGraphController*)m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CEffectivePrestressGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CEffectivePrestressGraphBuilder>(*this);
}

void CEffectivePrestressGraphBuilder::UpdateXAxis()
{
   CGirderGraphBuilderBase::UpdateXAxis();
   m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
}

void CEffectivePrestressGraphBuilder::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }

   m_Graph.YAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);
   m_Graph.PinYAxisAtZero(true);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   if ( ((CEffectivePrestressGraphController*)m_pGraphController)->IsStressGraph() )
   {
      const WBFL::Units::StressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new WBFL::Units::StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      m_Graph.SetYAxisTitle(std::_tstring(_T("fpe (")+m_pYFormat->UnitTag()+_T(")")).c_str());
   }
   else
   {
      const WBFL::Units::ForceData& forceUnit = pDisplayUnits->GetGeneralForceUnit();
      m_pYFormat = new WBFL::Units::ForceTool(forceUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      m_Graph.SetYAxisTitle(std::_tstring(_T("Ppe (")+m_pYFormat->UnitTag()+_T(")")).c_str());
   }
}

CGirderGraphControllerBase* CEffectivePrestressGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CEffectivePrestressGraphController;
}

bool CEffectivePrestressGraphBuilder::UpdateNow()
{
   GET_IFACE(IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GroupIndexType    grpIdx      = m_pGraphController->GetGirderGroup();
   GirderIndexType   gdrIdx      = m_pGraphController->GetGirder();
   DuctIndexType     ductIdx     = ((CEffectivePrestressGraphController*)m_pGraphController)->GetDuct();
   DuctType ductType = ((CEffectivePrestressGraphController*)m_pGraphController)->GetDuctType();

   UpdateGraphTitle(grpIdx,gdrIdx,ductType,ductIdx);

   UpdateYAxis();

   // get data to graph
   UpdateGraphData(grpIdx,gdrIdx,ductType,ductIdx);

   return true;
}

void CEffectivePrestressGraphBuilder::UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx, DuctType ductType, DuctIndexType ductIdx)
{
   CString strGraphSubTitle;
   GET_IFACE(IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();
   if ( ductIdx == INVALID_INDEX )
   {
      bool bPermanent = ((CEffectivePrestressGraphController*)m_pGraphController)->IsPermanentStrands();
      if (isPGSuper)
      {
         strGraphSubTitle.Format(_T("Span %s Girder %s %s Strands"), LABEL_SPAN(grpIdx), LABEL_GIRDER(gdrIdx), (bPermanent ? _T("Permanent") : _T("Temporary")));
      }
      else
      {
         strGraphSubTitle.Format(_T("Group %d Girder %s %s Strands"), LABEL_GROUP(grpIdx), LABEL_GIRDER(gdrIdx), (bPermanent ? _T("Permanent") : _T("Temporary")));
      }
   }
   else
   {
      ATLASSERT(!isPGSuper);
      if (ductType == Segment)
      {
         strGraphSubTitle.Format(_T("Group %d Girder %s Segment Tendon %d"), LABEL_GROUP(grpIdx), LABEL_GIRDER(gdrIdx), LABEL_DUCT(ductIdx));
      }
      else
      {
         strGraphSubTitle.Format(_T("Group %d Girder %s Tendon %d"), LABEL_GROUP(grpIdx), LABEL_GIRDER(gdrIdx), LABEL_DUCT(ductIdx));
      }
   }
   
   m_Graph.SetTitle(_T("Effective Prestress (without live load)"));
   m_Graph.SetSubtitle(strGraphSubTitle);
}

void CEffectivePrestressGraphBuilder::UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx, DuctType ductType, DuctIndexType ductIdx)
{
   if ( ductIdx == INVALID_INDEX )
   {
      UpdatePretensionGraphData(grpIdx,gdrIdx);
   }
   else
   {
      UpdatePosttensionGraphData(grpIdx,gdrIdx,ductType,ductIdx);
   }
}

void CEffectivePrestressGraphBuilder::UpdatePosttensionGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx, DuctType ductType, DuctIndexType ductIdx)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(grpIdx,gdrIdx,ALL_SEGMENTS);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, &vPoi);

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);


   IntervalIndexType firstIntervalIdx = ((CMultiIntervalGirderGraphControllerBase*)m_pGraphController)->GetFirstInterval();
   IntervalIndexType lastIntervalIdx  = ((CMultiIntervalGirderGraphControllerBase*)m_pGraphController)->GetLastInterval();
   WBFL::Graphing::GraphColor graphColor;

   std::vector<IntervalIndexType> vIntervals = ((CMultiIntervalGirderGraphControllerBase*)m_pGraphController)->GetSelectedIntervals();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx;
   if (ductType == CEffectivePrestressGraphBuilder::Segment)
   {
      stressTendonIntervalIdx = pIntervals->GetFirstSegmentTendonStressingInterval(CGirderKey(grpIdx, gdrIdx));
   }
   else
   {
      stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(CGirderKey(grpIdx, gdrIdx), ductIdx);
   }

   GET_IFACE_NOCHECK(IPosttensionForce,pPTForce); // going to need this in the loop... get it once

   GET_IFACE_NOCHECK(ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE_NOCHECK(IGirderTendonGeometry, pGirderTendonGeometry);

   bool bStresses = ((CEffectivePrestressGraphController*)m_pGraphController)->IsStressGraph();

   // remove all intervals that occur before the tendon is stressed
   vIntervals.erase(std::remove_if(vIntervals.begin(), vIntervals.end(), [&stressTendonIntervalIdx](const auto& intervalIdx) {return intervalIdx < stressTendonIntervalIdx;}), vIntervals.end());

   int penWeight = GRAPH_PEN_WEIGHT;

   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for ( ; intervalIter != intervalIterEnd; intervalIter++ )
   {
      IntervalIndexType intervalIdx = *intervalIter;

      COLORREF color = graphColor.GetColor(intervalIdx-firstIntervalIdx);

      IndexType dataSeries1, dataSeries2, dataSeries3;
      if ( intervalIdx == stressTendonIntervalIdx )
      {
         CString strLabel1, strLabel2, strLabel3;
         strLabel1.Format(_T("%s prior to anchor set"),  bStresses ? _T("fpe") : _T("Fpe"));
         strLabel2.Format(_T("%s after anchor set"),     bStresses ? _T("fpe") : _T("Fpe"));
         strLabel3.Format(_T("Avg. %s after anchor set"),bStresses ? _T("fpe") : _T("Fpe"));
         dataSeries1 = m_Graph.CreateDataSeries(strLabel1,PS_SOLID,penWeight,ORANGE);
         dataSeries2 = m_Graph.CreateDataSeries(strLabel2,PS_SOLID,penWeight,GREEN);
         dataSeries3 = m_Graph.CreateDataSeries(strLabel3,PS_DOT,  penWeight,BLUE);
      }
      else
      {
         CString strLabel;
         strLabel.Format(_T("Interval %d"),LABEL_INTERVAL(intervalIdx));
         dataSeries1 = m_Graph.CreateDataSeries(strLabel,PS_SOLID,penWeight,color);
      }

      auto iter(vPoi.begin());
      auto end(vPoi.end());
      auto xIter(xVals.begin());
      for ( ; iter != end; iter++, xIter++ )
      {
         const pgsPointOfInterest& poi = *iter;

         const auto& segmentKey(poi.GetSegmentKey());
         if (ductType == CEffectivePrestressGraphBuilder::Segment)
         {
            DuctIndexType nDuctsThisSegment = pSegmentTendonGeometry->GetDuctCount(segmentKey);
            if (ductIdx < nDuctsThisSegment && pSegmentTendonGeometry->IsOnDuct(poi))
            {
               Float64 X = *xIter;

               if (stressTendonIntervalIdx == intervalIdx)
               {
                  if (bStresses)
                  {
                     Float64 fpbt = pPTForce->GetSegmentTendonInitialStress(poi, ductIdx, false/*exclude anchor set*/);
                     Float64 fpat = pPTForce->GetSegmentTendonInitialStress(poi, ductIdx, true/*include anchor set*/);
                     Float64 fpe = pPTForce->GetSegmentTendonAverageInitialStress(segmentKey, ductIdx);

                     AddGraphPoint(dataSeries1, X, fpbt);
                     AddGraphPoint(dataSeries2, X, fpat);
                     AddGraphPoint(dataSeries3, X, fpe);
                  }
                  else
                  {
                     Float64 Fpbt = pPTForce->GetSegmentTendonInitialForce(poi, ductIdx, false/*exclude anchor set*/);
                     Float64 Fpat = pPTForce->GetSegmentTendonInitialForce(poi, ductIdx, true/*include anchor set*/);
                     Float64 Fpe = pPTForce->GetSegmentTendonAverageInitialForce(segmentKey, ductIdx);

                     AddGraphPoint(dataSeries1, X, Fpbt);
                     AddGraphPoint(dataSeries2, X, Fpat);
                     AddGraphPoint(dataSeries3, X, Fpe);
                  }
               }
               else
               {
                  if (bStresses)
                  {
                     Float64 fpe = pPTForce->GetSegmentTendonStress(poi, intervalIdx, pgsTypes::End, ductIdx);
                     AddGraphPoint(dataSeries1, X, fpe);
                  }
                  else
                  {
                     Float64 Fpe = pPTForce->GetSegmentTendonForce(poi, intervalIdx, pgsTypes::End, ductIdx);
                     AddGraphPoint(dataSeries1, X, Fpe);
                  }
               }
            }
         }
         else
         {
            if (pGirderTendonGeometry->IsOnDuct(poi, ductIdx))
            {
               Float64 X = *xIter;

               if (stressTendonIntervalIdx == intervalIdx)
               {
                  if (bStresses)
                  {
                     Float64 fpbt = pPTForce->GetGirderTendonInitialStress(poi, ductIdx, false/*exclude anchor set*/);
                     Float64 fpat = pPTForce->GetGirderTendonInitialStress(poi, ductIdx, true/*include anchor set*/);
                     Float64 fpe = pPTForce->GetGirderTendonAverageInitialStress(segmentKey, ductIdx);

                     AddGraphPoint(dataSeries1, X, fpbt);
                     AddGraphPoint(dataSeries2, X, fpat);
                     AddGraphPoint(dataSeries3, X, fpe);
                  }
                  else
                  {
                     Float64 Fpbt = pPTForce->GetGirderTendonInitialForce(poi, ductIdx, false/*exclude anchor set*/);
                     Float64 Fpat = pPTForce->GetGirderTendonInitialForce(poi, ductIdx, true/*include anchor set*/);
                     Float64 Fpe = pPTForce->GetGirderTendonAverageInitialForce(segmentKey, ductIdx);

                     AddGraphPoint(dataSeries1, X, Fpbt);
                     AddGraphPoint(dataSeries2, X, Fpat);
                     AddGraphPoint(dataSeries3, X, Fpe);
                  }
               }
               else
               {
                  if (bStresses)
                  {
                     Float64 fpe = pPTForce->GetGirderTendonStress(poi, intervalIdx, pgsTypes::End, ductIdx);
                     AddGraphPoint(dataSeries1, X, fpe);
                  }
                  else
                  {
                     Float64 Fpe = pPTForce->GetGirderTendonForce(poi, intervalIdx, pgsTypes::End, ductIdx);
                     AddGraphPoint(dataSeries1, X, Fpe);
                  }
               }
            }
         }
      }
   } // next interval
}

void CEffectivePrestressGraphBuilder::UpdatePretensionGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx)
{
   // clear graph
   m_Graph.ClearData();

   std::vector<IntervalIndexType> vIntervals = ((CMultiIntervalGirderGraphControllerBase*)m_pGraphController)->GetSelectedIntervals();
   if ( vIntervals.size() == 0 )
   {
      return; // there is nothing to plot
   }

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CGirderKey girderKey(grpIdx, gdrIdx);
   CSegmentKey segmentKey(girderKey,ALL_SEGMENTS);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, &vPoi);

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);

   GET_IFACE_NOCHECK(IStrandGeometry, pStrandGeom);
   GET_IFACE(IPretensionForce,pPSForce);
   GET_IFACE(IIntervals,pIntervals);

   bool bStresses = ((CEffectivePrestressGraphController*)m_pGraphController)->IsStressGraph();
   bool bPermanent = ((CEffectivePrestressGraphController*)m_pGraphController)->IsPermanentStrands();
   pgsTypes::StrandType strandType = (bPermanent ? pgsTypes::Permanent : pgsTypes::Temporary);

   IntervalIndexType firstIntervalIdx = ((CMultiIntervalGirderGraphControllerBase*)m_pGraphController)->GetFirstInterval();
   WBFL::Graphing::GraphColor graphColor;

   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for ( ; intervalIter != intervalIterEnd; intervalIter++ )
   {
      IntervalIndexType intervalIdx = *intervalIter;

      COLORREF color = graphColor.GetColor(intervalIdx-firstIntervalIdx);

      CString strLabel;
      strLabel.Format(_T("Interval %d"),LABEL_INTERVAL(intervalIdx));

      IndexType dataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,GRAPH_PEN_WEIGHT,color);
      IndexType dataSeries2 = m_Graph.CreateDataSeries(_T(""), PS_SOLID, GRAPH_PEN_WEIGHT, color);

      bool bPermanentStrandReleaseThisInterval = true;
      bool bTemporaryStrandInstallationThisInterval = true;

      auto iter(vPoi.begin());
      auto end(vPoi.end());
      auto xIter(xVals.begin());
      for ( ; iter != end; iter++, xIter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         const CSegmentKey& thisSegmentKey(poi.GetSegmentKey());

         IntervalIndexType stressStrandIntervalIdx = pIntervals->GetStressStrandInterval(thisSegmentKey);
         IntervalIndexType releaseIntervalIdx      = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

         IntervalIndexType stressTempStrandIntervalIdx       = pIntervals->GetTemporaryStrandStressingInterval(thisSegmentKey);
         IntervalIndexType tempStrandInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(thisSegmentKey);
         IntervalIndexType tempStrandRemovalIntervalIdx      = pIntervals->GetTemporaryStrandRemovalInterval(thisSegmentKey);

         bPermanentStrandReleaseThisInterval &= (intervalIdx == releaseIntervalIdx);
         bTemporaryStrandInstallationThisInterval &= (intervalIdx == tempStrandInstallationIntervalIdx);

         if ( !bPermanent && // plotting for temporary strands - AND -
               ((tempStrandInstallationIntervalIdx == INVALID_INDEX) || // temp strands not installed - OR -
               (intervalIdx < stressTempStrandIntervalIdx || tempStrandRemovalIntervalIdx < intervalIdx)) // temp strands don't exist in this interval
            )
         {
            continue; // continue
         }

         Float64 X = *xIter;

         // Plot for Permanent Strands
         pgsTypes::IntervalTimeType time = pgsTypes::End; // always plot fpe at end of interval
         if ( (bPermanent && (intervalIdx == stressStrandIntervalIdx || intervalIdx == releaseIntervalIdx))
               ||
               (!bPermanent && (intervalIdx == stressTempStrandIntervalIdx || intervalIdx == tempStrandInstallationIntervalIdx))
            )
         {
            time = pgsTypes::Start; // except if this is the strand stress or release intervals
                                    // we want to capture fpj and strand stress just before
                                    // release so there isn't any elastic shortening effect, only initial relaxation
         }

         Float64 Fpe = pPSForce->GetPrestressForce(poi, strandType, intervalIdx, time, true/*include elastic effects*/, pgsTypes::TransferLengthType::Minimum);
         if (bStresses)
         {
            Fpe = pPSForce->GetEffectivePrestress(poi, strandType, intervalIdx, time);
         }
         AddGraphPoint(dataSeries, X, Fpe);

         if ( (bPermanent && intervalIdx == releaseIntervalIdx)
              ||
              (!bPermanent && intervalIdx == tempStrandInstallationIntervalIdx)
            )
         {
            // if this is at release, also plot at the end of the interval so we capture the
            // elastic shortening that occurs during this interval
            Float64 Fpe = pPSForce->GetPrestressForce(poi, strandType, intervalIdx, pgsTypes::End, pgsTypes::TransferLengthType::Minimum);
            if (bStresses)
            {
               Fpe = pPSForce->GetEffectivePrestress(poi, strandType, intervalIdx, pgsTypes::End);
            }
            AddGraphPoint(dataSeries2, X, Fpe);
         }
      } // next poi

      if (bPermanentStrandReleaseThisInterval || bTemporaryStrandInstallationThisInterval)
      {
         strLabel.Format(_T("Interval %d, before xfer"), LABEL_INTERVAL(intervalIdx));
         m_Graph.SetDataLabel(dataSeries, strLabel);

         strLabel.Format(_T("Interval %d, after xfer"), LABEL_INTERVAL(intervalIdx));
         m_Graph.SetDataLabel(dataSeries2, strLabel);

      }
   } // next interval
}

void CEffectivePrestressGraphBuilder::GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx)
{
   CEffectivePrestressGraphController* pMyGraphController = (CEffectivePrestressGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());
   if (0 < vIntervals.size())
   {
      *pFirstIntervalIdx = vIntervals.front();
      *pLastIntervalIdx = vIntervals.back();
   }
   else
   {
      CGirderKey girderKey = pMyGraphController->GetGirderKey();
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
      *pFirstIntervalIdx = intervalIdx;
      *pLastIntervalIdx = *pFirstIntervalIdx;
   }
}

void CEffectivePrestressGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph,rstrDefaultFileName);
}