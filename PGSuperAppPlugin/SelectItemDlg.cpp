///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// SelectItemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "SelectItemDlg.h"
#include <System\Tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectItemDlg dialog


CSelectItemDlg::CSelectItemDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CSelectItemDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectItemDlg)
	m_ItemIdx = 0;
	//}}AFX_DATA_INIT
}


void CSelectItemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectItemDlg)
	DDX_Control(pDX, IDC_LIST, m_cbList);
	DDX_Control(pDX, IDC_LABEL, m_Label);

   if ( pDX->m_bSaveAndValidate )
   {
      int idx;
   	DDX_CBIndex(pDX, IDC_LIST, idx);
      m_ItemIdx = idx;
   }
   else
   {
      int idx = (int)m_ItemIdx;
   	DDX_CBIndex(pDX, IDC_LIST, idx);
   }
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectItemDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectItemDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectItemDlg message handlers

BOOL CSelectItemDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   m_Label.SetWindowText(m_strLabel);

   sysTokenizer tokenizer(_T("\n"));
   tokenizer.push_back(m_strItems);
   sysTokenizer::iterator iter;
   for ( iter = tokenizer.begin(); iter != tokenizer.end(); iter++ )
   {
      std::_tstring strItem = *iter;
      m_cbList.AddString(strItem.c_str());
   }

   if ( m_cbList.SetCurSel((int)m_ItemIdx) == CB_ERR )
      m_cbList.SetCurSel(0);

   SetWindowText(m_strTitle);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
