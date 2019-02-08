///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// EccentricityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EccentricityDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CEccentricityDlg dialog

IMPLEMENT_DYNAMIC(CEccentricityDlg, CDialog)

CEccentricityDlg::CEccentricityDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CEccentricityDlg::IDD, pParent)
   , m_Message(_T(""))
{

}

CEccentricityDlg::~CEccentricityDlg()
{
}

void CEccentricityDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_ECC_TEXT, m_Message);
}


BEGIN_MESSAGE_MAP(CEccentricityDlg, CDialog)
END_MESSAGE_MAP()


// CEccentricityDlg message handlers
