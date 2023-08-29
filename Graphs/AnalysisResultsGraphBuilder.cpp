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

#include "stdafx.h"
#include "resource.h"
#include <Graphs\AnalysisResultsGraphBuilder.h>
#include <Graphs\DrawBeamTool.h>
#include <Graphs\ExportGraphXYTool.h>
#include "AnalysisResultsGraphController.h"
#include "AnalysisResultsGraphDefinition.h"
#include "AnalysisResultsGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <Units\UnitValueNumericalFormatTools.h>
#include <PgsExt\SegmentArtifact.h>
#include <PgsExt\RatingArtifact.h>

#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Allowables.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Artifact.h>
#include <IFace\PrestressForce.h>
#include <IFace\Selection.h>
#include <IFace\PrincipalWebStress.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <PgsExt\ClosureJointData.h>

#include <MFCTools\MFCTools.h>

#include <WBFLSTL.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Utility class for managing map between principal web stress section elevations and graph data series
struct WebSectionManager
{
   void AddWebSection(const std::_tstring& name, IndexType webSectionIndex)
   {
      // Store unique web sections, building list as we go
      bool found = false;
      for (const auto& section : m_WebSections)
      {
         if (std::get<0>(section) == name)
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         m_WebSections.push_back(std::make_tuple(name, webSectionIndex, INVALID_INDEX,0.0));
      }
   }

   // Create data series for each of our web sections
   void CreateGraphSeries(CAnalysisResultsGraphController* pGraphController, CAnalysisResultsGraphBuilder& rBuilder, WBFL::Graphing::GraphXY& rGraph, const std::_tstring& graphName, IntervalIndexType intervalIdx,IndexType graphIdx)
   {
      IndexType idx = 0;
      for (auto& section : m_WebSections)
      {
         CString fullName;
         if (pGraphController->GetGraphMode() == CAnalysisResultsGraphController::Loading)
         {
            fullName = graphName.c_str() + CString(_T(" - ")) + std::get<0>(section).c_str();
         }
         else
         {
            fullName.Format(_T("%s, Interval %d"), std::get<0>(section).c_str(), LABEL_INTERVAL(intervalIdx));
         }

         IndexType data_series = rGraph.FindDataSeries(fullName);
         if (INVALID_INDEX == data_series)
         {
            COLORREF c(rBuilder.GetGraphColor(graphIdx*8 + idx++, intervalIdx));
            int pen = idx % 2 == 0 ? PS_SOLID : PS_DASH;
            data_series = rGraph.CreateDataSeries(fullName, pen , GRAPH_PEN_WEIGHT, c);
         }

         std::get<2>(section) = data_series;
      }
   }

   IndexType GetNumberOfWebSections() const
   {
      return m_WebSections.size();
   }

   void GetWebSectionData(IndexType wsIdx, IndexType* pSeriesIdx, Float64* pYvalue)
   {
      *pSeriesIdx = std::get<2>(m_WebSections[wsIdx]);
      *pYvalue    = std::get<3>(m_WebSections[wsIdx]);
   }

   void ResetYValues()
   {
      // Set Y values all to zero
      for (auto& webSection : m_WebSections)
      {
         std::get<3>(webSection) = 0.0;
      }
   }

   void SetYValue(const std::_tstring& strLocation, Float64 Yvalue)
   {
      for (auto& section : m_WebSections)
      {
         if (std::get<0>(section) == strLocation)
         {
            std::get<3>(section) = Yvalue;
            return;
         }
      }

      ATLASSERT(0);
   }

private:
   // vector of section name/original web section index/data series/Y value
   typedef std::vector<std::tuple<std::_tstring, IndexType, IndexType, Float64>> WebSectionColl;
   WebSectionColl m_WebSections;
};


inline std::_tstring Shear_Stress_String(const std::_tstring& loadName)
{
   return std::_tstring(loadName + _T(" - Shear"));
}

inline std::_tstring Axial_Stress_String(const std::_tstring& loadName)
{
   return std::_tstring(loadName + _T(" - Axial"));
}


// create a dummy unit conversion tool to pacify the graph constructor
static WBFL::Units::LengthData DUMMY(WBFL::Units::Measure::Meter);
static WBFL::Units::LengthTool DUMMY_TOOL(DUMMY);

// Pen styles for stresses at top and bottom of girder
#define PS_STRESS_TOP_GIRDER     PS_SOLID
#define PS_STRESS_BOTTOM_GIRDER  PS_DASH
#define PS_STRESS_TOP_DECK       PS_DOT
#define PS_STRESS_BOTTOM_DECK    PS_DASHDOT

BEGIN_MESSAGE_MAP(CAnalysisResultsGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CAnalysisResultsGraphBuilder::CAnalysisResultsGraphBuilder() :
CGirderGraphBuilderBase(),
m_pGraphColor(new WBFL::Graphing::GraphColor),
m_pGraphDefinitions(new CAnalysisResultsGraphDefinitions)
{
   Init();
}

CAnalysisResultsGraphBuilder::CAnalysisResultsGraphBuilder(const CAnalysisResultsGraphBuilder& other) :
CGirderGraphBuilderBase(other),
m_pGraphColor(new WBFL::Graphing::GraphColor),
m_pGraphDefinitions(new CAnalysisResultsGraphDefinitions)
{
   Init();
}

CAnalysisResultsGraphBuilder::~CAnalysisResultsGraphBuilder()
{
}

void CAnalysisResultsGraphBuilder::Init()
{
   SetName(_T("Analysis Results - After Erection"));

   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_ANALYSIS_RESULTS);

   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
}

