///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// HaunchSame4Bridge.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "HaunchByBearingDlg.h"
#include "EditHaunchDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CHaunchByBearingDlg dialog

IMPLEMENT_DYNAMIC(CHaunchByBearingDlg, CDialog)

CHaunchByBearingDlg::CHaunchByBearingDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CHaunchByBearingDlg::IDD, pParent)
{

}

CHaunchByBearingDlg::~CHaunchByBearingDlg()
{
}

void CHaunchByBearingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if (FALSE == m_Grid.UpdateData(pDX->m_bSaveAndValidate))
   {
      pDX->Fail();
   }
}

BEGIN_MESSAGE_MAP(CHaunchByBearingDlg, CDialog)
END_MESSAGE_MAP()


BOOL CHaunchByBearingDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_HAUNCH_GRID, this);
   m_Grid.CustomInit();

   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
