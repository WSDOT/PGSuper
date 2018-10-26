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
#include <Graphing\ConcretePropertyGraphBuilder.h>
#include "ConcretePropertyGraphController.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <PgsExt\ClosureJointData.h>

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

BEGIN_MESSAGE_MAP(CConcretePropertyGraphBuilder, CEAFGraphBuilderBase)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
END_MESSAGE_MAP()


CConcretePropertyGraphBuilder::CConcretePropertyGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_XAxisType(X_AXIS_TIME_LOG)
{
   m_pGraphController = new CConcretePropertyGraphController;

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;

   SetName(_T("Concrete Properties"));
}

CConcretePropertyGraphBuilder::CConcretePropertyGraphBuilder(const CConcretePropertyGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_XAxisType(X_AXIS_TIME_LOG)
{
   m_pGraphController = new CConcretePropertyGraphController;

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CConcretePropertyGraphBuilder::~CConcretePropertyGraphBuilder()
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

CEAFGraphControlWindow* CConcretePropertyGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

CGraphBuilder* CConcretePropertyGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CConcretePropertyGraphBuilder(*this);
}

int CConcretePropertyGraphBuilder::CreateControls(CWnd* pParent,UINT nID)
{
   // create the graph definitions before creating the graph controller.
   // our graph controller will call GetLoadCaseNames to populate the 
   // list of load cases
   EAFGetBroker(&m_pBroker);

   // let the base class do its thing
   CEAFAutoCalcGraphBuilder::CreateControls(pParent,nID);

   // create our controls
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_pGraphController->Create(pParent,IDD_CONCRETE_PROPERTY_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return -1; // failed to create
   }

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);

   m_Graph.SetTitle(_T("Concrete Properties"));

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   // x axis
   m_pTimeFormat = new ScalarTool(m_Scalar);
   m_pIntervalFormat = new ScalarTool(m_Scalar);
   m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   m_Graph.SetXAxisNumberOfMajorTics(11);
   m_pGraphController->CheckDlgButton(IDC_AGE_LOG,BST_CHECKED);
   m_XAxisType = X_AXIS_AGE_LOG;

   // y axis
   const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // Default Graph Data
   m_pGraphController->CheckDlgButton(IDC_PRECAST_SEGMENT,BST_CHECKED);
   m_pGraphController->CheckDlgButton(IDC_FC,BST_CHECKED);
   m_GraphElement = GRAPH_ELEMENT_SEGMENT;
   m_GraphType    = GRAPH_TYPE_FC;
   m_SegmentKey = CSegmentKey(0,0,0);
   m_ClosureKey = CClosureKey(0,0,0);

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.SetDoDrawGrid(); // show grid by default
   m_Graph.SetGridPenStyle(PS_DOT, 1, GRID_COLOR);

   return 0;
}

void CConcretePropertyGraphBuilder::OnShowGrid()
{
   m_Graph.SetDoDrawGrid( !m_Graph.GetDoDrawGrid() );
   GetView()->Invalidate();
}

bool CConcretePropertyGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   // Update the graph control data
   m_GraphElement = m_pGraphController->GetGraphElement();
   m_GraphType    = m_pGraphController->GetGraphType();
   m_SegmentKey   = m_pGraphController->GetSegmentKey();
   m_ClosureKey   = m_pGraphController->GetClosureKey();
   m_XAxisType    = m_pGraphController->GetXAxisType();

   // Update graph properties
   UpdateXAxis();
   UpdateYAxis();
   UpdateGraphTitle();
   UpdateGraphData();
   return true;
}

