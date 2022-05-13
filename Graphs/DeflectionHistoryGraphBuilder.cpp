///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Graphs/DeflectionHistoryGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>
#include "DeflectionHistoryGraphController.h"
#include "DeflectionHistoryGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

#include "GraphColor.h"
#include <PgsExt\IntervalTool.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <Units\UnitValueNumericalFormatTools.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PointOfInterest.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <Graphing/AxisXY.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// create a dummy unit conversion tool to pacify the graph constructor
static WBFL::Units::LengthData DUMMY(WBFL::Units::Measure::Meter);
static WBFL::Units::LengthTool DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CDeflectionHistoryGraphBuilder, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CDeflectionHistoryGraphBuilder::CDeflectionHistoryGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0)
{
   Init();
}

CDeflectionHistoryGraphBuilder::CDeflectionHistoryGraphBuilder(const CDeflectionHistoryGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0)
{
   Init();
}

void CDeflectionHistoryGraphBuilder::Init()
{
   m_XAxisType = X_AXIS_TIME_LOG;

   m_pGraphController = new CDeflectionHistoryGraphController;

   SetName(_T("Deflection History"));

   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_DEFLECTION_HISTORY);

   m_Time.Width = 7;
   m_Time.Precision = 0;
   m_Time.Format = sysNumericFormatTool::Fixed;

   m_Interval.Width = 7;
   m_Interval.Precision = 1;
   m_Interval.Format = sysNumericFormatTool::Fixed;
}

CDeflectionHistoryGraphBuilder::~CDeflectionHistoryGraphBuilder()
{
   if ( m_pGraphController != nullptr )
   {
      delete m_pGraphController;
      m_pGraphController = nullptr;
   }

   if ( m_pTimeFormat != nullptr )
   {
      delete m_pTimeFormat;
      m_pTimeFormat = nullptr;
   }

   if ( m_pIntervalFormat != nullptr )
   {
      delete m_pIntervalFormat;
      m_pIntervalFormat = nullptr;
   }

   if ( m_pYFormat != nullptr )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }

}

CEAFGraphControlWindow* CDeflectionHistoryGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

void CDeflectionHistoryGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CDeflectionHistoryGraphViewController>* pController;
   CComObject<CDeflectionHistoryGraphViewController>::CreateInstance(&pController);
   pController->Init(m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CDeflectionHistoryGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CDeflectionHistoryGraphBuilder>(*this);
}

int CDeflectionHistoryGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
{
   if ( CEAFAutoCalcGraphBuilder::InitializeGraphController(pParent,nID) < 0 )
   {
      return -1;
   }

   EAFGetBroker(&m_pBroker);

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);

   m_Graph.SetTitle(_T("Deflection History"));

   m_pGraphController->CheckDlgButton(IDC_TIME_LOG,BST_CHECKED);

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.DrawGrid(); // show grid by default

   return 0;
}

BOOL CDeflectionHistoryGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   // create our controls
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_pGraphController->Create(pParent,IDD_DEFLECTIONHISTORY_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CDeflectionHistoryGraphBuilder::ShowGrid(bool bShowGrid)
{
   m_Graph.DrawGrid(bShowGrid);
   GetView()->Invalidate();
}

bool CDeflectionHistoryGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   m_XAxisType = m_pGraphController->GetXAxisType();
   UpdateXAxis();
   UpdateYAxis();


   // Update graph properties
   pgsPointOfInterest poi = m_pGraphController->GetLocation();
   if ( poi.GetID() != INVALID_ID )
   {
      UpdateGraphTitle(poi);
      UpdateGraphData(poi);
   }
   return true;
}

void CDeflectionHistoryGraphBuilder::UpdateGraphTitle(const pgsPointOfInterest& poi)
{
   m_Graph.SetTitle(_T("Deflection History"));

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   CString strSubtitle;
   std::_tstring strAttributes = poi.GetAttributes(POI_SPAN,false);
   if ( strAttributes.size() == 0 )
   {
      strSubtitle.Format(_T("Span %s Girder %s (%s)"),
         LABEL_SPAN(spanKey.spanIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()));
   }
   else
   {
      strSubtitle.Format(_T("Span %s, Girder %s (%s (%s))"),
         LABEL_SPAN(spanKey.spanIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()),
         strAttributes.c_str());
   }

   m_Graph.SetSubtitle(strSubtitle);
}

