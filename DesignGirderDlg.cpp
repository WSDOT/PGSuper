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

// DesignGirderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "DesignGirderDlg.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <PsgLib\GirderLibraryEntry.h>
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg dialog


CDesignGirderDlg::CDesignGirderDlg(SpanIndexType span,GirderIndexType girder, bool enableA, bool designA, 
                                   IBroker* pBroker, CWnd* pParent /*=NULL*/)
	: CDialog(CDesignGirderDlg::IDD, pParent),
   m_EnableA(enableA),
   m_DesignA(designA)
{
   m_pBroker = pBroker;

	//{{AFX_DATA_INIT(CDesignGirderDlg)
	m_Girder = girder;
	m_Span = span;
	m_DesignForFlexure = TRUE;
	m_DesignForShear = TRUE;
	//}}AFX_DATA_INIT

   m_strToolTip = "Eugène Freyssinet\r\nFather of Prestressed Concrete";
}


void CDesignGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDesignGirderDlg)
	DDX_CBIndex(pDX, IDC_GIRDER, m_Girder);
	DDX_CBIndex(pDX, IDC_SPAN, m_Span);
	DDX_Check(pDX, IDC_DESIGN_FLEXURE, m_DesignForFlexure);
	DDX_Check(pDX, IDC_DESIGN_SHEAR, m_DesignForShear);
	//}}AFX_DATA_MAP

   if (m_EnableA)
   {
      CButton* pA= (CButton*)GetDlgItem( IDC_DESIGN_A );

      if (pDX->m_bSaveAndValidate)
      {
         m_DesignA = pA->GetCheck()==0; // control asks opposite
      }
      else
      {
         pA->SetCheck(m_DesignA?0:1);
      }
   }
}


BEGIN_MESSAGE_MAP(CDesignGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CDesignGirderDlg)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
	ON_CBN_SELCHANGE(IDC_SPAN, OnSpanChanged)
	ON_BN_CLICKED(IDC_DESIGN_FLEXURE, OnDesignFlexure)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg message handlers

BOOL CDesignGirderDlg::OnInitDialog() 
{
   // Load up the combo boxes with span and girder information
   GET_IFACE(IBridge,pBridge);

   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   CComboBox* pGdrBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   SpanIndexType cSpan = pBridge->GetSpanCount();
   for ( SpanIndexType i = 0; i < cSpan; i++ )
   {
      CString strSpan;
      strSpan.Format("Span %d",LABEL_SPAN(i));
      pSpanBox->AddString(strSpan);
   }

   pSpanBox->SetCurSel(m_Span);
   UpdateGirderComboBox(m_Span);

   // don't ask A design option unless it's enabled
   CWnd* pACheck = GetDlgItem( IDC_DESIGN_A );
   pACheck->ShowWindow(m_EnableA ? SW_SHOW : SW_HIDE);

	CDialog::OnInitDialog();
	
   EnableToolTips(TRUE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDesignGirderDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_DESIGNGIRDER );
}

void CDesignGirderDlg::OnSpanChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = (SpanIndexType)pCB->GetCurSel();

   UpdateGirderComboBox(spanIdx);
}

void CDesignGirderDlg::UpdateGirderComboBox(SpanIndexType spanIdx)
{
   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirder = pBridge->GetGirderCount( spanIdx );
   for ( GirderIndexType j = 0; j < cGirder; j++ )
   {
      CString strGdr;
      strGdr.Format( "Girder %s", LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
      pGdrBox->SetCurSel(0);
}

void CDesignGirderDlg::OnDesignFlexure() 
{
   // enable A design only if flexure is on
   if (m_EnableA)
   {
      CButton* pAFlex = (CButton*)GetDlgItem( IDC_DESIGN_FLEXURE );
      BOOL checked = (pAFlex->GetCheck()==1) ? TRUE:FALSE;

      CWnd* pADesA = GetDlgItem( IDC_DESIGN_A );
      pADesA->EnableWindow(checked);
   }
}

BOOL CDesignGirderDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_PICTURE:
         pTTT->lpszText = m_strToolTip.LockBuffer();
         pTTT->hinst = NULL;
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip

      return TRUE;
   }
   return FALSE;
}
