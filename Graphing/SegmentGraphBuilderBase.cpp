///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <Graphing\SegmentGraphBuilderBase.h>
#include <Graphing\DrawBeamTool.h>

#include "SegmentGraphControllerBase.h"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <UnitMgt\UnitValueNumericalFormatTools.h>

#include <Hints.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\EditByUI.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFGraphControlWindow.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool    DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CSegmentGraphBuilderBase, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CSegmentGraphBuilderBase::CSegmentGraphBuilderBase() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pXFormat(nullptr),
m_pYFormat(nullptr),
m_ZeroToleranceX(TOLERANCE),
m_ZeroToleranceY(TOLERANCE),
m_pGraphController(nullptr),
m_bShift(false)
{
}

CSegmentGraphBuilderBase::CSegmentGraphBuilderBase(const CSegmentGraphBuilderBase& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pXFormat(nullptr),
m_pYFormat(nullptr),
m_ZeroToleranceX(TOLERANCE),
m_ZeroToleranceY(TOLERANCE),
m_pGraphController(nullptr)
{
}

CSegmentGraphBuilderBase::~CSegmentGraphBuilderBase()
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

CEAFGraphControlWindow* CSegmentGraphBuilderBase::GetGraphControlWindow()
{
   ATLASSERT(m_pGraphController != nullptr);
   return m_pGraphController;
}

bool CSegmentGraphBuilderBase::HandleDoubleClick(UINT nFlags,CPoint point)
{
   const CSegmentKey segmentKey(m_pGraphController->GetSegmentKey());

   GET_IFACE(IEditByUI,pEditByUI);
   pEditByUI->EditSegmentDescription(segmentKey,EGS_GENERAL);

   return true;
}

int CSegmentGraphBuilderBase::InitializeGraphController(CWnd* pParent,UINT nID)
{
   EAFGetBroker(&m_pBroker);

   m_pGraphController = CreateGraphController();

   if ( CEAFAutoCalcGraphBuilder::InitializeGraphController(pParent,nID) < 0 )
   {
      return -1;
   }

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);

   // x axis
   UpdateXAxis();

   // y axis
   UpdateYAxis();

   // Show the grid by default... set the control to checked
   m_Graph.SetDoDrawGrid(m_pGraphController->ShowGrid());

   // Default settings for graph controller
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_pGraphController->CheckDlgButton(IDC_BEAM,BST_CHECKED);

   return 0;
}

void CSegmentGraphBuilderBase::UpdateXAxis()
{
   if ( m_pXFormat )
   {
      delete m_pXFormat;
      m_pXFormat = nullptr;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& lengthUnit = pDisplayUnits->GetSpanLengthUnit();
   m_pXFormat = new LengthTool(lengthUnit);
   m_Graph.SetXAxisValueFormat(*m_pXFormat);
   m_Graph.SetXAxisNumberOfMinorTics(0);
   m_Graph.SetXAxisNiceRange(false);
   m_Graph.SetXAxisNumberOfMajorTics(11);
}

void CSegmentGraphBuilderBase::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);
}

void CSegmentGraphBuilderBase::Shift(bool bShift)
{
   m_bShift = bShift;
}

bool CSegmentGraphBuilderBase::Shift() const
{
   return m_bShift;
}

void CSegmentGraphBuilderBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if (lHint == HINT_SELECTIONCHANGED)
      return; // none of the graphs are keyed to the selection, so if the selection changes, do nothing

   CEAFAutoCalcGraphBuilder::OnUpdate(pSender, lHint, pHint);
}

bool CSegmentGraphBuilderBase::UpdateNow()
{
   return true;
}

Float64 CSegmentGraphBuilderBase::ComputeShift(const CSegmentKey& segmentKey)
{
   if (segmentKey.groupIndex == ALL_GROUPS || segmentKey.segmentIndex == ALL_SEGMENTS)
   {
      ATLASSERT(0); // not sure what to do about this. shouldn't happen
      return 0;
   }

   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest poi(segmentKey,0.0); // start of our segment
   Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   poi.SetSegmentKey(CSegmentKey(0,segmentKey.girderIndex,0)); // start of bridge
   Float64 Xstart = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   Float64 shift = Xstart - Xgl;
   return shift;
}

void CSegmentGraphBuilderBase::GetXValues(const PoiList& vPoi,std::vector<Float64>* pXVals)
{
   GET_IFACE(IPointOfInterest,pPoi);

   pXVals->clear();
   pXVals->reserve(vPoi.size());

   Float64 shift = 0;
   if ( m_bShift )
   {
      const CSegmentKey& segmentKey(vPoi.front().get().GetSegmentKey());
      shift = ComputeShift(segmentKey);
   }

   for (const pgsPointOfInterest& poi : vPoi)
   {
      Float64 Xg = pPoi->ConvertPoiToGirderlineCoordinate(poi);
      
      Xg += shift;

      pXVals->push_back(Xg);
   }
}

void CSegmentGraphBuilderBase::AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<Float64>& yvals)
{
   std::vector<Float64>::const_iterator xIter(xvals.begin()), yIter(yvals.begin());
   std::vector<Float64>::const_iterator xIterEnd(xvals.end()), yIterEnd(yvals.end());
   for ( ; xIter != xIterEnd && yIter != yIterEnd; xIter++, yIter++ )
   {
      Float64 X = *xIter;
      Float64 Y = *yIter;
      AddGraphPoint(series,X,Y);
   }
}