BOOL CAnalysisResultsGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   CGirderKey girderKey(ALL_GROUPS,0);
   GET_IFACE(ISelection, pSelection);
   CSelection selection = pSelection->GetSelection();
   if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment)
   {
      girderKey.groupIndex = selection.GroupIdx;
      girderKey.girderIndex = selection.GirderIdx;
   }
   
   // update the graph definitions before creating the controller. the graph controller
   // uses the graph definitions to initialize the options within its controls
   UpdateGraphDefinitions(girderKey);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != nullptr);
   if ( !m_pGraphController->Create(pParent,IDD_ANALYSISRESULTS_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CAnalysisResultsGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CAnalysisResultsGraphViewController>* pController;
   CComObject<CAnalysisResultsGraphViewController>::CreateInstance(&pController);
   pController->Init((CAnalysisResultsGraphController*)m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

void CAnalysisResultsGraphBuilder::DumpLBAM()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Dumping Analysis Models"));

   GET_IFACE(IProductForces,pProductForces);
   pProductForces->DumpAnalysisModels( m_pGraphController->GetGirder() );

   AfxMessageBox(_T("Analysis Model Dump Complete"),MB_OK);
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CAnalysisResultsGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CAnalysisResultsGraphBuilder>(*this);
}

std::vector<IntervalIndexType> CAnalysisResultsGraphBuilder::AddTSRemovalIntervals(IntervalIndexType loadingIntervalIdx,const std::vector<IntervalIndexType>& vIntervals,const std::vector<IntervalIndexType>& vTSRIntervals)
{
   // given an interval when a loading occors, a vector of intervals to make results available, and a secondary vector of intervals
   // merge all the secondary intervals that occur at or after the loading interval into the primary vector
   std::vector<IntervalIndexType> intervals(vIntervals);

   // search the temporary support removal intervals for the first interval
   // that occurs at or after the loading interval
   std::vector<IntervalIndexType>::const_iterator iter(vTSRIntervals.begin());
   std::vector<IntervalIndexType>::const_iterator end(vTSRIntervals.end());
   for ( ; iter != end; iter++ )
   {
      IntervalIndexType intervalIdx = *iter;
      if (loadingIntervalIdx <= intervalIdx )
      {
         break; // found the first interval that occurs at or after the loading interval
      }
   }

   // insert the remaining intervals into the vector
   intervals.insert(intervals.end(),iter,end);

   // eliminate duplicates
   std::sort(intervals.begin(),intervals.end());
   intervals.erase( std::unique(intervals.begin(),intervals.end()), intervals.end() );
   return intervals;
}

void CAnalysisResultsGraphBuilder::UpdateGraphDefinitions(const CGirderKey& girderKey)
{
   m_pGraphDefinitions->Clear();

   IDType graphID = 0;

   GET_IFACE(IProductLoads,pProductLoads);

   // determine if there are temporary strands or pedestrian load for any of the girders
   // for this group
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   bool bTempStrand = false;
   bool bPedLoading = false;
   bool bSidewalk   = false;
   bool bShearKey   = false;

   for (const auto& thisGirderKey : vGirderKeys)
   {
      bPedLoading |= pProductLoads->HasPedestrianLoad(thisGirderKey);
      bSidewalk   |= pProductLoads->HasSidewalkLoad(thisGirderKey);
      bShearKey   |= pProductLoads->HasShearKeyLoad(thisGirderKey);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
         bTempStrand |= ( 0 < Nt );
      } // next segIdx
   } // next groupIdx

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   bool bLongitudinalJoint = pBridgeDesc->HasStructuralLongitudinalJoints();

   // Get intervals for reporting
   GET_IFACE(IIntervals,pIntervals);

   // spec check intervals
   GET_IFACE(IStressCheck, pStressCheck);
   std::vector<IntervalIndexType> vSpecCheckIntervals(pStressCheck->GetStressCheckIntervals(girderKey));

   // initial intervals
   IntervalIndexType nIntervals               = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetFirstStorageInterval(girderKey);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   IntervalIndexType firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   std::vector<IntervalIndexType> vInitialIntervals;
   vInitialIntervals.push_back(releaseIntervalIdx);
   vInitialIntervals.push_back(storageIntervalIdx);
   vInitialIntervals.push_back(erectSegmentIntervalIdx);

   EventIndexType castDeckEventIdx = pIBridgeDesc->GetCastDeckEventIndex();
   std::vector<IntervalIndexType> vDeckAndDiaphragmIntervals;
   if (castDeckEventIdx != INVALID_INDEX)
   {
      const CTimelineEvent* pEvent = pIBridgeDesc->GetEventByIndex(castDeckEventIdx);
      const auto& castDeckActivity = pEvent->GetCastDeckActivity();
      ATLASSERT(castDeckActivity.IsEnabled());
      IndexType nDeckCastings = castDeckActivity.GetCastingCount(); // number of times deck casting happens (not the same as number of deck casting regions)
      for (IndexType castingIdx = 0; castingIdx < nDeckCastings; castingIdx++)
      {
         std::vector<IndexType> vRegions = castDeckActivity.GetRegions(castingIdx); // regions casted during this casting
         IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(vRegions.front()); // casting interval is the same for all regions during this casting, so just get interval for first region in the list
         vDeckAndDiaphragmIntervals.push_back(castDeckIntervalIdx);
      }
   }

   std::vector<IntervalIndexType> vRailingSystemIntervals;
   for ( IntervalIndexType rintervalIdx = railingSystemIntervalIdx; rintervalIdx < nIntervals; rintervalIdx++ )
   {
      vRailingSystemIntervals.push_back(rintervalIdx);
   }

   std::vector<IntervalIndexType> vOverlayIntervals;
   vOverlayIntervals.push_back(overlayIntervalIdx);
   
   // all intervals
   std::vector<IntervalIndexType> vAllIntervals;
   for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vAllIntervals.push_back(intervalIdx);
   }

   // intervals only after live load is applied
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   std::vector<IntervalIndexType> vLiveLoadIntervals;
   for ( IntervalIndexType intervalIdx = liveLoadIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vLiveLoadIntervals.push_back(intervalIdx);
   }

   // intervals only after load rating live load is applied
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();
   std::vector<IntervalIndexType> vLoadRatingIntervals;
   for ( IntervalIndexType intervalIdx = loadRatingIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vLoadRatingIntervals.push_back(intervalIdx);
   }

   std::vector<IntervalIndexType> vTempSupportRemovalIntervals;
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      IntervalIndexType tsrIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);
      vTempSupportRemovalIntervals.push_back(tsrIntervalIdx);
   }

   //////////////////////////////////////////////////////////////////////////
   // Product Load Cases
   //////////////////////////////////////////////////////////////////////////

   // girder self-weight
   std::vector<IntervalIndexType> intervals( AddTSRemovalIntervals(erectSegmentIntervalIdx,vInitialIntervals,vTempSupportRemovalIntervals) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftGirder), pgsTypes::pftGirder, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION));
   
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftPretension), pgsTypes::pftPretension, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION));

   intervals.clear();
   intervals = AddTSRemovalIntervals(firstCastDeckIntervalIdx,vDeckAndDiaphragmIntervals,vTempSupportRemovalIntervals);
   GET_IFACE(IUserDefinedLoadData,pUserLoads);
   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftConstruction), pgsTypes::pftConstruction,  vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   CAnalysisResultsGraphDefinition diaphragmGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm), pgsTypes::pftDiaphragm, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION);
   diaphragmGraphDef.AddIntervals(vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition(diaphragmGraphDef);

   // slab dead load
   if (pBridge->GetDeckType() != pgsTypes::sdtNone)
   {
      CAnalysisResultsGraphDefinition slabGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSlab), pgsTypes::pftSlab, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION);
      slabGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(slabGraphDef);

      CAnalysisResultsGraphDefinition haunchGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad), pgsTypes::pftSlabPad, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION);
      haunchGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(haunchGraphDef);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      CAnalysisResultsGraphDefinition slabPanelGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel), pgsTypes::pftSlabPanel, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION);
      slabPanelGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition( slabPanelGraphDef );
   }

   if (bShearKey)
   {
      CAnalysisResultsGraphDefinition shearKeyGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftShearKey), pgsTypes::pftShearKey, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION);
      shearKeyGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(shearKeyGraphDef);
   }

   if (bLongitudinalJoint)
   {
      CAnalysisResultsGraphDefinition shearKeyGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint), pgsTypes::pftLongitudinalJoint, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION);
      shearKeyGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(shearKeyGraphDef);
   }

   // Deck shrinkage is different animal
   GET_IFACE(ILosses, pLosses);
   if( pLosses->IsDeckShrinkageApplicable() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Deck Shrinkage"),   graphDeckShrinkageStress ,   vRailingSystemIntervals) );
   }

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), pgsTypes::pftPostTensioning, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSecondaryEffects),      pgsTypes::pftSecondaryEffects,      vAllIntervals, ACTIONS_FORCE_STRESS | ACTIONS_X_DEFLECTION) );
   }


   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier), pgsTypes::pftTrafficBarrier, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );

   if ( bSidewalk )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk), pgsTypes::pftSidewalk, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), pgsTypes::pftOverlay, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );


   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
   if ( prestress_loss_criteria.LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftCreep),      pgsTypes::pftCreep,      vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage),  pgsTypes::pftShrinkage,  vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), pgsTypes::pftRelaxation, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   // User Defined Static Loads
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftUserDC),   pgsTypes::pftUserDC,   vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftUserDW),   pgsTypes::pftUserDW,   vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM), pgsTypes::pftUserLLIM, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );


   ////////////////////////////////////////////////////////
   // Live Load 
   ////////////////////////////////////////////////////////

   // Individual Truck Responses
   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   std::vector<pgsTypes::LiveLoadType> vLiveLoadTypes;
   vLiveLoadTypes.push_back(pgsTypes::lltDesign);
   vLiveLoadTypes.push_back(pgsTypes::lltFatigue);

   if ( bPermit )
   {
      vLiveLoadTypes.push_back(pgsTypes::lltPermit);
   }

   std::vector<pgsTypes::LiveLoadType>::iterator iter;
   for ( iter = vLiveLoadTypes.begin(); iter != vLiveLoadTypes.end(); iter++ )
   {
      pgsTypes::LiveLoadType llType = *iter;

      std::_tstring strBase;
      switch(llType)
      {
      case pgsTypes::lltDesign:
         strBase = _T("Design");
         break;

      case pgsTypes::lltFatigue:
         strBase = _T("Fatigue");
         break;

      case pgsTypes::lltPermit:
         strBase = _T("Permit");
         break;

      case pgsTypes::lltLegalRating_Routine:
         strBase = _T("Legal Rating (Routine)");
         break;

      case pgsTypes::lltLegalRating_Special:
         strBase = _T("Legal Rating (Special)");
         break;

      case pgsTypes::lltLegalRating_Emergency:
         strBase = _T("Legal Rating (Emergency)");
         break;

      case pgsTypes::lltPermitRating_Routine:
         strBase = _T("Permit Rating (Routine)");
         break;

      case pgsTypes::lltPermitRating_Special:
         strBase = _T("Permit Rating (Special)");
         break;

      default:
         ATLASSERT(false); // should never get here
      }

      std::vector<std::_tstring> strLLNames( pProductLoads->GetVehicleNames(llType,girderKey) );
      long colorIdx = 0;
      int action;
      switch(llType)
      {
      case pgsTypes::lltDesign:
      case pgsTypes::lltFatigue:
      case pgsTypes::lltLegalRating_Routine:
      case pgsTypes::lltLegalRating_Special:
      case pgsTypes::lltLegalRating_Emergency:
      case pgsTypes::lltPermitRating_Routine:
      case pgsTypes::lltPermitRating_Special:
         action = ACTIONS_ALL;
         break;

      case pgsTypes::lltPermit:
         action = ACTIONS_FORCE_DEFLECTION;
         break;

      default:
         ATLASSERT(false);
      }

      VehicleIndexType vehicleIdx = 0;
      std::vector<std::_tstring>::iterator iter(strLLNames.begin());
      std::vector<std::_tstring>::iterator end(strLLNames.end());
      for ( ; iter != end; iter++, vehicleIdx++ )
      {
         std::_tstring& strName( *iter );

         // skip the dummy live load
         if ( strName == NO_LIVE_LOAD_DEFINED )
         {
            continue;
         }

         std::_tstring strLLName( strBase + _T(" - ") + strName );

         CAnalysisResultsGraphDefinition def(graphID++,
                                             strLLName,
                                             llType,
                                             vehicleIdx,
                                             vLiveLoadIntervals,
                                             action);

         m_pGraphDefinitions->AddGraphDefinition(def);
      }

      std::_tstring strLLName( strBase + _T(" - LL+IM") );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName, llType,  INVALID_INDEX, vLiveLoadIntervals,  ACTIONS_ALL) );
   }

   std::vector<pgsTypes::LoadRatingType> vLoadRatingTypes;
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory))
   {
      vLoadRatingTypes.push_back(pgsTypes::lrDesign_Inventory);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating))
   {
      vLoadRatingTypes.push_back(pgsTypes::lrDesign_Operating);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine))
   {
      vLoadRatingTypes.push_back(pgsTypes::lrLegal_Routine);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      vLoadRatingTypes.push_back(pgsTypes::lrLegal_Special);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      vLoadRatingTypes.push_back(pgsTypes::lrLegal_Emergency);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      vLoadRatingTypes.push_back(pgsTypes::lrPermit_Routine);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      vLoadRatingTypes.push_back(pgsTypes::lrPermit_Special);
   }

   for(auto ratingType : vLoadRatingTypes)
   {
      pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);
      std::_tstring strBase;
      switch(llType)
      {
      case pgsTypes::lltDesign:
         strBase = _T("Design");
         break;

      case pgsTypes::lltFatigue:
         strBase = _T("Fatigue");
         break;

      case pgsTypes::lltPermit:
         strBase = _T("Permit");
         break;

      case pgsTypes::lltLegalRating_Routine:
         strBase = _T("Legal Rating (Routine)");
         break;

      case pgsTypes::lltLegalRating_Special:
         strBase = _T("Legal Rating (Special)");
         break;

      case pgsTypes::lltLegalRating_Emergency:
         strBase = _T("Legal Rating (Emergency)");
         break;

      case pgsTypes::lltPermitRating_Routine:
         strBase = _T("Permit Rating (Routine)");
         break;

      case pgsTypes::lltPermitRating_Special:
         strBase = _T("Permit Rating (Special)");
         break;

      default:
         ATLASSERT(false); // should never get here
      }

      std::vector<std::_tstring> strLLNames( pProductLoads->GetVehicleNames(llType,girderKey) );
      long colorIdx = 0;
      int action;
      switch(llType)
      {
      case pgsTypes::lltDesign:
      case pgsTypes::lltFatigue:
      case pgsTypes::lltLegalRating_Routine:
      case pgsTypes::lltLegalRating_Special:
      case pgsTypes::lltLegalRating_Emergency:
      case pgsTypes::lltPermitRating_Routine:
      case pgsTypes::lltPermitRating_Special:
         action = ACTIONS_ALL;
         break;

      case pgsTypes::lltPermit:
         action = ACTIONS_FORCE_DEFLECTION;
         break;

      default:
         ATLASSERT(false);
      }

      pgsTypes::LimitState limitState = GetStrengthLimitStateType(ratingType);
      pgsTypes::LimitState stressLimitState = GetServiceLimitStateType(ratingType);

      std::_tstring strRating;
      if (ratingType == pgsTypes::lrDesign_Inventory)
      {
         strRating = _T("Inventory");
      }
      else if (ratingType == pgsTypes::lrDesign_Operating)
      {
         strRating = _T("Operating");
      }
      else
      {
         strRating = strBase;
      }
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strRating + _T(", Moment"), limitState, graphLoadRating, actionMoment, INVALID_INDEX, vLoadRatingIntervals));

      if (pRatingSpec->RateForShear(ratingType))
      {
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strRating + _T(", Shear"), limitState, graphLoadRating, actionShear, INVALID_INDEX, vLoadRatingIntervals));
      }

      if (pRatingSpec->RateForStress(ratingType))
      {
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strRating + _T(", Stress"), stressLimitState, graphLoadRating, actionStress, INVALID_INDEX, vLoadRatingIntervals));
      }


      VehicleIndexType vehicleIdx = 0;
      std::vector<std::_tstring>::iterator iter(strLLNames.begin());
      std::vector<std::_tstring>::iterator end(strLLNames.end());
      for (; iter != end; iter++, vehicleIdx++)
      {
         std::_tstring& strName(*iter);

         // skip the dummy live load
         if (strName == NO_LIVE_LOAD_DEFINED)
         {
            continue;
         }

         std::_tstring strLLName(strRating + _T(" - ") + strName);

         CAnalysisResultsGraphDefinition def(graphID++,
            strLLName,
            llType,
            vehicleIdx,
            vLoadRatingIntervals,
            action);

         m_pGraphDefinitions->AddGraphDefinition(def);

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName + _T(", Moment"), limitState, graphLoadRating, actionMoment, vehicleIdx, vLoadRatingIntervals));

         if (pRatingSpec->RateForShear(ratingType))
         {
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName + _T(", Shear"), limitState, graphLoadRating, actionShear, vehicleIdx, vLoadRatingIntervals));
         }

         if (pRatingSpec->RateForStress(ratingType))
         {
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName + _T(", Stress"), stressLimitState, graphLoadRating, actionStress, vehicleIdx, vLoadRatingIntervals));
         }
      }

      if (ratingType != pgsTypes::lrDesign_Inventory && ratingType != pgsTypes::lrDesign_Operating)
      {
         std::_tstring strLLName( strBase + _T(" - LL+IM") );
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName, llType,  INVALID_INDEX, vLoadRatingIntervals,  ACTIONS_ALL) );
      }
   }

   // Combined Results
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcDC), lcDC, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcDW), lcDW, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );

   if ( prestress_loss_criteria.LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcCR), lcCR, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcSH), lcSH, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcRE), lcRE, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcPS), lcPS, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   if ( bPedLoading )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("PL"),   pgsTypes::lltPedestrian, vLiveLoadIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Design)"), pgsTypes::lltDesign, vLiveLoadIntervals, ACTIONS_ALL) );

   if (WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Fatigue)"), pgsTypes::lltFatigue, vLiveLoadIntervals, ACTIONS_ALL) );
   }

   if (bPermit)
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit)"), pgsTypes::lltPermit, vLiveLoadIntervals, ACTIONS_ALL) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Routine)"), pgsTypes::lltLegalRating_Routine, vLoadRatingIntervals, ACTIONS_ALL) );
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Special)"), pgsTypes::lltLegalRating_Special, vLoadRatingIntervals, ACTIONS_ALL));
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Emergency)"), pgsTypes::lltLegalRating_Emergency, vLoadRatingIntervals, ACTIONS_ALL));
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Routine)"), pgsTypes::lltPermitRating_Routine, vLoadRatingIntervals, ACTIONS_ALL) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Special)"), pgsTypes::lltPermitRating_Special, vLoadRatingIntervals, ACTIONS_ALL) );
   }

   // Limit States and Capacities
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I (Design)"), pgsTypes::ServiceI, vAllIntervals, ACTIONS_ALL_NO_REACTION | ACTIONS_X_DEFLECTION) );
   
   if (WBFL::LRFD::LRFDVersionMgr::GetVersion() < WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA (Design)"), pgsTypes::ServiceIA, vLiveLoadIntervals, ACTIONS_STRESS) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III (Design)"),          pgsTypes::ServiceIII,               vLiveLoadIntervals,  ACTIONS_STRESS | ACTIONS_SHEAR) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design)"),           pgsTypes::StrengthI,                vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design)"),  pgsTypes::StrengthI, graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR) );
   
   if (WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I"), pgsTypes::FatigueI, vLiveLoadIntervals, ACTIONS_STRESS) );
   }

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bStrII = pLimitStateForces->IsStrengthIIApplicable(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0));

   if ( bStrII )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Permit)"),          pgsTypes::StrengthII,               vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Permit)"), pgsTypes::StrengthII,graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Inventory)"),           pgsTypes::StrengthI_Inventory,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Inventory)"),  pgsTypes::StrengthI_Inventory, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Operating)"),           pgsTypes::StrengthI_Operating,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Operating)"),  pgsTypes::StrengthI_Operating, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Routine)"),           pgsTypes::StrengthI_LegalRoutine,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Legal Rating, Routine)"),  pgsTypes::StrengthI_LegalRoutine, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR) );
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Special)"), pgsTypes::StrengthI_LegalSpecial, vLoadRatingIntervals, ACTIONS_MOMENT_SHEAR));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity, (Legal Rating, Special)"), pgsTypes::StrengthI_LegalSpecial, graphCapacity, vLoadRatingIntervals, ACTIONS_SHEAR));
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Emergency)"), pgsTypes::StrengthI_LegalEmergency, vLoadRatingIntervals, ACTIONS_MOMENT_SHEAR));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity, (Legal Rating, Emergency)"), pgsTypes::StrengthI_LegalEmergency, graphCapacity, vLoadRatingIntervals, ACTIONS_SHEAR));
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Routine Permit Rating)"),           pgsTypes::StrengthII_PermitRoutine,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Routine Permit Rating)"),  pgsTypes::StrengthII_PermitRoutine, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Special Permit Rating)"),           pgsTypes::StrengthII_PermitSpecial,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Special Permit Rating)"),  pgsTypes::StrengthII_PermitSpecial, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Moment Capacity"),      pgsTypes::StrengthI, graphCapacity,    vLiveLoadIntervals,  ACTIONS_MOMENT) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Min Moment Capacity"),  pgsTypes::StrengthI, graphMinCapacity, vLiveLoadIntervals,  ACTIONS_MOMENT) );

   // Demand and Allowable
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Demand (Design)"),     pgsTypes::ServiceI,  graphDemand,    vAllIntervals,ACTIONS_STRESS | ACTIONS_DEFLECTION | ACTIONS_X_DEFLECTION) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Limit (Design)"),  pgsTypes::ServiceI,  graphAllowable, vSpecCheckIntervals) );
   
   if (WBFL::LRFD::LRFDVersionMgr::GetVersion() < WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Demand (Design)"),    pgsTypes::ServiceIA, graphDemand,    vLiveLoadIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Limit (Design)"), pgsTypes::ServiceIA, graphAllowable, vLiveLoadIntervals) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design)"),   pgsTypes::ServiceIII,graphDemand,    vLiveLoadIntervals) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Design)"),pgsTypes::ServiceIII,graphAllowable, vLiveLoadIntervals) );

   ISpecification::PrincipalWebStressCheckType pswType = pSpec->GetPrincipalWebStressCheckType(CSegmentKey(girderKey, 0));
   if (ISpecification::pwcNotApplicable != pswType)
   {
      if (ISpecification::pwcNCHRPTimeStepMethod == pswType)
      {
         // for time step method, we add a bunch of graphs
         // Shear stress due to product loads
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftGirder)), pgsTypes::pftGirder, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftPretension)), pgsTypes::pftPretension, vAllIntervals, ACTIONS_WEB_STRESS));

         CAnalysisResultsGraphDefinition diaphragmGraphDef(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm)), pgsTypes::pftDiaphragm, vAllIntervals, ACTIONS_WEB_STRESS);
         diaphragmGraphDef.AddIntervals(vTempSupportRemovalIntervals);
         m_pGraphDefinitions->AddGraphDefinition(diaphragmGraphDef);

         // slab dead load
         if (pBridge->GetDeckType() != pgsTypes::sdtNone)
         {
            CAnalysisResultsGraphDefinition slabGraphDef(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSlab)), pgsTypes::pftSlab, vAllIntervals, ACTIONS_WEB_STRESS);
            slabGraphDef.AddIntervals(vTempSupportRemovalIntervals);
            m_pGraphDefinitions->AddGraphDefinition(slabGraphDef);

            CAnalysisResultsGraphDefinition haunchGraphDef(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad)), pgsTypes::pftSlabPad, vAllIntervals, ACTIONS_WEB_STRESS);
            haunchGraphDef.AddIntervals(vTempSupportRemovalIntervals);
            m_pGraphDefinitions->AddGraphDefinition(haunchGraphDef);
         }

         GET_IFACE(IDocumentType, pDocType);
         if (pDocType->IsPGSpliceDocument())
         {
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning)), pgsTypes::pftPostTensioning, vAllIntervals, ACTIONS_WEB_STRESS));
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSecondaryEffects)), pgsTypes::pftSecondaryEffects, vAllIntervals, ACTIONS_WEB_STRESS));
         }


         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier)), pgsTypes::pftTrafficBarrier, vAllIntervals, ACTIONS_WEB_STRESS));

         if (bSidewalk)
         {
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk)), pgsTypes::pftSidewalk, vAllIntervals, ACTIONS_WEB_STRESS));
         }

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftOverlay)), pgsTypes::pftOverlay, vAllIntervals, ACTIONS_WEB_STRESS));

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftCreep)), pgsTypes::pftCreep, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage)), pgsTypes::pftShrinkage, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation)), pgsTypes::pftRelaxation, vAllIntervals, ACTIONS_WEB_STRESS));

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftUserDC)), pgsTypes::pftUserDC, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftUserDW)), pgsTypes::pftUserDW, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Shear_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM)), pgsTypes::pftUserLLIM, vAllIntervals, ACTIONS_WEB_STRESS));

         // Axial stress due to product loads
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftGirder)), pgsTypes::pftGirder, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftPretension)), pgsTypes::pftPretension, vAllIntervals, ACTIONS_WEB_STRESS));

         CAnalysisResultsGraphDefinition diaphragmGraphDefA(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm)), pgsTypes::pftDiaphragm, vAllIntervals, ACTIONS_WEB_STRESS);
         diaphragmGraphDefA.AddIntervals(vTempSupportRemovalIntervals);
         m_pGraphDefinitions->AddGraphDefinition(diaphragmGraphDefA);

         // slab dead load
         if (pBridge->GetDeckType() != pgsTypes::sdtNone)
         {
            CAnalysisResultsGraphDefinition slabGraphDef(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSlab)), pgsTypes::pftSlab, vAllIntervals, ACTIONS_WEB_STRESS);
            slabGraphDef.AddIntervals(vTempSupportRemovalIntervals);
            m_pGraphDefinitions->AddGraphDefinition(slabGraphDef);

            CAnalysisResultsGraphDefinition haunchGraphDef(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad)), pgsTypes::pftSlabPad, vAllIntervals, ACTIONS_WEB_STRESS);
            haunchGraphDef.AddIntervals(vTempSupportRemovalIntervals);
            m_pGraphDefinitions->AddGraphDefinition(haunchGraphDef);
         }

         if (pDocType->IsPGSpliceDocument())
         {
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning)), pgsTypes::pftPostTensioning, vAllIntervals, ACTIONS_WEB_STRESS));
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSecondaryEffects)), pgsTypes::pftSecondaryEffects, vAllIntervals, ACTIONS_WEB_STRESS));
         }

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier)), pgsTypes::pftTrafficBarrier, vAllIntervals, ACTIONS_WEB_STRESS));

         if (bSidewalk)
         {
            m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk)), pgsTypes::pftSidewalk, vAllIntervals, ACTIONS_WEB_STRESS));
         }

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftOverlay)), pgsTypes::pftOverlay, vAllIntervals, ACTIONS_WEB_STRESS));

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftCreep)), pgsTypes::pftCreep, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage)), pgsTypes::pftShrinkage, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation)), pgsTypes::pftRelaxation, vAllIntervals, ACTIONS_WEB_STRESS));

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftUserDC)), pgsTypes::pftUserDC, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftUserDW)), pgsTypes::pftUserDW, vAllIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, Axial_Stress_String(pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM)), pgsTypes::pftUserLLIM, vAllIntervals, ACTIONS_WEB_STRESS));

         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM Web Shear Stress (Design)"), pgsTypes::lltDesign, vLiveLoadIntervals, ACTIONS_WEB_STRESS));
         m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM Web Axial Stress (Design)"), pgsTypes::lltDesign, vLiveLoadIntervals, ACTIONS_WEB_STRESS));
      } // principal web stress

      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Web Shear Stress (Design)"), pgsTypes::ServiceIII, graphWebShearStress, vLiveLoadIntervals, ACTIONS_WEB_STRESS));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Web Axial Stress (Design)"), pgsTypes::ServiceIII, graphWebAxialStress, vLiveLoadIntervals, ACTIONS_WEB_STRESS));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Principal Stress Demand (Design)"), pgsTypes::ServiceIII, graphPrincipalWebStressDemand, vLiveLoadIntervals, ACTIONS_WEB_STRESS));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Principal Stress Limit (Design)"), pgsTypes::ServiceIII, graphPrincipalWebStressLimit, vLiveLoadIntervals, ACTIONS_WEB_STRESS));
   }

   if (WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Demand"),    pgsTypes::FatigueI, graphDemand,    vLiveLoadIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Limit"), pgsTypes::FatigueI, graphAllowable, vLiveLoadIntervals) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design Rating, Inventory)"),   pgsTypes::ServiceIII_Inventory,graphDemand,    vLoadRatingIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Design Rating, Inventory)"),pgsTypes::ServiceIII_Inventory,graphAllowable, vLoadRatingIntervals) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Routine)"),   pgsTypes::ServiceIII_LegalRoutine,graphDemand,    vLoadRatingIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Legal Rating, Routine)"),pgsTypes::ServiceIII_LegalRoutine,graphAllowable, vLoadRatingIntervals) );
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Special)"), pgsTypes::ServiceIII_LegalSpecial, graphDemand, vLoadRatingIntervals));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Legal Rating, Special)"), pgsTypes::ServiceIII_LegalSpecial, graphAllowable, vLoadRatingIntervals));
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Emergency)"), pgsTypes::ServiceIII_LegalEmergency, graphDemand, vLoadRatingIntervals));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Legal Rating, Emergency)"), pgsTypes::ServiceIII_LegalEmergency, graphAllowable, vLoadRatingIntervals));
   }
}

