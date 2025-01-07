///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <Graphs\SegmentAnalysisResultsGraphBuilder.h>
#include <Graphs\DrawBeamTool.h>
#include <Graphs\ExportGraphXYTool.h>
#include "SegmentAnalysisResultsGraphController.h"
#include "SegmentAnalysisResultsGraphDefinition.h"
#include "SegmentAnalysisResultsGraphViewControllerImp.h"
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
#include <IFace/Limits.h>
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
   void CreateGraphSeries(CSegmentAnalysisResultsGraphController* pGraphController, CSegmentAnalysisResultsGraphBuilder& rBuilder, WBFL::Graphing::GraphXY& rGraph, const std::_tstring& graphName, IntervalIndexType intervalIdx,IndexType graphIdx)
   {
      IndexType idx = 0;
      for (auto& section : m_WebSections)
      {
         CString fullName;
         if (pGraphController->GetGraphMode() == CSegmentAnalysisResultsGraphController::Loading)
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

BEGIN_MESSAGE_MAP(CSegmentAnalysisResultsGraphBuilder, CSegmentGraphBuilderBase)
END_MESSAGE_MAP()


CSegmentAnalysisResultsGraphBuilder::CSegmentAnalysisResultsGraphBuilder() :
CSegmentGraphBuilderBase(),
m_pGraphColor(std::make_unique<WBFL::Graphing::GraphColor>()),
m_pGraphDefinitions(new CSegmentAnalysisResultsGraphDefinitions)
{
   Init();
}

CSegmentAnalysisResultsGraphBuilder::CSegmentAnalysisResultsGraphBuilder(const CSegmentAnalysisResultsGraphBuilder& other) :
CSegmentGraphBuilderBase(other),
m_pGraphColor(std::make_unique<WBFL::Graphing::GraphColor>()),
m_pGraphDefinitions(new CSegmentAnalysisResultsGraphDefinitions)
{
   Init();
}

CSegmentAnalysisResultsGraphBuilder::~CSegmentAnalysisResultsGraphBuilder()
{
}

void CSegmentAnalysisResultsGraphBuilder::Init()
{
   SetName(_T("Analysis Results - Before Erection"));

   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_ANALYSIS_RESULTS);

   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
}

BOOL CSegmentAnalysisResultsGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   // we only graph single segments
   CSegmentKey segmentKey(0,0,0);
   GET_IFACE(ISelection, pSelection);
   CSelection selection = pSelection->GetSelection();
   if (selection.Type == CSelection::Girder || selection.Type == CSelection::Segment)
   {
      segmentKey.groupIndex = selection.GroupIdx == ALL_GROUPS ? 0 : selection.GroupIdx;
      segmentKey.girderIndex = selection.GirderIdx == ALL_GIRDERS ? 0 : selection.GirderIdx;
      segmentKey.segmentIndex = selection.SegmentIdx!= ALL_SEGMENTS ? selection.SegmentIdx : 0;
   }
   
   // update the graph definitions before creating the controller. the graph controller
   // uses the graph definitions to initialize the options within its controls
   UpdateGraphDefinitions(segmentKey);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != nullptr);
   if ( !m_pGraphController->Create(pParent,IDD_SEGMENT_ANALYSISRESULTS_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CSegmentAnalysisResultsGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CSegmentAnalysisResultsGraphViewController>* pController;
   CComObject<CSegmentAnalysisResultsGraphViewController>::CreateInstance(&pController);
   pController->Init((CSegmentAnalysisResultsGraphController*)m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}


std::unique_ptr<WBFL::Graphing::GraphBuilder> CSegmentAnalysisResultsGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CSegmentAnalysisResultsGraphBuilder>(*this);
}

void CSegmentAnalysisResultsGraphBuilder::UpdateGraphDefinitions(const CSegmentKey& segmentKey)
{
   m_pGraphDefinitions->Clear();

   IDType graphID = 0;

   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(IIntervals,pIntervals);
   CGirderKey girderKey(segmentKey.groupIndex,segmentKey.girderIndex);

   // Get intervals for reporting
   // spec check intervals
   GET_IFACE(IStressCheck, pStressCheck);
   std::vector<IntervalIndexType> vSpecCheckIntervals(pStressCheck->GetStressCheckIntervals(girderKey));

   // initial intervals
   IntervalIndexType nIntervals               = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType lastErectSegmentIntervalIdx = pIntervals->GetLastSegmentErectionInterval(girderKey);
   IntervalIndexType haulSegmentIntervalIdx  = pIntervals->GetHaulSegmentInterval(segmentKey);

   std::vector<IntervalIndexType> vInitialIntervals;
   vInitialIntervals.push_back(releaseIntervalIdx);
   vInitialIntervals.push_back(storageIntervalIdx);
   vInitialIntervals.push_back(erectSegmentIntervalIdx);

   std::vector<IntervalIndexType> vAllIntervals;
   for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx <= lastErectSegmentIntervalIdx; intervalIdx++ )
   {
      vAllIntervals.push_back(intervalIdx);
   }

   // intervals were we want to plot unrecoverable deflections from girder load
   std::vector<IntervalIndexType> vUnrecoverableDeflIntervals;
   for (IntervalIndexType intervalIdx = haulSegmentIntervalIdx; intervalIdx <= lastErectSegmentIntervalIdx; intervalIdx++)
   {
      vUnrecoverableDeflIntervals.push_back(intervalIdx);
   }

   //////////////////////////////////////////////////////////////////////////
   // Product Load Cases
   //////////////////////////////////////////////////////////////////////////

   // girder self-weight
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftGirder), pgsTypes::pftGirder, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION));
   
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftPretension), pgsTypes::pftPretension, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION));

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), pgsTypes::pftPostTensioning, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSecondaryEffects),      pgsTypes::pftSecondaryEffects,      vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
   if ( prestress_loss_criteria.LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftCreep),      pgsTypes::pftCreep,      vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage),  pgsTypes::pftShrinkage,  vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), pgsTypes::pftRelaxation, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   // Diaphragm loads never occur 
