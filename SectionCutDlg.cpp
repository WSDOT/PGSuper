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

// SectionCutDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "SectionCutDlg.h"
#include <ostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSectionCutDlg dialog

CSectionCutDlg::CSectionCutDlg(Float64 value, Float64 lowerBound, Float64 upperBound, 
                               const CString& tag, CWnd* pParent):
CDialog(CSectionCutDlg::IDD, pParent),
m_UnitTag(tag),
m_Value(value),
m_LowerBound(lowerBound),
m_UpperBound(upperBound)
{
}


CSectionCutDlg::CSectionCutDlg(CWnd* pParent /*=nullptr*/)
: CDialog(CSectionCutDlg::IDD, pParent),
m_Value(0.0),
m_LowerBound(0.0),
m_UpperBound(0.0)
{
	//{{AFX_DATA_INIT(CSectionCutDlg)
	//}}AFX_DATA_INIT
}

void CSectionCutDlg::GetBounds(Float64* plowerBound, Float64* pupperBound)
{
   *plowerBound=m_LowerBound;
   *pupperBound=m_UpperBound;
}

void CSectionCutDlg::SetBounds(Float64 lowerBound, Float64 upperBound)
{ 
   m_LowerBound = lowerBound;
   m_UpperBound = upperBound;
}


void CSectionCutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSectionCutDlg) 
	DDX_Text(pDX, IDC_VALUE, m_Value);
	DDV_MinMaxDouble(pDX, m_Value, m_LowerBound, m_UpperBound);
	DDX_Text(pDX, IDC_VALUE_UNITS, m_UnitTag);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSectionCutDlg, CDialog)
	//{{AFX_MSG_MAP(CSectionCutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSectionCutDlg message handlers

BOOL CSectionCutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   CStatic* pprompt = (CStatic*)GetDlgItem(IDC_USER_CUT);
   ASSERT(pprompt); 

   CString str;
   str.Format(_T("Enter a Value Between %g and %g %s"),m_LowerBound, m_UpperBound, m_UnitTag);
   pprompt->SetWindowText(str);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSectionCutDlg::SetValue(Float64 value)
{
   m_Value=value;
}

Float64 CSectionCutDlg::GetValue()const
{
   return m_Value;
}