std::vector<std::pair<std::_tstring,IDType>> CAnalysisResultsGraphBuilder::GetLoadings(IntervalIndexType intervalIdx,ActionType actionType)
{
   return m_pGraphDefinitions->GetLoadings(intervalIdx,actionType);
}

GraphType CAnalysisResultsGraphBuilder::GetGraphType(IDType graphID)
{
   CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);
   return graphDef.m_GraphType;
}

CGirderGraphControllerBase* CAnalysisResultsGraphBuilder::CreateGraphController()
{
   return new CAnalysisResultsGraphController;
}

bool CAnalysisResultsGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   UpdateYAxisUnits();
   UpdateXAxisTitle();
   UpdateGraphTitle();

   // we want a tighter zero tolerance for rotations
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   Float64 tolerance = m_ZeroToleranceY;
   if ( actionType == actionRotation )
   {
      m_ZeroToleranceY = 1e-07;
   }

   UpdateGraphData();

   m_ZeroToleranceY = tolerance;

   return true;
}

void CAnalysisResultsGraphBuilder::UpdateYAxisUnits()
{
   delete m_pYFormat;

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(actionType)
   {
   case actionMoment:
      {
      const WBFL::Units::MomentData& momentUnit = pDisplayUnits->GetMomentUnit();
      m_pYFormat = new WBFL::Units::MomentTool(momentUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Moment (") + ((WBFL::Units::MomentTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionShear:
      {
      const WBFL::Units::ForceData& shearUnit = pDisplayUnits->GetShearUnit();
      m_pYFormat = new WBFL::Units::ShearTool(shearUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Shear (") + ((WBFL::Units::ShearTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionAxial:
      {
      const WBFL::Units::ForceData& axialUnit = pDisplayUnits->GetGeneralForceUnit();
      m_pYFormat = new WBFL::Units::ForceTool(axialUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Axial (") + ((WBFL::Units::ForceTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionDeflection:
   case actionXDeflection:
   {
      const WBFL::Units::LengthData& deflectionUnit = pDisplayUnits->GetDeflectionUnit();
      m_pYFormat = new WBFL::Units::DeflectionTool(deflectionUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Deflection (") + ((WBFL::Units::DeflectionTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionRotation:
      {
      const WBFL::Units::AngleData& rotationUnit = pDisplayUnits->GetRadAngleUnit();
      m_pYFormat = new WBFL::Units::RotationTool(rotationUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Rotation (") + ((WBFL::Units::RotationTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionStress:
   case actionPrincipalWebStress:
   {
      const WBFL::Units::StressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new WBFL::Units::StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Stress (") + ((WBFL::Units::StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionReaction:
      {
      const WBFL::Units::ForceData& shearUnit = pDisplayUnits->GetShearUnit();
      m_pYFormat = new WBFL::Units::ShearTool(shearUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Reaction (") + ((WBFL::Units::ShearTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionLoadRating:
   {
      const auto& scalar = pDisplayUnits->GetScalarFormat();
      m_pYFormat = new WBFL::Units::ScalarTool(scalar);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      m_Graph.SetYAxisTitle(_T("Rating Factor"));
      break;
   }
   default:
      ASSERT(0); 
   }
}

void CAnalysisResultsGraphBuilder::UpdateXAxisTitle()
{
   CAnalysisResultsGraphController* pMyGraphController = (CAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());
   IntervalIndexType intervalIdx = INVALID_INDEX;
   if ( 0 < vIntervals.size() )
   {
      intervalIdx = vIntervals.back();
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));
   if ( intervalIdx == releaseIntervalIdx )
   {
      m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
   }
   else
   {
      m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From Left End of Left-Most Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
   }
}

void CAnalysisResultsGraphBuilder::UpdateGraphTitle()
{
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   CString strCombo = (resultsType == rtIncremental ? _T("Incremental") : _T("Cumulative"));

   ActionType actionType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   CString strAction;
   switch(actionType)
   {
   case actionAxial:
      strAction = _T("Axial");
      break;
   case actionShear:
      strAction = _T("Shear");
      break;
   case actionMoment:
      strAction = _T("Moment");
      break;
   case actionDeflection:
      strAction = _T("Deflection");
      break;
   case actionXDeflection:
      strAction = _T("Deflection X");
      break;
   case actionRotation:
      strAction = _T("Rotation");
      break;
   case actionStress:
      strAction = _T("Stress");
      break;
   case actionReaction:
      strAction = _T("Reaction");
      break;
   case actionLoadRating:
      strAction = _T("Rating Factor");
      break;
   case actionPrincipalWebStress:
      strAction = _T("Web Stress");
      break;
   default:
      ASSERT(0);
   }

   CAnalysisResultsGraphController* pMyGraphController = (CAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());

   GroupIndexType  grpIdx = m_pGraphController->GetGirderGroup();
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();

   CGirderKey girderKey(grpIdx == ALL_GROUPS ? 0 : grpIdx,gdrIdx);

   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading)
   {
      // Plotting by loading
      IntervalIndexType intervalIdx = vIntervals.back();

      GET_IFACE(IIntervals,pIntervals);
      CString strInterval( pIntervals->GetDescription(intervalIdx).c_str() );

      CString strGraphTitle;
      if (actionType == actionLoadRating)
      {
         if (grpIdx == ALL_GROUPS)
         {
            strGraphTitle.Format(_T("Girder Line %s - Interval %d: %s - %s"), LABEL_GIRDER(gdrIdx), LABEL_INTERVAL(intervalIdx), strInterval, strAction);
         }
         else
         {
            strGraphTitle.Format(_T("%s - Interval %d: %s - %s"), GIRDER_LABEL(CGirderKey(grpIdx, gdrIdx)), LABEL_INTERVAL(intervalIdx), strInterval, strAction);
         }
      }
      else
      {
         if ( grpIdx == ALL_GROUPS )
         {
            strGraphTitle.Format(_T("Girder Line %s - Interval %d: %s - %s %s"),LABEL_GIRDER(gdrIdx),LABEL_INTERVAL(intervalIdx),strInterval,strCombo,strAction);
         }
         else
         {
            strGraphTitle.Format(_T("%s - Interval %d: %s - %s %s"),GIRDER_LABEL(CGirderKey(grpIdx,gdrIdx)),LABEL_INTERVAL(intervalIdx),strInterval,strCombo,strAction);
         }
      }

      m_Graph.SetTitle(strGraphTitle);
   }
   else
   {
      // Plotting by interval
      IDType graphID = ((CAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(0);
      const CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);

      CString strGraphTitle;
      if ( grpIdx == ALL_GROUPS )
      {
         strGraphTitle.Format(_T("Girder Line %s - %s - %s %s"),LABEL_GIRDER(gdrIdx),graphDef.m_Name.c_str(),strCombo,strAction);
      }
      else
      {
         strGraphTitle.Format(_T("%s - %s - %s %s"),GIRDER_LABEL(CGirderKey(grpIdx,gdrIdx)),graphDef.m_Name.c_str(),strCombo,strAction);
      }

      m_Graph.SetTitle(strGraphTitle);
   }

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      CString strSubtitle;
      switch ( GetAnalysisType() )
      {
      case pgsTypes::Simple:
         strSubtitle = _T("Simple span");
         break;

      case pgsTypes::Continuous:
         strSubtitle = _T("Simple made continuous");
         break;

      case pgsTypes::Envelope:
         strSubtitle = _T("Envelope of simple span and simple made continuous");
         break;
      }

      m_Graph.SetSubtitle(strSubtitle);
   }
}

COLORREF CAnalysisResultsGraphBuilder::GetGraphColor(IndexType graphIdx,IntervalIndexType intervalIdx)
{
   COLORREF c;
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading)
   {
      c = m_pGraphColor->GetColor(graphIdx);
   }
   else
   {
      c = m_pGraphColor->GetColor(graphIdx+intervalIdx);
   }

   return c;
}

CString CAnalysisResultsGraphBuilder::GetDataLabel(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx)
{
   CString strDataLabel;

   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading)
   {
      std::set<IndexType>::iterator found(m_UsedDataLabels.find(graphIdx));
      if ( found == m_UsedDataLabels.end() )
      {
         strDataLabel.Format(_T("%s"),graphDef.m_Name.c_str());
         m_UsedDataLabels.insert(graphIdx);
      }
   }
   else
   {
      std::set<IndexType>::iterator found(m_UsedDataLabels.find(intervalIdx));
      if ( found == m_UsedDataLabels.end() )
      {
         strDataLabel.Format(_T("Interval %d"),LABEL_INTERVAL(intervalIdx));
         m_UsedDataLabels.insert(intervalIdx);
      }
   }

   return strDataLabel;
}

void CAnalysisResultsGraphBuilder::UpdateGraphData()
{
   // clear graph
   m_Graph.ClearData();
   m_UsedDataLabels.clear();

   const auto& girderKey = m_pGraphController->GetGirderKey();

   ActionType actionType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

   std::vector<IntervalIndexType> vIntervals = ((CAnalysisResultsGraphController*)m_pGraphController)->GetSelectedIntervals();
   if (0 == vIntervals.size())
   {
      // if there aren't any intervals to plot, there is nothing to plot
      return;
   }

   // If the segments are simple span elements, we want to draw a graph for each segment individually.
   // Determine if the segments are simple spans during the intervals being graphed
   GET_IFACE(IIntervals, pIntervals);
   bool bSimpleSpanSegments = true;
   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);

   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for (; intervalIter != intervalIterEnd; intervalIter++)
   {
      IntervalIndexType intervalIdx = *intervalIter;

      for(const auto& thisGirderKey : vGirderKeys)
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
         if (1 < nSegments)
         {
            for (SegmentIndexType segIdx = 0; segIdx < nSegments - 1; segIdx++)
            {
               CClosureKey closureKey(thisGirderKey, segIdx);
               if (pIntervals->GetCompositeClosureJointInterval(closureKey) <= intervalIdx)
               {
                  bSimpleSpanSegments = false;
                  break;
               }
            }
         }
      }

      // if we got this far either the interval is before any continuity has occured
      // or the girder groups have one segment each which means this is a precast girder
      // bridge.... if this is a conventional precast girder bridge, check to see if the
      // deck is composite with the girders (in which case we can assume continuity has occured)
      if (bSimpleSpanSegments)
      {
         if (pIntervals->GetLastCompositeDeckInterval() <= intervalIdx && girderKey.groupIndex == ALL_GROUPS)
         {
            bSimpleSpanSegments = false;
         }
      }
   }

   // Determine the interval when the first segment is erected for the girder groups
   // that are being plotted
   IntervalIndexType firstSegmentErectionIntervalIdx = INVALID_INDEX;
   for(const auto& thisGirderKey : vGirderKeys)
   {
      firstSegmentErectionIntervalIdx = Min(firstSegmentErectionIntervalIdx, pIntervals->GetFirstSegmentErectionInterval(thisGirderKey));
   }

   IntervalIndexType firstPlottingIntervalIdx = vIntervals.front();
   IntervalIndexType lastPlottingIntervalIdx = vIntervals.back();

   CAnalysisResultsGraphController::GraphModeType graphMode = ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode();

   IndexType nSelectedGraphs = ((CAnalysisResultsGraphController*)m_pGraphController)->GetSelectedGraphCount();

   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   for(const auto& thisGirderKey : vGirderKeys)
   {
      m_GroupOffset = -ComputeShift(thisGirderKey); // shift is negative, we want positive value... change sign

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);

      for (IndexType graphIdx = 0; graphIdx < nSelectedGraphs; graphIdx++)
      {
         IDType graphID = ((CAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(graphIdx);
         const CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);

         bool bSimpleSpanSegmentsThisGraph = bSimpleSpanSegments;
         if (graphDef.m_GraphType == graphProduct && graphDef.m_LoadType.ProductLoadType == pgsTypes::pftPretension)
         {
            // pretensioning is always simple span
            bSimpleSpanSegmentsThisGraph = true;
         }
         
         SegmentIndexType endSegmentIdx = (bSimpleSpanSegmentsThisGraph ? nSegments-1 : 0);
         for (SegmentIndexType segIdx = 0; segIdx <= endSegmentIdx; segIdx++)
         {
            CSegmentKey segmentKey(thisGirderKey, segIdx);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            IntervalIndexType segmentErectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
            IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
            // Get the X locations for the graph - we have to do this for each graph type because some
            // items graph as simple spans and some graph on the continuous span
            GET_IFACE(IPointOfInterest, pIPoi);
            PoiList vPoi;

            // Pois for pricipal web stress are unique. 
            ActionType actionType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
            if (actionPrincipalWebStress == actionType)
            {
               GET_IFACE(IPrincipalWebStress, pPrincipalWebStress);
               pPrincipalWebStress->GetPrincipalWebStressPointsOfInterest(CSegmentKey(thisGirderKey, bSimpleSpanSegmentsThisGraph ? segIdx : ALL_SEGMENTS), lastPlottingIntervalIdx, &vPoi);
            }
            else
            {
               pIPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, bSimpleSpanSegmentsThisGraph ? segIdx : ALL_SEGMENTS), &vPoi);

               // There are some blips (bugs likely) in computing deflections within closure joints. Clean out off-segment POIs to make graphs look pretty
               if (bSimpleSpanSegmentsThisGraph || actionType==actionDeflection || actionType == actionRotation)
               {
                  // these POI are between segments so they don't apply
                  vPoi.erase(std::remove_if(vPoi.begin(), vPoi.end(), [pIPoi](const auto& poi) {return pIPoi->IsOffSegment(poi); }), vPoi.end());
               }
            }

            // Map POI coordinates to X-values for the graph
            std::vector<Float64> xVals;
            Shift(girderKey.groupIndex == ALL_GROUPS ? false : true);
            GetXValues(vPoi, &xVals);

            IntervalIndexType intervalIdx = INVALID_INDEX;
            if ( graphMode == CAnalysisResultsGraphController::Loading)
            {
               ATLASSERT(vIntervals.size() == 1);
               intervalIdx = vIntervals.back();
            }
            else
            {
               // if plotting by interval, the graph index is the index into the selected intervals
               intervalIdx = vIntervals[graphIdx];
            }

            bool bIsHauilngInterval = pIntervals->IsHaulSegmentInterval(intervalIdx);
            if( (intervalIdx < releaseIntervalIdx) ||
                (!bIsHauilngInterval && firstSegmentErectionIntervalIdx <= intervalIdx && intervalIdx < segmentErectionIntervalIdx) ||
                (bIsHauilngInterval && intervalIdx != haulSegmentIntervalIdx)
              )
            {
               // this interval is 
               // 1) before the segment exists (no prestress release yet) -OR-
               // 2) when hauling is not occuring and some segments have been erected but is before this segment is erected -OR-
               // 3) when hauling is occuring, but this segment is not being hauled
               // skip it
               continue;
            }

            IndexType selectedGraphIdx = m_pGraphDefinitions->GetGraphIndex(graphID);

            switch( graphDef.m_GraphType )
            {
            case graphProduct:
               if ( actionType == actionReaction )
               {
                  ProductReactionGraph(selectedGraphIdx,graphDef,intervalIdx,thisGirderKey,bSimpleSpanSegments ? segIdx : ALL_SEGMENTS);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   ProductLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  ProductLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphCombined:
               if ( actionType == actionReaction )
               {
                  CombinedReactionGraph(selectedGraphIdx,graphDef,intervalIdx,thisGirderKey,bSimpleSpanSegments ? segIdx : ALL_SEGMENTS);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   CombinedLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  CombinedLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphAllowable:
               if(intervalIdx == releaseIntervalIdx && actionType == actionStress)
               {
                  // Casting yard stress is its own animal
                  CyStressCapacityGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
                  break; // only break if we ploatted the allowables... otherwise drop through
               }
            case graphDemand:
            case graphLimitState:
            case graphCapacity:
            case graphMinCapacity:
               if ( actionType == actionReaction )
               {
                  ATLASSERT(0); //   LimitStateReactionGraph(selectedGraphIdx,graphDef,intervalIdx,thisGirderKey,bSimpleSpanSegments ? segIdx : ALL_SEGMENTS);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   LimitStateLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  LimitStateLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphLiveLoad:
               if ( actionType == actionReaction )
               {
                  LiveLoadReactionGraph(graphIdx,graphDef,intervalIdx,thisGirderKey);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   LiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  LiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphVehicularLiveLoad:
               if ( actionType == actionReaction )
               {
                  VehicularLiveLoadReactionGraph(graphIdx,graphDef,intervalIdx,thisGirderKey);
               }
               else
               {
                  VehicularLiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphDeckShrinkageStress:
               {
                  ATLASSERT(actionType == actionStress);
                  DeckShrinkageStressGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
                  break; 
               }

            case graphLoadRating:
            {
               ATLASSERT(actionType == actionLoadRating);
               RatingFactorGraph(selectedGraphIdx, graphDef, intervalIdx, vPoi, xVals);
               break;
            }

            case graphWebShearStress:
            case graphWebAxialStress:
            case graphPrincipalWebStressDemand:
            case graphPrincipalWebStressLimit:
            {
               ATLASSERT(actionType == actionPrincipalWebStress);
               GET_IFACE(ISpecification,pSpec);
               if (ISpecification::pwcNCHRPTimeStepMethod == pSpec->GetPrincipalWebStressCheckType(vPoi.front().get().GetSegmentKey()))
               {
                  TimeStepPrincipalWebStressGraph(selectedGraphIdx, graphDef, intervalIdx, vPoi, xVals);
               }
               else
               {
                  PrincipalWebStressGraph(selectedGraphIdx, graphDef, intervalIdx, vPoi, xVals);
               }
               break;
            }

            default:
               ASSERT(false); // should never get here
            } // end switch-case
         } // next graph
      } // next segment
   } // next group
}

void CAnalysisResultsGraphBuilder::InitializeGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, ActionType actionType, IntervalIndexType intervalIdx, bool bIsFinalShear, std::array<IndexType, 4>* pDataSeriesID, std::array<pgsTypes::BridgeAnalysisType, 4>* pBat, std::array<pgsTypes::StressLocation,4>* pStressLocations, IndexType* pAnalysisTypeCount)
{
   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if (girderKey.groupIndex == ALL_GROUPS)
   {
      girderKey.groupIndex = 0;
   }

   CString strDataLabel(GetDataLabel(graphIdx, graphDef, intervalIdx));

   COLORREF c(GetGraphColor(graphIdx, intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   if (actionType == actionShear)
   {
      GET_IFACE(IProductForces, pProductForces);
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      int penStyle = (/*liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT :*/ PS_SOLID);

      if (analysisType == pgsTypes::Envelope)
      {
         *pAnalysisTypeCount = 2;
         (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
         (*pDataSeriesID)[1] = m_Graph.CreateDataSeries(_T(""), penStyle, penWeight, c);

         (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);
         (*pBat)[1] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
         ATLASSERT((*pBat)[0] == pgsTypes::MinSimpleContinuousEnvelope);
         ATLASSERT((*pBat)[1] == pgsTypes::MaxSimpleContinuousEnvelope);
      }
      else
      {
         *pAnalysisTypeCount = 1;
         (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);

         (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
         ATLASSERT((*pBat)[0] == (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
      }
   }
   else if (actionType == actionAxial ||
      actionType == actionMoment ||
      actionType == actionDeflection ||
      actionType == actionXDeflection ||
      actionType == actionRotation ||
      actionType == actionReaction)
   {
      // For moments and deflections
      GET_IFACE(IProductForces, pProductForces);
      if (analysisType == pgsTypes::Envelope)
      {
         *pAnalysisTypeCount = 2;
         (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);
         (*pDataSeriesID)[1] = m_Graph.CreateDataSeries(_T(""), PS_SOLID, penWeight, c);

         (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Minimize);
         (*pBat)[1] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);

         ATLASSERT((*pBat)[0] == pgsTypes::MinSimpleContinuousEnvelope);
         ATLASSERT((*pBat)[1] == pgsTypes::MaxSimpleContinuousEnvelope);
      }
      else
      {
         *pAnalysisTypeCount = 1;
         (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);

         (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
         ATLASSERT((*pBat)[0] == (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
      }
   }
   else if (actionType == actionStress || actionType == actionPrincipalWebStress)
   {
      // for stresses
      std::array<CString, 4> strStressLabel = { _T(" - Bottom Girder"), _T(" - Top Girder"), _T(" - Bottom Deck"), _T(" - Top Deck") };
      std::array<int, 4> penStyle = { PS_STRESS_BOTTOM_GIRDER,PS_STRESS_TOP_GIRDER,PS_STRESS_BOTTOM_DECK,PS_STRESS_TOP_DECK };
      std::array<pgsTypes::BridgeAnalysisType, 4> bat = { pgsTypes::MaxSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope };

      *pAnalysisTypeCount = 0;
      for (int i = 0; i < 4; i++)
      {
         pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

         bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);

         if (bPlotStresses)
         {
            CString strStressDataLabel(strDataLabel);
            if (!strStressDataLabel.IsEmpty())
            {
               strStressDataLabel += strStressLabel[stressLocation];
            }

            (*pDataSeriesID)[*pAnalysisTypeCount] = m_Graph.CreateDataSeries(strStressDataLabel, penStyle[stressLocation], penWeight, c);
            (*pStressLocations)[*pAnalysisTypeCount] = stressLocation;

            if (analysisType == pgsTypes::Envelope)
            {
               (*pBat)[*pAnalysisTypeCount] = bat[stressLocation];
            }
            else
            {
               (*pBat)[*pAnalysisTypeCount] = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
            }

            (*pAnalysisTypeCount)++;
         }
      } // next stress location
   }
   else if (actionType == actionLoadRating)
   {
      *pAnalysisTypeCount = 1;
      (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);

      GET_IFACE(IProductForces, pProductForces);
      (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
      ATLASSERT((*pBat)[0] == (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
   }
   else
   {
      ATLASSERT(false); // is there a new action type?
   }
}

void CAnalysisResultsGraphBuilder::ProductLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   if (actionPrincipalWebStress != actionType)
   {
      // Product forces
      GET_IFACE(IProductForces2, pForces);

      std::array<IndexType, 4> data_series_id;
      std::array<pgsTypes::BridgeAnalysisType, 4> bat;
      std::array<pgsTypes::StressLocation, 4> stressLocation;
      IndexType nAnalysisTypes;
      InitializeGraph(graphIdx, graphDef, actionType, intervalIdx, bIsFinalShear, &data_series_id, &bat, &stressLocation, &nAnalysisTypes);

      if (nAnalysisTypes == 0)
         return;

      for (IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++)
      {
         switch (actionType)
         {
         case actionAxial:
         {
            std::vector<Float64> forces(pForces->GetAxial(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType));
            AddGraphPoints(data_series_id[analysisIdx], xVals, forces);
            break;
         }
         case actionShear:
         {
            std::vector<WBFL::System::SectionValue> shears(pForces->GetShear(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType));
            AddGraphPoints(data_series_id[analysisIdx], xVals, shears);
            break;
         }
         case actionMoment:
         {
            std::vector<Float64> moments(pForces->GetMoment(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType));
            AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
            break;
         }
         case actionDeflection:
         {
            bool bIncludeElevationAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludeUnrecoverableDefl = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();
            std::vector<Float64> deflections(pForces->GetDeflection(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType, bIncludeElevationAdjustment, bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl));
            AddGraphPoints(data_series_id[analysisIdx], xVals, deflections);
            break;
         }
         case actionXDeflection:
         {
            std::vector<Float64> deflections(pForces->GetXDeflection(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType));
            AddGraphPoints(data_series_id[analysisIdx], xVals, deflections);
            break;
         }
         case actionRotation:
         {
            bool bIncludeSlopeAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludeUnrecoverableDefl = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();
            std::vector<Float64> rotations(pForces->GetRotation(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType, bIncludeSlopeAdjustment, bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl));
            AddGraphPoints(data_series_id[analysisIdx], xVals, rotations);
            break;
         }
         case actionStress:
         {
            pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::TopGirder : pgsTypes::TopDeck);
            pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

            std::vector<Float64> fTop, fBot;
            pForces->GetStress(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType, topLocation, botLocation, &fTop, &fBot);

            if (IsTopStressLocation(stressLocation[analysisIdx]))
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fTop);
            }
            else
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fBot);
            }
            break;
         }

         case actionLoadRating: // should not get here
         default:
            ATLASSERT(false);
         }
      } // next analysis type
   }
   else
   {
      TimeStepProductLoadPrincipalWebStressGraph(graphIdx, graphDef, intervalIdx, vPoi, xVals);
   }
}

void CAnalysisResultsGraphBuilder::CombinedLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   // Combined forces
   LoadingCombinationType combination_type(graphDef.m_LoadType.CombinedLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   
   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   std::array<IndexType, 4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType, 4> bat;
   std::array<pgsTypes::StressLocation, 4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,bIsFinalShear,&data_series_id,&bat,&stressLocation,&nAnalysisTypes);

   if (nAnalysisTypes == 0)
      return;

   GET_IFACE(ICombinedForces2, pForces);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(actionType)
      {
      case actionAxial:
         {
            std::vector<Float64> forces = pForces->GetAxial( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType );
            AddGraphPoints(data_series_id[analysisIdx], xVals, forces);
            break;
         }

      case actionShear:
         {
            std::vector<WBFL::System::SectionValue> shear = pForces->GetShear( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType );
            AddGraphPoints(data_series_id[analysisIdx], xVals, shear);
            break;
         }

      case actionMoment:
         {
            std::vector<Float64> moments = pForces->GetMoment( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType );
            AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
            break;
         }

      case actionDeflection:
         {
            bool bIncludeElevationAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludeUnrecoverableDefl = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();
            std::vector<Float64> displ = pForces->GetDeflection( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, bIncludeElevationAdjustment, bIncludeUnrecoverableDefl,bIncludeUnrecoverableDefl);
            AddGraphPoints(data_series_id[analysisIdx], xVals, displ);
            break;
         }

      case actionXDeflection:
      {
         std::vector<Float64> displ = pForces->GetXDeflection(intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType);
         AddGraphPoints(data_series_id[analysisIdx], xVals, displ);
         break;
      }

      case actionRotation:
         {
            bool bIncludeSlopeAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludeUnrecoverableDefl = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();
            std::vector<Float64> rotations = pForces->GetRotation( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, bIncludeSlopeAdjustment,bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl );
            AddGraphPoints(data_series_id[analysisIdx], xVals, rotations);
            break;
         }

      case actionStress:
         {
            pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
            pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

            std::vector<Float64> fTop, fBot;
            pForces->GetStress( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, topLocation, botLocation, &fTop, &fBot );

            if ( IsTopStressLocation(stressLocation[analysisIdx]) )
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fTop);
            }
            else
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fBot);
            }
         break;
         }

      case actionPrincipalWebStress: // should not get here
      case actionLoadRating: // should not get here
      default:
         ATLASSERT(false);
      }
   }
}

void CAnalysisResultsGraphBuilder::LimitStateLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);
   GraphType graphType(graphDef.m_GraphType);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   bool bProcessNegativeMoments = false;
   if ( graphDef.m_GraphType == graphCapacity || graphDef.m_GraphType == graphMinCapacity )
   {
      GET_IFACE(IBridge,pBridge);
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         if ( pBridge->ProcessNegativeMoments(spanIdx) )
         {
            bProcessNegativeMoments = true;
            break;
         }
      }
   }

   // Limit state forces
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = (graphType == graphAllowable || graphType == graphCapacity ? 3 : 2);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   // data series for moment, shears and deflections
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),      PS_SOLID,penWeight,c);

   // data series for allowable stresses and capacity
   IndexType max_girder_capacity_series = m_Graph.CreateDataSeries(strDataLabel+(strDataLabel.IsEmpty() ? _T("") : _T(" - Girder")),PS_SOLID,3,c);
   IndexType min_girder_capacity_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);
   IndexType max_deck_capacity_series = m_Graph.CreateDataSeries(strDataLabel+(strDataLabel.IsEmpty() ? _T("") : _T(" - Deck")),PS_DOT,3,c);
   IndexType min_deck_capacity_series = m_Graph.CreateDataSeries(_T(""),PS_DOT,penWeight,c);

   // data series for stresses
   std::array<IndexType, 4> stress_max, stress_min;
   stress_max[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Top Girder")),   PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_min[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Bottom Girder")),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   stress_max[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Top Deck")),   PS_STRESS_TOP_DECK,   penWeight,c);
   stress_min[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_TOP_DECK,   penWeight,c);
   stress_max[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Bottom Deck")),PS_STRESS_BOTTOM_DECK,penWeight,c);
   stress_min[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_BOTTOM_DECK,penWeight,c);

   switch(actionType)
   {
   case actionAxial:
      {
         GET_IFACE(ILimitStateForces2,pForces);
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> mmax, mmin;
            pForces->GetAxial( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &mmin, &mmax );
            AddGraphPoints(max_data_series, xVals, mmax);

            pForces->GetAxial( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &mmin, &mmax );
            AddGraphPoints(min_data_series, xVals, mmin);
         }
         else
         {
            std::vector<Float64> mmax, mmin;
            pForces->GetAxial( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &mmin, &mmax );
            AddGraphPoints(max_data_series, xVals, mmax);
            AddGraphPoints(min_data_series, xVals, mmin);
         }
      break;
      }
   case actionShear:
      {
         if ( graphType == graphCapacity )
         {
            GET_IFACE(IShearCapacity,pCapacity);
            std::vector<Float64> pVn = pCapacity->GetShearCapacity(limitState, intervalIdx, vPoi);
            AddGraphPoints(max_girder_capacity_series, xVals,  pVn);

            std::vector<Float64>::iterator iter;
            std::vector<Float64> nVn;
            for ( iter = pVn.begin(); iter != pVn.end(); iter++ )
            {
               nVn.push_back(-1*(*iter));
            }
            AddGraphPoints(min_girder_capacity_series, xVals, nVn);
         }
         else
         {
            GET_IFACE(ILimitStateForces2,pForces);
            if ( analysisType == pgsTypes::Envelope )
            {
               std::vector<WBFL::System::SectionValue> shearMin, shearMax;
               pForces->GetShear( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &shearMin, &shearMax );
               AddGraphPoints(min_data_series, xVals, shearMin);

               pForces->GetShear( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &shearMin, &shearMax );
               AddGraphPoints(max_data_series, xVals, shearMax);
            }
            else
            {
               std::vector<WBFL::System::SectionValue> shearMin, shearMax;
               pForces->GetShear( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &shearMin, &shearMax );
               AddGraphPoints(min_data_series, xVals, shearMin);
               AddGraphPoints(max_data_series, xVals, shearMax);
            }
         }
      break;
      }
   case actionMoment:
      {
         if ( graphType == graphCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMn = pCapacity->GetMomentCapacity(intervalIdx,vPoi,true);
            AddGraphPoints(max_girder_capacity_series, xVals, pMn);

            if ( bProcessNegativeMoments )
            {
               std::vector<Float64> nMn = pCapacity->GetMomentCapacity(intervalIdx,vPoi,false);
               AddGraphPoints(min_girder_capacity_series, xVals, nMn);
            }
         }
         else if ( graphType == graphMinCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMrMin = pCapacity->GetMinMomentCapacity(intervalIdx,vPoi,true);
            AddGraphPoints(max_girder_capacity_series, xVals, pMrMin);

            if ( bProcessNegativeMoments )
            {
               std::vector<Float64> nMrMin = pCapacity->GetMinMomentCapacity(intervalIdx,vPoi,false);
               AddGraphPoints(min_girder_capacity_series, xVals, nMrMin);
            }
         }
         else
         {
            GET_IFACE(ILimitStateForces2,pForces);
            if ( analysisType == pgsTypes::Envelope )
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);
               
               if ( liveLoadIntervalIdx <= intervalIdx && IsStrengthLimitState(limitState) )
               {
                  mmin = pForces->GetSlabDesignMoment(limitState,vPoi, pgsTypes::MinSimpleContinuousEnvelope );
               }
               else
               {
                  pForces->GetMoment( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &mmin, &mmax );
               }

               AddGraphPoints(min_data_series, xVals, mmin);
            }
            else
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);

               if ( liveLoadIntervalIdx <= intervalIdx && IsStrengthLimitState(limitState) )
               {
                  mmin = pForces->GetSlabDesignMoment(limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan );
               }

               AddGraphPoints(min_data_series, xVals, mmin);
            }
         }
      break;
      }
   case actionDeflection:
      {
        GET_IFACE(ILimitStateForces2,pForces);
         bool bIncPrestress = (graphType == graphDemand ? true : false);
         bool bIncludeLiveLoad = false;
         bool bIncludeElevationAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
         bool bIncludeUnrecoverableDefl = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDeflection( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeElevationAdjustment,bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);

            pForces->GetDeflection( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeElevationAdjustment, bIncludeUnrecoverableDefl,bIncludeUnrecoverableDefl, &dispmn, &dispmx);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
         else
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDeflection( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, bIncludeLiveLoad, bIncludeElevationAdjustment, bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
      break;
      }
   case actionXDeflection:
      {
      GET_IFACE(ILimitStateForces2, pForces);
      bool bIncPrestress = (graphType == graphDemand ? true : false);
      if (analysisType == pgsTypes::Envelope)
      {
         std::vector<Float64> dispmn, dispmx;
         pForces->GetXDeflection(intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, &dispmn, &dispmx);
         AddGraphPoints(min_data_series, xVals, dispmn);

         pForces->GetXDeflection(intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, &dispmn, &dispmx);
         AddGraphPoints(max_data_series, xVals, dispmx);
      }
      else
      {
         std::vector<Float64> dispmn, dispmx;
         pForces->GetXDeflection(intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, &dispmn, &dispmx);
         AddGraphPoints(min_data_series, xVals, dispmn);
         AddGraphPoints(max_data_series, xVals, dispmx);
      }
      break;
   }
   case actionRotation:
      {
         GET_IFACE(ILimitStateForces2,pForces);
         bool bIncPrestress = (graphType == graphDemand ? true : false);
         bool bIncludeLiveLoad = false;
         bool bIncludeSlopeAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
         bool bIncludeUnrecoverableDefl = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> minRotation, maxRotation;
            pForces->GetRotation( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeSlopeAdjustment, bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl, &minRotation, &maxRotation);
            AddGraphPoints(min_data_series, xVals, minRotation);

            pForces->GetRotation( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeSlopeAdjustment, bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl, &minRotation, &maxRotation);
            AddGraphPoints(max_data_series, xVals, maxRotation);
         }
         else
         {
            std::vector<Float64> minRotation, maxRotation;
            pForces->GetRotation( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, bIncludeLiveLoad, bIncludeSlopeAdjustment, bIncludeUnrecoverableDefl, bIncludeUnrecoverableDefl, &minRotation, &maxRotation);
            AddGraphPoints(min_data_series, xVals, minRotation);
            AddGraphPoints(max_data_series, xVals, maxRotation);
         }
      break;
      }
   case actionStress:
      {
         if ( graphType == graphAllowable )
         {
            GET_IFACE(IAllowableConcreteStress,pAllowable);

            if ( ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopGirder) ||
                 ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomGirder) )
            {
               if ( pAllowable->IsStressCheckApplicable(girderKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension)) )
               {
                  std::vector<Float64> t(pAllowable->GetGirderAllowableTensionStress(vPoi,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension),false/*without rebar*/,false/*not in PTZ*/));
                  AddGraphPoints(min_girder_capacity_series, xVals, t);
                  m_Graph.SetDataLabel(min_girder_capacity_series,strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Girder")));
               }

               if ( pAllowable->IsStressCheckApplicable(girderKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Compression)) )
               {
                  std::vector<Float64> c( pAllowable->GetGirderAllowableCompressionStress(vPoi,StressCheckTask(intervalIdx,limitState,pgsTypes::Compression)) );
                  AddGraphPoints(max_girder_capacity_series, xVals, c);
               }
            }

            if ( ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopDeck) ||
                 ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomDeck) )
            {
               if ( pAllowable->IsStressCheckApplicable(girderKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension)) )
               {
                  std::vector<Float64> t(pAllowable->GetDeckAllowableTensionStress(vPoi,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension),false/*without rebar*/));
                  AddGraphPoints(min_deck_capacity_series, xVals, t);
                  m_Graph.SetDataLabel(min_deck_capacity_series,strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Deck")));
               }

               if ( pAllowable->IsStressCheckApplicable(girderKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Compression)) )
               {
                  std::vector<Float64> c( pAllowable->GetDeckAllowableCompressionStress(vPoi,StressCheckTask(intervalIdx,limitState,pgsTypes::Compression)) );
                  AddGraphPoints(max_deck_capacity_series, xVals, c);
               }
            }
         }
         else
         {
            GET_IFACE(ILimitStateForces2,pForces);
            bool bIncPrestress = (graphType == graphDemand ? true : false);
            std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;

            for ( int i = 0; i < 4; i++ )
            {
               pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

               bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
               if ( bPlotStresses )
               {
                  pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
                  pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);
                  if ( analysisType == pgsTypes::Envelope )
                  {
                     std::vector<Float64> dummy;
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, topLocation, &dummy,   &fTopMax);
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, topLocation, &fTopMin, &dummy);
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, botLocation, &dummy,   &fBotMax);
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, botLocation, &fBotMin, &dummy);
                  }
                  else
                  {
                     pForces->GetStress( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, topLocation, &fTopMin, &fTopMax);
                     pForces->GetStress( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, botLocation, &fBotMin, &fBotMax);
                  }

                  if ( IsTopStressLocation(stressLocation) )
                  {
                     AddGraphPoints(stress_min[stressLocation], xVals, fTopMin);
                     AddGraphPoints(stress_max[stressLocation], xVals, fTopMax);
                  }
                  else
                  {
                     AddGraphPoints(stress_min[stressLocation], xVals, fBotMin);
                     AddGraphPoints(stress_max[stressLocation], xVals, fBotMax);
                  }
               }
            } // next stress location
         }
      break;
      }

   case actionPrincipalWebStress: // should not get here
   case actionLoadRating: // should not get here
   default:
         ATLASSERT(false);
   }
}

