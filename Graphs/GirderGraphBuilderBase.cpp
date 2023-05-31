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
#include <Graphs\GirderGraphBuilderBase.h>
#include <Graphs\DrawBeamTool.h>

#include "GirderGraphControllerBase.h"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <Units\UnitValueNumericalFormatTools.h>

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
static WBFL::Units::LengthData DUMMY(WBFL::Units::Measure::Meter);
static WBFL::Units::LengthTool DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CGirderGraphBuilderBase, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CGirderGraphBuilderBase::CGirderGraphBuilderBase() :
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

CGirderGraphBuilderBase::CGirderGraphBuilderBase(const CGirderGraphBuilderBase& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pXFormat(nullptr),
m_pYFormat(nullptr),
m_ZeroToleranceX(TOLERANCE),
m_ZeroToleranceY(TOLERANCE),
m_pGraphController(nullptr)
{
}

CGirderGraphBuilderBase::~CGirderGraphBuilderBase()
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

CEAFGraphControlWindow* CGirderGraphBuilderBase::GetGraphControlWindow()
{
   ATLASSERT(m_pGraphController != nullptr);
   return m_pGraphController;
}

bool CGirderGraphBuilderBase::HandleDoubleClick(UINT nFlags,CPoint point)
{
   GET_IFACE(IEditByUI, pEditByUI);
   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if (girderKey.groupIndex == ALL_GROUPS)
   {
      // we don't know which group to edit since there are multiple groups displayed in the graph
      // This method of EditGirderDescription prompts if a girder is not defined by the selection context
      // of the Bridge View window.
      pEditByUI->EditGirderDescription();
   }
   else
   {
      pEditByUI->EditGirderDescription(girderKey, EGD_GENERAL);
   }

   return true;
}


int CGirderGraphBuilderBase::InitializeGraphController(CWnd* pParent,UINT nID)
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
   m_Graph.DrawGrid(m_pGraphController->ShowGrid());

   // Default settings for graph controller
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_pGraphController->CheckDlgButton(IDC_BEAM,BST_CHECKED);

   return 0;
}

void CGirderGraphBuilderBase::UpdateXAxis()
{
   if ( m_pXFormat )
   {
      delete m_pXFormat;
      m_pXFormat = nullptr;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::LengthData& lengthUnit = pDisplayUnits->GetSpanLengthUnit();
   m_pXFormat = new WBFL::Units::LengthTool(lengthUnit);
   m_Graph.SetXAxisValueFormat(*m_pXFormat);
   m_Graph.SetXAxisNumberOfMinorTics(0);
   m_Graph.XAxisNiceRange(false);
   m_Graph.SetXAxisNumberOfMajorTics(11);
}

void CGirderGraphBuilderBase::UpdateYAxis()
{
   if ( m_pYFormat )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::StressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new WBFL::Units::StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.YAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);
}

void CGirderGraphBuilderBase::Shift(bool bShift)
{
   m_bShift = bShift;
}

bool CGirderGraphBuilderBase::Shift() const
{
   return m_bShift;
}

void CGirderGraphBuilderBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if (lHint == HINT_SELECTIONCHANGED)
      return; // none of the graphs are keyed to the selection, so if the selection changes, do nothing

   CEAFAutoCalcGraphBuilder::OnUpdate(pSender, lHint, pHint);
}

bool CGirderGraphBuilderBase::UpdateNow()
{
   return true;
}

Float64 CGirderGraphBuilderBase::ComputeShift(const CGirderKey& girderKey)
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.groupIndex == 0 )
   {
      // we are showing the first group so there isn't a shift
      return 0;
   }

   GET_IFACE(IPointOfInterest,pPoi);
   pgsPointOfInterest poi(CSegmentKey(girderKey,0),0.0);
   Float64 Xgl = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   poi.SetSegmentKey(CSegmentKey(0,girderKey.girderIndex,0));
   Float64 Xstart = pPoi->ConvertPoiToGirderlineCoordinate(poi);

   Float64 shift = Xstart - Xgl;
   return shift;
}

void CGirderGraphBuilderBase::GetXValues(const PoiList& vPoi,std::vector<Float64>* pXVals)
{
   GET_IFACE(IPointOfInterest,pPoi);

   pXVals->clear();
   pXVals->reserve(vPoi.size());

   Float64 shift = 0;
   if ( m_bShift )
   {
      const CGirderKey& girderKey(vPoi.front().get().GetSegmentKey());
      shift = ComputeShift(girderKey);
   }

   for (const pgsPointOfInterest& poi : vPoi)
   {
      Float64 Xg = pPoi->ConvertPoiToGirderlineCoordinate(poi);
      
      Xg += shift;

      pXVals->push_back(Xg);
   }
}

void CGirderGraphBuilderBase::AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<Float64>& yvals)
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

