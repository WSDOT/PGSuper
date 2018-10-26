///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

// DealWithLoadDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DealWithLoadDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDealWithLoadDlg dialog


CDealWithLoadDlg::CDealWithLoadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDealWithLoadDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDealWithLoadDlg)
	m_Message = _T("Error - message not set");
	//}}AFX_DATA_INIT
}


void CDealWithLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDealWithLoadDlg)
	DDX_Text(pDX, IDC_MESSAGE, m_Message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDealWithLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CDealWithLoadDlg)
	ON_BN_CLICKED(ID_DELETE_LOAD, OnDeleteLoad)
	ON_BN_CLICKED(ID_EDIT_LOAD, OnEditLoad)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDealWithLoadDlg message handlers

void CDealWithLoadDlg::OnDeleteLoad() 
{
   EndDialog(IDDELETELOAD);
}

void CDealWithLoadDlg::OnEditLoad() 
{
   EndDialog(IDEDITLOAD);
}
