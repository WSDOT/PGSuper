///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Graphing\GirderGraphBuilderBase.h>
#include <Graphing\DrawBeamTool.h>

#include "GirderGraphControllerBase.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>

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

BEGIN_MESSAGE_MAP(CGirderGraphBuilderBase, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CGirderGraphBuilderBase::CGirderGraphBuilderBase() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pXFormat(0),
m_pYFormat(0),
m_GraphStartOffset(0),
m_pGraphController(0),
m_bShowBeam(true)
{
}

CGirderGraphBuilderBase::CGirderGraphBuilderBase(const CGirderGraphBuilderBase& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pXFormat(0),
m_pYFormat(0),
m_GraphStartOffset(0),
m_pGraphController(0),
m_bShowBeam(other.m_bShowBeam)
{
}

CGirderGraphBuilderBase::~CGirderGraphBuilderBase()
{
   if ( m_pGraphController != NULL )
   {
      delete m_pGraphController;
      m_pGraphController = NULL;
   }

   if ( m_pXFormat != NULL )
   {
      delete m_pXFormat;
      m_pXFormat = NULL;
   }

   if ( m_pYFormat != NULL )
   {
      delete m_pYFormat;
      m_pYFormat = NULL;
   }

}

CEAFGraphControlWindow* CGirderGraphBuilderBase::GetGraphControlWindow()
{
   if ( m_pGraphController == NULL )
      m_pGraphController = CreateGraphController();

   return m_pGraphController;
}

int CGirderGraphBuilderBase::CreateControls(CWnd* pParent,UINT nID)
{
   // let the base class do its thing
   CEAFAutoCalcGraphBuilder::CreateControls(pParent,nID);

   EAFGetBroker(&m_pBroker);

   // create our controls
   if ( !InitGraphController(pParent,nID) )
   {
      TRACE0("Failed to create control bar\n");
      return -1; // failed to create
   }

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);

   // x axis
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& lengthUnit = pDisplayUnits->GetSpanLengthUnit();
   m_pXFormat = new LengthTool(lengthUnit);
   m_Graph.SetXAxisValueFormat(*m_pXFormat);
   m_Graph.SetXAxisNumberOfMinorTics(0);
   m_Graph.SetXAxisNiceRange(false);
   m_Graph.SetXAxisNumberOfMajorTics(11);

   // y axis
   const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.SetDoDrawGrid(); // show grid by default
   m_Graph.SetGridPenStyle(PS_DOT, 1, GRID_COLOR);

   // show the beam by default
   m_pGraphController->CheckDlgButton(IDC_BEAM,BST_CHECKED);
   m_bShowBeam = true;

   return 0;
}

void CGirderGraphBuilderBase::ShowGrid(bool bShow)
{
   m_Graph.SetDoDrawGrid( bShow );
   GetView()->Invalidate();
}

void CGirderGraphBuilderBase::ShowBeam(bool bShow)
{
   m_bShowBeam = bShow;
   GetView()->Invalidate();
}

bool CGirderGraphBuilderBase::UpdateNow()
{
   // Update graph properties
   GroupIndexType    grpIdx      = m_pGraphController->GetGirderGroup();
   GirderIndexType   gdrIdx      = m_pGraphController->GetGirder();
   IntervalIndexType intervalIdx = m_pGraphController->GetInterval();

   CSegmentKey segmentKey((grpIdx == ALL_GROUPS ? 0 : grpIdx),gdrIdx,0);

   // When graphing analysis results, X = 0 is located at the CL Bearing
   // of the first segment for which results are plotted. In this case,
   // CL Bearing means the location of the bearing point for the analysis
   // and not necessarily the CL Bearing in the bridge product model.
   // In the casting yard, the CL Bearing is at the end of the segment.
   // In storage, the CL Bearing is at the storage location.

   GET_IFACE(IBridge,pBridge);
   Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 end_dist  = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = brgOffset - end_dist;

   // m_GraphStartOffset is added to the poi coordinate, converted to Girder Path Coordiantes,
   // to get the graph coordinate.

   GET_IFACE(IIntervals,pIntervals);
   if ( intervalIdx == pIntervals->GetPrestressReleaseInterval(segmentKey) )
   {
      m_GraphStartOffset = -start_offset;
   }
   else if ( intervalIdx == pIntervals->GetStorageInterval(segmentKey) )
   {
      Float64 start_support_location, end_support_location;
      GET_IFACE(IGirderSegment,pSegment);
      pSegment->GetSegmentStorageSupportLocations(segmentKey,&start_support_location,&end_support_location);
      m_GraphStartOffset = -(start_offset + start_support_location);
   }
   else
   {
      m_GraphStartOffset = -brgOffset;
   }

   return true;
}

void CGirderGraphBuilderBase::GetXValues(const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>& xVals)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPoi);

   Float64 graphStartOffset = m_GraphStartOffset;

   CSegmentKey segmentKey(vPoi.front().GetSegmentKey());

   xVals.clear();
   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator e(vPoi.end());
   for ( ; i != e; i++ )
   {
      const pgsPointOfInterest& poi = *i;
      if ( segmentKey.groupIndex != poi.GetSegmentKey().groupIndex )
      {
         // group is changing
         ATLASSERT(segmentKey.groupIndex == poi.GetSegmentKey().groupIndex-1); // better be next group
         graphStartOffset += pBridge->GetGirderLayoutLength(segmentKey); // segment key can double as a girder key
         segmentKey = poi.GetSegmentKey();
      }

      Float64 Xgp = pPoi->ConvertPoiToGirderPathCoordinate(poi);
      Float64 x = graphStartOffset + Xgp;
      xVals.push_back(x);
   }
}

