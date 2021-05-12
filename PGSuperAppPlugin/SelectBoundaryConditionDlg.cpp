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

// SelectBoundaryConditionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SelectBoundaryConditionDlg.h"


// CSelectBoundaryConditionDlg dialog

IMPLEMENT_DYNAMIC(CSelectBoundaryConditionDlg, CDialog)

CSelectBoundaryConditionDlg::CSelectBoundaryConditionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SELECT_BOUNDARY_CONDITION, pParent)
{

}

CSelectBoundaryConditionDlg::~CSelectBoundaryConditionDlg()
{
}

void CSelectBoundaryConditionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_BOUNDARY_CONDITIONS, m_cbBoundaryCondition);
   DDX_CBItemData(pDX, IDC_BOUNDARY_CONDITIONS, m_BoundaryCondition);
}


BEGIN_MESSAGE_MAP(CSelectBoundaryConditionDlg, CDialog)
END_MESSAGE_MAP()


// CSelectBoundaryConditionDlg message handlers


BOOL CSelectBoundaryConditionDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   m_cbBoundaryCondition.Initialize(m_bIsBoundaryPier, m_Connections, m_bIsNoDeck);
   m_cbBoundaryCondition.SetPierType(m_PierType);

   // TODO:  Add extra initialization here
   CWnd* pWnd = GetDlgItem(IDC_PROMPT);
   pWnd->SetWindowText(m_strPrompt);


   //CDataExchange dx(this, FALSE);
   //DDX_CBItemData(&dx, IDC_BOUNDARY_CONDITIONS, m_BoundaryCondition);

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}
