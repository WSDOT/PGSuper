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

// ResolveGirderSpacingDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ResolveGirderSpacingDlg.h"
#include <System\Tokenizer.h>
#include <MfcTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResolveGirderSpacingDlg dialog


CResolveGirderSpacingDlg::CResolveGirderSpacingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CResolveGirderSpacingDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CResolveGirderSpacingDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_ItemIdx = 0;
   m_MeasurementDatum = 0;
   m_RestrictSpacing = false;
}


void CResolveGirderSpacingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResolveGirderSpacingDlg)
	DDX_Control(pDX, IDC_MEASUREMENT_DATUM, m_cbDatum);
	DDX_Control(pDX, IDC_SPACING, m_cbList);
	//}}AFX_DATA_MAP

   DDX_CBItemData(pDX, IDC_MEASUREMENT_DATUM, m_MeasurementDatum);
	DDX_CBIndex(pDX, IDC_SPACING, m_ItemIdx);
}


BEGIN_MESSAGE_MAP(CResolveGirderSpacingDlg, CDialog)
	//{{AFX_MSG_MAP(CResolveGirderSpacingDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResolveGirderSpacingDlg message handlers

BOOL CResolveGirderSpacingDlg::OnInitDialog() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_MEASUREMENT_DATUM);
   int idx = pCB->AddString(_T("Measured at and along the abutment/pier line"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pCB->SetItemData(idx,item_data);
   
   idx = pCB->AddString(_T("Measured normal to alignment at abutment/pier line"));
   item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem);
   pCB->SetItemData(idx,item_data);
   
   if (!m_RestrictSpacing)
   {
      idx = pCB->AddString(_T("Measured at and along the CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem);
      pCB->SetItemData(idx,item_data);

      idx = pCB->AddString(_T("Measured normal to alignment at CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem);
      pCB->SetItemData(idx,item_data);
   }

   CDialog::OnInitDialog();

   // fill up the list of girder spacings
   sysTokenizer tokenizer(_T("\n"));
   tokenizer.push_back(m_strSpacings);
   sysTokenizer::iterator iter;
   for ( iter = tokenizer.begin(); iter != tokenizer.end(); iter++ )
   {
      std::_tstring strItem = *iter;
      m_cbList.AddString(strItem.c_str());
   }

   if ( m_cbList.SetCurSel(m_ItemIdx) == CB_ERR )
      m_cbList.SetCurSel(0);


	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
