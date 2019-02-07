///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

#include "stdafx.h"

#include "resource.h"
#include "TestDialogBar.h"

IMPLEMENT_DYNCREATE(CTestDialogBar,CEAFGraphControlWindow)

CTestDialogBar::CTestDialogBar()
{
}

BEGIN_MESSAGE_MAP(CTestDialogBar, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CTestDialogBar)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_BUTTON1,&CTestDialogBar::OnButton)
END_MESSAGE_MAP()

BOOL CTestDialogBar::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();
   CheckRadioButton(IDC_SINE,IDC_COSINE,IDC_SINE);
   return TRUE;
}

void CTestDialogBar::OnButton()
{
   AfxMessageBox(_T("OnButton in CTestDialogBar"));
}

#ifdef _DEBUG
void CTestDialogBar::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CTestDialogBar::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG

int CTestDialogBar::GetGraphType()
{
   if( GetCheckedRadioButton(IDC_SINE,IDC_COSINE) == IDC_SINE )
      return SINE_GRAPH;
   else
      return COSINE_GRAPH;
}