void CDeflectionHistoryGraphBuilder::UpdateGraphData(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // clear graph
   m_Graph.ClearData();
   m_Graph.SetMinimumSize(m_XAxisType == X_AXIS_INTERVAL ? 1 : 0,1,0,1);

   COLORREF color = BLUE;

   int penWeight = GRAPH_PEN_WEIGHT;

   IndexType dataSeries = m_Graph.CreateDataSeries(_T(""), PS_SOLID, penWeight, color);

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   GET_IFACE(IIntervals,pIntervals);

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   IntervalIndexType startIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   if ( m_XAxisType == X_AXIS_INTERVAL )
   {
      m_Graph.SetXAxisForcedRange((Float64)LABEL_INTERVAL(startIntervalIdx),(Float64)LABEL_INTERVAL(nIntervals),0.5);
      IntervalTool* pIntervalTool = dynamic_cast<IntervalTool*>(m_pIntervalFormat);
      pIntervalTool->SetLastValue((Float64)LABEL_INTERVAL(nIntervals));

      Float64 x = GetX(segmentKey,startIntervalIdx,pgsTypes::Start,pIntervals);
      PlotDeflection(x,poi,startIntervalIdx,dataSeries,bat,pLimitStateForces);
   }

   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      if ( m_XAxisType == X_AXIS_INTERVAL )
      {
         Float64 x = GetX(segmentKey,intervalIdx,pgsTypes::Middle,pIntervals);
         PlotDeflection(x,poi,intervalIdx,dataSeries,bat,pLimitStateForces);
      }

      Float64 x = GetX(segmentKey,intervalIdx,pgsTypes::End,pIntervals);
      PlotDeflection(x,poi,intervalIdx,dataSeries,bat,pLimitStateForces);
   }
}

Float64 CDeflectionHistoryGraphBuilder::GetX(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IIntervals* pIntervals)
{
   Float64 x;
   if ( m_XAxisType == X_AXIS_TIME_LINEAR || m_XAxisType == X_AXIS_TIME_LOG )
   {
      x = pIntervals->GetTime(intervalIdx,timeType);
   }
   else
   {
      x = (Float64)LABEL_INTERVAL(intervalIdx);
      if ( timeType == pgsTypes::Middle )
      {
         x += 0.5;
      }
      else if ( timeType == pgsTypes::End )
      {
         x += 1.0;
      }
   }

   if ( m_XAxisType == X_AXIS_TIME_LOG && IsZero(x) )
   {
      // x can't be zero for log scale because log(0) is undefined
      x = 1.0;
   }

   return x;
}

void CDeflectionHistoryGraphBuilder::PlotDeflection(Float64 x,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,IndexType dataSeries,pgsTypes::BridgeAnalysisType bat,ILimitStateForces* pLimitStateForces)
{
   bool bIncludePrestress = true;
   bool bIncludeLiveLoad  = false;
   bool bIncludeElevationAdjustment = ((CDeflectionHistoryGraphController*)m_pGraphController)->IncludeElevationAdjustment();
   bool bIncludePrecamber = ((CDeflectionHistoryGraphController*)m_pGraphController)->IncludeUnrecoverableDefl();

   Float64 Ymin, Ymax;
   pLimitStateForces->GetDeflection(intervalIdx,pgsTypes::ServiceI,poi,bat,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,bIncludePrecamber,bIncludePrecamber,&Ymin,&Ymax);
   AddGraphPoint(dataSeries, x, Ymin);
}

void CDeflectionHistoryGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   const WBFL::Units::PhysicalConverter* pcx = dynamic_cast<const WBFL::Units::PhysicalConverter*>(m_Graph.GetXAxisValueFormat());
   ASSERT(pcx);
   const WBFL::Units::PhysicalConverter* pcy = dynamic_cast<const WBFL::Units::PhysicalConverter*>(m_Graph.GetYAxisValueFormat());
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   Float64 y = pcy->Convert(yval);
   m_Graph.AddPoint(series, WBFL::Graphing::Point(x,y));
}

void CDeflectionHistoryGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int save = pDC->SaveDC();

   // The graph is valided and there was not an error
   // updating data.... draw the graph
   CRect rect = GetView()->GetDrawingRect();

   m_Graph.SetOutputRect(rect);
   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());
   m_Graph.Draw(pDC->GetSafeHdc());

   pDC->RestoreDC(save);
}

void CDeflectionHistoryGraphBuilder::UpdateXAxis()
{
   // x axis
   delete m_pTimeFormat;
   delete m_pIntervalFormat;

   m_pTimeFormat = new WBFL::Units::ScalarTool(m_Time);
   m_pIntervalFormat = new IntervalTool(m_Interval);
   m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   m_Graph.SetXAxisNumberOfMajorTics(11);

   if ( m_XAxisType == X_AXIS_TIME_LINEAR )
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Linear);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.XAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_TIME_LOG )
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Logarithmic);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.XAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Linear);
      m_Graph.SetXAxisTitle(_T("Interval"));
      m_Graph.XAxisNiceRange(false);
      m_Graph.SetXAxisNumberOfMinorTics(0);
      m_Graph.SetXAxisValueFormat(*m_pIntervalFormat);
   }
}

void CDeflectionHistoryGraphBuilder::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::LengthData& deflectionUnit = pDisplayUnits->GetDeflectionUnit();
   m_pYFormat = new WBFL::Units::DeflectionTool(deflectionUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisTitle(std::_tstring(_T("Deflection (")+m_pYFormat->UnitTag()+_T(")")).c_str());
   m_Graph.YAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);
}

void CDeflectionHistoryGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph,rstrDefaultFileName);
}