void CAnalysisResultsGraphBuilder::LiveLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   // Live Load
   pgsTypes::LiveLoadType llType( graphDef.m_LoadType.LiveLoadType );

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);
   ATLASSERT(vehicleIdx == INVALID_INDEX);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }

   GET_IFACE_NOCHECK(ICombinedForces2,pForces);

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per girder)");
   }

   int penWeight = GRAPH_PEN_WEIGHT;

   // data series for moment, shears and deflections
   IndexType min_data_series;
   IndexType max_data_series;
   if (actionType == actionShear )
   {
      //if (actionType == actionShear && !bIsFinalShear)
      //{
      //   strDataLabel = _T("");
      //}

      int penStyle = (/*liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT :*/ PS_SOLID);

      min_data_series = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
      max_data_series = m_Graph.CreateDataSeries(_T(""),       penStyle, penWeight, c);
   }
   else
   {
      min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
      max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);
   }

   // data series for stresses
   std::array<IndexType, 4> stress_max, stress_min;
   stress_max[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Girder"),   PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_min[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Girder"),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   stress_max[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Deck"),   PS_STRESS_TOP_DECK,   penWeight,c);
   stress_min[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_TOP_DECK,   penWeight,c);
   stress_max[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Deck"),PS_STRESS_BOTTOM_DECK,penWeight,c);
   stress_min[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_BOTTOM_DECK,penWeight,c);

   switch(actionType)
   {
   case actionAxial:
      {
         std::vector<Float64> Pmin, Pmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Pmin, &Pmax);
            AddGraphPoints(min_data_series, xVals, Pmin);

            pForces->GetCombinedLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Pmin, &Pmax);
            AddGraphPoints(max_data_series, xVals, Pmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadAxial(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Pmin, &Pmax);
            AddGraphPoints(min_data_series, xVals, Pmin);
            AddGraphPoints(max_data_series, xVals, Pmax);
         }
      break;
      }

   case actionShear:
      {
         std::vector<WBFL::System::SectionValue> Vmin, Vmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);

            pForces->GetCombinedLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, &Vmin, &Vmax);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadShear(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }

   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);

            pForces->GetCombinedLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadMoment(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;
      }

   case actionDeflection:
   {
      std::vector<Float64> Dmin, Dmax;
      if (analysisType == pgsTypes::Envelope)
      {
         pForces->GetCombinedLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Dmin, &Dmax);
         AddGraphPoints(min_data_series, xVals, Dmin);

         pForces->GetCombinedLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Dmin, &Dmax);
         AddGraphPoints(max_data_series, xVals, Dmax);
      }
      else
      {
         pForces->GetCombinedLiveLoadDeflection(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Dmin, &Dmax);
         AddGraphPoints(min_data_series, xVals, Dmin);
         AddGraphPoints(max_data_series, xVals, Dmax);
      }
      break;
   }

   case actionXDeflection:
   {
      // there are no lateral live load deflections
      std::vector<Float64> Dmin, Dmax;
      Dmin.resize(vPoi.size(), 0.0);
      Dmax.resize(vPoi.size(), 0.0);
      AddGraphPoints(min_data_series, xVals, Dmin);
      AddGraphPoints(max_data_series, xVals, Dmax);
      break;
   }

   case actionRotation:
      {
         std::vector<Float64> Rmin, Rmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Rmin, &Rmax);
            AddGraphPoints(min_data_series, xVals, Rmin);

            pForces->GetCombinedLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Rmin, &Rmax);
            AddGraphPoints(max_data_series, xVals, Rmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadRotation(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Rmin, &Rmax);
            AddGraphPoints(min_data_series, xVals, Rmin);
            AddGraphPoints(max_data_series, xVals, Rmax);
         }
      break;
      }

   case actionStress:
      {
         std::vector<Float64> fTopMin,fTopMax,fBotMin,fBotMax;
         for ( int i = 0; i < 4; i++ )
         {
            pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

            bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
            if ( bPlotStresses )
            {
               pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
               pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);
               if ( analysisType == pgsTypes::Envelope )
               {
                  pForces->GetCombinedLiveLoadStress(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
               }
               else
               {
                  pForces->GetCombinedLiveLoadStress(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
               }

               if ( IsTopStressLocation(stressLocation) )
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fTopMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fTopMax);
               }
               else
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fBotMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fBotMax);
               }
            } // if bPlotStresses
         } // next stress location
      break;
      }
   case actionPrincipalWebStress: // should not get here
      TimeStepPrincipalWebStressLiveLoadGraph(graphIdx, graphDef, intervalIdx, vPoi, xVals);
      break;

   case actionLoadRating: // should not get here
   default:
         ATLASSERT(false);
   }
}

