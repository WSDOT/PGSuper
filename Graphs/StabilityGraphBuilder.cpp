///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Graphs/StabilityGraphBuilder.h>
#include "StabilityGraphController.h"
#include "StabilityGraphViewControllerImp.h"
#include <Graphs/ExportGraphXYTool.h>
#include "..\Documentation\PGSuper.hh"

#include "GraphColor.h"

#include <Units\UnitValueNumericalFormatTools.h>
#include <PgsExt\HaulingAnalysisArtifact.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFGraphView.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFDocument.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\DocumentType.h>

#include <MFCTools\Text.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const COLORREF CURVE1_COLOR      = RGB(0,0,200);
static const COLORREF CURVE2_COLOR      = RGB(200,0,0);
static const COLORREF CURVE3_COLOR      = RGB(0,200,0);

static const int      CURVE_PEN_WEIGHT  = GRAPH_PEN_WEIGHT;
static const int      CURVE_STYLE       = PS_SOLID;
static const int      LIMIT_STYLE       = PS_DASH;

// create a dummy unit conversion tool to pacify the graph constructor
static WBFL::Units::LengthData DUMMY(WBFL::Units::Measure::Meter);
static WBFL::Units::LengthTool DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CStabilityGraphBuilder, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CStabilityGraphBuilder::CStabilityGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(&DUMMY_TOOL,&DUMMY_TOOL),
m_pXFormat(nullptr),
m_pYFormat(nullptr)
{
   m_pGraphController = new CStabilityGraphController;
   SetName(_T("Girder Stability"));

   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_STABILITY_VIEW);
}

CStabilityGraphBuilder::CStabilityGraphBuilder(const CStabilityGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(&DUMMY_TOOL, &DUMMY_TOOL),
m_pXFormat(nullptr),
m_pYFormat(nullptr)
{
   m_pGraphController = new CStabilityGraphController;
}

CStabilityGraphBuilder::~CStabilityGraphBuilder()
{
   if ( m_pGraphController != nullptr )
   {
      delete m_pGraphController;
      m_pGraphController = nullptr;
   }

   if ( m_pXFormat != nullptr )
   {
      delete m_pXFormat;
      m_pXFormat = nullptr;
   }

   if ( m_pYFormat != nullptr )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }
}

CEAFGraphControlWindow* CStabilityGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

void CStabilityGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CStabilityGraphViewController>* pController;
   CComObject<CStabilityGraphViewController>::CreateInstance(&pController);
   pController->Init(m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CStabilityGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CStabilityGraphBuilder>(*this);
}

int CStabilityGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
{
   if ( CEAFAutoCalcGraphBuilder::InitializeGraphController(pParent,nID) < 0 )
   {
      return -1;
   }

   EAFGetBroker(&m_pBroker);

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);

   m_Graph.SetYAxisTitle(_T("Factor of Safety"));

   // x axis
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::LengthData& lengthUnit = pDisplayUnits->GetSpanLengthUnit();
   m_pXFormat = new WBFL::Units::LengthTool(lengthUnit);
   m_Graph.SetXAxisValueFormat(m_pXFormat);
   m_Graph.SetXAxisNumberOfMinorTics(0);
   m_Graph.XAxisNiceRange(false);
   m_Graph.SetXAxisNumberOfMajorTics(11);

   // y axis
   const WBFL::Units::ScalarData& scalarUnit = pDisplayUnits->GetScalarFormat();
   m_pYFormat = new WBFL::Units::ScalarTool(scalarUnit);
   m_Graph.SetYAxisValueFormat(m_pYFormat);
   m_Graph.YAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.DrawGrid(); // show grid by default

   return 0;
}

