///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#pragma once

#include <Graphing\GraphingExp.h>
#include <EAF\EAFAutoCalcGraphBuilder.h>
#include <GraphicsLib\GraphXY.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PointOfInterest.h>

class CStressHistoryGraphController;
class arvPhysicalConverter;
interface ILimitStateForces;
interface ICombinedForces;
interface IIntervals;

class GRAPHINGCLASS CStressHistoryGraphBuilder : public CEAFAutoCalcGraphBuilder
{
public:
   CStressHistoryGraphBuilder();
   CStressHistoryGraphBuilder(const CStressHistoryGraphBuilder& other);
   virtual ~CStressHistoryGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID) override;
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;
   virtual void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC) override;
   virtual CGraphBuilder* Clone() const override;

   virtual CEAFGraphControlWindow* GetGraphControlWindow() override;

protected:
   void Init();

   CStressHistoryGraphController* m_pGraphController;

   afx_msg void OnShowGrid();
   afx_msg void OnTopDeck();
   afx_msg void OnBottomDeck();
   afx_msg void OnTopGirder();
   afx_msg void OnBottomGirder();

   DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   unitmgtScalar m_Time;
   unitmgtScalar m_Interval;
   arvPhysicalConverter* m_pTimeFormat;
   arvPhysicalConverter* m_pIntervalFormat;
   arvPhysicalConverter* m_pYFormat;
   grGraphXY m_Graph;

   bool m_bPlot[4];

   int m_XAxisType;

   virtual bool UpdateNow() override;

   pgsTypes::LimitState GetLimitState();
   void UpdateXAxis();
   void UpdateYAxis();
   void UpdateGraphTitle(const pgsPointOfInterest& poi);
   void UpdateGraphData(const pgsPointOfInterest& poi);
   Float64 GetX(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IIntervals* pIntervals);
   void PlotStressPoints(Float64 x,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IndexType dataSeries,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64 gLL,IntervalIndexType liveLoadIntervalIdx,ICombinedForces* pCombinedForces,ILimitStateForces* pLimitStateForces);
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
};
