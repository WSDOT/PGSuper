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

// GraphView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "GraphView.h"
#include <Graphs\AnalysisResultsGraphBuilder.h>

#include "PGSuperCalculationSheet.h"
#include <IFace\VersionInfo.h>

#include <EAF\PluginApp.h>

#include "Hints.h"


/////////////////////////////////////////////////////////////////////////////
// CGraphView

IMPLEMENT_DYNCREATE(CGraphView, CEAFAutoCalcGraphView)

CGraphView::CGraphView()
{
}

CGraphView::~CGraphView()
{
}

BEGIN_MESSAGE_MAP(CGraphView, CEAFAutoCalcGraphView)
	//{{AFX_MSG_MAP(CGraphView)
   ON_COMMAND(ID_DUMP_LBAM,DumpLBAM)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphView drawing

void CGraphView::DumpLBAM()
{
   // Alt + Ctrl + L
   std::unique_ptr<WBFL::Graphing::GraphBuilder>& pGraphBuilder(GetGraphBuilder());
   CAnalysisResultsGraphBuilder* pMyGraphBuilder = dynamic_cast<CAnalysisResultsGraphBuilder*>(pGraphBuilder.get());
   if ( pMyGraphBuilder )
   {
      pMyGraphBuilder->DumpLBAM();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CGraphView diagnostics

#ifdef _DEBUG
void CGraphView::AssertValid() const
{
	CEAFAutoCalcGraphView::AssertValid();
}

void CGraphView::Dump(CDumpContext& dc) const
{
	CEAFAutoCalcGraphView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGraphView message handlers
bool CGraphView::DoResultsExist()
{
   std::unique_ptr<WBFL::Graphing::GraphBuilder>& pGraphBuilder(GetGraphBuilder());
   CEAFGraphBuilderBase* pMyGraphBuilder = dynamic_cast<CEAFGraphBuilderBase*>(pGraphBuilder.get());
   return pMyGraphBuilder->IsValidGraph();
}

void CGraphView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
   
   auto pBroker = EAFGetBroker();

   // get paper size
   PGSuperCalculationSheet border(pBroker);

   CRect rcPrint = border.Print(pDC, 1);

   if (rcPrint.IsRectEmpty())
   {
      CHECKX(0,_T("Can't print border - page too small?"));
      rcPrint = pInfo->m_rectDraw;
   }

   m_PrintRect = rcPrint;
	CEAFAutoCalcGraphView::OnPrint(pDC, pInfo);
}

void CGraphView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if (lHint == HINT_SELECTIONCHANGED)
   {
      return; // none of the graphs are keyed to the selection, so if the selection changes, do nothing...
      // if we let this go through, then, if autocalc mode is disabled, the graphs will get a license plate frame
      // even though nothing has changed except the selection
   }


   CEAFAutoCalcGraphView::OnUpdate(pSender, lHint, pHint);
}
