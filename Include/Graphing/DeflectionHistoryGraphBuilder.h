///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

class CDeflectionHistoryGraphController;
class arvPhysicalConverter;
interface IIntervals;
interface ILimitStateForces;

class GRAPHINGCLASS CDeflectionHistoryGraphBuilder : public CEAFAutoCalcGraphBuilder
{
public:
   CDeflectionHistoryGraphBuilder();
   CDeflectionHistoryGraphBuilder(const CDeflectionHistoryGraphBuilder& other);
   virtual ~CDeflectionHistoryGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID) override;
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;
   virtual void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC) override;
   virtual CGraphBuilder* Clone() const override;

   virtual CEAFGraphControlWindow* GetGraphControlWindow() override;

protected:
   void Init();

   CDeflectionHistoryGraphController* m_pGraphController;

   afx_msg void OnShowGrid();

   DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   unitmgtScalar m_Time;
   unitmgtScalar m_Interval;
   arvPhysicalConverter* m_pTimeFormat;
   arvPhysicalConverter* m_pIntervalFormat;
   arvPhysicalConverter* m_pYFormat;
   grGraphXY m_Graph;

   int m_XAxisType;

   virtual bool UpdateNow() override;

   void UpdateXAxis();
   void UpdateYAxis();
   void UpdateGraphTitle(const pgsPointOfInterest& poi);
   void UpdateGraphData(const pgsPointOfInterest& poi);
   Float64 GetX(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IIntervals* pIntervals);
   void PlotDeflection(Float64 x,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,IndexType dataSeries,pgsTypes::BridgeAnalysisType bat,ILimitStateForces* pLimitStateForces);
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
};