void CAnalysisResultsGraphBuilder::VehicularLiveLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   // Live Load
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }

   GET_IFACE_NOCHECK(IProductForces2,pForces);

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per lane)");
   }

   // data series for moment, shears and deflections
   IndexType min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);
   if (actionType == actionShear )
   {
      //if (actionType == actionShear && !bIsFinalShear)
      //{
      //   strDataLabel = _T("");
      //}

      int penStyle = (/*liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT :*/ PS_SOLID);

      min_data_series = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
      max_data_series = m_Graph.CreateDataSeries(_T(""), penStyle, penWeight, c);
   }

   // data series for stresses
   std::array<IndexType, 4> stress_max, stress_min;
   stress_max[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Girder"),   PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_min[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Girder"),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   stress_max[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Deck"),   PS_STRESS_TOP_DECK,   penWeight,c);
   stress_min[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_TOP_DECK,   penWeight,c);
   stress_max[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Deck"),PS_STRESS_BOTTOM_DECK,penWeight,c);
   stress_min[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_BOTTOM_DECK,penWeight,c);

   switch(actionType)
   {
   case actionAxial:
      {
         std::vector<Float64> Pmin, Pmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadAxial(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }
            else
            {
               pForces->GetLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }

            AddGraphPoints(min_data_series, xVals, Pmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadAxial(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }
            else
            {
               pForces->GetLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }

            AddGraphPoints(max_data_series, xVals, Pmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadAxial(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Pmin, &Pmax, nullptr, nullptr);
            }
            else
            {
               pForces->GetLiveLoadAxial(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Pmin, &Pmax, nullptr, nullptr);
            }

            AddGraphPoints(min_data_series, xVals, Pmin);
            AddGraphPoints(max_data_series, xVals, Pmax);
         }
      break;
      }

   case actionShear:
      {
         std::vector<WBFL::System::SectionValue> Vmin, Vmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadShear(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }
            else
            {
               pForces->GetLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }
 
            AddGraphPoints(min_data_series, xVals, Vmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadShear(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }
            else
            {
               pForces->GetLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }

            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadShear(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax);
            }
            else
            {
               pForces->GetLiveLoadShear(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax);
            }

            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }

   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }
            else
            {
               pForces->GetLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }

            AddGraphPoints(min_data_series, xVals, Mmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }
            else
            {
               pForces->GetLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }

            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, nullptr, nullptr);
            }
            else
            {
               pForces->GetLiveLoadMoment(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, nullptr, nullptr);
            }

            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;
      }

   case actionDeflection:
      {
         std::vector<Float64> Dmin, Dmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadDeflection(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }
            else
            {
               pForces->GetLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }

            AddGraphPoints(min_data_series, xVals, Dmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadDeflection(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }
            else
            {
               pForces->GetLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }

            AddGraphPoints(max_data_series, xVals, Dmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadDeflection(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax);
            }
            else
            {
               pForces->GetLiveLoadDeflection(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax);
            }

            AddGraphPoints(min_data_series, xVals, Dmin);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
      break;
      }


   case actionXDeflection:
   {
      // there are no lateral live load deflections
      std::vector<Float64> Dmin, Dmax;
      Dmin.resize(vPoi.size(), 0.0);
      Dmax.resize(vPoi.size(), 0.0);
      AddGraphPoints(min_data_series, xVals, Dmin);
      AddGraphPoints(max_data_series, xVals, Dmax);
      break;
   }

   case actionRotation:
      {
         std::vector<Float64> Rmin, Rmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadRotation(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }
            else
            {
               pForces->GetLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }

            AddGraphPoints(min_data_series, xVals, Rmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadRotation(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }
            else
            {
               pForces->GetLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }

            AddGraphPoints(max_data_series, xVals, Rmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadRotation(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Rmin, &Rmax);
            }
            else
            {
               pForces->GetLiveLoadRotation(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Rmin, &Rmax);
            }

            AddGraphPoints(min_data_series, xVals, Rmin);
            AddGraphPoints(max_data_series, xVals, Rmax);
         }
      break;
      }

   case actionStress:
      {
         std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;

         for ( int i = 0; i < 4; i++ )
         {
            pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

            bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
            if ( bPlotStresses )
            {
               pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
               pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

               if ( analysisType == pgsTypes::Envelope )
               {
                  if ( vehicleIdx != INVALID_INDEX )
                  {
                     pForces->GetVehicularLiveLoadStress(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
                  else
                  {
                     pForces->GetLiveLoadStress(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
               }
               else
               {
                  if ( vehicleIdx != INVALID_INDEX )
                  {
                     pForces->GetVehicularLiveLoadStress(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
                  else
                  {
                     pForces->GetLiveLoadStress(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
               }

               if ( IsTopStressLocation(stressLocation) )
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fTopMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fTopMax);
               }
               else
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fBotMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fBotMax);
               }
            } // end if bPlotStresses
         } // next stress location
      break;
      }

   case actionPrincipalWebStress: // should not get here
   case actionLoadRating: // should not get here
   default:
         ATLASSERT(false);
   } // end switch
}

void CAnalysisResultsGraphBuilder::GetSegmentXValues(const CGirderKey& girderKey,SegmentIndexType segIdx,IntervalIndexType intervalIdx,std::vector<CSegmentKey>* pSegments,std::vector<Float64>* pLeftXVals,std::vector<Float64>* pRightXVals)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPointOfInterest,pIPoi);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // get the segments
   if ( segIdx == ALL_SEGMENTS )
   {
      for ( SegmentIndexType i = 0; i < nSegments; i++ )
      {
         pSegments->push_back(CSegmentKey(girderKey,i));
      }
   }
   else
   {
      pSegments->push_back(CSegmentKey(girderKey,segIdx));
   }

   // get the left and right support points
   std::vector<CSegmentKey>::iterator segmentKeyIter(pSegments->begin());
   std::vector<CSegmentKey>::iterator segmentKeyIterEnd(pSegments->end());
   for ( ; segmentKeyIter != segmentKeyIterEnd; segmentKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segmentKeyIter;

      PoiAttributeType poiReference;
      if (intervalIdx == pIntervals->GetPrestressReleaseInterval(segmentKey))
      {
         poiReference = POI_RELEASED_SEGMENT;
      }
      else if (intervalIdx == pIntervals->GetLiftSegmentInterval(segmentKey))
      {
         poiReference = POI_LIFT_SEGMENT;
      }
      else if (intervalIdx == pIntervals->GetStorageInterval(segmentKey))
      {
         poiReference = POI_STORAGE_SEGMENT;
      }
      else if (intervalIdx == pIntervals->GetHaulSegmentInterval(segmentKey))
      {
         poiReference = POI_HAUL_SEGMENT;
      }
      else
      {
         poiReference = POI_ERECTED_SEGMENT;
      }

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | poiReference, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      
      Float64 Xg = pIPoi->ConvertPoiToGirderCoordinate(pgsPointOfInterest(segmentKey,0.0));
      pLeftXVals->push_back(Xg); // want to start plotting at the start face of the segment

      Xg = pIPoi->ConvertPoiToGirderCoordinate(vPoi.front());
      pLeftXVals->push_back(Xg);

      Xg = pIPoi->ConvertPoiToGirderCoordinate(vPoi.back());
      pRightXVals->push_back(Xg);

      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      Xg = pIPoi->ConvertPoiToGirderCoordinate(pgsPointOfInterest(segmentKey,Ls));
      pRightXVals->push_back(Xg); // end graph at end face of segment
   }
}

void CAnalysisResultsGraphBuilder::GetSupportXValues(const CGirderKey& girderKey,bool bIncludeTemporarySupports,std::vector<Float64>* pXVals,std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>* pSupports)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pIPoi);
   std::map<Float64,std::pair<SupportIndexType,pgsTypes::SupportType>> supportMap;
   
   pXVals->push_back(0.0); // want to start the graph at the left end of the girder

   // get pier locations
   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      Float64 Xgp;
      VERIFY( pBridge->GetPierLocation(girderKey,pierIdx,&Xgp) );
      Float64 Xg = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(girderKey,Xgp);

      // if the pier is an abutment, adjust the location so that the reactions get plotted
      // at the centerline of bearing instead of CL pier
      if ( pBridge->IsAbutment(pierIdx) )
      {
         if ( pierIdx == 0 )
         {
            Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(girderKey,0));
            Xg += brgOffset;
         }
         else
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
            Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(CSegmentKey(girderKey,nSegments-1));
            Xg -= brgOffset;
         }
      }
      supportMap.insert(std::make_pair(Xg,std::make_pair(pierIdx,pgsTypes::stPier)));
   }

   // get temporary support locations
   if ( bIncludeTemporarySupports )
   {
      SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
      SpanIndexType endSpanIdx = (SpanIndexType)(endPierIdx-1);

      SupportIndexType nTS = pBridge->GetTemporarySupportCount();
      for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
      {
         if ( pBridge->GetTemporarySupportType(tsIdx) == pgsTypes::ErectionTower )
         {
            SpanIndexType spanIdx;
            Float64 Xspan;
            pBridge->GetTemporarySupportLocation(tsIdx,girderKey.girderIndex,&spanIdx,&Xspan);
            if ( startSpanIdx <= spanIdx && spanIdx <= endSpanIdx )
            {
               Float64 Xgp = pBridge->GetTemporarySupportLocation(tsIdx,girderKey.girderIndex);
               Float64 Xg = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(girderKey,Xgp);
               supportMap.insert(std::make_pair(Xg,std::make_pair(tsIdx,pgsTypes::stTemporary)));
            }
         }
      }
   }

   // setup the location and support vectors
   std::map<Float64,std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportMapIter(supportMap.begin());
   std::map<Float64,std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportMapIterEnd(supportMap.end());
   for ( ; supportMapIter != supportMapIterEnd; supportMapIter++ )
   {
      pXVals->push_back(supportMapIter->first);
      pSupports->push_back(supportMapIter->second);
   }

   Float64 Lg = pBridge->GetGirderLength(girderKey);
   pXVals->push_back(Lg); // last point is at the end of the girder
}

