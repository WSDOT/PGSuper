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

// BridgeDescDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"
#include "BridgeDescDlg.h"
#include <PgsExt\DeckRebarData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDlg

IMPLEMENT_DYNAMIC(CBridgeDescDlg, CPropertySheet)

CBridgeDescDlg::CBridgeDescDlg(const CBridgeDescription2& bridgeDesc,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Bridge Description"), pParentWnd, iSelectPage)
{
   SetBridgeDescription(bridgeDesc);
   Init();
}

CBridgeDescDlg::~CBridgeDescDlg()
{
}

void CBridgeDescDlg::SetBridgeDescription(const CBridgeDescription2& bridgeDesc)
{
   m_BridgeDesc = bridgeDesc;
   m_DeckRebarPage.m_RebarData = m_BridgeDesc.GetDeckDescription()->DeckRebarData;
}

const CBridgeDescription2& CBridgeDescDlg::GetBridgeDescription()
{
   m_BridgeDesc.GetDeckDescription()->DeckRebarData = m_DeckRebarPage.m_RebarData;
   return m_BridgeDesc;
}

void CBridgeDescDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_GeneralPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_FramingPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_RailingSystemPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_DeckDetailsPage.m_psp.dwFlags    |= PSP_HASHELP;
   m_DeckRebarPage.m_psp.dwFlags      |= PSP_HASHELP;
   m_EnvironmentalPage.m_psp.dwFlags  |= PSP_HASHELP;

   AddPage( &m_GeneralPage );
   AddPage( &m_FramingPage );
   AddPage( &m_RailingSystemPage );
   AddPage( &m_DeckDetailsPage );
   AddPage( &m_DeckRebarPage );
   AddPage( &m_EnvironmentalPage );
}

BEGIN_MESSAGE_MAP(CBridgeDescDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CBridgeDescDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   WBFL_ON_PROPSHEET_OK
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDlg message handlers
BOOL CBridgeDescDlg::OnInitDialog()
{
	CPropertySheet::OnInitDialog();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_EDIT_BRIDGE),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   UpdateData(FALSE); // calls DoDataExchange
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CBridgeDescDlg::OnOK()
{
   BOOL bOK = UpdateData(TRUE);
   return !bOK;
}

void CBridgeDescDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      // force the active page to update its data
      CPropertyPage* pPage = GetActivePage();
      pPage->UpdateData(TRUE);

      // Do bridge-wide validation

      // Make sure there isn't negative moment rebar at non-continuous piers
      // (Note that the latest rebar data is still in the rebar page, not in the bridge model)
      std::vector<CDeckRebarData::NegMomentRebarData>::iterator iter(m_DeckRebarPage.m_RebarData.NegMomentRebar.begin());
      std::vector<CDeckRebarData::NegMomentRebarData>::iterator end(m_DeckRebarPage.m_RebarData.NegMomentRebar.end());
      for ( ; iter != end; iter++ )
      {
         CDeckRebarData::NegMomentRebarData& nmRebarData = *iter;
         CPierData2* pPier = m_BridgeDesc.GetPier(nmRebarData.PierIdx);
         ATLASSERT(pPier->IsBoundaryPier());
         pgsTypes::PierConnectionType connectionType = pPier->GetPierConnectionType();
         if ( connectionType == pgsTypes::Hinge || connectionType == pgsTypes::Roller )
         {
            CString strMsg;
            strMsg.Format(_T("Pier %d has a roller/hinge boundary condition. Supplemental deck reinforcement is not permitted at this pier. Change the boundary conditions or remove the supplemental reinforcement."),LABEL_PIER(nmRebarData.PierIdx));
            AfxMessageBox(strMsg);
            pDX->Fail();
         }
      }
   }
}