//   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++,pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm),pgsTypes::pftDiaphragm,vAllIntervals,ACTIONS_ALL | ACTIONS_X_DEFLECTION));

   // Special case for unrecoverable deflection from girder load
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++,_T("Unrecoverable Girder Dead Load"),pgsTypes::ProductForceType(PL_UNRECOVERABLE),vUnrecoverableDeflIntervals,ACTIONS_DEFLECTION | ACTIONS_X_DEFLECTION));

   // Combined Results
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcDC), lcDC, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION) );

   if ( prestress_loss_criteria.LossMethod == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcCR), lcCR, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcSH), lcSH, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcRE), lcRE, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
      m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcPS), lcPS, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION) );
   }

   // Limit States and Capacities
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, _T("Service I (Design)"), pgsTypes::ServiceI, vAllIntervals, ACTIONS_ALL_NO_REACTION | ACTIONS_X_DEFLECTION) );
   
   // Demand and Allowable
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, _T("Service I Demand (Design)"),     pgsTypes::ServiceI,  graphDemand,    vAllIntervals,ACTIONS_STRESS | ACTIONS_DEFLECTION | ACTIONS_X_DEFLECTION) );
   m_pGraphDefinitions->AddGraphDefinition(CSegmentAnalysisResultsGraphDefinition(graphID++, _T("Service I Limit (Design)"),  pgsTypes::ServiceI,  graphAllowable, vSpecCheckIntervals) );
}

std::vector<std::pair<std::_tstring,IDType>> CSegmentAnalysisResultsGraphBuilder::GetLoadings(IntervalIndexType intervalIdx,ActionType actionType)
{
   return m_pGraphDefinitions->GetLoadings(intervalIdx,actionType);
}

GraphType CSegmentAnalysisResultsGraphBuilder::GetGraphType(IDType graphID)
{
   CSegmentAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);
   return graphDef.m_GraphType;
}

CSegmentGraphControllerBase* CSegmentAnalysisResultsGraphBuilder::CreateGraphController()
{
   return new CSegmentAnalysisResultsGraphController;
}

bool CSegmentAnalysisResultsGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CSegmentGraphBuilderBase::UpdateNow();

   // Update graph properties
   UpdateYAxisUnits();
   UpdateXAxisTitle();
   UpdateGraphTitle();

   // we want a tighter zero tolerance for rotations
   ActionType actionType  = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   Float64 tolerance = m_ZeroToleranceY;
   if ( actionType == actionRotation )
   {
      m_ZeroToleranceY = 1e-07;
   }

   UpdateGraphData();

   m_ZeroToleranceY = tolerance;

   return true;
}

void CSegmentAnalysisResultsGraphBuilder::UpdateYAxisUnits()
{
   delete m_pYFormat;

   ActionType actionType  = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

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
   default:
      ASSERT(false); 
   }
}

void CSegmentAnalysisResultsGraphBuilder::UpdateXAxisTitle()
{
   CSegmentAnalysisResultsGraphController* pMyGraphController = (CSegmentAnalysisResultsGraphController*)m_pGraphController;
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

void CSegmentAnalysisResultsGraphBuilder::UpdateGraphTitle()
{
   ResultsType resultsType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   CString strCombo = (resultsType == rtIncremental ? _T("Incremental") : _T("Cumulative"));

   ActionType actionType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
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

   CSegmentAnalysisResultsGraphController* pMyGraphController = (CSegmentAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());

   const auto& segmentKey = m_pGraphController->GetSegmentKey();

   if ( ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CSegmentAnalysisResultsGraphController::Loading)
   {
      // Plotting by loading
      IntervalIndexType intervalIdx = vIntervals.back();

      GET_IFACE(IIntervals,pIntervals);
      CString strInterval( pIntervals->GetDescription(intervalIdx).c_str() );

      CString strGraphTitle;
      strGraphTitle.Format(_T("%s - Interval %d: %s - %s %s"),SEGMENT_LABEL(segmentKey),LABEL_INTERVAL(intervalIdx),strInterval,strCombo,strAction);

      m_Graph.SetTitle(strGraphTitle);
   }
   else
   {
      // Plotting by interval
      IDType graphID = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(0);
      const CSegmentAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);

      CString strGraphTitle;
      strGraphTitle.Format(_T("%s - %s - %s %s"),SEGMENT_LABEL(segmentKey),graphDef.m_Name.c_str(),strCombo,strAction);

      m_Graph.SetTitle(strGraphTitle);
   }

   CString strSubtitle = _T("Simple span");
   m_Graph.SetSubtitle(strSubtitle);
}

COLORREF CSegmentAnalysisResultsGraphBuilder::GetGraphColor(IndexType graphIdx,IntervalIndexType intervalIdx)
{
   COLORREF c;
   if ( ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CSegmentAnalysisResultsGraphController::Loading)
   {
      c = m_pGraphColor->GetColor(graphIdx);
   }
   else
   {
      c = m_pGraphColor->GetColor(graphIdx+intervalIdx);
   }

   return c;
}

void CSegmentAnalysisResultsGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph, rstrDefaultFileName);
}