BOOL CStabilityGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   if ( !m_pGraphController->CreateControls(pParent,nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CStabilityGraphBuilder::UpdateXAxis()
{
   if ( m_pXFormat )
   {
      delete m_pXFormat;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::LengthData& lengthUnit = pDisplayUnits->GetSpanLengthUnit();
   m_pXFormat = new WBFL::Units::LengthTool(lengthUnit);
   m_Graph.SetXAxisValueFormat(m_pXFormat);
}

void CStabilityGraphBuilder::ShowGrid(bool bShowGrid)
{
   m_Graph.DrawGrid(bShowGrid);
   GetView()->Invalidate();
}

bool CStabilityGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   UpdateXAxis();

   CWaitCursor wait;

   int graphType = m_pGraphController->GetGraphType();
   const CSegmentKey& segmentKey(m_pGraphController->GetSegment());
   bool bShowGrid = m_pGraphController->ShowGrid();

   m_Graph.ClearData();

   m_Graph.DrawGrid(bShowGrid);

   IndexType seriesFS1 = m_Graph.CreateDataSeries(_T("FS Against Cracking (FScr)"), CURVE_STYLE, CURVE_PEN_WEIGHT, CURVE1_COLOR);
   IndexType seriesFS2 = m_Graph.CreateDataSeries(_T("FS Against Failure (FSf)"), CURVE_STYLE, CURVE_PEN_WEIGHT, CURVE2_COLOR);
   IndexType seriesFS3 = m_Graph.CreateDataSeries(_T("FS Against Rollover (FSro)"), CURVE_STYLE, CURVE_PEN_WEIGHT, CURVE3_COLOR);
   IndexType limitFS1 = m_Graph.CreateDataSeries(_T("Min FScr"), LIMIT_STYLE, CURVE_PEN_WEIGHT, CURVE1_COLOR);
   IndexType limitFS2;
   if (m_pGraphController->GetGraphType() == GT_LIFTING)
   {
      limitFS2 = m_Graph.CreateDataSeries(_T("Min FSf"), LIMIT_STYLE, CURVE_PEN_WEIGHT, CURVE2_COLOR);
   }
   else
   {
      limitFS2 = m_Graph.CreateDataSeries(_T("Min FSf/FSro"), LIMIT_STYLE, CURVE_PEN_WEIGHT, CURVE2_COLOR);
   }

   GET_IFACE_NOCHECK(IArtifact,pArtifact);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IBridge,pBridge);

   m_PrintSubtitle = SEGMENT_LABEL(segmentKey);

   Float64 hp1,hp2;
   pStrandGeom->GetHarpingPointLocations(segmentKey,&hp1,&hp2);

   // if the harp point is at mid-span, move it back just a little
   // we can't do a lifting analysis with both lifting points at mid-span
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   if ( IsEqual(hp1,segment_length/2) )
   {
      hp1 -= 0.005;
   }

   Float64 max_overhang = 0.2113*segment_length; // = (3 - sqrt(3))*segment_length / 6;
   hp1 = Min(hp1, max_overhang);

   if ( graphType == GT_LIFTING )
   {
      GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
      if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
      {
         CString strTitle;
         strTitle.Format(_T("Effect of support location during lifting from casting bed - %s"),m_PrintSubtitle.c_str());
         m_Graph.SetTitle(strTitle);
         m_Graph.SetXAxisTitle(std::_tstring(_T("Lift Point Location from End of Girder (") + m_pXFormat->UnitTag() + _T(")")).c_str());

         Float64 FS1 = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();
         Float64 FS2 = pSegmentLiftingSpecCriteria->GetLiftingFailureFs();

         Float64 loc = 0.0;
         Float64 stepSize = (hp1-loc)/20;
         while ( loc <= hp1 )
         {
            pProgress->UpdateMessage(_T("Working..."));

            WBFL::Stability::LiftingCheckArtifact artifact;
            pArtifact->CreateLiftingCheckArtifact(segmentKey,loc,&artifact);

            const auto& results = artifact.GetLiftingResults();
            AddGraphPoint(seriesFS1,loc,results.FScrMin);
            AddGraphPoint(seriesFS2,loc,results.MinAdjFsFailure);

            AddGraphPoint(limitFS1,loc,FS1);
            AddGraphPoint(limitFS2,loc,FS2);

            loc += stepSize;
         }
      }
   }
   else
   {
      GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
      if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() && pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::HaulingAnalysisMethod::WSDOT)
      {
         CString strTitle;
         strTitle.Format(_T("Effect of support location during hauling to bridge site - %s"),m_PrintSubtitle.c_str());
         m_Graph.SetTitle(strTitle);
         m_Graph.SetXAxisTitle(std::_tstring(_T("Truck Support Location from End of Girder (") + m_pXFormat->UnitTag() + _T(")")).c_str());

         Float64 FS1 = pSegmentHaulingSpecCriteria->GetHaulingCrackingFs();
         Float64 FS2 = pSegmentHaulingSpecCriteria->GetHaulingRolloverFs();

         Float64 loc = Min(pSegmentHaulingSpecCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metStart),
                           pSegmentHaulingSpecCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metEnd));
         Float64 stepSize = (hp1-loc)/20;
         while ( loc <= hp1 )
         {
            pProgress->UpdateMessage(_T("Working..."));

            // NOTE: assuming equal overhangs is probably the best thing to do with this view... 
            // but... give it some thought. could do the interaction surface that Dave Chapman showed me

            const pgsHaulingAnalysisArtifact* artifact_base = pArtifact->CreateHaulingAnalysisArtifact(segmentKey,loc,loc);
            // Only works for wsdot analysis
            const pgsWsdotHaulingAnalysisArtifact* pArtifact = dynamic_cast<const pgsWsdotHaulingAnalysisArtifact*>(artifact_base);
            if ( pArtifact )
            {
               Float64 FScr = Min(pArtifact->GetMinFsForCracking(WBFL::Stability::HaulingSlope::CrownSlope),pArtifact->GetMinFsForCracking(WBFL::Stability::HaulingSlope::Superelevation));
               Float64 FSf  = Min(pArtifact->GetFsFailure(WBFL::Stability::HaulingSlope::CrownSlope),pArtifact->GetFsFailure(WBFL::Stability::HaulingSlope::Superelevation));
               Float64 FSro = Min(pArtifact->GetFsRollover(WBFL::Stability::HaulingSlope::CrownSlope), pArtifact->GetFsRollover(WBFL::Stability::HaulingSlope::Superelevation));

               AddGraphPoint(seriesFS1,loc,FScr);
               AddGraphPoint(seriesFS2,loc,FSf );
               AddGraphPoint(seriesFS3,loc,FSro);

               AddGraphPoint(limitFS1,loc,FS1);
               AddGraphPoint(limitFS2,loc,FS2);
            }
            else
            {
               ATLASSERT(false); // should not happent
            }

            loc += stepSize;
         }
      }
   }

   return true;
}

void CStabilityGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   WBFL::Units::PhysicalConverter* pcx = dynamic_cast<WBFL::Units::PhysicalConverter*>(m_pXFormat);
   ASSERT(pcx);
   WBFL::Units::PhysicalConverter* pcy = dynamic_cast<WBFL::Units::PhysicalConverter*>(m_pYFormat);
   ASSERT(pcy);
   m_Graph.AddPoint(series, WBFL::Graphing::Point(pcx->Convert(xval),pcy->Convert(yval)));
}

void CStabilityGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int graphType = m_pGraphController->GetGraphType();

   if ( graphType == GT_LIFTING )
   {
      GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
      if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
      {
         DrawTheGraph(pGraphWnd,pDC);
      }
      else
      {
         CFont font;
         CFont* pOldFont = nullptr;
         if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         {
            pOldFont = pDC->SelectObject(&font);
         }

         MultiLineTextOut(pDC,0,0,_T("Lifting Analysis Disabled (See Project Criteria Library)\nUnable to create lifting stability graph."));

         if ( pOldFont )
         {
            pDC->SelectObject(pOldFont);
         }
      }
   }
   else
   {
      GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
      if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
      {
         DrawTheGraph(pGraphWnd,pDC);
      }
      else
      {
         CFont font;
         CFont* pOldFont = nullptr;
         if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         {
            pOldFont = pDC->SelectObject(&font);
         }

         MultiLineTextOut(pDC,0,0,_T("Hauling Analysis Disabled (See Project Criteria Library)\nUnable to create hauling stability graph."));

         if ( pOldFont )
         {
            pDC->SelectObject(pOldFont);
         }
      }
   }
}

void CStabilityGraphBuilder::DrawTheGraph(CWnd* pGraphWnd,CDC* pDC)
{
   int save = pDC->SaveDC();

   // The graph is valided and there was not an error
   // updating data.... draw the graph
   CRect rect = GetView()->GetDrawingRect();

   m_Graph.SetOutputRect(rect);
   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());

   TCHAR buffer[45];
   if (  m_pGraphController->GetGraphType() == GT_LIFTING )
   {
      GET_IFACE(ISegmentLiftingSpecCriteria,pCriteria);
      _stprintf_s(buffer,sizeof(buffer)/sizeof(TCHAR),_T("Min. FScr = %3.1f, Min. FSf = %3.1f"),
         pCriteria->GetLiftingCrackingFs(),
         pCriteria->GetLiftingFailureFs() );
   }
   else
   {
      GET_IFACE(ISegmentHaulingSpecCriteria,pCriteria);
      _stprintf_s(buffer,sizeof(buffer)/sizeof(TCHAR),_T("Min. FScr = %3.1f, Min. FSf/FSro = %3.1f"),
         pCriteria->GetHaulingCrackingFs(),
         pCriteria->GetHaulingRolloverFs() );
   }
   m_Graph.SetSubtitle(buffer);

   m_Graph.Draw(pDC->GetSafeHdc());


   pDC->RestoreDC(save);
}

void CStabilityGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph,rstrDefaultFileName);
}