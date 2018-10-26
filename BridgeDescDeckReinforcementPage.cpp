///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// BridgeDescDeckReinforcementPage.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "BridgeDescDeckReinforcementPage.h"
#include "BridgeDescDlg.h"
#include <MFCTools\CustomDDX.h>

#include <IFace\DisplayUnits.h>

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckReinforcementPage property page

IMPLEMENT_DYNCREATE(CBridgeDescDeckReinforcementPage, CPropertyPage)

CBridgeDescDeckReinforcementPage::CBridgeDescDeckReinforcementPage() : CPropertyPage(CBridgeDescDeckReinforcementPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescDeckReinforcementPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CBridgeDescDeckReinforcementPage::~CBridgeDescDeckReinforcementPage()
{
}

void CBridgeDescDeckReinforcementPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeDescDeckReinforcementPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_CBStringExactCase(pDX,IDC_MILD_STEEL_SELECTOR,m_RebarData.strRebarMaterial);

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);

   DDX_UnitValueAndTag(pDX, IDC_TOP_COVER,    IDC_TOP_COVER_UNIT,    m_RebarData.TopCover,    pDispUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BOTTOM_COVER, IDC_BOTTOM_COVER_UNIT, m_RebarData.BottomCover, pDispUnits->GetComponentDimUnit() );

   DDX_CBItemData(pDX,IDC_TOP_MAT_BAR,    m_RebarData.TopRebarKey);
   DDX_CBItemData(pDX,IDC_BOTTOM_MAT_BAR, m_RebarData.BottomRebarKey);

   DDX_UnitValueAndTag(pDX, IDC_TOP_MAT_BAR_SPACING,    IDC_TOP_MAT_BAR_SPACING_UNIT,    m_RebarData.TopSpacing,    pDispUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BOTTOM_MAT_BAR_SPACING, IDC_BOTTOM_MAT_BAR_SPACING_UNIT, m_RebarData.BottomSpacing, pDispUnits->GetComponentDimUnit() );

   DDX_UnitValueAndTag(pDX, IDC_TOP_MAT_LUMP_SUM,    IDC_TOP_MAT_LUMP_SUM_UNIT,    m_RebarData.TopLumpSum,    pDispUnits->GetAvOverSUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BOTTOM_MAT_LUMP_SUM, IDC_BOTTOM_MAT_LUMP_SUM_UNIT, m_RebarData.BottomLumpSum, pDispUnits->GetAvOverSUnit() );

   if ( pDX->m_bSaveAndValidate )
      m_Grid.GetRebarData(m_RebarData.NegMomentRebar);
   else
      m_Grid.FillGrid(m_RebarData.NegMomentRebar);
}


BEGIN_MESSAGE_MAP(CBridgeDescDeckReinforcementPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescDeckReinforcementPage)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckReinforcementPage message handlers

void CBridgeDescDeckReinforcementPage::OnAdd() 
{
	m_Grid.AddRow();
}

void CBridgeDescDeckReinforcementPage::OnRemove() 
{
   m_Grid.RemoveSelectedRows();
}

BOOL CBridgeDescDeckReinforcementPage::OnInitDialog() 
{
   m_Grid.SubclassDlgItem(IDC_GRID, this);
   m_Grid.CustomInit();

   CComboBox* pcbTopRebar    = (CComboBox*)GetDlgItem(IDC_TOP_MAT_BAR);
   CComboBox* pcbBottomRebar = (CComboBox*)GetDlgItem(IDC_BOTTOM_MAT_BAR);
   FillRebarComboBox(pcbTopRebar);
   FillRebarComboBox(pcbBottomRebar);

   CPropertyPage::OnInitDialog();


   // select the one and only material
   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   ASSERT(pc);
   pc->SetCurSel(0);
   
   EnableToolTips(TRUE);
   
   EnableRemoveBtn(false); // start off with the button disabled... enabled once a selection is made
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
   
void CBridgeDescDeckReinforcementPage::FillRebarComboBox(CComboBox* pcbRebar)
{
   // Item data is the rebar key for the lrfdRebarPool object
   int idx = pcbRebar->AddString("None");
   pcbRebar->SetItemData(idx,-1);

   idx = pcbRebar->AddString("#4");
   pcbRebar->SetItemData(idx,4);

   idx = pcbRebar->AddString("#5");
   pcbRebar->SetItemData(idx,5);

   idx = pcbRebar->AddString("#6");
   pcbRebar->SetItemData(idx,6);

   idx = pcbRebar->AddString("#7");
   pcbRebar->SetItemData(idx,7);

   idx = pcbRebar->AddString("#8");
   pcbRebar->SetItemData(idx,8);

   idx = pcbRebar->AddString("#9");
   pcbRebar->SetItemData(idx,9);
}

void CBridgeDescDeckReinforcementPage::EnableRemoveBtn(bool bEnable)
{
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnable);
}

