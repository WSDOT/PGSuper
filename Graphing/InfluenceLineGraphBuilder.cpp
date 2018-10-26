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
#include <Graphing\InfluenceLineGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "InfluencelineGraphController.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>

#include <EAF\EAFGraphView.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CInfluenceLineGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CInfluenceLineGraphBuilder::CInfluenceLineGraphBuilder() :
CGirderGraphBuilderBase()
{
   SetName(_T("Influence Lines"));
}

CInfluenceLineGraphBuilder::CInfluenceLineGraphBuilder(const CInfluenceLineGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CInfluenceLineGraphBuilder::~CInfluenceLineGraphBuilder()
{
}

CGraphBuilder* CInfluenceLineGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CInfluenceLineGraphBuilder(*this);
}

int CInfluenceLineGraphBuilder::CreateControls(CWnd* pParent,UINT nID)
{
   CGirderGraphBuilderBase::CreateControls(pParent,nID);

   m_Graph.SetXAxisTitle(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
#pragma Reminder("UPDATE: need to update Y axis label based on graph type") // see analysis results graph
   m_Graph.SetYAxisTitle(_T("Moment (")+m_pYFormat->UnitTag()+_T(")"));
   m_Graph.SetPinYAxisAtZero(true);

   return 0;
}

CGirderGraphControllerBase* CInfluenceLineGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CInfluenceLineGraphController;
}

BOOL CInfluenceLineGraphBuilder::InitGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);
   return m_pGraphController->Create(pParent,IDD_INFLUENCELINE_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

bool CInfluenceLineGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();

   CInfluenceLineGraphBuilder::GraphType graphType = ((CInfluenceLineGraphController*)m_pGraphController)->GetGraphType();

   UpdateYAxisUnits(graphType);

   UpdateGraphTitle(gdrIdx,graphType);

   // get data to graph
   UpdateGraphData(gdrIdx,graphType);

   return true;
}

void CInfluenceLineGraphBuilder::UpdateYAxisUnits(CInfluenceLineGraphBuilder::GraphType graphType)
{
   delete m_pYFormat;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(graphType)
   {
   case CInfluenceLineGraphBuilder::Moment:
      {
      const unitmgtMomentData& momentUnit = pDisplayUnits->GetMomentUnit();
      m_pYFormat = new MomentTool(momentUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Moment (") + ((MomentTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   default:
      ASSERT(0); 
   }
}

void CInfluenceLineGraphBuilder::UpdateGraphTitle(GirderIndexType gdrIdx,CInfluenceLineGraphBuilder::GraphType graphType)
{
#pragma Reminder("UPDATE: need to include influence line type in graph title")
   CString strGraphTitle;
   strGraphTitle.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
   
   m_Graph.SetTitle(std::_tstring(strGraphTitle));
}

void CInfluenceLineGraphBuilder::UpdateGraphData(GirderIndexType gdrIdx,CInfluenceLineGraphBuilder::GraphType graphType)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(ALL_GROUPS,gdrIdx,ALL_SEGMENTS);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

#pragma Reminder("UPDATE: need correct bridge analysis type for influence lines... using dummy value here")
   pgsTypes::BridgeAnalysisType bat = pgsTypes::ContinuousSpan;

#pragma Reminder("UPDATE: need to get POIs for which to create influence lines")
   pgsPointOfInterest unitLoadPOI(vPoi[vPoi.size()/2]); // this is just a dummy value for testing/development

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,xVals);

#pragma Reminder("IMPLEMENT")
   // Need a data series for each POI we are plotting influence lines for
   IndexType dataSeries = m_Graph.CreateDataSeries(_T("POI"),PS_SOLID,1,RED);

   IntervalIndexType intervalIdx = m_pGraphController->GetInterval();

   GET_IFACE(IInfluenceResults,pInfluenceResults);
   std::vector<Float64> inflValues( pInfluenceResults->GetUnitLoadMoment(vPoi,unitLoadPOI,bat,intervalIdx) );

   AddGraphPoints(dataSeries,xVals,inflValues);
}