void CAnalysisResultsGraphBuilder::ProductReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   
   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   std::vector<CSegmentKey> segmentKeys;

   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // reactions are begin requested before any of the segments are erected
      // so we are going to use IReaction::GetSegmentReaction. We need the
      // segments to get reactions for and the location of the support points
      GetSegmentXValues(girderKey,segIdx,intervalIdx,&segmentKeys,&leftXVals,&rightXVals);
      std::transform(leftXVals.cbegin(), leftXVals.cend(), leftXVals.begin(), [&](const auto& value) {return value + m_GroupOffset;});
      std::transform(rightXVals.cbegin(),rightXVals.cend(),rightXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }
   else
   {
      // segments have been erected so we are getting reactions from the bridge model
      // IReactions::GetReactions

      // Get locations of piers and temporary supports. 
      GetSupportXValues(girderKey,true,&leftXVals,&vSupports);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }

   GET_IFACE(IReactions,pReactions);

   std::array<IndexType, 4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType, 4> bat;
   std::array<pgsTypes::StressLocation, 4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,false,&data_series_id,&bat,&stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      if ( intervalIdx < firstSegmentErectionIntervalIdx )
      {
         std::vector<Float64> Rleft, Rright;
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,pfType,bat[analysisIdx],resultsType,&Rleft,&Rright);

         // put values of 0.0 at the locations that correspond to the face of the member locations 
         // in leftXVals and rightXVals
         int i = 0;
         std::vector<CSegmentKey>::iterator segKeyIter(segmentKeys.begin());
         std::vector<CSegmentKey>::iterator segKeyIterEnd(segmentKeys.end());
         for ( ; segKeyIter != segKeyIterEnd; segKeyIter++, i+=2 )
         {
            Rleft.insert(Rleft.begin()+i,0.0);
            Rright.insert(Rright.begin()+i+1,0.0);
         }

         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
         std::vector<Float64>::iterator RleftIter(Rleft.begin());
         std::vector<Float64>::iterator RrightIter(Rright.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, rightXValIter++, RleftIter++, RrightIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,*RleftIter);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);

            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,*RrightIter);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
         }
      }
      else
      {
         std::vector<REACTION> reactions;
         reactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pfType,bat[analysisIdx],resultsType);
         reactions.insert(reactions.begin(),REACTION()); // matches first point in leftXVals
         reactions.push_back(REACTION()); // matches last point in leftXVals
         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<REACTION>::iterator reactionIter(reactions.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, reactionIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,(*reactionIter).Fy);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
         }
      }
   } // next analysis type
}

