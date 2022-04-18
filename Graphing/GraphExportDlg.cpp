///////////////////////////////////////////////////////////////////////
// EAF - Extensible Application Framework
// Copyright © 1999-2022  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This library is a part of the Washington Bridge Foundation Libraries
// and was developed as part of the Alternate Route Project
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the Alternate Route Library Open Source License as published by 
// the Washington State Department of Transportation, Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but is distributed 
// AS IS, WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE. See the Alternate Route Library Open Source 
// License for more details.
//
// You should have received a copy of the Alternate Route Library Open Source License 
// along with this program; if not, write to the Washington State Department of 
// Transportation, Bridge and Structures Office, P.O. Box  47340, 
// Olympia, WA 98503, USA or e-mail Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////


// GraphExportDlg.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "GraphExportDlg.h"

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphExportDlg dialog


CGraphExportDlg::CGraphExportDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CGraphExportDlg::IDD,pParent), 
	m_FileFormat(ffExcel)
{
	//{{AFX_DATA_INIT(CGraphExportDlg)
	//}}AFX_DATA_INIT
}


void CGraphExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphExportDlg)
	// 
	DDX_CBEnum(pDX,IDC_FILE_FORMAT,m_FileFormat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGraphExportDlg,CDialog)
	//{{AFX_MSG_MAP(CGraphExportDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
