///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <Graphing\StressHistoryGraphBuilder.h>
#include "StressHistoryGraphController.h"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <UnitMgt\UnitValueNumericalFormatTools.h>
#include <PgsExt\LoadFactors.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PointOfInterest.h>

#include <EAF\EAFGraphView.h>

#include <GraphicsLib\AxisXY.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool    DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CStressHistoryGraphBuilder, CEAFGraphBuilderBase)
   ON_BN_CLICKED(IDC_TOPDECK,OnTopDeck)
   ON_BN_CLICKED(IDC_BOTTOMDECK,OnBottomDeck)
   ON_BN_CLICKED(IDC_TOPGIRDER,OnTopGirder)
   ON_BN_CLICKED(IDC_BOTTOMGIRDER,OnBottomGirder)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
END_MESSAGE_MAP()


CStressHistoryGraphBuilder::CStressHistoryGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_XAxisType(X_AXIS_TIME_LOG)
{
   for ( int i = 0; i < 4; i++ )
   {
      m_bPlot[i] = false;
   }

   m_pGraphController = new CStressHistoryGraphController;

   SetName(_T("Stress History"));

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CStressHistoryGraphBuilder::CStressHistoryGraphBuilder(const CStressHistoryGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_XAxisType(X_AXIS_TIME_LOG)
{
   for ( int i = 0; i < 4; i++ )
   {
      m_bPlot[i] = other.m_bPlot[i];
   }

   m_pGraphController = new CStressHistoryGraphController;

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CStressHistoryGraphBuilder::~CStressHistoryGraphBuilder()
{
   if ( m_pGraphController != NULL )
   {
      delete m_pGraphController;
      m_pGraphController = NULL;
   }

   if ( m_pTimeFormat != NULL )
   {
      delete m_pTimeFormat;
      m_pTimeFormat = NULL;
   }

   if ( m_pIntervalFormat != NULL )
   {
      delete m_pIntervalFormat;
      m_pIntervalFormat = NULL;
   }

   if ( m_pYFormat != NULL )
   {
      delete m_pYFormat;
      m_pYFormat = NULL;
   }

}

CEAFGraphControlWindow* CStressHistoryGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

CGraphBuilder* CStressHistoryGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CStressHistoryGraphBuilder(*this);
}

int CStressHistoryGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
{
   if ( CEAFAutoCalcGraphBuilder::InitializeGraphController(pParent,nID) < 0 )
   {
      return -1;
   }

   // create the graph definitions before creating the graph controller.
   // our graph controller will call GetLoadCaseNames to populate the 
   // list of load cases
   EAFGetBroker(&m_pBroker);

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);

   m_Graph.SetTitle(_T("Stress History"));

   m_pGraphController->CheckDlgButton(IDC_TIME_LOG,BST_CHECKED);

   // x axis
   m_pTimeFormat = new ScalarTool(m_Scalar);
   m_pIntervalFormat = new ScalarTool(m_Scalar);
   m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   m_Graph.SetXAxisNumberOfMajorTics(11);
   m_XAxisType = X_AXIS_TIME_LOG;

   // y axis
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisTitle(std::_tstring(_T("Stress (")+m_pYFormat->UnitTag()+_T(")")).c_str());
   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.SetDoDrawGrid(); // show grid by default

   return 0;
}

BOOL CStressHistoryGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   // create our controls
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_pGraphController->Create(pParent,IDD_STRESSHISTORY_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CStressHistoryGraphBuilder::OnTopDeck()
{
   m_bPlot[pgsTypes::TopDeck] = !m_bPlot[pgsTypes::TopDeck];
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnBottomDeck()
{
   m_bPlot[pgsTypes::BottomDeck] = !m_bPlot[pgsTypes::BottomDeck];
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnTopGirder()
{
   m_bPlot[pgsTypes::TopGirder] = !m_bPlot[pgsTypes::TopGirder];
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnBottomGirder()
{
   m_bPlot[pgsTypes::BottomGirder] = !m_bPlot[pgsTypes::BottomGirder];
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnShowGrid()
{
   m_Graph.SetDoDrawGrid( !m_Graph.GetDoDrawGrid() );
   GetView()->Invalidate();
}

bool CStressHistoryGraphBuilder::UpdateNow()
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
   UpdateGraphTitle(poi);
   UpdateGraphData(poi);
   return true;
}

void CStressHistoryGraphBuilder::UpdateGraphTitle(const pgsPointOfInterest& poi)
{
   pgsTypes::LimitState limitState = GetLimitState();

   GET_IFACE(IProductLoads,pProductLoads);

   CString strGraphTitle;
   strGraphTitle.Format(_T("Stress History (%s - Without Live Load)"),pProductLoads->GetLimitStateName(limitState));
   m_Graph.SetTitle(strGraphTitle);

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
      strSubtitle.Format(_T("Group %d Girder %s Span %d (%s)"),
         LABEL_GROUP(segmentKey.groupIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         LABEL_SPAN(spanKey.spanIndex),
         FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()));
   }
   else
   {
      strSubtitle.Format(_T("Group %d Girder %s Span %d, (%s (%s))"),
         LABEL_GROUP(segmentKey.groupIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         LABEL_SPAN(spanKey.spanIndex),
         FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()),
         strAttributes.c_str());
   }

   m_Graph.SetSubtitle(strSubtitle);
}

void CStressHistoryGraphBuilder::UpdateGraphData(const pgsPointOfInterest& poi)
{
   // clear graph
   m_Graph.ClearData();

   int penWeight = GRAPH_PEN_WEIGHT;

   IndexType dataSeries[4];
   dataSeries[pgsTypes::TopDeck]         = m_Graph.CreateDataSeries(_T("Top of Slab"),     PS_SOLID, penWeight, ORANGE);
   dataSeries[pgsTypes::BottomDeck]      = m_Graph.CreateDataSeries(_T("Bottom of Slab"),  PS_SOLID, penWeight, RED);
   dataSeries[pgsTypes::TopGirder]       = m_Graph.CreateDataSeries(_T("Top of Girder"),   PS_SOLID, penWeight, GREEN);
   dataSeries[pgsTypes::BottomGirder]    = m_Graph.CreateDataSeries(_T("Bottom of Girder"),PS_SOLID, penWeight, BLUE);

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE_NOCHECK(ILimitStateForces,pLimitStateForces); // only used if a stress location is checked
   GET_IFACE_NOCHECK(ICombinedForces,pCombinedForces);

   pgsTypes::LimitState limitState = GetLimitState();
   GET_IFACE(ILoadFactors,pLoadFactors);
   Float64 gLL = pLoadFactors->GetLoadFactors()->LLIMmin[limitState];

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   IntervalIndexType startIntervalIdx = 0;

   // Plot points at start of first interval, then at the end of all the intervals
   Float64 x = GetX(segmentKey,startIntervalIdx,pgsTypes::Start,pIntervals);
   for ( int i = 0; i < 4; i++ )
   {
      if ( m_bPlot[i] )
      {
         pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
         PlotStressPoints(x,poi,stressLocation,startIntervalIdx,dataSeries[i],limitState,bat,gLL,liveLoadIntervalIdx,pCombinedForces,pLimitStateForces);
      }
   }

   // now plot points at the end of all the intervals
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 x = GetX(segmentKey,intervalIdx,pgsTypes::End,pIntervals);
      for ( int i = 0; i < 4; i++ )
      {
         if ( m_bPlot[i] )
         {
            pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
            PlotStressPoints(x,poi,stressLocation,intervalIdx,dataSeries[i],limitState,bat,gLL,liveLoadIntervalIdx,pCombinedForces,pLimitStateForces);
         }
      }
   }
}

Float64 CStressHistoryGraphBuilder::GetX(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IIntervals* pIntervals)
{
   Float64 x;
   if ( m_XAxisType == X_AXIS_TIME_LINEAR || m_XAxisType == X_AXIS_TIME_LOG )
   {
      x = pIntervals->GetTime(intervalIdx,timeType);
   }
   else
   {
      x = (Float64)LABEL_INTERVAL(intervalIdx);
   }

   if ( m_XAxisType == X_AXIS_TIME_LOG && IsZero(x) )
   {
      // x can't be zero for log scale because log(0) is undefined
      x = 1.0;
   }

   return x;
}

void CStressHistoryGraphBuilder::PlotStressPoints(Float64 x,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,IndexType dataSeries,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64 gLL,IntervalIndexType liveLoadIntervalIdx,ICombinedForces* pCombinedForces,ILimitStateForces* pLimitStateForces)
{
   bool bIncludePrestress = true;

   Float64 fMinEnd, fMaxEnd;
   pLimitStateForces->GetStress(intervalIdx,limitState,poi,bat,bIncludePrestress,stressLocation,&fMinEnd,&fMaxEnd);

   // get live load stress
   Float64 fTopMin(0), fTopMax(0), fBotMin(0), fBotMax(0);
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      pCombinedForces->GetCombinedLiveLoadStress(intervalIdx,pgsTypes::lltDesign,poi,bat,stressLocation,pgsTypes::BottomGirder,&fTopMin,&fTopMax,&fBotMin,&fBotMax);
   }

   // remove live load from combintation
   fMinEnd -= gLL*fTopMin;

   AddGraphPoint(dataSeries,x,fMinEnd);
}

void CStressHistoryGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   const arvPhysicalConverter* pcx = dynamic_cast<const arvPhysicalConverter*>(m_Graph.GetXAxisValueFormat());
   ASSERT(pcx);
   const arvPhysicalConverter* pcy = dynamic_cast<const arvPhysicalConverter*>(m_Graph.GetYAxisValueFormat());
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   Float64 y = pcy->Convert(yval);
   m_Graph.AddPoint(series, gpPoint2d(x,y));
}

void CStressHistoryGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
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

pgsTypes::LimitState CStressHistoryGraphBuilder::GetLimitState()
{
   return pgsTypes::ServiceI;
}

void CStressHistoryGraphBuilder::UpdateXAxis()
{
   if ( m_XAxisType == X_AXIS_TIME_LINEAR )
   {
      m_Graph.SetXAxisScale(grAxisXY::LINEAR);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.SetXAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_TIME_LOG )
   {
      m_Graph.SetXAxisScale(grAxisXY::LOGARITHMIC);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.SetXAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else
   {
      m_Graph.SetXAxisScale(grAxisXY::INTEGRAL);
      m_Graph.SetXAxisTitle(_T("Interval"));
      m_Graph.SetXAxisNiceRange(false);
      m_Graph.SetXAxisNumberOfMinorTics(0);
      m_Graph.SetXAxisValueFormat(*m_pIntervalFormat);
   }
}

void CStressHistoryGraphBuilder::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisTitle(std::_tstring(_T("Stress (")+m_pYFormat->UnitTag()+_T(")")).c_str());
}