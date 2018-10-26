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

// FactorOfSafetyChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "pgsuper.h"
#include "pgsuperDoc.h"
#include "FactorOfSafetyChildFrame.h"
#include "FactorOfSafetyView.h"
#include <IFace\Bridge.h>
#include <PgsExt\PointOfInterest.h>
#include "PGSuperTypes.h"
#include "htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyChildFrame

IMPLEMENT_DYNCREATE(CFactorOfSafetyChildFrame, CEAFOutputChildFrame)

CFactorOfSafetyChildFrame::CFactorOfSafetyChildFrame():
m_GirderIdx(0),
m_SpanIdx(0),
m_Stage(Lifting),
m_Grid(true)
{
}

CFactorOfSafetyChildFrame::~CFactorOfSafetyChildFrame()
{
}

BEGIN_MESSAGE_MAP(CFactorOfSafetyChildFrame, CEAFOutputChildFrame)
	//{{AFX_MSG_MAP(CFactorOfSafetyChildFrame)
	ON_WM_CREATE()
   ON_CBN_SELCHANGE( IDC_GIRDER   , OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_SPAN     , OnSpanChanged )
   ON_CBN_SELCHANGE( IDC_STAGE    , OnStageChanged )
   ON_BN_CLICKED(IDC_GRID, OnGridClicked)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyChildFrame message handlers

GirderIndexType CFactorOfSafetyChildFrame::GetGirderIdx() const 
{
   return m_GirderIdx;
}

SpanIndexType CFactorOfSafetyChildFrame::GetSpanIdx() const             
{
   return m_SpanIdx;
}

CFactorOfSafetyChildFrame::Stage  CFactorOfSafetyChildFrame::GetStage() const     
{
   return m_Stage;
}

int CFactorOfSafetyChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEAFOutputChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if ( !m_SettingsBar.Create( this, IDD_STABILITY_BAR, CBRS_TOP, IDD_STABILITY_BAR) )
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}
	
   // fill stage bar since it won't change
   CComboBox* pstg_ctrl = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_STAGE);
   ASSERT(pstg_ctrl!=0);
   pstg_ctrl->AddString("Lifting");
   pstg_ctrl->AddString("Transportation");
   pstg_ctrl->SetCurSel(0);
   m_Stage = Lifting;

   // grid buttton
   CButton* pgrid_btn = (CButton*)m_SettingsBar.GetDlgItem(IDC_GRID);
   ASSERT(pgrid_btn!=0);
   pgrid_btn->SetCheck(m_Grid ? TRUE : FALSE);

   //Update(); // Initialize the combo boxes in the control bar

	return 0;
}

