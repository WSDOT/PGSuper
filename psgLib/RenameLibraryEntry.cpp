///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// RenameLibraryEntry.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "RenameLibraryEntry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRenameLibraryEntry dialog


CRenameLibraryEntry::CRenameLibraryEntry(CWnd* pParent /*=nullptr*/)
	: CDialog(CRenameLibraryEntry::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRenameLibraryEntry)
	m_EntryName = _T("");
	//}}AFX_DATA_INIT
}


void CRenameLibraryEntry::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRenameLibraryEntry)
	DDX_Text(pDX, IDC_EDIT1, m_EntryName);
	DDV_MaxChars(pDX, m_EntryName, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameLibraryEntry, CDialog)
	//{{AFX_MSG_MAP(CRenameLibraryEntry)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRenameLibraryEntry message handlers
