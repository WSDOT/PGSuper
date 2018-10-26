///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Graphing\DeflectionHistoryGraphBuilder.h>
#include "DeflectionHistoryGraphController.h"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

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

BEGIN_MESSAGE_MAP(CDeflectionHistoryGraphBuilder, CEAFGraphBuilderBase)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
END_MESSAGE_MAP()


CDeflectionHistoryGraphBuilder::CDeflectionHistoryGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_XAxisType(X_AXIS_TIME_LOG)
{
   m_pGraphController = new CDeflectionHistoryGraphController;

   SetName(_T("Deflection History"));

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CDeflectionHistoryGraphBuilder::CDeflectionHistoryGraphBuilder(const CDeflectionHistoryGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_XAxisType(X_AXIS_TIME_LOG)
{
   m_pGraphController = new CDeflectionHistoryGraphController;

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CDeflectionHistoryGraphBuilder::~CDeflectionHistoryGraphBuilder()
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

CEAFGraphControlWindow* CDeflectionHistoryGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

CGraphBuilder* CDeflectionHistoryGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CDeflectionHistoryGraphBuilder(*this);
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
   m_Graph.SetDoDrawGrid(); // show grid by default

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

void CDeflectionHistoryGraphBuilder::OnShowGrid()
{
   m_Graph.SetDoDrawGrid( !m_Graph.GetDoDrawGrid() );
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
   UpdateGraphTitle(poi);
   UpdateGraphData(poi);
   return true;
}

void CDeflectionHistoryGraphBuilder::UpdateGraphTitle(const pgsPointOfInterest& poi)
{
   CString strGraphTitle;
   strGraphTitle.Format(_T("Deflection History"));
   m_Graph.SetTitle(std::_tstring(strGraphTitle));

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IPointOfInterest,pPoi);
   SpanIndexType spanIdx;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanIdx,&Xspan);

   CString strSubtitle;
   std::_tstring strAttributes = poi.GetAttributes(POI_SPAN,false);
   if ( strAttributes.size() == 0 )
   {
      strSubtitle.Format(_T("Group %d Girder %s Span %d (%s)"),
         LABEL_GROUP(segmentKey.groupIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         LABEL_SPAN(spanIdx),
         FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()));
   }
   else
   {
      strSubtitle.Format(_T("Group %d Girder %s Span %d, (%s (%s))"),
         LABEL_GROUP(segmentKey.groupIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         LABEL_SPAN(spanIdx),
         FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()),
         strAttributes.c_str());
   }

   m_Graph.SetSubtitle(std::_tstring(strSubtitle));
}

void CDeflectionHistoryGraphBuilder::UpdateGraphData(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // clear graph
   m_Graph.ClearData();

   COLORREF color = BLUE;

   int penWeight = GRAPH_PEN_WEIGHT;

   IndexType minDataSeriesIdx = m_Graph.CreateDataSeries(_T(""), PS_DOT,   penWeight, color);
   IndexType maxDataSeriesIdx = m_Graph.CreateDataSeries(_T(""), PS_DOT,   penWeight, color);
   IndexType dataSeriesIdx    = m_Graph.CreateDataSeries(_T(""), PS_SOLID, penWeight, color);

   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(ILimitStateForces,pLimitStateForces);
   GET_IFACE(IIntervals,pIntervals);

   bool bIncludeElevationAdjustment = ((CDeflectionHistoryGraphController*)m_pGraphController)->IncludeElevationAdjustment();

   IntervalIndexType nIntervals          = pIntervals->GetIntervalCount(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
   for ( IntervalIndexType intervalIdx = 1; intervalIdx < nIntervals; intervalIdx++ )
   {
      IntervalIndexType prevIntervalIdx = intervalIdx-1;

      Float64 xStart, xEnd;
      if ( m_XAxisType == X_AXIS_TIME_LINEAR || m_XAxisType == X_AXIS_TIME_LOG )
      {
         xStart = pIntervals->GetStart(segmentKey,intervalIdx);
         xEnd   = pIntervals->GetEnd(segmentKey,intervalIdx);
      }
      else
      {
         xEnd   = (Float64)LABEL_INTERVAL(intervalIdx);
         xStart = xEnd - 1;
      }

      Float64 yStartMin(0), yStartMax(0);
      Float64 yEndMin(0), yEndMax(0);
      Float64 yStartNoLLMin(0), yStartNoLLMax(0);
      Float64 yEndNoLLMin(0), yEndNoLLMax(0);
      if ( releaseIntervalIdx <= intervalIdx )
      {
         pgsTypes::BridgeAnalysisType batMin = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);
         pgsTypes::BridgeAnalysisType batMax = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

         bool bIncludePrestress = true;
         bool bIncludeLiveLoad = true;
         Float64 yDummy;
         if ( batMin == batMax )
         {
            pLimitStateForces->GetDeflection(prevIntervalIdx,pgsTypes::ServiceI,poi,batMin,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&yStartMin,&yStartMax);
            pLimitStateForces->GetDeflection(    intervalIdx,pgsTypes::ServiceI,poi,batMin,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&yEndMin,  &yEndMax);
         }
         else
         {
            pLimitStateForces->GetDeflection(prevIntervalIdx,pgsTypes::ServiceI,poi,batMin,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&yStartMin,&yDummy);
            pLimitStateForces->GetDeflection(    intervalIdx,pgsTypes::ServiceI,poi,batMin,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&yEndMin,  &yDummy);

            pLimitStateForces->GetDeflection(prevIntervalIdx,pgsTypes::ServiceI,poi,batMax,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&yDummy,&yStartMax);
            pLimitStateForces->GetDeflection(    intervalIdx,pgsTypes::ServiceI,poi,batMax,bIncludePrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&yDummy,&yEndMax);
         }

         pLimitStateForces->GetDeflection(prevIntervalIdx,pgsTypes::ServiceI,poi,batMin,bIncludePrestress,false,bIncludeElevationAdjustment,&yStartNoLLMin,&yStartNoLLMax);
         pLimitStateForces->GetDeflection(    intervalIdx,pgsTypes::ServiceI,poi,batMin,bIncludePrestress,false,bIncludeElevationAdjustment,&yEndNoLLMin,  &yEndNoLLMax);
      }

      AddGraphPoint(minDataSeriesIdx,xStart,yStartMin);
      AddGraphPoint(minDataSeriesIdx,xEnd,  yEndMin);

      AddGraphPoint(dataSeriesIdx, xStart, yStartNoLLMin);
      AddGraphPoint(dataSeriesIdx, xEnd,   yEndNoLLMin);

      AddGraphPoint(maxDataSeriesIdx,xStart,yStartMax);
      AddGraphPoint(maxDataSeriesIdx,xEnd,  yEndMax);
   }
}

void CDeflectionHistoryGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
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

   m_pTimeFormat = new ScalarTool(m_Scalar);
   m_pIntervalFormat = new ScalarTool(m_Scalar);
   m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   m_Graph.SetXAxisNumberOfMajorTics(11);

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

void CDeflectionHistoryGraphBuilder::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& deflectionUnit = pDisplayUnits->GetDeflectionUnit();
   m_pYFormat = new DeflectionTool(deflectionUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisTitle(_T("Deflection (")+m_pYFormat->UnitTag()+_T(")"));
   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);
}
