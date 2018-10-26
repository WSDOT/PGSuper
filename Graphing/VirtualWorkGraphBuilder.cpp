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
#include <Graphing\VirtualWorkGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "VirtualWorkGraphController.h"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>

#include <EAF\EAFGraphView.h>

#include <MFCTools\MFCTools.h>

#include <WBFLSTL.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CVirtualWorkGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CVirtualWorkGraphBuilder::CVirtualWorkGraphBuilder() :
CGirderGraphBuilderBase()
{
   SetName(_T("Virtual Work"));
}

CVirtualWorkGraphBuilder::CVirtualWorkGraphBuilder(const CVirtualWorkGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CVirtualWorkGraphBuilder::~CVirtualWorkGraphBuilder()
{
}

CGraphBuilder* CVirtualWorkGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CVirtualWorkGraphBuilder(*this);
}

int CVirtualWorkGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
{
   if ( CGirderGraphBuilderBase::InitializeGraphController(pParent,nID) < 0 )
   {
      return -1;
   }

   m_Graph.SetXAxisTitle(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
#pragma Reminder("UPDATE: need to update Y axis label based on graph type") // see analysis results graph
   m_Graph.SetYAxisTitle(_T("Moment (")+m_pYFormat->UnitTag()+_T(")"));
   m_Graph.SetPinYAxisAtZero(true);

   return 0;
}

BOOL CVirtualWorkGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);
   return m_pGraphController->Create(pParent,IDD_VIRTUALWORK_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

CGirderGraphControllerBase* CVirtualWorkGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CVirtualWorkGraphController;
}

bool CVirtualWorkGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();
   CVirtualWorkGraphBuilder::GraphType graphType = ((CVirtualWorkGraphController*)m_pGraphController)->GetGraphType();

   UpdateYAxisUnits();

   UpdateGraphTitle(gdrIdx,graphType);

   // get data to graph
   UpdateGraphData(gdrIdx,graphType);

   return true;
}

void CVirtualWorkGraphBuilder::UpdateYAxisUnits()
{
   delete m_pYFormat;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtMomentData& momentUnit = pDisplayUnits->GetMomentUnit();
   m_pYFormat = new MomentTool(momentUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   std::_tstring strYAxisTitle = _T("Moment (") + ((MomentTool*)m_pYFormat)->UnitTag() + _T(")");
   m_Graph.SetYAxisTitle(strYAxisTitle);
}

void CVirtualWorkGraphBuilder::UpdateGraphTitle(GirderIndexType gdrIdx,CVirtualWorkGraphBuilder::GraphType graphType)
{
#pragma Reminder("UPDATE: need to include location of unit load in graph title (poi location)")
   CString strGraphType(graphType == CVirtualWorkGraphBuilder::UnitForce ? _T("Unit Force") : _T("Unit Moment"));
   CString strGraphTitle;
   strGraphTitle.Format(_T("Girder %s - Moments due to %s"),LABEL_GIRDER(gdrIdx),strGraphType);
   
   m_Graph.SetTitle(std::_tstring(strGraphTitle));
}

void CVirtualWorkGraphBuilder::UpdateGraphData(GirderIndexType gdrIdx,CVirtualWorkGraphBuilder::GraphType graphType)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(ALL_GROUPS,gdrIdx,ALL_SEGMENTS);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   pgsPointOfInterest unitLoadPoi = ((CVirtualWorkGraphController*)m_pGraphController)->GetLocation();

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);

   int penWeight = GRAPH_PEN_WEIGHT;

   IndexType dataSeries = m_Graph.CreateDataSeries(_T("POI"),PS_SOLID,penWeight,RED);

   IntervalIndexType intervalIdx = ((CIntervalGirderGraphControllerBase*)m_pGraphController)->GetInterval();

   GET_IFACE(IVirtualWork,pVirtualWork);
   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   // unit load effects are based on a load of magnitude 1.0 in system units
   // we want to make the results look like they are based on a load of magnitude 1.0
   // in display units. compute a scale factor to adjust the results
   Float64 scaleFactor = 1.0;
   if ( pDisplayUnits->GetUnitMode() == eafTypes::umSI )
   {
      scaleFactor = ::ConvertToSysUnits(1.0,unitMeasure::Kilonewton);
   }
   else
   {
      scaleFactor = ::ConvertToSysUnits(1.0,unitMeasure::Kip);
   }

   pgsTypes::BridgeAnalysisType batMin = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batMax = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   if ( graphType == CVirtualWorkGraphBuilder::UnitForce )
   {
      if ( batMin == batMax )
      {
         std::vector<Float64> values( pVirtualWork->GetUnitLoadMoment(vPoi,unitLoadPoi,batMax,intervalIdx) );
         std::transform(values.begin(),values.end(),values.begin(),FactorElements<Float64>(scaleFactor));
         AddGraphPoints(dataSeries,xVals,values);
      }
      else
      {
         std::vector<Float64> minValues( pVirtualWork->GetUnitLoadMoment(vPoi,unitLoadPoi,batMin,intervalIdx) );
         std::transform(minValues.begin(),minValues.end(),minValues.begin(),FactorElements<Float64>(scaleFactor));
         AddGraphPoints(dataSeries,xVals,minValues);

         std::vector<Float64> maxValues( pVirtualWork->GetUnitLoadMoment(vPoi,unitLoadPoi,batMax,intervalIdx) );
         std::transform(maxValues.begin(),maxValues.end(),maxValues.begin(),FactorElements<Float64>(scaleFactor));
         AddGraphPoints(dataSeries,xVals,maxValues);
      }
   }
   else
   {
      if ( batMin == batMax )
      {
         std::vector<sysSectionValue> values( pVirtualWork->GetUnitCoupleMoment(vPoi,unitLoadPoi,batMax,intervalIdx) );
         std::transform(values.begin(),values.end(),values.begin(),FactorElements<sysSectionValue>(scaleFactor));
         AddGraphPoints(dataSeries,xVals,values);
      }
      else
      {
         std::vector<sysSectionValue> minValues( pVirtualWork->GetUnitCoupleMoment(vPoi,unitLoadPoi,batMin,intervalIdx) );
         std::transform(minValues.begin(),minValues.end(),minValues.begin(),FactorElements<sysSectionValue>(scaleFactor));
         AddGraphPoints(dataSeries,xVals,minValues);

         std::vector<sysSectionValue> maxValues( pVirtualWork->GetUnitCoupleMoment(vPoi,unitLoadPoi,batMax,intervalIdx) );
         std::transform(maxValues.begin(),maxValues.end(),maxValues.begin(),FactorElements<sysSectionValue>(scaleFactor));
         AddGraphPoints(dataSeries,xVals,maxValues);
      }
   }

}

IntervalIndexType CVirtualWorkGraphBuilder::GetBeamDrawInterval()
{
   return ((CVirtualWorkGraphController*)m_pGraphController)->GetInterval();
}
