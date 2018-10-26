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
#include <Graphing\InitialStrainGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "InitialStrainGraphController.h"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PrestressForce.h>

#include <EAF\EAFGraphView.h>

#include <MFCTools\MFCTools.h>

#include <WBFLSTL.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CInitialStrainGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CInitialStrainGraphBuilder::CInitialStrainGraphBuilder() :
CGirderGraphBuilderBase()
{
   SetName(_T("Initial Strain"));
}

CInitialStrainGraphBuilder::CInitialStrainGraphBuilder(const CInitialStrainGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CInitialStrainGraphBuilder::~CInitialStrainGraphBuilder()
{
}

CGraphBuilder* CInitialStrainGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CInitialStrainGraphBuilder(*this);
}

int CInitialStrainGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
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

BOOL CInitialStrainGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);
   return m_pGraphController->Create(pParent,IDD_INITIALSTRAIN_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

CGirderGraphControllerBase* CInitialStrainGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CInitialStrainGraphController;
}

bool CInitialStrainGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();
   CInitialStrainGraphBuilder::GraphType graphType = ((CInitialStrainGraphController*)m_pGraphController)->GetGraphType();

   UpdateYAxisUnits(graphType);

   UpdateGraphTitle(gdrIdx,graphType);

   // get data to graph
   UpdateGraphData(gdrIdx,graphType);

   return true;
}

void CInitialStrainGraphBuilder::UpdateYAxisUnits(CInitialStrainGraphBuilder::GraphType graphType)
{
   delete m_pYFormat;

   std::_tstring strYAxisTitle;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   switch(graphType)
   {
   case CInitialStrainGraphBuilder::InitialStrain:
      {
      const unitmgtScalar& scalar = pDisplayUnits->GetScalarFormat();
      m_pYFormat = new ScalarTool(scalar);
      strYAxisTitle = _T("Initial Strain (x10^6)");
      }
      break;

   case CInitialStrainGraphBuilder::InitialCurvature:
      {
      const unitmgtPerLengthData& curvatureUnit = pDisplayUnits->GetCurvatureUnit();
      m_pYFormat = new CurvatureTool(curvatureUnit);
      strYAxisTitle = _T("Initial Curvature (x10^6) (") + ((CurvatureTool*)m_pYFormat)->UnitTag() + _T(")");
      }
      break;

   case CInitialStrainGraphBuilder::RestrainingForce:
      {
      const unitmgtForceData& forceUnit = pDisplayUnits->GetGeneralForceUnit();
      m_pYFormat = new ForceTool(forceUnit);
      strYAxisTitle = _T("Restraining Force (") + ((ForceTool*)m_pYFormat)->UnitTag() + _T(")");
      }
      break;

   case CInitialStrainGraphBuilder::RestrainingMoment:
      {
      const unitmgtMomentData& momentUnit = pDisplayUnits->GetMomentUnit();
      m_pYFormat = new MomentTool(momentUnit);
      strYAxisTitle = _T("Restraining Moment (") + ((MomentTool*)m_pYFormat)->UnitTag() + _T(")");
      }
      break;
   }

   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisTitle(strYAxisTitle);
}

void CInitialStrainGraphBuilder::UpdateGraphTitle(GirderIndexType gdrIdx,CInitialStrainGraphBuilder::GraphType graphType)
{
   CString strGraphType;
   switch(graphType)
   {
   case CInitialStrainGraphBuilder::InitialStrain:
      strGraphType = _T("Initial Strain");
      break;

   case CInitialStrainGraphBuilder::InitialCurvature:
      strGraphType = _T("Initial Curvature");
      break;

   case CInitialStrainGraphBuilder::RestrainingForce:
      strGraphType = _T("Restraining Force");
      break;

   case CInitialStrainGraphBuilder::RestrainingMoment:
      strGraphType = _T("Restraining Moment");
      break;
   }

   CString strGraphTitle;
   strGraphTitle.Format(_T("Girder %s - %s"),LABEL_GIRDER(gdrIdx),strGraphType);
   
   m_Graph.SetTitle(std::_tstring(strGraphTitle));
}

