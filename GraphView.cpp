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

// GraphView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "GraphView.h"
#include <Graphing\AnalysisResultsGraphBuilder.h>

#include "PGSuperCalculationSheet.h"
#include <IFace\VersionInfo.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
   boost::shared_ptr<CGraphBuilder> pGraphBuilder(GetGraphBuilder());
   boost::shared_ptr<CAnalysisResultsGraphBuilder> pMyGraphBuilder = boost::shared_dynamic_cast<CAnalysisResultsGraphBuilder>(pGraphBuilder);
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
   boost::shared_ptr<CGraphBuilder> pGraphBuilder(GetGraphBuilder());
   boost::shared_ptr<CEAFGraphBuilderBase> pMyGraphBuilder = boost::shared_dynamic_cast<CEAFGraphBuilderBase>(pGraphBuilder);
   return pMyGraphBuilder->IsValidGraph();
}

void CGraphView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IVersionInfo,pVerInfo);
   GET_IFACE2(pBroker,IDocumentType,pDocType);

   // get paper size
   PGSuperCalculationSheet border(pBroker);
   CString strBottomTitle;
   if ( pDocType->IsPGSuperDocument() )
      strBottomTitle.Format(_T("PGSuper™ Version %s, Copyright © %4d, WSDOT, All rights reserved"),pVerInfo->GetVersion(true),sysDate().Year());
   else
      strBottomTitle.Format(_T("PGSplice™ Version %s, Copyright © %4d, WSDOT, All rights reserved"),pVerInfo->GetVersion(true),sysDate().Year());

   border.SetTitle(strBottomTitle);
   CDocument* pdoc = GetDocument();
   CString path = pdoc->GetPathName();
   border.SetFileName(path);
   CRect rcPrint = border.Print(pDC, 1);

   if (rcPrint.IsRectEmpty())
   {
      CHECKX(0,_T("Can't print border - page too small?"));
      rcPrint = pInfo->m_rectDraw;
   }

   m_PrintRect = rcPrint;
	CEAFAutoCalcGraphView::OnPrint(pDC, pInfo);
}