CString CSegmentAnalysisResultsGraphBuilder::GetDataLabel(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx)
{
   CString strDataLabel;

   if ( ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CSegmentAnalysisResultsGraphController::Loading)
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

void CSegmentAnalysisResultsGraphBuilder::UpdateGraphData()
{
   // clear graph
   m_Graph.ClearData();
   m_UsedDataLabels.clear();

   const auto& segmentKey = m_pGraphController->GetSegmentKey();

   ActionType actionType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

   std::vector<IntervalIndexType> vIntervals = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetSelectedIntervals();
   if (0 == vIntervals.size())
   {
      // if there aren't any intervals to plot, there is nothing to plot
      return;
   }

   // Get the X locations for the graph
   GET_IFACE(IPointOfInterest, pIPoi);

   IntervalIndexType firstPlottingIntervalIdx = vIntervals.front();
   IntervalIndexType lastPlottingIntervalIdx = vIntervals.back();

   CSegmentAnalysisResultsGraphController::GraphModeType graphMode = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode();

   IndexType nSelectedGraphs = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetSelectedGraphCount();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType segmentErectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, &vPoi);

   // previous call can return poi's that are not within segment. Trim those off
   vPoi.erase(std::remove_if(vPoi.begin(),vPoi.end(),[pIPoi](const auto& poi) {return pIPoi->IsOffSegment(poi); }),vPoi.end());

   // Map POI coordinates to X-values for the graph
   Shift(true);
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);

   for ( IndexType graphIdx = 0; graphIdx < nSelectedGraphs; graphIdx++ )
   {
      IntervalIndexType intervalIdx = INVALID_INDEX;
      if ( graphMode == CSegmentAnalysisResultsGraphController::Loading)
      {
         ATLASSERT(vIntervals.size() == 1);
         intervalIdx = vIntervals.back();
      }
      else
      {
         // if plotting by interval, the graph index is the index into the selected intervals
         intervalIdx = vIntervals[graphIdx];
      }

      bool bIsHauilngInterval = intervalIdx == haulSegmentIntervalIdx;
      if( intervalIdx < releaseIntervalIdx)
      {
         // this interval is before the segment exists (no prestress release yet). skip it
         continue;
      }

      IDType graphID = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(graphIdx);
      const CSegmentAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);

      IndexType selectedGraphIdx = m_pGraphDefinitions->GetGraphIndex(graphID);

      switch( graphDef.m_GraphType )
      {
      case graphProduct:
         if ( actionType == actionReaction )
         {
            ProductReactionGraph(selectedGraphIdx,graphDef,intervalIdx,segmentKey);
         }
         else
         {
            ProductLoadGraph(selectedGraphIdx,graphDef, intervalIdx, vPoi, xVals);
         }
         break;

      case graphCombined:
         if ( actionType == actionReaction )
         {
            CombinedReactionGraph(selectedGraphIdx,graphDef,intervalIdx,segmentKey);
         }
         else
         {
            CombinedLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
         }
         break;

      case graphAllowable:
         if(intervalIdx == releaseIntervalIdx && actionType == actionStress)
         {
            // Casting yard stress is its own animal
            CyStressCapacityGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
            break;
         }
      case graphDemand:
      case graphLimitState:
      case graphCapacity:
      case graphMinCapacity:
         if ( actionType == actionReaction )
         {
            ATLASSERT(0);
         }
         else
         {
            LimitStateLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
         }
      break;
      default:
         ASSERT(false); // should never get here
      } // end switch-case
   } // next graph
}

pgsTypes::AnalysisType CSegmentAnalysisResultsGraphBuilder::GetAnalysisType()
{
   GET_IFACE(ISpecification,pSpec);
   return pSpec->GetAnalysisType();
}

bool CSegmentAnalysisResultsGraphBuilder::IncludeUnrecoverableDefl(IntervalIndexType interval)
{
   return ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->IncludeUnrecoverableDefl(interval);
}