void CInitialStrainGraphBuilder::UpdateGraphData(GirderIndexType gdrIdx,CInitialStrainGraphBuilder::GraphType graphType)
{
   // clear graph
   m_Graph.ClearData();

   bool bCreep      = ((CInitialStrainGraphController*)m_pGraphController)->Creep();
   bool bShrinkage  = ((CInitialStrainGraphController*)m_pGraphController)->Shrinkage();
   bool bRelaxation = ((CInitialStrainGraphController*)m_pGraphController)->Relaxation();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(ALL_GROUPS,gdrIdx,ALL_SEGMENTS);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);

   int penWeight = GRAPH_PEN_WEIGHT;

   IndexType crDataSeries = m_Graph.CreateDataSeries(_T("Creep (R)"),     PS_SOLID,penWeight,RED);
   IndexType shDataSeries = m_Graph.CreateDataSeries(_T("Shrinkage (R)"), PS_SOLID,penWeight,BLUE);
   IndexType reDataSeries = m_Graph.CreateDataSeries(_T("Relaxation (R)"),PS_SOLID,penWeight,GREEN);

   IndexType crDataSeries2 = m_Graph.CreateDataSeries(_T("Creep"),     PS_DASHDOTDOT,penWeight,ORANGERED);
   IndexType shDataSeries2 = m_Graph.CreateDataSeries(_T("Shrinkage"), PS_DASHDOTDOT,penWeight,DODGERBLUE);
   IndexType reDataSeries2 = m_Graph.CreateDataSeries(_T("Relaxation"),PS_DASHDOTDOT,penWeight,DARKGREEN);

   IndexType crGirderDataSeries = m_Graph.CreateDataSeries(_T("CR - Girder"), PS_DASH,penWeight,RED);
   IndexType shGirderDataSeries = m_Graph.CreateDataSeries(_T("SH - Girder"), PS_DASH,penWeight,BLUE);

   IndexType crDeckDataSeries = m_Graph.CreateDataSeries(_T("CR - Deck"), PS_DOT,penWeight,RED);
   IndexType shDeckDataSeries = m_Graph.CreateDataSeries(_T("SH - Deck"), PS_DOT,penWeight,BLUE);

   IndexType reStraightDataSeries  = m_Graph.CreateDataSeries(_T("RE - Straight Strands"),   PS_SOLID,penWeight,RED);
   IndexType reHarpedDataSeries    = m_Graph.CreateDataSeries(_T("RE - Harped Strands"),     PS_SOLID,penWeight,BLUE);
   IndexType reTemporaryDataSeries = m_Graph.CreateDataSeries(_T("RE - Temporary Strands"),  PS_SOLID,penWeight,GREEN);

   IntervalIndexType intervalIdx = ((CIntervalGirderGraphControllerBase*)m_pGraphController)->GetInterval();

   GET_IFACE(ILosses,pLosses);
   GET_IFACE(IProductForces,pProductForces);

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   std::vector<Float64>::iterator xIter(xVals.begin());
   for ( ; poiIter != poiIterEnd; poiIter++, xIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 X = *xIter;

      const LOSSDETAILS* pLossDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

      if ( graphType == CInitialStrainGraphBuilder::InitialStrain )
      {
         Float64 e;
         if ( bCreep )
         {
            e = tsDetails.Girder.eci;
            AddGraphPoint(crGirderDataSeries,X,1e6*e);

            e = tsDetails.Deck.eci;
            AddGraphPoint(crDeckDataSeries,X,1e6*e);
         }

         if ( bShrinkage )
         {
            e = tsDetails.Girder.esi;
            AddGraphPoint(shGirderDataSeries,X,1e6*e);

            e = tsDetails.Deck.esi;
            AddGraphPoint(shDeckDataSeries,X,1e6*e);
         }

         if ( bRelaxation )
         {
            e = tsDetails.Strands[pgsTypes::Straight].er;
            AddGraphPoint(reStraightDataSeries,X,1e6*e);

            e = tsDetails.Strands[pgsTypes::Harped].er;
            AddGraphPoint(reHarpedDataSeries,X,1e6*e);

            e = tsDetails.Strands[pgsTypes::Temporary].er;
            AddGraphPoint(reTemporaryDataSeries,X,1e6*e);
         }
      }
      else if ( graphType == CInitialStrainGraphBuilder::InitialCurvature )
      {
         Float64 r;
         if ( bCreep )
         {
            r = tsDetails.Girder.rci;
            AddGraphPoint(crGirderDataSeries,X,1e6*r);

            r = tsDetails.Deck.rci;
            AddGraphPoint(crDeckDataSeries,X,1e6*r);
         }

         if ( bShrinkage )
         {
         }

         if ( bRelaxation )
         {
         }
      }
      else if ( graphType == CInitialStrainGraphBuilder::RestrainingForce )
      {
         Float64 P;

         if ( bCreep )
         {
            P = tsDetails.Pr[TIMESTEP_CR];
            AddGraphPoint(crDataSeries,X,P);

            P = pProductForces->GetAxial(intervalIdx,pftCreep,poi,pgsTypes::ContinuousSpan,rtIncremental);
            AddGraphPoint(crDataSeries2,X,P);

            P = tsDetails.Girder.PrCreep;
            AddGraphPoint(crGirderDataSeries,X,P);

            P = tsDetails.Deck.PrCreep;
            AddGraphPoint(crDeckDataSeries,X,P);
         }

         if ( bShrinkage )
         {
            P = tsDetails.Pr[TIMESTEP_SH];
            AddGraphPoint(shDataSeries,X,P);

            P = pProductForces->GetAxial(intervalIdx,pftShrinkage,poi,pgsTypes::ContinuousSpan,rtIncremental);
            AddGraphPoint(shDataSeries2,X,P);

            P = tsDetails.Girder.PrShrinkage;
            AddGraphPoint(shGirderDataSeries,X,P);

            P = tsDetails.Deck.PrShrinkage;
            AddGraphPoint(shDeckDataSeries,X,P);
         }

         if ( bRelaxation )
         {
            P = tsDetails.Pr[TIMESTEP_RE];
            AddGraphPoint(reDataSeries,X,P);

            P = pProductForces->GetAxial(intervalIdx,pftRelaxation,poi,pgsTypes::ContinuousSpan,rtIncremental);
            AddGraphPoint(reDataSeries2,X,P);

            P = tsDetails.Strands[pgsTypes::Straight].PrRelaxation;
            AddGraphPoint(reStraightDataSeries,X,P);

            P = tsDetails.Strands[pgsTypes::Harped].PrRelaxation;
            AddGraphPoint(reHarpedDataSeries,X,P);

            P = tsDetails.Strands[pgsTypes::Temporary].PrRelaxation;
            AddGraphPoint(reTemporaryDataSeries,X,P);
         }
      }
      else if ( graphType == CInitialStrainGraphBuilder::RestrainingMoment )
      {
         Float64 M;
         
         if ( bCreep )
         {
            M = tsDetails.Mr[TIMESTEP_CR];
            AddGraphPoint(crDataSeries,X,M);

            M = pProductForces->GetMoment(intervalIdx,pftCreep,poi,pgsTypes::ContinuousSpan,rtIncremental);
            AddGraphPoint(crDataSeries2,X,M);

            M = tsDetails.Girder.MrCreep + tsDetails.Girder.PrCreep*(tsDetails.Ytr - tsDetails.Girder.Yn);
            AddGraphPoint(crGirderDataSeries,X,M);

            M = tsDetails.Deck.MrCreep + tsDetails.Deck.PrCreep*(tsDetails.Ytr - tsDetails.Deck.Yn);
            AddGraphPoint(crDeckDataSeries,X,M);
         }

         if ( bShrinkage )
         {
            M = tsDetails.Mr[TIMESTEP_SH];
            AddGraphPoint(shDataSeries,X,M);

            M = pProductForces->GetMoment(intervalIdx,pftShrinkage,poi,pgsTypes::ContinuousSpan,rtIncremental);
            AddGraphPoint(shDataSeries2,X,M);

            M = tsDetails.Girder.PrShrinkage*(tsDetails.Ytr - tsDetails.Girder.Yn);
            AddGraphPoint(shGirderDataSeries,X,M);

            M = tsDetails.Deck.PrShrinkage*(tsDetails.Ytr - tsDetails.Deck.Yn);
            AddGraphPoint(shDeckDataSeries,X,M);
         }

         if ( bRelaxation )
         {
            M = tsDetails.Mr[TIMESTEP_RE];
            AddGraphPoint(reDataSeries,X,M);

            M = pProductForces->GetMoment(intervalIdx,pftRelaxation,poi,pgsTypes::ContinuousSpan,rtIncremental);
            AddGraphPoint(reDataSeries2,X,M);

            M = tsDetails.Strands[pgsTypes::Straight].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Straight].Ys);
            AddGraphPoint(reStraightDataSeries,X,M);

            M = tsDetails.Strands[pgsTypes::Harped].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Harped].Ys);
            AddGraphPoint(reHarpedDataSeries,X,M);

            M = tsDetails.Strands[pgsTypes::Temporary].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Temporary].Ys);
            AddGraphPoint(reTemporaryDataSeries,X,M);
         }
      }
   }
}

IntervalIndexType CInitialStrainGraphBuilder::GetBeamDrawInterval()
{
   return ((CInitialStrainGraphController*)m_pGraphController)->GetInterval();
}
