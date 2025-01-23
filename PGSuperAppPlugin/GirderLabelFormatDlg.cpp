///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// GirderLabelFormatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "GirderLabelFormatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CGirderLabelFormatDlg dialog

IMPLEMENT_DYNAMIC(CGirderLabelFormatDlg, CDialog)

CGirderLabelFormatDlg::CGirderLabelFormatDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CGirderLabelFormatDlg::IDD, pParent)
   , m_Format(0)
{

}

CGirderLabelFormatDlg::~CGirderLabelFormatDlg()
{
}

void CGirderLabelFormatDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Radio(pDX, IDC_ALPHA, m_Format);
}


BEGIN_MESSAGE_MAP(CGirderLabelFormatDlg, CDialog)
END_MESSAGE_MAP()


// CGirderLabelFormatDlg message handlers