void CSegmentAnalysisResultsGraphBuilder::InitializeGraph(IndexType graphIdx, const CSegmentAnalysisResultsGraphDefinition& graphDef, ActionType actionType, IntervalIndexType intervalIdx, std::array<IndexType, 4>* pDataSeriesID, std::array<pgsTypes::BridgeAnalysisType, 4>* pBat, std::array<pgsTypes::StressLocation,4>* pStressLocations, IndexType* pAnalysisTypeCount)
{
   CString strDataLabel(GetDataLabel(graphIdx, graphDef, intervalIdx));

   COLORREF c(GetGraphColor(graphIdx, intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   if (actionType == actionShear)
   {
      GET_IFACE(IProductForces, pProductForces);
      int penStyle = PS_SOLID;

      *pAnalysisTypeCount = 1;
      (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
      (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
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
      *pAnalysisTypeCount = 1;
      (*pDataSeriesID)[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);
      (*pBat)[0] = pProductForces->GetBridgeAnalysisType(analysisType, pgsTypes::Maximize);
   }
   else if (actionType == actionStress)
   {
      // for stresses
      std::array<CString, 2> strStressLabel = { _T(" - Bottom Girder"), _T(" - Top Girder")};
      std::array<int, 2> penStyle = { PS_STRESS_BOTTOM_GIRDER,PS_STRESS_TOP_GIRDER };
      std::array<pgsTypes::BridgeAnalysisType, 2> bat = { pgsTypes::MaxSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope };

      *pAnalysisTypeCount = 0;
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

         bool bPlotStresses = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);

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
   else
   {
      ATLASSERT(false); // is there a new action type?
   }
}

void CSegmentAnalysisResultsGraphBuilder::ProductLoadGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   ActionType actionType  = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   // Product forces
   GET_IFACE_NOCHECK(IProductForces2, pForces);

   std::array<IndexType, 4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType, 4> bat;
   std::array<pgsTypes::StressLocation, 4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx, graphDef, actionType, intervalIdx, &data_series_id, &bat, &stressLocation, &nAnalysisTypes);

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
         bool bIncludeUnrecoverableDefl = IncludeUnrecoverableDefl(intervalIdx);
         std::vector<Float64> deflections;
         if (PL_UNRECOVERABLE == pfType && rtCumulative == resultsType)
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType haulInterval = pIntervals->GetHaulSegmentInterval(vPoi.front().get().GetSegmentKey());

            if (bIncludeUnrecoverableDefl && intervalIdx >= haulInterval) // modulus hardening doesn't happen until after storage
            {
               IProductForces2::sagInterval sagInt = intervalIdx == haulInterval ? IProductForces2::sagHauling : IProductForces2::sagErection;

               deflections = pForces->GetUnrecoverableGirderDeflectionFromStorage(sagInt,bat[analysisIdx],vPoi);
            }
            else
            {
               deflections.assign(vPoi.size(),0.0);
            }
         }
         else
         {
            bool bIncludeElevationAdjustment = true;
            deflections = pForces->GetDeflection(intervalIdx,pfType,vPoi,bat[analysisIdx],resultsType,bIncludeElevationAdjustment,bIncludeUnrecoverableDefl,bIncludeUnrecoverableDefl);
         }
         AddGraphPoints(data_series_id[analysisIdx], xVals, deflections);
         break;
      }
      case actionXDeflection:
      {
         std::vector<Float64> deflections;
         if (PL_UNRECOVERABLE == pfType && rtCumulative == resultsType)
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType haulInterval = pIntervals->GetHaulSegmentInterval(vPoi.front().get().GetSegmentKey());

            if (intervalIdx >= haulInterval) // modulus hardening doesn't happen until after storage
            {
               IProductForces2::sagInterval sagInt = intervalIdx == haulInterval ? IProductForces2::sagHauling : IProductForces2::sagErection;

               deflections = pForces->GetUnrecoverableGirderXDeflectionFromStorage(sagInt,bat[analysisIdx],vPoi);
            }
            else
            {
               deflections.assign(vPoi.size(),0.0);
            }
         }
         else
         {
            deflections = pForces->GetXDeflection(intervalIdx,pfType,vPoi,bat[analysisIdx],resultsType);
         }
         AddGraphPoints(data_series_id[analysisIdx], xVals, deflections);
         break;
      }
      case actionRotation:
      {
         bool bIncludeUnrecoverableDefl = IncludeUnrecoverableDefl(intervalIdx);
         std::vector<Float64> rotations;

         if (PL_UNRECOVERABLE == pfType) 
         {
            GET_IFACE(IIntervals,pIntervals);
            const CSegmentKey& segmentKey = vPoi.front().get().GetSegmentKey();
            IntervalIndexType haulInterval = pIntervals->GetHaulSegmentInterval(segmentKey);
            IntervalIndexType erectInterval = pIntervals->GetErectSegmentInterval(segmentKey);
            IProductForces2::sagInterval sagInt = intervalIdx == haulInterval ? IProductForces2::sagHauling : IProductForces2::sagErection;

            bool showUnrec = (bIncludeUnrecoverableDefl && intervalIdx >= haulInterval) &&
                           ((rtCumulative == resultsType) ||
                           (sagInt == IProductForces2::sagHauling && intervalIdx == haulInterval) ||
                           (sagInt == IProductForces2::sagErection && intervalIdx == erectInterval));

            if (showUnrec) 
            {
               rotations = pForces->GetUnrecoverableGirderRotationFromStorage(sagInt,bat[analysisIdx],vPoi);
            }
            else
            {
               rotations.assign(vPoi.size(),0.0);
            }
         }
         else
         {
            bool bIncludeSlopeAdjustment = false;
            bool bIncludePreCamber = false;
            rotations = pForces->GetRotation(intervalIdx,pfType,vPoi,bat[analysisIdx],resultsType,bIncludeSlopeAdjustment,bIncludePreCamber,bIncludeUnrecoverableDefl);
         }

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

void CSegmentAnalysisResultsGraphBuilder::CombinedLoadGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   // Combined forces
   LoadingCombinationType combination_type(graphDef.m_LoadType.CombinedLoadType);

   ActionType actionType  = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   
   std::array<IndexType, 4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType, 4> bat;
   std::array<pgsTypes::StressLocation, 4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,&data_series_id,&bat,&stressLocation,&nAnalysisTypes);

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
            bool bIncludeElevationAdjustment = false;
            bool bIncludeUnrecoverableDefl = IncludeUnrecoverableDefl(intervalIdx);
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
            bool bIncludeSlopeAdjustment = false;
            bool bIncludeUnrecoverableDefl = IncludeUnrecoverableDefl(intervalIdx);
            std::vector<Float64> rotations = pForces->GetRotation( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, bIncludeSlopeAdjustment, false, bIncludeUnrecoverableDefl );
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