void CConcretePropertyGraphBuilder::UpdateXAxis()
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
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_AGE_LINEAR )
   {
      m_Graph.SetXAxisScale(grAxisXY::LINEAR);
      m_Graph.SetXAxisTitle(_T("Age (days)"));
      m_Graph.SetXAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_AGE_LOG )
   {
      m_Graph.SetXAxisScale(grAxisXY::LOGARITHMIC);
      m_Graph.SetXAxisTitle(_T("Age (days)"));
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

void CConcretePropertyGraphBuilder::UpdateYAxis()
{
   delete m_pYFormat;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(m_GraphType)
   {
   case GRAPH_TYPE_FC:
      {
      const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("f'c (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   case GRAPH_TYPE_EC:
      {
      const unitmgtStressData& stressUnit = pDisplayUnits->GetModEUnit();
      m_pYFormat = new StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Ec (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   default:
      ASSERT(0); 
   }
}

void CConcretePropertyGraphBuilder::UpdateGraphTitle()
{
   CString strType(m_GraphType == GRAPH_TYPE_FC ? _T("Concrete Strength") : _T("Elastic Modulus"));
   CString strElement;
   if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
   {
      strElement.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(m_SegmentKey.groupIndex),LABEL_GIRDER(m_SegmentKey.girderIndex),LABEL_SEGMENT(m_SegmentKey.segmentIndex));
   }
   else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(m_ClosureKey);
      CString strLabel;
      if ( pClosure->GetTemporarySupport() )
         strLabel = GetLabel(pClosure->GetTemporarySupport(),pDisplayUnits);
      else
         strLabel = GetLabel(pClosure->GetPier(),pDisplayUnits);


      strElement.Format(_T("Closure Joint at %s"),strLabel);
   }
   else
   {
      ATLASSERT(m_GraphElement == GRAPH_ELEMENT_DECK);
      strElement = _T("Bridge Deck");
   }

   CString strGraphTitle;
   strGraphTitle.Format(_T("%s for %s"),strType,strElement);
   m_Graph.SetTitle(std::_tstring(strGraphTitle));
}

void CConcretePropertyGraphBuilder::UpdateGraphData()
{
   // clear graph
   m_Graph.ClearData();

   CString strLabel = (m_GraphType == GRAPH_TYPE_FC ? _T("f'c") : _T("Ec"));
   IndexType dataSeries = m_Graph.CreateDataSeries(strLabel, PS_SOLID,1,ORANGE);

   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType startIntervalIdx = 0;
   if ( m_XAxisType == X_AXIS_AGE_LINEAR  || m_XAxisType == X_AXIS_AGE_LOG )
   {
      if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
         startIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
      else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
         startIntervalIdx = pIntervals->GetCompositeClosureJointInterval(m_ClosureKey);
      else
         startIntervalIdx = pIntervals->GetCompositeDeckInterval();
   }

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 xMiddle;
      if ( m_XAxisType == X_AXIS_TIME_LINEAR || m_XAxisType == X_AXIS_TIME_LOG )
      {
         xMiddle = pIntervals->GetMiddle(intervalIdx);
      }
      else if ( m_XAxisType == X_AXIS_AGE_LINEAR || m_XAxisType == X_AXIS_AGE_LOG )
      {
         if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
            xMiddle = pMaterials->GetSegmentConcreteAge(m_SegmentKey,intervalIdx);
         else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
            xMiddle = pMaterials->GetClosureJointConcreteAge(m_ClosureKey,intervalIdx);
         else
            xMiddle = pMaterials->GetDeckConcreteAge(intervalIdx);
      }
      else
      {
         xMiddle = (Float64)LABEL_INTERVAL(intervalIdx);
      }

      // this is value at middle of interval...
      Float64 value;
      if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
      {
         if ( m_GraphType == GRAPH_TYPE_FC )
         {
            value = pMaterials->GetSegmentFc(m_SegmentKey,intervalIdx);
         }
         else
         {
            value = pMaterials->GetSegmentEc(m_SegmentKey,intervalIdx);
         }
      }
      else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
      {
         if ( m_GraphType == GRAPH_TYPE_FC )
         {
            value = pMaterials->GetClosureJointFc(m_ClosureKey,intervalIdx);
         }
         else
         {
            value = pMaterials->GetClosureJointEc(m_ClosureKey,intervalIdx);
         }
      }
      else if ( m_GraphElement = GRAPH_ELEMENT_DECK )
      {
         if ( m_GraphType == GRAPH_TYPE_FC )
         {
            value = pMaterials->GetDeckFc(intervalIdx);
         }
         else
         {
            value = pMaterials->GetDeckEc(intervalIdx);
         }
      }
      AddGraphPoint(dataSeries,xMiddle,value);
   }
}

void CConcretePropertyGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
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

void CConcretePropertyGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
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
