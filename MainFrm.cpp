///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PGSuper.h"
#include "MainFrm.h"
#include "Resource.h"
#include <IFace\StatusCenter.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CEAFMainFrame)

BEGIN_MESSAGE_MAP(CMainFrame, CEAFMainFrame)
   //{{AFX_MSG_MAP(CMainFrame)
   ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
CEAFMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

CEAFStatusBar* CMainFrame::CreateStatusBar()
{
   // replace base class implemenation and use our own status bar
   std::auto_ptr<CPGSuperStatusBar> pStatusBar(new CPGSuperStatusBar());
	if (!pStatusBar->Create(this) )
	{
      return NULL;
	}

   return pStatusBar.release();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if (CEAFMainFrame::OnCreate(lpCreateStruct) == -1)
      return -1;


   // Status bar is created by the base class

   // since the document isn't open yet, don't display the analysis mode or the autocalc mode
   CPGSuperStatusBar* pStatusBar = (CPGSuperStatusBar*)m_pStatusBar;
   pStatusBar->Reset();

   return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
   CEAFMainFrame::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
   CEAFMainFrame::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::AutoCalcEnabled( bool bEnable )
{
   CEAFDocument* pDoc = GetDocument();
   if ( pDoc )
   {
      CString status_text;
      if ( bEnable )
         status_text.LoadString(ID_INDICATOR_AUTOCALC_ON);
      else
         status_text.LoadString(ID_INDICATOR_AUTOCALC_OFF);

      CPGSuperStatusBar* pStatusBar = (CPGSuperStatusBar*)m_pStatusBar;
      int idx = pStatusBar->GetAutoCalcPaneIndex();
      m_pStatusBar->SetPaneText(idx, status_text, TRUE);
   }
}

void CMainFrame::SetAnalysisTypeStatusIndicator(pgsTypes::AnalysisType analysisType)
{
   CString strAnalysisType;
   switch( analysisType )
   {
   case pgsTypes::Simple:
      strAnalysisType = "Simple Span";
      break;
   case pgsTypes::Continuous:
      strAnalysisType = "Continuous";
      break;
   case pgsTypes::Envelope:
      strAnalysisType = "Envelope";
      break;
   }

   CPGSuperStatusBar* pStatusBar = (CPGSuperStatusBar*)m_pStatusBar;
   int idx = pStatusBar->GetAnalysisModePaneIndex();
   m_pStatusBar->SetPaneText(idx,strAnalysisType,TRUE);
}