void CSegmentAnalysisResultsGraphBuilder::LimitStateLoadGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   ActionType actionType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   pgsTypes::AnalysisType analysisType = GetAnalysisType();
   pgsTypes::BridgeAnalysisType bridgeAnalysisType = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);
   GraphType graphType(graphDef.m_GraphType);

   // Limit state forces
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = (graphType == graphAllowable || graphType == graphCapacity ? 3 : 2);

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   // data series for moment, shears and deflections
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);

   // data series for allowable stresses and capacity
   IndexType max_girder_capacity_series = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Girder")),PS_SOLID,3,c);
   IndexType min_girder_capacity_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);

   // data series for stresses
   std::array<IndexType,4> stress_max,stress_min;
   stress_max[pgsTypes::TopGirder] = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Top Girder")),PS_STRESS_TOP_GIRDER,penWeight,c);
   stress_min[pgsTypes::TopGirder] = m_Graph.CreateDataSeries(_T(""),PS_STRESS_TOP_GIRDER,penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Bottom Girder")),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   switch (actionType)
   {
   case actionAxial:
   {
      GET_IFACE(ILimitStateForces2,pForces);
      std::vector<Float64> mmax,mmin;
      pForces->GetAxial(intervalIdx,limitState,vPoi,bridgeAnalysisType,&mmin,&mmax);
      AddGraphPoints(max_data_series,xVals,mmax);
      AddGraphPoints(min_data_series,xVals,mmin);
      break;
   }
   case actionShear:
   {
      if (graphType == graphCapacity)
      {
         ATLASSERT(0);
      }
      else
      {
         GET_IFACE(ILimitStateForces2,pForces);
         std::vector<WBFL::System::SectionValue> shearMin,shearMax;
         pForces->GetShear(intervalIdx,limitState,vPoi,bridgeAnalysisType,&shearMin,&shearMax);
         AddGraphPoints(min_data_series,xVals,shearMin);
         AddGraphPoints(max_data_series,xVals,shearMax);
      }
      break;
   }
   case actionMoment:
   {
      GET_IFACE(ILimitStateForces2,pForces);
      std::vector<Float64> mmax,mmin;
      pForces->GetMoment(intervalIdx,limitState,vPoi,bridgeAnalysisType,&mmin,&mmax);
      AddGraphPoints(max_data_series,xVals,mmax);
      AddGraphPoints(min_data_series,xVals,mmin);
      break;
   }
   case actionDeflection:
   {
      GET_IFACE(ILimitStateForces2,pForces);
      bool bIncPrestress = (graphType == graphDemand ? true : false);
      bool bIncludeLiveLoad = false;
      bool bIncludeElevationAdjustment = false;
      bool bIncludeUnrecoverableDefl = IncludeUnrecoverableDefl(intervalIdx);
      std::vector<Float64> dispmn,dispmx;
      pForces->GetDeflection(intervalIdx,limitState,vPoi,bridgeAnalysisType,bIncPrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,bIncludeUnrecoverableDefl,bIncludeUnrecoverableDefl,&dispmn,&dispmx);
      AddGraphPoints(min_data_series,xVals,dispmn);
      AddGraphPoints(max_data_series,xVals,dispmx);
      break;
   }
   case actionXDeflection:
   {
      GET_IFACE(ILimitStateForces2,pForces);
      bool bIncPrestress = (graphType == graphDemand ? true : false);
      std::vector<Float64> dispmn,dispmx;
      pForces->GetXDeflection(intervalIdx,limitState,vPoi,bridgeAnalysisType,bIncPrestress,&dispmn,&dispmx);
      AddGraphPoints(min_data_series,xVals,dispmn);
      AddGraphPoints(max_data_series,xVals,dispmx);
   }
   break;
   case actionRotation:
   {
      GET_IFACE(ILimitStateForces2,pForces);
      bool bIncPrestress = (graphType == graphDemand ? true : false);
      bool bIncludeLiveLoad = false;
      bool bIncludeSlopeAdjustment = false;
      bool bIncludeUnrecoverableDefl = IncludeUnrecoverableDefl(intervalIdx);
      std::vector<Float64> minRotation,maxRotation;
      pForces->GetRotation(intervalIdx,limitState,vPoi,bridgeAnalysisType,bIncPrestress,bIncludeLiveLoad,bIncludeSlopeAdjustment,bIncludeUnrecoverableDefl,bIncludeUnrecoverableDefl,&minRotation,&maxRotation);
      AddGraphPoints(min_data_series,xVals,minRotation);
      AddGraphPoints(max_data_series,xVals,maxRotation);

      break;
   }
   case actionStress:
   {
      if (graphType == graphAllowable)
      {
         GET_IFACE(IConcreteStressLimits,pLimits);
         const CSegmentKey& segmentKey(m_pGraphController->GetSegmentKey());

         if (((CSegmentAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopGirder) ||
            ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomGirder))
         {
            if (pLimits->IsConcreteStressLimitApplicable(segmentKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension)))
            {
               std::vector<Float64> t(pLimits->GetGirderConcreteTensionStressLimit(vPoi,StressCheckTask(intervalIdx,limitState,pgsTypes::Tension),false/*without rebar*/,false/*not in PTZ*/));
               AddGraphPoints(min_girder_capacity_series,xVals,t);
               m_Graph.SetDataLabel(min_girder_capacity_series,strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Girder")));
            }

            if (pLimits->IsConcreteStressLimitApplicable(segmentKey,StressCheckTask(intervalIdx,limitState,pgsTypes::Compression)))
            {
               std::vector<Float64> c(pLimits->GetGirderConcreteCompressionStressLimit(vPoi,StressCheckTask(intervalIdx,limitState,pgsTypes::Compression)));
               AddGraphPoints(max_girder_capacity_series,xVals,c);
            }
         }
      }
      else
      {
         GET_IFACE(ILimitStateForces2,pForces);
         bool bIncPrestress = (graphType == graphDemand ? true : false);
         std::vector<Float64> fTopMin,fTopMax,fBotMin,fBotMax;

         for (int i = 0; i < 2; i++)
         {
            pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

            bool bPlotStresses = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
            if (bPlotStresses)
            {
               pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
               pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;
               pForces->GetStress(intervalIdx,limitState,vPoi,bridgeAnalysisType,bIncPrestress,topLocation,&fTopMin,&fTopMax);
               pForces->GetStress(intervalIdx,limitState,vPoi,bridgeAnalysisType,bIncPrestress,botLocation,&fBotMin,&fBotMax);
            }

            if (IsTopStressLocation(stressLocation))
            {
               AddGraphPoints(stress_min[stressLocation],xVals,fTopMin);
               AddGraphPoints(stress_max[stressLocation],xVals,fTopMax);
            }
            else
            {
               AddGraphPoints(stress_min[stressLocation],xVals,fBotMin);
               AddGraphPoints(stress_max[stressLocation],xVals,fBotMax);
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

void CSegmentAnalysisResultsGraphBuilder::GetSegmentXValues(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,std::vector<Float64>* pLeftXVals,std::vector<Float64>* pRightXVals)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPointOfInterest,pIPoi);

   // get the left and right support points
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

void CSegmentAnalysisResultsGraphBuilder::ProductReactionGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   ActionType actionType  = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);

   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   GetSegmentXValues(segmentKey,intervalIdx,&leftXVals,&rightXVals);
   if (leftXVals.size() != 2 || rightXVals.size() != 2)
   {
      ATLASSERT(0); // our assumption broke
      return; 
   }

   GET_IFACE(IReactions,pReactions);
   std::array<IndexType, 4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType, 4> bat;
   std::array<pgsTypes::StressLocation, 4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,&data_series_id,&bat,&stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      Float64 RleftVal, RrightVal;
      pReactions->GetSegmentReactions(segmentKey,intervalIdx,pfType,bat[analysisIdx],resultsType,&RleftVal,&RrightVal);

      // put values of 0.0 at the locations that correspond to the face of the member locations 
      std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
      std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter++,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,RleftVal);
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);

      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,RrightVal);
      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter++,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
   } // next analysis type
}