void CAnalysisResultsGraphBuilder::CombinedReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx)
{
   LoadingCombinationType comboType(graphDef.m_LoadType.CombinedLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   
   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   std::vector<CSegmentKey> segmentKeys;

   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // reactions are begin requested before any of the segments are erected
      // so we are going to use IReaction::GetSegmentReaction. We need the
      // segments to get reactions for and the location of the support points
      GetSegmentXValues(girderKey,segIdx,intervalIdx,&segmentKeys,&leftXVals,&rightXVals);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
      std::transform(rightXVals.cbegin(),rightXVals.cend(),rightXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }
   else
   {
      // segments have been erected so we are getting reactions from the bridge model
      // IReactions::GetReactions

      // Get locations of piers and temporary supports. 
      GetSupportXValues(girderKey,true,&leftXVals,&vSupports);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }

   GET_IFACE(IReactions,pReactions);

   std::array<IndexType, 4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType, 4> bat;
   std::array<pgsTypes::StressLocation, 4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,false,&data_series_id,&bat,&stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      if ( intervalIdx < firstSegmentErectionIntervalIdx )
      {
         std::vector<Float64> Rleft, Rright;
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,comboType,bat[analysisIdx],resultsType,&Rleft,&Rright);

         // put values of 0.0 at the locations that correspond to the face of the member locations 
         // in leftXVals and rightXVals
         int i = 0;
         std::vector<CSegmentKey>::iterator segKeyIter(segmentKeys.begin());
         std::vector<CSegmentKey>::iterator segKeyIterEnd(segmentKeys.end());
         for ( ; segKeyIter != segKeyIterEnd; segKeyIter++, i+=2 )
         {
            Rleft.insert(Rleft.begin()+i,0.0);
            Rright.insert(Rright.begin()+i+1,0.0);
         }

         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
         std::vector<Float64>::iterator RleftIter(Rleft.begin());
         std::vector<Float64>::iterator RrightIter(Rright.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, rightXValIter++, RleftIter++, RrightIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,*RleftIter);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);

            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,*RrightIter);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
         }
      }
      else
      {
         std::vector<REACTION> reactions;
         reactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,comboType,bat[analysisIdx],resultsType);
         reactions.insert(reactions.begin(),REACTION()); // matches first point in leftXVals
         reactions.push_back(REACTION()); // matches last point in leftXVals
         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<REACTION>::iterator reactionIter(reactions.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, reactionIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,(*reactionIter).Fy);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
         }
      }
   } // next analysis type
}

void CAnalysisResultsGraphBuilder::LiveLoadReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   // Live Load
   bool bIncludeImpact = true;

   std::vector<Float64> Xvals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;

   GetSupportXValues(girderKey,false/*no temp supports*/,&Xvals,&vSupports);
   std::transform(Xvals.cbegin(),Xvals.cend(),Xvals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIter(vSupports.begin());
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIterEnd(vSupports.end());
   for ( ; supportIter != supportIterEnd; supportIter++ )
   {
      ATLASSERT(supportIter->second == pgsTypes::stPier);
      vPiers.push_back(supportIter->first);
   }

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per girder)");
   }

   // data series for moment, shears and deflections
   IndexType min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);

   GET_IFACE(IReactions,pReactions);

   std::vector<Float64> Rmin, Rmax, Rdummy;

   if ( analysisType == pgsTypes::Envelope )
   {
      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MinSimpleContinuousEnvelope, bIncludeImpact, &Rmin, &Rdummy);

      Rmin.insert(Rmin.begin(),0.0);
      Rmin.push_back(0.0);

      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MaxSimpleContinuousEnvelope, bIncludeImpact, &Rdummy, &Rmax);

      Rmax.insert(Rmax.begin(),0.0);
      Rmax.push_back(0.0);
   }
   else
   {
      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, &Rmin, &Rmax);

      Rmin.insert(Rmin.begin(),0.0);
      Rmin.push_back(0.0);

      Rmax.insert(Rmax.begin(),0.0);
      Rmax.push_back(0.0);
   }

   std::vector<Float64>::iterator XvalIter(Xvals.begin());
   std::vector<Float64>::iterator XvalIterEnd(Xvals.end());
   std::vector<Float64>::iterator RminIter(Rmin.begin());
   std::vector<Float64>::iterator RmaxIter(Rmax.begin());
   for ( ; XvalIter != XvalIterEnd; XvalIter++, RminIter++, RmaxIter++ )
   {
      AddGraphPoint(max_data_series,*XvalIter,0.0);
      AddGraphPoint(max_data_series,*XvalIter,*RmaxIter);
      AddGraphPoint(max_data_series,*XvalIter,0.0);

      AddGraphPoint(min_data_series,*XvalIter,0.0);
      AddGraphPoint(min_data_series,*XvalIter,*RminIter);
      AddGraphPoint(min_data_series,*XvalIter,0.0);
   }
}

void CAnalysisResultsGraphBuilder::VehicularLiveLoadReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   // Live Load
   std::vector<Float64> Xvals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;

   GetSupportXValues(girderKey,false/*no temp supports*/,&Xvals,&vSupports);
   std::transform(Xvals.cbegin(),Xvals.cend(),Xvals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIter(vSupports.begin());
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIterEnd(vSupports.end());
   for ( ; supportIter != supportIterEnd; supportIter++ )
   {
      ATLASSERT(supportIter->second == pgsTypes::stPier);
      vPiers.push_back(supportIter->first);
   }

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }


   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per lane)");
   }

   // data series for moment, shears and deflections
   IndexType min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);

   std::vector<REACTION> Rmin, Rmax, Rdummy;
   GET_IFACE(IReactions,pReactions);

   if ( analysisType == pgsTypes::Envelope )
   {
      if ( vehicleIdx != INVALID_INDEX )
      {
         pReactions->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx, vPiers, girderKey, pgsTypes::MinSimpleContinuousEnvelope, true, &Rmin, &Rdummy);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MinSimpleContinuousEnvelope, true, pgsTypes::fetFy, &Rmin, &Rdummy);
      }

      Rmin.insert(Rmin.begin(),REACTION());
      Rmin.push_back(REACTION());

      if ( vehicleIdx != INVALID_INDEX )
      {
         pReactions->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx, vPiers, girderKey, pgsTypes::MaxSimpleContinuousEnvelope, true, &Rdummy, &Rmax);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MaxSimpleContinuousEnvelope, true, pgsTypes::fetFy, &Rdummy, &Rmax);
      }

      Rmax.insert(Rmax.begin(),REACTION());
      Rmax.push_back(REACTION());
   }
   else
   {
      if ( vehicleIdx != INVALID_INDEX )
      {
         pReactions->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx, vPiers, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, &Rmin, &Rmax);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, pgsTypes::fetFy, &Rmin, &Rmax);
      }

      Rmin.insert(Rmin.begin(),REACTION());
      Rmin.push_back(REACTION());

      Rmax.insert(Rmax.begin(),REACTION());
      Rmax.push_back(REACTION());
   }

   std::vector<Float64>::iterator XvalIter(Xvals.begin());
   std::vector<Float64>::iterator XvalIterEnd(Xvals.end());
   std::vector<REACTION>::iterator RminIter(Rmin.begin());
   std::vector<REACTION>::iterator RmaxIter(Rmax.begin());
   for ( ; XvalIter != XvalIterEnd; XvalIter++, RminIter++, RmaxIter++ )
   {
      AddGraphPoint(max_data_series,*XvalIter,0.0);
      AddGraphPoint(max_data_series,*XvalIter,(*RmaxIter).Fy);
      AddGraphPoint(max_data_series,*XvalIter,0.0);

      AddGraphPoint(min_data_series,*XvalIter,0.0);
      AddGraphPoint(min_data_series,*XvalIter,(*RminIter).Fy);
      AddGraphPoint(min_data_series,*XvalIter,0.0);
   }
}

void CAnalysisResultsGraphBuilder::CyStressCapacityGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);

   int penWeight = 3;

   // data series min/max capacity(allowable)
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),      PS_SOLID,penWeight,c);

   // Allowable tension in cy is dependent on capacity - must get spec check results
   // First get pois using same request as spec check report
   GET_IFACE(IArtifact,pIArtifact);

   Float64 cap_prev = 0;
   Float64 x_prev = xVals.front(); // tension capacity can jump at a location. we must capture this
   bool first(true);
   auto i(vPoi.begin());
   auto end(vPoi.end());
   auto xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;

      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(poi.GetSegmentKey());
      const pgsFlexuralStressArtifact* pMaxArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(StressCheckTask(intervalIdx,limitState,pgsTypes::Tension),poi.GetID());
      const pgsFlexuralStressArtifact* pMinArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(StressCheckTask(intervalIdx, limitState, pgsTypes::Compression),poi.GetID());

      Float64 maxcap, mincap;
      if (pMaxArtifact != nullptr)
      {
         // compression is easy
         Float64 fTop = pMinArtifact->GetCapacity(pgsTypes::TopGirder);
         Float64 fBot = pMinArtifact->GetCapacity(pgsTypes::BottomGirder);
         mincap = Min(fTop,fBot);
         AddGraphPoint(min_data_series, x, mincap);

         // Tension - we must catch jumps
         // Use a simple rule: Jumps happen at starts/ends of high points
         if (pMaxArtifact->WasWithRebarAllowableStressUsed(pgsTypes::TopGirder))
         {
            fTop = pMaxArtifact->GetAlternativeAllowableTensileStress(pgsTypes::TopGirder);
         }
         else
         {
            fTop = pMaxArtifact->GetCapacity(pgsTypes::TopGirder);
         }

         if (pMaxArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
         {
            fBot = pMaxArtifact->GetAlternativeAllowableTensileStress(pgsTypes::BottomGirder);
         }
         else
         {
            fBot = pMaxArtifact->GetCapacity(pgsTypes::BottomGirder);
         }
         maxcap = Max(fTop,fBot);
         if (!first && !IsEqual(maxcap,cap_prev))
         {
            if (cap_prev < maxcap)
            {
               // We are going up hill. Jump is at this location
               AddGraphPoint(max_data_series, x, cap_prev);
            }
            else
            {
               // We went down hill. Jump was at last location
               AddGraphPoint(max_data_series, x_prev, maxcap);
            }
         }

         AddGraphPoint(max_data_series, x, maxcap);

         cap_prev = maxcap;
         x_prev = x;
      }

      first = false;
   }
}

void CAnalysisResultsGraphBuilder::DeckShrinkageStressGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   int penWeight = GRAPH_PEN_WEIGHT;

   bool bPlotTop = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopGirder);
   bool bPlotBot = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomGirder);

   // Deck shrinkage call is not stage dependent - assume deck shrinks after railing in installed
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType dsIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

   // data series top/bot
   IndexType top_data_series(0), bot_data_series(0);

   if (bPlotTop)
   {
      CString dl = strDataLabel + _T(" - Top");
      top_data_series = m_Graph.CreateDataSeries(dl,PS_STRESS_TOP_GIRDER,penWeight,c);
   }

   if (bPlotBot)
   {
      CString dl = strDataLabel + _T(" - Bottom");
      bot_data_series = m_Graph.CreateDataSeries(dl, PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   }

   GET_IFACE_NOCHECK(IProductForces,pProductForces);

   auto i(vPoi.begin());
   auto end(vPoi.end());
   auto xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;

      Float64 fTop(0.0), fBot(0.0);
      if(dsIntervalIdx == intervalIdx || (dsIntervalIdx < intervalIdx && resultsType == rtCumulative))
      {
         pProductForces->GetDeckShrinkageStresses(poi, pgsTypes::TopGirder, pgsTypes::BottomGirder, &fTop, &fBot);
      }

      if(bPlotTop)
      {
         AddGraphPoint(top_data_series, x, fTop);
      }

      if(bPlotBot)
      {
         AddGraphPoint(bot_data_series, x, fBot);
      }
   }
}

void CAnalysisResultsGraphBuilder::RatingFactorGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals)
{
   COLORREF c(GetGraphColor(graphIdx, intervalIdx));
   CString strDataLabel(GetDataLabel(graphIdx, graphDef, intervalIdx));
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);

   ATLASSERT(IsRatingLimitState(limitState));

   int penWeight = GRAPH_PEN_WEIGHT;

   IndexType data_series = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);

   pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(graphDef.m_LoadType.LimitStateType);

   GET_IFACE(IArtifact, pArtifact);
   CGirderKey girderKey;


   const pgsRatingArtifact* pRatingArtifact = nullptr;

   auto i(vPoi.begin());
   auto end(vPoi.end());
   auto xIter(xVals.begin());
   for (; i != end; i++, xIter++)
   {
      const pgsPointOfInterest& poi = *i;
      CGirderKey thisGirderKey(poi.GetSegmentKey());
      if (!girderKey.IsEqual(thisGirderKey))
      {
         girderKey = thisGirderKey;
         pRatingArtifact = pArtifact->GetRatingArtifact(girderKey, ratingType, graphDef.m_VehicleIndex);
      }

      bool bSkip = false;
      Float64 RF;
      if (graphDef.m_RatingAction == actionMoment)
      {
         const auto* pPmRA = pRatingArtifact->GetMomentRatingArtifact(poi, true);
         const auto* pNmRA = pRatingArtifact->GetMomentRatingArtifact(poi, false);
         Float64 pmrf = Float64_Max;
         Float64 nmrf = Float64_Max;
         if (pPmRA == nullptr && pNmRA == nullptr)
         {
            bSkip = true;
         }
         else
         {
            if (pPmRA)
            {
               pmrf = pPmRA->GetRatingFactor();
            }

            if (pNmRA)
            {
               nmrf = pNmRA->GetRatingFactor();
            }
            RF = Min(pmrf, nmrf);
         }
      }
      else if (graphDef.m_RatingAction == actionShear)
      {
         const auto* pRA = pRatingArtifact->GetShearRatingArtifact(poi);
         if (pRA == nullptr)
         {
            bSkip = true;
         }
         else
         {
            RF = pRA->GetRatingFactor();
         }
      }
      else if (graphDef.m_RatingAction == actionStress)
      {
         const auto* pRA = pRatingArtifact->GetStressRatingArtifact(poi);
         if (pRA == nullptr)
         {
            bSkip = true;
         }
         else
         {
            RF = pRA->GetRatingFactor();
         }
      }

      if (!bSkip)
      {
         RF = (10 < RF ? 10 : RF);
         Float64 x = *xIter;
         AddGraphPoint(data_series, x, RF);
      }
   }
}