void CGirderGraphBuilderBase::AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<Float64>& yvals)
{
   std::vector<Float64>::const_iterator xIter, yIter;
   for ( xIter = xvals.begin(), yIter = yvals.begin(); xIter != xvals.end() && yIter != yvals.end(); xIter++, yIter++ )
   {
      AddGraphPoint(series,*xIter,*yIter);
   }
}

void CGirderGraphBuilderBase::AddGraphPoints(IndexType series, const std::vector<Float64>& xvals,const std::vector<sysSectionValue>& yvals)
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

void CGirderGraphBuilderBase::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   arvPhysicalConverter* pcx = dynamic_cast<arvPhysicalConverter*>(m_pXFormat);
   ASSERT(pcx);
   arvPhysicalConverter* pcy = dynamic_cast<arvPhysicalConverter*>(m_pYFormat);
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   x = IsZero(x) ? 0 : x;
   Float64 y = pcy->Convert(yval);
   y = IsZero(y) ? 0 : y;
   m_Graph.AddPoint(series, gpPoint2d(x,y));
}

void CGirderGraphBuilderBase::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int save = pDC->SaveDC();

   // The graph is valided and there was not an error
   // updating data.... draw the graph
   CRect rect = GetView()->GetDrawingRect();

   m_Graph.SetOutputRect(rect);
   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());
   m_Graph.DrawBackground(pDC->GetSafeHdc());

   // superimpose beam on graph
   // do it before the graph so the graph draws on top of it
   if ( m_bShowBeam && 0 < m_pGraphController->GetGraphCount() )
   {
      CGirderKey girderKey(m_pGraphController->GetGirderGroup(),m_pGraphController->GetGirder());
      IntervalIndexType intervalIdx = m_pGraphController->GetInterval();
      grlibPointMapper mapper( m_Graph.GetClientAreaPointMapper(pDC->GetSafeHdc()) );
      CDrawBeamTool drawBeam;
      drawBeam.DrawBeam(m_pBroker,pDC,m_GraphStartOffset,mapper,m_pXFormat,intervalIdx,girderKey);
   }

   m_Graph.DrawDataSeries(pDC->GetSafeHdc());

   pDC->RestoreDC(save);
}