void CSegmentAnalysisResultsGraphBuilder::CombinedReactionGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey)
{
   LoadingCombinationType comboType(graphDef.m_LoadType.CombinedLoadType);

   ActionType actionType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CSegmentAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);

   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   GetSegmentXValues(segmentKey,intervalIdx,&leftXVals,&rightXVals);
   if (leftXVals.size() != 2 || rightXVals.size() != 2)
   {
      ATLASSERT(0); // our assumption broke
      return;
   }

   GET_IFACE(IReactions,pReactions);
   std::array<IndexType,4> data_series_id;
   std::array<pgsTypes::BridgeAnalysisType,4> bat;
   std::array<pgsTypes::StressLocation,4> stressLocation;
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,&data_series_id,&bat,&stressLocation,&nAnalysisTypes);

   for (IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++)
   {
      Float64 RleftVal,RrightVal;
      pReactions->GetSegmentReactions(segmentKey,intervalIdx,comboType,bat[analysisIdx],resultsType,&RleftVal,&RrightVal);

      // put values of 0.0 at the locations that correspond to the face of the member locations 
      std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
      std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter++,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,RleftVal);
      AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);

      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,RrightVal);
      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter++,0.0);
      AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
   } // next analysis type
}

void CSegmentAnalysisResultsGraphBuilder::CyStressCapacityGraph(IndexType graphIdx,const CSegmentAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
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

void CSegmentAnalysisResultsGraphBuilder::GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx)
{
   CSegmentAnalysisResultsGraphController* pMyGraphController = (CSegmentAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());
   if (0 < vIntervals.size())
   {
      *pFirstIntervalIdx = vIntervals.front();
      *pLastIntervalIdx = vIntervals.back();
   }
   else
   {
      const CSegmentKey& segmentKey(m_pGraphController->GetSegmentKey());
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      *pFirstIntervalIdx = intervalIdx;
      *pLastIntervalIdx = *pFirstIntervalIdx;
   }
}

DWORD CSegmentAnalysisResultsGraphBuilder::GetDrawBeamStyle() const
{
   return DBS_ERECTED_SEGMENTS_ONLY | DBS_HAULED_SEGMENTS_ONLY;
}