void CSegmentGraphBuilderBase::AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<sysSectionValue>& yvals)
{
   std::vector<Float64>::const_iterator xIter;
   std::vector<sysSectionValue>::const_iterator yIter;
   for ( xIter = xvals.begin(), yIter = yvals.begin(); xIter != xvals.end() && yIter != yvals.end(); xIter++, yIter++ )
   {
      Float64 X = *xIter;
      Float64 Yleft = (*yIter).Left();
      Float64 Yright = (*yIter).Right();
      AddGraphPoint(series,X,Yleft);
      AddGraphPoint(series,X,Yright);
   }
}

void CSegmentGraphBuilderBase::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   arvPhysicalConverter* pcx = dynamic_cast<arvPhysicalConverter*>(m_pXFormat);
   ASSERT(pcx);
   arvPhysicalConverter* pcy = dynamic_cast<arvPhysicalConverter*>(m_pYFormat);
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   x = IsZero(x,m_ZeroToleranceX) ? 0 : x;
   Float64 y = pcy->Convert(yval);
   y = IsZero(y,m_ZeroToleranceY) ? 0 : y;
   m_Graph.AddPoint(series, gpPoint2d(x,y));
}

void CSegmentGraphBuilderBase::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int save = pDC->SaveDC();

   // The graph is valided and there was not an error
   // updating data.... draw the graph
   CRect rect = GetView()->GetDrawingRect();

   // Draw beam at bottom 10% of view
   Float64 beamViewOffsetFac = 1.0;
   if (m_pGraphController->ShowBeam() && m_pGraphController->ShowBeamBelowGraph())
   {
      beamViewOffsetFac = 0.90;
   }

   rect.bottom = (LONG)(rect.bottom * beamViewOffsetFac);
   m_Graph.SetOutputRect(rect);

   // before drawing the graph background, which also draws the axes
   // make sure the graph is big enough to hold the beam
   const CSegmentKey segmentKey(m_pGraphController->GetSegmentKey());
   if (m_pGraphController->ShowBeam())
   {
      Float64 shift = 0;
      if ( m_bShift )
      {
         shift = ComputeShift(segmentKey);
      }

      // make the minimum size of the graph include the size of the girder. this makes the girder display
      // properly when there aren't any points to graph
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IPointOfInterest,pPoi);
      pgsPointOfInterest startPoi(segmentKey, 0.0);
      Float64 Xstart = pPoi->ConvertPoiToGirderlineCoordinate(startPoi);

      Float64 segLen = pBridge->GetSegmentLength(segmentKey);

      Float64 Xend = Xstart + segLen;

      Xstart += shift;
      Xend   += shift;
      Xstart = m_pXFormat->Convert(Xstart);
      Xend   = m_pXFormat->Convert(Xend);
      m_Graph.SetMinimumSize(Xstart,Xend,0,1.0e-06);
   }

   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());

   // draw the background
   m_Graph.DrawBackground(pDC->GetSafeHdc());

   // superimpose the beam on the background
   if ( m_pGraphController->ShowBeam() )
   {
      Float64 shift = 0;
      if ( m_bShift )
      {
         shift = ComputeShift(segmentKey);
      }

      IntervalIndexType firstIntervalIdx, lastIntervalIdx;
      GetBeamDrawIntervals(&firstIntervalIdx,&lastIntervalIdx);

      CDrawBeamTool drawBeam;
      drawBeam.SetMinAspectRatio(30.0); // shrink beam height to reasonable aspect if needed so it will fit nicely on graph.
      drawBeam.SetStyle(GetDrawBeamStyle());

      grlibPointMapper mapper;
      if (!m_pGraphController->ShowBeamBelowGraph())
      {
         // typical case where beam is embedded in graph
         mapper = drawBeam.CreatePointMapperAtGraphZero( m_Graph.GetClientAreaPointMapper( pDC->GetSafeHdc() ) );
      }
      else
      {
         // create mapper to draw beam at bottom of window below graph
         mapper = m_Graph.GetClientAreaPointMapper(pDC->GetSafeHdc());

         LONG dox, doy;
         mapper.GetDeviceOrg(&dox, &doy);
         LONG dx, dy;
         mapper.GetDeviceExt(&dx, &dy);

         gpSize2d wExt = mapper.GetWorldExt();
         gpPoint2d wOrg = mapper.GetWorldOrg();

         // get device point at World (0,0)
         LONG x, y;
         mapper.WPtoDP(0, 0, &x, &y);

         CSize supsize = drawBeam.GetSupportSize(pDC);

         // set to isotropic scaling
         mapper.SetDeviceExt(dx, dx);

         // Move beam to just above bottom of window
         mapper.SetDeviceOrg(dox - dx / 2, doy + dy / 2 + (LONG)(rect.bottom * (1.0 - beamViewOffsetFac) - supsize.cy));

         mapper.SetWorldOrg(0, 0);
         mapper.SetWorldExt(wExt.Dx(), wExt.Dx());
      }

      drawBeam.DrawBeamSegment(m_pBroker,pDC,mapper,m_pXFormat,firstIntervalIdx,lastIntervalIdx,segmentKey,shift);
   }

   // draw the data series
   m_Graph.DrawDataSeries(pDC->GetSafeHdc());

   pDC->RestoreDC(save);
}

DWORD CSegmentGraphBuilderBase::GetDrawBeamStyle() const
{
   return 0;
}

void CSegmentGraphBuilderBase::ShowGrid(bool bShow)
{
   m_Graph.SetDoDrawGrid(bShow);
   GetView()->Invalidate();
}

void CSegmentGraphBuilderBase::ShowBeam(bool bShow)
{
   GetView()->Invalidate();
}
