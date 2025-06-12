///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

class WBFL::Units::PhysicalConverter;
class CStabilityGraphController;

class GRAPHCLASS CStabilityGraphBuilder : public CEAFAutoCalcGraphBuilder
{
public:
   CStabilityGraphBuilder();
   CStabilityGraphBuilder(const CStabilityGraphBuilder& other);
   virtual ~CStabilityGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID) override;
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;
   virtual void DrawGraphNow(CWnd* pGraphWnd,CDC* pDC) override;
   virtual std::unique_ptr<WBFL::Graphing::GraphBuilder> Clone() const override;

   virtual CEAFGraphControlWindow* GetGraphControlWindow() override;

   virtual void CreateViewController(IEAFViewController** ppController) override;

   // sets the grid state and redraws the graph. This is more efficient
   // than updating the entire graph
   void ShowGrid(bool bShowGrid);

   void ExportGraphData(LPCTSTR rstrDefaultFileName);
protected:

   CStabilityGraphController* m_pGraphController;

   DECLARE_MESSAGE_MAP()

   std::shared_ptr<WBFL::EAF::Broker> m_pBroker;

   WBFL::Units::PhysicalConverter* m_pXFormat;
   WBFL::Units::PhysicalConverter* m_pYFormat;
   WBFL::Graphing::GraphXY m_Graph;

   std::_tstring m_PrintSubtitle;


   void UpdateXAxis();

   virtual bool UpdateNow() override;
   void AddGraphPoint(IndexType series, Float64 xval, Float64 yval);

   void DrawTheGraph(CWnd* pGraphWnd,CDC* pDC);
};
