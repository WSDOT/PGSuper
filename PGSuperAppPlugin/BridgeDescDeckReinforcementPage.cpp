///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// BridgeDescDeckReinforcementPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "BridgeDescDeckReinforcementPage.h"
#include "BridgeDescDlg.h"
#include "PGSuperDoc.h"

#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\Helpers.h>

#include <Lrfd\RebarPool.h>

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

   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_cbRebar);
   DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,m_RebarData.TopRebarType,m_RebarData.TopRebarGrade);
   DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,m_RebarData.BottomRebarType,m_RebarData.BottomRebarGrade);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag(pDX, IDC_TOP_COVER,    IDC_TOP_COVER_UNIT,    m_RebarData.TopCover,    pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BOTTOM_COVER, IDC_BOTTOM_COVER_UNIT, m_RebarData.BottomCover, pDisplayUnits->GetComponentDimUnit() );

   DDX_CBItemData(pDX,IDC_TOP_MAT_BAR,    m_RebarData.TopRebarSize);
   DDX_CBItemData(pDX,IDC_BOTTOM_MAT_BAR, m_RebarData.BottomRebarSize);

   DDX_UnitValueAndTag(pDX, IDC_TOP_MAT_BAR_SPACING,    IDC_TOP_MAT_BAR_SPACING_UNIT,    m_RebarData.TopSpacing,    pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BOTTOM_MAT_BAR_SPACING, IDC_BOTTOM_MAT_BAR_SPACING_UNIT, m_RebarData.BottomSpacing, pDisplayUnits->GetComponentDimUnit() );

   if ( pDX->m_bSaveAndValidate )
   {
      if ( m_RebarData.TopRebarSize != matRebar::bsNone && IsLE(m_RebarData.TopSpacing,0.0) )
      {
         AfxMessageBox(_T("Spacing of top rebar must be greater than zero"));
         pDX->PrepareEditCtrl(IDC_TOP_MAT_BAR_SPACING);
         pDX->Fail();
      }

      if ( m_RebarData.BottomRebarSize != matRebar::bsNone && IsLE(m_RebarData.BottomSpacing,0.0) )
      {
         AfxMessageBox(_T("Spacing of bottom rebar must be greater than zero"));
         pDX->PrepareEditCtrl(IDC_BOTTOM_MAT_BAR_SPACING);
         pDX->Fail();
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_TOP_MAT_LUMP_SUM,    IDC_TOP_MAT_LUMP_SUM_UNIT,    m_RebarData.TopLumpSum,    pDisplayUnits->GetAvOverSUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BOTTOM_MAT_LUMP_SUM, IDC_BOTTOM_MAT_LUMP_SUM_UNIT, m_RebarData.BottomLumpSum, pDisplayUnits->GetAvOverSUnit() );

   DDV_UnitValueZeroOrMore(pDX, IDC_TOP_MAT_LUMP_SUM,    m_RebarData.TopLumpSum,    pDisplayUnits->GetAvOverSUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BOTTOM_MAT_LUMP_SUM, m_RebarData.BottomLumpSum, pDisplayUnits->GetAvOverSUnit() );

   DDV_GXGridWnd(pDX,&m_Grid);
   if ( pDX->m_bSaveAndValidate )
   {
      if ( !m_Grid.GetRebarData(m_RebarData.NegMomentRebar) )
      {
         pDX->Fail();
      }

      CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
      pParent->m_BridgeDesc.GetDeckDescription()->DeckRebarData = m_RebarData;
   }
   else
   {
      m_Grid.FillGrid(m_RebarData.NegMomentRebar);
   }
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

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   m_RebarData = pParent->m_BridgeDesc.GetDeckDescription()->DeckRebarData;

   CPropertyPage::OnInitDialog();
   
   EnableToolTips(TRUE);
   
   EnableRemoveBtn(false); // start off with the button disabled... enabled once a selection is made
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescDeckReinforcementPage::GetRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade)
{
   return m_cbRebar.GetMaterial(pType,pGrade);
}

void CBridgeDescDeckReinforcementPage::FillRebarComboBox(CComboBox* pcbRebar)
{
   int idx = pcbRebar->AddString(_T("None"));
   pcbRebar->SetItemData(idx,(DWORD_PTR)matRebar::bsNone);
   lrfdRebarIter rebarIter(m_RebarData.TopRebarType,m_RebarData.TopRebarGrade);
   for ( rebarIter.Begin(); rebarIter; rebarIter.Next() )
   {
      const matRebar* pRebar = rebarIter.GetCurrentRebar();
      idx = pcbRebar->AddString(pRebar->GetName().c_str());
      pcbRebar->SetItemData(idx,(DWORD_PTR)pRebar->GetSize());
   }
}