BOOL CBridgeDescDeckReinforcementPage::OnSetActive() 
{
   // If the deck type is sdtNone then there isn't a deck and isn't any need for rebar
   // If the deck type is sdtCompositeSIP then there is only a top mat of rebar (deck panels for the bottom mat)
   BOOL bEnableTop    = TRUE;
   BOOL bEnableBottom = TRUE;
   
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   {
      bEnableTop    = FALSE;
      bEnableBottom = FALSE;
      GetDlgItem(IDC_DECK)->SetWindowText("Deck reinforcement cannot be described because deck type is None.");
   }
   else if ( pParent->m_BridgeDesc.GetDeckDescription()->DeckType == pgsTypes::sdtCompositeSIP )
   {
      bEnableTop    = TRUE;
      bEnableBottom = FALSE;
      GetDlgItem(IDC_DECK)->SetWindowText("Bottom mat reinforcement cannot be described because deck type is Stay in Place deck panels.");
   }
   else
   {
      bEnableTop    = TRUE;
      bEnableBottom = TRUE;
      GetDlgItem(IDC_DECK)->SetWindowText("Describe top and bottom mat of deck reinforcement.");
   }

   GetDlgItem(IDC_TOP_COVER)->EnableWindow(bEnableTop);
   GetDlgItem(IDC_TOP_COVER_UNIT)->EnableWindow(bEnableTop);

   GetDlgItem(IDC_TOP_MAT_BAR)->EnableWindow(bEnableTop);

   GetDlgItem(IDC_TOP_MAT_BAR_SPACING)->EnableWindow(bEnableTop);
   GetDlgItem(IDC_TOP_MAT_BAR_SPACING_UNIT)->EnableWindow(bEnableTop);

   GetDlgItem(IDC_TOP_MAT_LUMP_SUM)->EnableWindow(bEnableTop);
   GetDlgItem(IDC_TOP_MAT_LUMP_SUM_UNIT)->EnableWindow(bEnableTop);
	

   GetDlgItem(IDC_BOTTOM_COVER)->EnableWindow(bEnableBottom);
   GetDlgItem(IDC_BOTTOM_COVER_UNIT)->EnableWindow(bEnableBottom);

   GetDlgItem(IDC_BOTTOM_MAT_BAR)->EnableWindow(bEnableBottom);

   GetDlgItem(IDC_BOTTOM_MAT_BAR_SPACING)->EnableWindow(bEnableBottom);
   GetDlgItem(IDC_BOTTOM_MAT_BAR_SPACING_UNIT)->EnableWindow(bEnableBottom);

   GetDlgItem(IDC_BOTTOM_MAT_LUMP_SUM)->EnableWindow(bEnableBottom);
   GetDlgItem(IDC_BOTTOM_MAT_LUMP_SUM_UNIT)->EnableWindow(bEnableBottom);

   if ( !bEnableTop && !bEnableBottom )
      m_Grid.EnableWindow(FALSE);

   m_Grid.EnableMats(bEnableTop,bEnableBottom);
   GetDlgItem(IDC_ADD)->EnableWindow(bEnableTop || bEnableBottom);
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnableTop || bEnableBottom);
	
   return CPropertyPage::OnSetActive();
}

void CBridgeDescDeckReinforcementPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGEDESC_DECKREBAR );
}

BOOL CBridgeDescDeckReinforcementPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_TOP_COVER:
         ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,300); // makes it a multi-line tooltip
         m_strTip = "Measured from top of deck to center of top mat of reinforcing";
         break;

      case IDC_BOTTOM_COVER:
         ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,300); // makes it a multi-line tooltip
         m_strTip = "Measured from bottom of deck to center of bottom mat of reinforcing";
         break;

      default:
         return FALSE;
      }

      pTTT->lpszText = m_strTip.LockBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}