void CAnalysisResultsGraphBuilder::PrincipalWebStressGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals)
{
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);

   ATLASSERT(limitState == pgsTypes::ServiceIII);


   GET_IFACE(IArtifact, pArtifact);
   GET_IFACE_NOCHECK(IPrincipalWebStress, pPrincipalWebStress);
   CSegmentKey segmentKey;

   const pgsPrincipalTensionStressArtifact* pPrincipalStressArtifact = nullptr;
   pPrincipalStressArtifact = pArtifact->GetSegmentArtifact(vPoi.front().get().GetSegmentKey())->GetPrincipalTensionStressArtifact();
   if (pPrincipalStressArtifact->IsApplicable())
   {
      const auto* pDetails = pPrincipalWebStress->GetPrincipalWebStressDetails(pPrincipalStressArtifact->GetPrincipalTensionStressArtifact(0)->GetPointOfInterest());
      const auto& vWebSections = pDetails->WebSections;
      pPrincipalStressArtifact = nullptr;

      int penWeight = GRAPH_PEN_WEIGHT;
      std::vector<IndexType> vDataSeries;
      if (graphDef.m_GraphType == graphPrincipalWebStressLimit)
      {
         CString strDataLabel(GetDataLabel(graphIdx, graphDef, intervalIdx));
         COLORREF c(GetGraphColor(graphIdx, intervalIdx));
         IndexType data_series = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);
         vDataSeries.push_back(data_series);
      }
      else
      {
         IndexType idx = 0;
         for (const auto& webSection : vWebSections)
         {
            COLORREF c(GetGraphColor(graphIdx + idx++, intervalIdx));
            IndexType data_series = m_Graph.CreateDataSeries(webSection.strLocation.c_str(), PS_SOLID, penWeight, c);
            vDataSeries.push_back(data_series);
         }
      }

      auto i(vPoi.begin());
      auto end(vPoi.end());
      auto xIter(xVals.begin());
      for (; i != end; i++, xIter++)
      {
         const pgsPointOfInterest& poi = *i;
         CSegmentKey thisSegmentKey(poi.GetSegmentKey());
         if (!segmentKey.IsEqual(thisSegmentKey))
         {
            segmentKey = thisSegmentKey;
            pPrincipalStressArtifact = pArtifact->GetSegmentArtifact(segmentKey)->GetPrincipalTensionStressArtifact();
         }

         const auto* pSectionArtifact = pPrincipalStressArtifact->GetPrincipalTensionStressArtifactAtPoi(poi.GetID());
         if (pSectionArtifact)
         {
            if (graphDef.m_GraphType == graphPrincipalWebStressLimit)
            {
               Float64 x = *xIter;
               AddGraphPoint(vDataSeries.front(), x, pSectionArtifact->GetStressLimit());
            }
            else
            {
               const auto* pDetails = pPrincipalWebStress->GetPrincipalWebStressDetails(pSectionArtifact->GetPointOfInterest());
               const auto& vWebSections = pDetails->WebSections;

               IndexType data_series = 0;
               for (const auto& webSection : vWebSections)
               {
                  Float64 x = *xIter;
                  if (graphDef.m_GraphType == graphPrincipalWebStressDemand)
                  {
                     AddGraphPoint(vDataSeries[data_series++], x, webSection.fmax);
                  }
                  else if (graphDef.m_GraphType == graphWebShearStress)
                  {
                     AddGraphPoint(vDataSeries[data_series++], x, webSection.t);
                  }
                  else if (graphDef.m_GraphType == graphWebAxialStress)
                  {
                     AddGraphPoint(vDataSeries[data_series++], x, webSection.fpcx);
                  }
               }
            }
         }
      }
   }
}


void CAnalysisResultsGraphBuilder::TimeStepPrincipalWebStressGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals)
{
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);

   ATLASSERT(limitState == pgsTypes::ServiceIII);

   GET_IFACE(IArtifact, pArtifact);
   GET_IFACE_NOCHECK(IPrincipalWebStress, pPrincipalWebStress);
   CSegmentKey segmentKey;

   const pgsPrincipalTensionStressArtifact* pPrincipalStressArtifact = nullptr;
   pPrincipalStressArtifact = pArtifact->GetSegmentArtifact(vPoi.front().get().GetSegmentKey())->GetPrincipalTensionStressArtifact();
   if (pPrincipalStressArtifact->IsApplicable())
   {
      if (graphDef.m_GraphType == graphPrincipalWebStressLimit)
      {
         // limit is easy part
         CString strDataLabel(GetDataLabel(graphIdx, graphDef, intervalIdx));
         COLORREF c(GetGraphColor(graphIdx, intervalIdx));
         IndexType data_series = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, GRAPH_PEN_WEIGHT, c);

         auto i(vPoi.begin());
         auto end(vPoi.end());
         auto xIter(xVals.begin());
         for (; i != end; i++, xIter++)
         {
            const pgsPointOfInterest& poi = *i;
            CSegmentKey thisSegmentKey(poi.GetSegmentKey());
            if (!segmentKey.IsEqual(thisSegmentKey))
            {
               segmentKey = thisSegmentKey;
               pPrincipalStressArtifact = pArtifact->GetSegmentArtifact(segmentKey)->GetPrincipalTensionStressArtifact();
            }

            const auto* pSectionArtifact = pPrincipalStressArtifact->GetPrincipalTensionStressArtifactAtPoi(poi.GetID());
            if (pSectionArtifact)
            {
               Float64 x = *xIter;
               AddGraphPoint(data_series, x, pSectionArtifact->GetStressLimit());
            }
         }
      }
      else
      {
         // Demands
         // TRICKY: Number of web sections (elevations) can vary per POI depending on whether ducts are located within the web region, If the location
         //         is not there, the Y value should be set to zero.
         //         Need to cycle through pois to find all web locations and create a data series for each elevation name. 
         //         Use a utility class to aid in this.
         WebSectionManager webSectionManager;
         for (const auto& poi : vPoi)
         {
            const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* pDetails = pPrincipalWebStress->GetTimeStepPrincipalWebStressDetails(poi,intervalIdx);
            IndexType webIdx = 0;
            for (const auto& webSection : *pDetails)
            {
               webSectionManager.AddWebSection(webSection.strLocation, webIdx);
               webIdx++;
            }
         }

         // Create data series going into this graph
         webSectionManager.CreateGraphSeries((CAnalysisResultsGraphController*)m_pGraphController, *this, m_Graph, graphDef.m_Name, intervalIdx, graphIdx);

         auto i(vPoi.begin());
         auto end(vPoi.end());
         auto xIter(xVals.begin());
         for (; i != end; i++, xIter++)
         {
            Float64 x = *xIter;

            const pgsPointOfInterest& poi = *i;
            CSegmentKey thisSegmentKey(poi.GetSegmentKey());
            if (!segmentKey.IsEqual(thisSegmentKey))
            {
               segmentKey = thisSegmentKey;
               pPrincipalStressArtifact = pArtifact->GetSegmentArtifact(segmentKey)->GetPrincipalTensionStressArtifact();
            }

            const auto* pSectionArtifact = pPrincipalStressArtifact->GetPrincipalTensionStressArtifactAtPoi(poi.GetID());
            if (pSectionArtifact)
            {
               // Set Y value for all web sections to zero
               webSectionManager.ResetYValues();

               const auto* pDetails = pPrincipalWebStress->GetTimeStepPrincipalWebStressDetails(pSectionArtifact->GetPointOfInterest(),intervalIdx);
               for (const auto& detail : *pDetails)
               {
                  // Set value using manager for each section that exists in this artifact. Locations not in artifact will have values set to zero
                  if (graphDef.m_GraphType == graphPrincipalWebStressDemand)
                  {
                     webSectionManager.SetYValue(detail.strLocation, detail.Service3PrincipalStress);
                  }
                  else if (graphDef.m_GraphType == graphWebShearStress)
                  {
                     webSectionManager.SetYValue(detail.strLocation, detail.Service3Tau);
                  }
                  else if (graphDef.m_GraphType == graphWebAxialStress)
                  {
                     webSectionManager.SetYValue(detail.strLocation, detail.Service3Fpcx);
                  }
               }

               // Loop through master list of web sections and add graph points
               IndexType numWs = webSectionManager.GetNumberOfWebSections();
               for (IndexType iWs = 0; iWs < numWs; iWs++)
               {
                  IndexType seriesIdx;
                  Float64 yValue;
                  webSectionManager.GetWebSectionData(iWs, &seriesIdx, &yValue);
                  AddGraphPoint(seriesIdx, x, yValue);
               }
            }
         } // pois
      }
   }
}

void CAnalysisResultsGraphBuilder::TimeStepPrincipalWebStressLiveLoadGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals)
{
   pgsTypes::LiveLoadType liveLoadType(graphDef.m_LoadType.LiveLoadType);
   ATLASSERT(liveLoadType == pgsTypes::lltDesign);

   // Kind of a hack here, but we are eating our own dog food. Search for the word "Shear" in graphdef to determine whether to plot shear or axial stres
   bool bIsShear = std::wstring::npos != graphDef.m_Name.find(_T("Shear"));

   GET_IFACE(IArtifact, pArtifact);
   GET_IFACE_NOCHECK(IPrincipalWebStress, pPrincipalWebStress);
   CSegmentKey segmentKey;

   const pgsPrincipalTensionStressArtifact* pPrincipalStressArtifact = nullptr;
   pPrincipalStressArtifact = pArtifact->GetSegmentArtifact(vPoi.front().get().GetSegmentKey())->GetPrincipalTensionStressArtifact();
   if (pPrincipalStressArtifact->IsApplicable())
   {
      // TRICKY: Number of web sections (elevations) can vary per POI depending on whether ducts are located within the web region, If the location
      //         is not there, the Y value should be set to zero.
      //         Need to cycle through pois to find all web locations and create a data series for each elevation name. 
      //         Use a utility class to aid in this.
      WebSectionManager webSectionManager;
      for (const auto& poi : vPoi)
      {
         const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* pDetails = pPrincipalWebStress->GetTimeStepPrincipalWebStressDetails(poi,intervalIdx);
         IndexType webIdx = 0;
         for (const auto& webSection : *pDetails)
         {
            webSectionManager.AddWebSection(webSection.strLocation, webIdx);
            webIdx++;
         }
      }

      // Create data series going into this graph
      webSectionManager.CreateGraphSeries((CAnalysisResultsGraphController*)m_pGraphController, *this, m_Graph, graphDef.m_Name, intervalIdx, graphIdx);

      auto i(vPoi.begin());
      auto end(vPoi.end());
      auto xIter(xVals.begin());
      for (; i != end; i++, xIter++)
      {
         Float64 x = *xIter;

         const pgsPointOfInterest& poi = *i;
         // Set Y value for all web sections to zero
         webSectionManager.ResetYValues();

         const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* pWebStressDetails = pPrincipalWebStress->GetTimeStepPrincipalWebStressDetails(poi, intervalIdx);

         for (const auto& webDetail : *pWebStressDetails)
         {
            // Set value using manager for each section that exists in this artifact. Locations not in artifact will have values set to zero
            if (bIsShear)
            {
               webSectionManager.SetYValue(webDetail.strLocation, webDetail.LL_Tau);
            }
            else
            {
               webSectionManager.SetYValue(webDetail.strLocation, webDetail.LL_Fpcx);
            }
         }

         // Loop through master list of web sections and add graph points
         IndexType numWs = webSectionManager.GetNumberOfWebSections();
         for (IndexType iWs = 0; iWs < numWs; iWs++)
         {
            IndexType seriesIdx;
            Float64 yValue;
            webSectionManager.GetWebSectionData(iWs, &seriesIdx, &yValue);
            AddGraphPoint(seriesIdx, x, yValue);
         }
      } // pois
   }
}

void CAnalysisResultsGraphBuilder::TimeStepProductLoadPrincipalWebStressGraph(IndexType graphIdx, const CAnalysisResultsGraphDefinition& graphDef, IntervalIndexType intervalIdx, const PoiList& vPoi, const std::vector<Float64>& xVals)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   // Kind of a hack here, but we are eating our own dog food. Search for the word "Shear" in graphdef to determine whether to plot shear or axial stres
   bool bIsShear = std::wstring::npos != graphDef.m_Name.find(_T("Shear"));

   GET_IFACE(ILosses,pLosses);
   // TRICKY: Number of web sections (elevations) can vary per POI depending on whether ducts are located within the web region, If the location
   //         is not there, the Y value should be set to zero.
   //         Need to cycle through pois to find all web locations and create a data series for each elevation name. 
   //         Use a utility class to aid in this.
   WebSectionManager webSectionManager;
   for (const auto& poi : vPoi)
   {
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);
      const TIME_STEP_PRINCIPALSTRESSINWEBDETAILS& prDetails = tsDetails.PrincipalStressDetails[pfType];

      IndexType webIdx = 0;
      for (const auto& webSection : prDetails.WebSections)
      {
         webSectionManager.AddWebSection(webSection.strLocation, webIdx);
         webIdx++;
      }
   }

   // Create data series going into this graph
   webSectionManager.CreateGraphSeries((CAnalysisResultsGraphController*)m_pGraphController, *this, m_Graph, graphDef.m_Name, intervalIdx, graphIdx);

   auto i(vPoi.begin());
   auto end(vPoi.end());
   auto xIter(xVals.begin());
   for (; i != end; i++, xIter++)
   {
      Float64 x = *xIter;
      const pgsPointOfInterest& poi = *i;

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_PRINCIPALSTRESSINWEBDETAILS& prDetails = tsDetails.PrincipalStressDetails[pfType];

      // Set Y value for all web sections to zero
      webSectionManager.ResetYValues();

      for (const auto& detail : prDetails.WebSections)
      {
         if (bIsShear)
         {
            webSectionManager.SetYValue(detail.strLocation, detail.tau_s);
         }
         else
         {
            webSectionManager.SetYValue(detail.strLocation, detail.fpcx_s);
         }
      }

      // Loop through master list of web sections and add graph points
      IndexType numWs = webSectionManager.GetNumberOfWebSections();
      for (IndexType iWs = 0; iWs < numWs; iWs++)
      {
         IndexType seriesIdx;
         Float64 yValue;
         webSectionManager.GetWebSectionData(iWs, &seriesIdx, &yValue);
         AddGraphPoint(seriesIdx, x, yValue);
      }
   } // pois
}

pgsTypes::AnalysisType CAnalysisResultsGraphBuilder::GetAnalysisType()
{
   return ((CAnalysisResultsGraphController*)m_pGraphController)->GetAnalysisType();
}

void CAnalysisResultsGraphBuilder::GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx)
{
   CAnalysisResultsGraphController* pMyGraphController = (CAnalysisResultsGraphController*)m_pGraphController;
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

DWORD CAnalysisResultsGraphBuilder::GetDrawBeamStyle() const
{
   return DBS_ERECTED_SEGMENTS_ONLY | DBS_HAULED_SEGMENTS_ONLY;
}

void CAnalysisResultsGraphBuilder::GetSecondaryXValues(const PoiList& vPoi,const std::vector<Float64>& xVals,PoiList* pvPoi,std::vector<Float64>* pXvalues)
{
   *pXvalues = xVals;

   GET_IFACE(IShearCapacity,pShearCapacity);
   GET_IFACE(IPointOfInterest,pPoi);
   const std::vector<CRITSECTDETAILS>& vCSDetails = pShearCapacity->GetCriticalSectionDetails(pgsTypes::StrengthI,vPoi.front().get().GetSegmentKey());

   for (const pgsPointOfInterest& poi : vPoi)
   {
      ZoneIndexType csZoneIdx = pShearCapacity->GetCriticalSectionZoneIndex(pgsTypes::StrengthI,poi);
      if ( csZoneIdx != INVALID_INDEX )
      {
#if defined _DEBUG
         Float64 start,end;
         pShearCapacity->GetCriticalSectionZoneBoundary(pgsTypes::StrengthI,poi.GetSegmentKey(),csZoneIdx,&start,&end);
         Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
         ATLASSERT( ::InRange(start,Xg,end) );
#endif

         PoiList vCSPoi;
         pPoi->GetCriticalSections(pgsTypes::StrengthI, poi.GetSegmentKey(), &vCSPoi);
         pvPoi->push_back(vCSPoi[csZoneIdx]);
      }
      else
      {
         pvPoi->push_back(poi);
      }
    }
}

void CAnalysisResultsGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph, rstrDefaultFileName);
}