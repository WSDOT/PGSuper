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

// AutoCalcView.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"
#include "AutoCalcView.h"
#include "LicensePlateChildFrm.h"
#include "AutoCalcDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CAutoCalcView,CView)

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcView

CAutoCalcView::CAutoCalcView()
{
   // Until some results exist, the lp frame is not in use.
   // While the lp frame is not in use, we don't have to worry about
   // turning it on and off
   m_bLpFrameEnabled = false;
}

CAutoCalcView::~CAutoCalcView()
{
}


BEGIN_MESSAGE_MAP(CAutoCalcView, CView)
	//{{AFX_MSG_MAP(CAutoCalcView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

bool CAutoCalcView::IsLpFrameEnabled() const
{
   return m_bLpFrameEnabled;
}

void CAutoCalcView::EnableLpFrame(bool bEnable)
{

   // don't want a lp frame if nothing to report
   if ( !DoResultsExist() )
      bEnable=false;

   if ( m_bLpFrameEnabled != bEnable )
   {
      // We are switching modes
      CLicensePlateChildFrame* pLpFrame = CLicensePlateChildFrame::GetLpFrame( this );
      pLpFrame->SetLicensePlateMode( bEnable ? CLicensePlateChildFrame::On : CLicensePlateChildFrame::Off );
      m_bLpFrameEnabled = bEnable;
   }
}

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcView diagnostics

#ifdef _DEBUG
void CAutoCalcView::AssertValid() const
{
	CView::AssertValid();
}

void CAutoCalcView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAutoCalcView message handlers

void CAutoCalcView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   CView::OnUpdate( pSender, lHint, pHint );

   CAutoCalcDoc* pDoc = GetDocument();
   EnableLpFrame( pDoc->IsAutoCalcEnabled() ? false : true );
}

BOOL CAutoCalcView::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.style |= WS_CLIPCHILDREN;
	
	return CView::PreCreateWindow(cs);
}

void CAutoCalcView::OnUpdateNow()
{
   UpdateNow();

   EnableLpFrame( false );
}

void CAutoCalcView::OnInitialUpdate() 
{
	// We don't want to receive the initial OnUpdate call OnInitalUpdate generates.
	CView::OnInitialUpdate();

   CLicensePlateChildFrame* pLpFrame = CLicensePlateChildFrame::GetLpFrame( this );
   pLpFrame->SetBackground(IDB_LPFRAME);
   pLpFrame->SetLicensePlateText("Warning: Contents out of date");
   pLpFrame->SetView( this );
}