CFactorOfSafetyView* CFactorOfSafetyChildFrame::GetFactorOfSafetyView() const
{
   CWnd* pwnd = GetActiveView();
   CFactorOfSafetyView* pvw = dynamic_cast<CFactorOfSafetyView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

void CFactorOfSafetyChildFrame::UpdateViews()
{
   GetFactorOfSafetyView()->UpdateFromBar();
}

void CFactorOfSafetyChildFrame::Update()
{
   UpdateBar();
}

void CFactorOfSafetyChildFrame::UpdateBar()
{
   CComboBox* pspan_ctrl     = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   CComboBox* pgirder_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   CComboBox* pstage_ctrl    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_STAGE);
   ASSERT(pspan_ctrl);
   ASSERT(pgirder_ctrl);
   ASSERT(pstage_ctrl);

   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);

   // make sure controls are in sync with actual data
   // spans
   SpanIndexType num_spans = pBridge->GetSpanCount();
   ASSERT(num_spans!=0);
   int nc_items = pspan_ctrl->GetCount();
   if (nc_items != num_spans)
   {
      // number of spans has changed, need to refill control
      int sel = pspan_ctrl->GetCurSel();
      if (sel == CB_ERR)
      {
         sel = pDoc->GetSpanIdx();
         if ( sel == ALL_SPANS )
            sel = 0;

         if (num_spans <= SpanIndexType(sel))
            sel = int(num_spans-1);
      }

      pspan_ctrl->ResetContent();
      CString csv;
      for (SpanIndexType i=0; i<num_spans; i++)
      {
         csv.Format("Span %i", i+1);
         pspan_ctrl->AddString(csv);
      }
      if (SpanIndexType(sel) < num_spans)
         pspan_ctrl->SetCurSel(sel);
      else
         pspan_ctrl->SetCurSel(0);
   }

   m_SpanIdx = pspan_ctrl->GetCurSel();
   ASSERT(m_SpanIdx!=CB_ERR);
   if (m_SpanIdx==CB_ERR) m_SpanIdx=0;

   // girders
   GirderIndexType num_girders = pBridge->GetGirderCount(m_SpanIdx);
   int sel = pgirder_ctrl->GetCurSel();
   if (sel==CB_ERR) 
   {
      sel = pDoc->GetGirderIdx();
      if ( sel < 0 )
         sel = 0;

      if (num_girders <= GirderIndexType(sel) )
         sel = int(num_girders-1);
   }

   pgirder_ctrl->ResetContent();
   CString csv;
   for (GirderIndexType i=0; i<num_girders; i++)
   {
      csv.Format("Girder %s", LABEL_GIRDER(i));
      pgirder_ctrl->AddString(csv);
   }
   if (GirderIndexType(sel) < num_girders)
      pgirder_ctrl->SetCurSel(sel);
   else
      pgirder_ctrl->SetCurSel(0);

   m_GirderIdx = GirderIndexType(pgirder_ctrl->GetCurSel());
   ASSERT(m_GirderIdx != ALL_GIRDERS);
   if (m_GirderIdx == ALL_GIRDERS)
      m_GirderIdx = 0;

}

void CFactorOfSafetyChildFrame::OnGirderChanged()
{
   CComboBox* pgirder_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   ASSERT(pgirder_ctrl);

   int girdidx = pgirder_ctrl->GetCurSel();
   ASSERT(girdidx!=CB_ERR);
   if (girdidx==CB_ERR) 
   {
      m_GirderIdx=0;
      pgirder_ctrl->SetCurSel(0);
      UpdateViews();
   }
   else if (girdidx!=m_GirderIdx)
   {
      m_GirderIdx = girdidx;
      UpdateViews();
   }
}

void CFactorOfSafetyChildFrame::OnSpanChanged()
{
   CComboBox* pspan_ctrl     = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   ASSERT(pspan_ctrl);

   int spnidx = pspan_ctrl->GetCurSel();
   ASSERT(spnidx!=CB_ERR);
   if (spnidx==CB_ERR) 
   {
      m_SpanIdx=0;
      pspan_ctrl->SetCurSel(0);
   }
   else if (spnidx!=m_SpanIdx)
   {
      m_SpanIdx = spnidx;
   }
   UpdateBar();
   UpdateViews();
}

void CFactorOfSafetyChildFrame::OnStageChanged()
{
   // stages
   Stage  stage;
   bool changed = false;

   CComboBox* pstage_ctrl    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_STAGE);
   ASSERT(pstage_ctrl);

   int idx = pstage_ctrl->GetCurSel();
   ASSERT(idx!=CB_ERR);
   if (idx==CB_ERR) 
   {
      stage = Lifting;
      pstage_ctrl->SetCurSel(0);
   }
   else if (idx==0)
      stage = Lifting;
   else if (idx==1)
      stage = Hauling;
   else
   {
      ASSERT(0); // stage out of range
      stage = Lifting;
   }

   if (stage != m_Stage)
   {
      m_Stage = stage;

      UpdateViews();
   }
}

void CFactorOfSafetyChildFrame::OnGridClicked()
{
   m_Grid = !m_Grid;
   GetFactorOfSafetyView()->UpdateGrid();
}

void CFactorOfSafetyChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if ( bAddToTitle )
   {
      // TRICKY: we expect our view to provide is with text
      CView* pView = this->GetActiveView();
      if ( pView )
      {
         CString name;
         pView->GetWindowText(name);
		   AfxSetWindowText(m_hWnd, name);
      }
   }
}

LRESULT CFactorOfSafetyChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_STABILITY_VIEW );
   return TRUE;
}
