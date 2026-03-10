///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

#include "stdafx.h"
#include "ExtensionAgent.h"

#include "resource.h"
#include "TestGraphBuilder.h"

#include <EAF\EAFGraphChildFrame.h>
#include <EAF\EAFGraphView.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <MathEx.h>
#include <Units\UnitValueNumericalFormatTools.h>

#include <Colors.h>

#include <Graphing/GraphXY.h>


BEGIN_MESSAGE_MAP(CTestGraphBuilder, CEAFGraphBuilderBase)
   ON_BN_CLICKED(IDC_SINE, &CTestGraphBuilder::OnGraphTypeChanged)
   ON_BN_CLICKED(IDC_COSINE, &CTestGraphBuilder::OnGraphTypeChanged)
END_MESSAGE_MAP()


CTestGraphBuilder::CTestGraphBuilder()
{
   SetName(_T("Test Graph Builder"));
}

CTestGraphBuilder::CTestGraphBuilder(const CTestGraphBuilder& other) :
CEAFGraphBuilderBase(other)
{
}

CEAFGraphControlWindow* CTestGraphBuilder::GetGraphControlWindow()
{
   return &m_GraphControls;
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CTestGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CTestGraphBuilder>(*this);
}

void CTestGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   *ppController = nullptr; // not supplying a graph controller
}

BOOL CTestGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_GraphControls.Create(pParent,IDD_TEST_GRAPH_CONTROLS, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CTestGraphBuilder::OnGraphTypeChanged()
{
   CEAFGraphView* pGraphView = m_pFrame->GetGraphView();
   pGraphView->Invalidate();
   pGraphView->UpdateWindow();
}

void CTestGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   int graphType = m_GraphControls.GetGraphType();

   // first x axis
   const WBFL::Units::ScalarData& scalar = pDisplayUnits->GetScalarFormat();
   WBFL::Units::ScalarTool scalar_tool(scalar);
   WBFL::Graphing::GraphXY graph(&scalar_tool,&scalar_tool);

   IndexType idx = graph.CreateDataSeries();
   for ( int i = 0; i <= 360; i++ )
   {
      Float64 angle = ::ToRadians((Float64)(i));
      Float64 y;
      if ( graphType == SINE_GRAPH )
         y = sin(angle);
      else
         y = cos(angle);

      WBFL::Graphing::Point point(angle,y);
      graph.AddPoint(idx,point);
   }


   CRect rect = GetView()->GetDrawingRect();
   graph.SetOutputRect(rect);
   graph.Draw(pDC->GetSafeHdc());
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTestGraphBuilder2, CEAFGraphBuilderBase)
   ON_BN_CLICKED(IDC_SINE, &CTestGraphBuilder2::OnGraphTypeChanged)
   ON_BN_CLICKED(IDC_COSINE, &CTestGraphBuilder2::OnGraphTypeChanged)
END_MESSAGE_MAP()


CTestGraphBuilder2::CTestGraphBuilder2()
{
   SetName(_T("Test Graph Builder 2"));
}

CTestGraphBuilder2::CTestGraphBuilder2(const CTestGraphBuilder2& other) :
CEAFGraphBuilderBase(other)
{
}

CEAFGraphControlWindow* CTestGraphBuilder2::GetGraphControlWindow()
{
   return &m_GraphControls;
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CTestGraphBuilder2::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CTestGraphBuilder2>(*this);
}

void CTestGraphBuilder2::CreateViewController(IEAFViewController** ppController)
{
   *ppController = nullptr; // not supplying a graph controller
}

BOOL CTestGraphBuilder2::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_GraphControls.Create(pParent,IDD_TEST_GRAPH_CONTROLS1, CBRS_RIGHT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CTestGraphBuilder2::OnGraphTypeChanged()
{
   CEAFGraphView* pGraphView = m_pFrame->GetGraphView();
   pGraphView->Invalidate();
   pGraphView->UpdateWindow();
}

void CTestGraphBuilder2::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   int graphType = m_GraphControls.GetGraphType();

   // first x axis
   const WBFL::Units::ScalarData& scalar = pDisplayUnits->GetScalarFormat();
   WBFL::Units::ScalarTool scalar_tool(scalar);
   WBFL::Graphing::GraphXY graph(&scalar_tool, &scalar_tool);

   IndexType idx = graph.CreateDataSeries();
   for ( int i = 0; i <= 360; i++ )
   {
      Float64 angle = ::ToRadians((Float64)(i));
      Float64 y;
      if ( graphType == SINE_GRAPH )
         y = sin(angle);
      else
         y = cos(angle);

      WBFL::Graphing::Point point(angle,y);
      graph.AddPoint(idx,point);
   }

   graph.SetClientAreaColor(RGB(123,231,111));

   CRect rect = GetView()->GetDrawingRect();
   graph.SetOutputRect(rect);
   graph.Draw(pDC->GetSafeHdc());
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTestGraphBuilder3, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CTestGraphBuilder3::CTestGraphBuilder3()
{
   SetName(_T("Test Graph Builder 3"));
}

CTestGraphBuilder3::CTestGraphBuilder3(const CTestGraphBuilder3& other) :
CEAFGraphBuilderBase(other)
{
}

CEAFGraphControlWindow* CTestGraphBuilder3::GetGraphControlWindow()
{
   return nullptr;
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CTestGraphBuilder3::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CTestGraphBuilder3>(*this);
}

void CTestGraphBuilder3::CreateViewController(IEAFViewController** ppController)
{
   *ppController = nullptr; // not supplying a graph controller
}

BOOL CTestGraphBuilder3::CreateGraphController(CWnd* pParent,UINT nID)
{
   return TRUE;
}

void CTestGraphBuilder3::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // first x axis
   const WBFL::Units::ScalarData& scalar = pDisplayUnits->GetScalarFormat();
   WBFL::Units::ScalarTool scalar_tool(scalar);
   WBFL::Graphing::GraphXY graph(&scalar_tool, &scalar_tool);

   IndexType series1 = graph.CreateDataSeries(_T("f(x)=10^x"),    PS_SOLID,1,RED);
   IndexType series2 = graph.CreateDataSeries(_T("f(x)=x"),       PS_SOLID,1,GREEN);
   IndexType series3 = graph.CreateDataSeries(_T("f(x)=log10(x)"),PS_SOLID,1,BLUE);

   graph.DrawGrid(true);
   graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Logarithmic);
   //graph.SetYAxisScale(grAxisXY::LOGARITHMIC);

   Float64 x[] = { 0.1, 1.0, 10.0 ,100.0, 1000.0};
   int n = (graph.GetXAxisScale() == WBFL::Graphing::AxisXY::AxisScale::Linear ? 3 : 5);
   for ( int i = 0; i < n; i++ )
   {
      Float64 X = x[i];
      if ( i < 2 )
         graph.AddPoint(series1, WBFL::Graphing::Point(X,pow(10,X)));

      if ( i < 3 )
         graph.AddPoint(series2, WBFL::Graphing::Point(X,X));

      if ( !(graph.GetYAxisScale() == WBFL::Graphing::AxisXY::AxisScale::Logarithmic && i < 2) )
         graph.AddPoint(series3, WBFL::Graphing::Point(X,log10(X)));
   }

   CRect rect = GetView()->GetDrawingRect();
   graph.SetOutputRect(rect);
   graph.Draw(pDC->GetSafeHdc());
}
