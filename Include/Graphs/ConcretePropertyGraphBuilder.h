///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include <Graphs/GraphsExp.h>
#include <EAF\EAFAutoCalcGraphBuilder.h>
#include <Graphing/GraphXY.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PointOfInterest.h>


// Graph Types
#define GRAPH_TYPE_FC   0  // Plot concrete strength
#define GRAPH_TYPE_EC   1  // Plot modulus of elasticity
#define GRAPH_TYPE_SH   2  // Plot of shrinkage strain
#define GRAPH_TYPE_CR   3  // Plot of creep coefficients

// Graph Elements
#define GRAPH_ELEMENT_SEGMENT 0 // plot properties for a segment
#define GRAPH_ELEMENT_CLOSURE 1 // plot properties for a clousre joint
#define GRAPH_ELEMENT_DECK    2 // plot properties for the bridge deck

// X-Axis Display Type
#define X_AXIS_TIME_LINEAR  0
#define X_AXIS_TIME_LOG     1
#define X_AXIS_AGE_LINEAR   2
#define X_AXIS_AGE_LOG      3
#define X_AXIS_INTERVAL     4


class CConcretePropertyGraphController;
class WBFL::Units::PhysicalConverter;

class GRAPHCLASS CConcretePropertyGraphBuilder : public CEAFAutoCalcGraphBuilder
{
public:
   CConcretePropertyGraphBuilder();
   CConcretePropertyGraphBuilder(const CConcretePropertyGraphBuilder& other);
   virtual ~CConcretePropertyGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID) override;
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;
   virtual void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC) override;
   virtual std::unique_ptr<WBFL::Graphing::GraphBuilder> Clone() const override;

   virtual CEAFGraphControlWindow* GetGraphControlWindow() override;

   virtual void CreateViewController(IEAFViewController** ppController) override;

   void ShowGrid(bool bShow);

   void ExportGraphData(LPCTSTR rstrDefaultFileName);
protected:
   void Init();
   CConcretePropertyGraphController* m_pGraphController;

   DECLARE_MESSAGE_MAP()

   CComPtr<IBroker> m_pBroker;

   WBFL::Units::ScalarData m_Time;
   WBFL::Units::ScalarData m_Interval;
   WBFL::Units::ScalarData m_StrainScalar;
   WBFL::Units::ScalarData m_CreepScalar;
   WBFL::Units::PhysicalConverter* m_pTimeFormat;
   WBFL::Units::PhysicalConverter* m_pIntervalFormat;
   WBFL::Units::PhysicalConverter* m_pYFormat;
   WBFL::Graphing::GraphXY m_Graph;

   int m_XAxisType;

   int m_GraphType;
   int m_GraphElement;
   CSegmentKey m_SegmentKey;
   CClosureKey m_ClosureKey;
   IndexType m_DeckCastingRegionIdx;

   virtual bool UpdateNow() override;

   void UpdateXAxis();
   void UpdateYAxis();
   void UpdateGraphTitle();
   void UpdateGraphData();
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);
};