void CGirderGraphBuilderBase::AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<WBFL::System::SectionValue>& yvals)
{
   std::vector<Float64>::const_iterator xIter;
   std::vector<WBFL::System::SectionValue>::const_iterator yIter;
   for ( xIter = xvals.begin(), yIter = yvals.begin(); xIter != xvals.end() && yIter != yvals.end(); xIter++, yIter++ )
   {
      Float64 X = *xIter;
      Float64 Yleft = (*yIter).Left();
      Float64 Yright = (*yIter).Right();
      AddGraphPoint(series,X,Yleft);
      AddGraphPoint(series,X,Yright);
   }
}

void CGirderGraphBuilderBase::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   WBFL::Units::PhysicalConverter* pcx = dynamic_cast<WBFL::Units::PhysicalConverter*>(m_pXFormat);
   ASSERT(pcx);
   WBFL::Units::PhysicalConverter* pcy = dynamic_cast<WBFL::Units::PhysicalConverter*>(m_pYFormat);
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   x = IsZero(x,m_ZeroToleranceX) ? 0 : x;
   Float64 y = pcy->Convert(yval);
   y = IsZero(y,m_ZeroToleranceY) ? 0 : y;
   m_Graph.AddPoint(series, WBFL::Graphing::Point(x,y));
}

void CGirderGraphBuilderBase::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
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
   if (m_pGraphController->ShowBeam())
   {
      CGirderKey girderKey(m_pGraphController->GetGirderGroup(),m_pGraphController->GetGirder());

      Float64 shift = 0;
      if ( m_bShift )
      {
         shift = ComputeShift(girderKey);
      }

      // make the minimum size of the graph include the size of the girder. this makes the girder display
      // properly when there aren't any points to graph
      GET_IFACE(IBridge, pBridge);
      std::vector<CGirderKey> vGirderKeys;
      pBridge->GetGirderline(girderKey, &vGirderKeys);

      GET_IFACE(IPointOfInterest,pPoi);
      pgsPointOfInterest startPoi(CSegmentKey(vGirderKeys.front(),0),0.0);
      Float64 Xstart = pPoi->ConvertPoiToGirderlineCoordinate(startPoi);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(vGirderKeys.back());
      CSegmentKey lastSegmentKey(vGirderKeys.back(),nSegments-1);
      Float64 Ls = pBridge->GetSegmentLength(lastSegmentKey);
      pgsPointOfInterest endPoi(lastSegmentKey,Ls);
      Float64 Xend = pPoi->ConvertPoiToGirderlineCoordinate(endPoi);

      Xstart += shift;
      Xend   += shift;
      Xstart = m_pXFormat->Convert(Xstart);
      Xend   = m_pXFormat->Convert(Xend);

      WBFL::Graphing::Rect wrect = m_Graph.GetRawWorldRect();

      m_Graph.SetMinimumSize(Xstart,Xend,wrect.Bottom(),wrect.Top());
   }
   else
   {
      WBFL::Graphing::Rect wrect = m_Graph.GetRawWorldRect();
      m_Graph.SetMinimumSize(wrect.Left(), wrect.Right(),wrect.Bottom(),wrect.Top());
   }

   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());

   // draw the background
   m_Graph.DrawBackground(pDC->GetSafeHdc());

   // superimpose the beam on the background
   if ( m_pGraphController->ShowBeam() )
   {
      CGirderKey girderKey(m_pGraphController->GetGirderGroup(),m_pGraphController->GetGirder());

      Float64 shift = 0;
      if ( m_bShift )
      {
         shift = ComputeShift(girderKey);
      }

      IntervalIndexType firstIntervalIdx, lastIntervalIdx;
      GetBeamDrawIntervals(&firstIntervalIdx,&lastIntervalIdx);

      CDrawBeamTool drawBeam;
      drawBeam.SetMinAspectRatio(25.0); // shrink beam height to reasonable aspect if needed so it will fit nicely on graph.
      drawBeam.SetStyle(GetDrawBeamStyle());

      WBFL::Graphing::PointMapper mapper;
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

         WBFL::Graphing::Size wExt = mapper.GetWorldExt();
         WBFL::Graphing::Point wOrg = mapper.GetWorldOrg();

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

      drawBeam.DrawBeam(m_pBroker,pDC,mapper,m_pXFormat,firstIntervalIdx,lastIntervalIdx,girderKey,shift);
   }

   // draw the data series
   m_Graph.DrawDataSeries(pDC->GetSafeHdc());

   pDC->RestoreDC(save);
}

DWORD CGirderGraphBuilderBase::GetDrawBeamStyle() const
{
   return 0;
}

void CGirderGraphBuilderBase::ShowGrid(bool bShow)
{
   m_Graph.DrawGrid(bShow);
   GetView()->Invalidate();
}

void CGirderGraphBuilderBase::ShowBeam(bool bShow)
{
   GetView()->Invalidate();
}