void CBridgeDescDeckReinforcementPage::EnableAddBtn(bool bEnable)
{
   GetDlgItem(IDC_ADD)->EnableWindow(bEnable);
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

   m_RebarData = pParent->m_BridgeDesc.GetDeckDescription()->DeckRebarData;

   pgsTypes::SupportedDeckType deckType = pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType();
   if ( IsNonstructuralDeck(deckType) )
   {
      bEnableTop    = FALSE;
      bEnableBottom = FALSE;
      CString strText;
      strText.Format(_T("The deck reinforcement cannot be described because the deck type is \"%s\""), GetDeckTypeName(deckType));
      GetDlgItem(IDC_DECK)->SetWindowText(strText);
   }
   else if ( deckType == pgsTypes::sdtCompositeSIP )
   {
      bEnableTop    = TRUE;
      bEnableBottom = FALSE;
      CString strText;
      strText.Format(_T("Bottom mat reinforcement cannot be described because deck type is \"%s\""), GetDeckTypeName(deckType));
      GetDlgItem(IDC_DECK)->SetWindowText(strText);
   }
   else
   {
      bEnableTop    = TRUE;
      bEnableBottom = TRUE;
      GetDlgItem(IDC_DECK)->SetWindowText(_T("Describe top and bottom mat of deck reinforcement."));
   }

   GetDlgItem(IDC_TOP_COVER_LABEL)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_COVER)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_COVER_UNIT)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_TOP_MAT_BAR_LABEL)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_MAT_BAR)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_MAT_BAR_AT)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_MAT_BAR_PLUS)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_TOP_MAT_BAR_SPACING)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_MAT_BAR_SPACING_UNIT)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_TOP_MAT_LUMP_SUM)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_TOP_MAT_LUMP_SUM_UNIT)->ShowWindow(bEnableTop ? SW_SHOW : SW_HIDE);
	

   GetDlgItem(IDC_BOTTOM_COVER_LABEL)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_COVER)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_COVER_UNIT)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_BOTTOM_MAT_BAR_LABEL)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_MAT_BAR)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_MAT_BAR_AT)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_MAT_BAR_PLUS)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_BOTTOM_MAT_BAR_SPACING)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_MAT_BAR_SPACING_UNIT)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_BOTTOM_MAT_LUMP_SUM)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BOTTOM_MAT_LUMP_SUM_UNIT)->ShowWindow(bEnableBottom ? SW_SHOW : SW_HIDE);

   GetDlgItem(IDC_MILD_STEEL_SELECTOR)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_PRIMARY_REBAR_GROUP)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_PRIMARY_REBAR_GROUP_LABEL)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_SECONDARY_REBAR_GROUP)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_SECONDARY_REBAR_GROUP_LABEL)->ShowWindow(SW_SHOW);
   m_Grid.ShowWindow(SW_SHOW);
   GetDlgItem(IDC_ADD)->ShowWindow(SW_SHOW);
   GetDlgItem(IDC_REMOVE)->ShowWindow(SW_SHOW);

   m_Grid.EnableMats(bEnableTop,bEnableBottom);
   GetDlgItem(IDC_ADD)->EnableWindow(bEnableTop || bEnableBottom);
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnableTop || bEnableBottom);

   m_Grid.UpdatePierList();

   if ( !bEnableTop && !bEnableBottom )
   {
      m_Grid.ShowWindow(SW_HIDE);
      //m_Grid.EnableWindow(FALSE);
      GetDlgItem(IDC_MILD_STEEL_SELECTOR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PRIMARY_REBAR_GROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PRIMARY_REBAR_GROUP_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SECONDARY_REBAR_GROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SECONDARY_REBAR_GROUP_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_ADD)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_REMOVE)->ShowWindow(SW_HIDE);
   }
	
   return CPropertyPage::OnSetActive();
}

void CBridgeDescDeckReinforcementPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_DECKREBAR );
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
         m_strTip = _T("Measured from top of deck to center of top mat of reinforcing");
         break;

      case IDC_BOTTOM_COVER:
         m_strTip = _T("Measured from bottom of deck to center of bottom mat of reinforcing");
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.LockBuffer();
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}
