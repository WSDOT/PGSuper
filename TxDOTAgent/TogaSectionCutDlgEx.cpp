///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "TxDOTOptionalDesignGirderViewPage.h"
#include "TogaSectionCutDlgEx.h"
#include <ostream>
#include <MfcTools\CustomDDx.h>
#include <EAF\EAFDisplayUnits.h>
#include "HtmlHelp\TogaHelp.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTogaSectionCutDlgEx dialog

CTogaSectionCutDlgEx::CTogaSectionCutDlgEx(long nHarpPoints,Float64 value, Float64 lowerBound, Float64 upperBound, 
                     CTxDOTOptionalDesignGirderViewPage::CutLocation location, CWnd* pParent):
CDialog(CTogaSectionCutDlgEx::IDD, pParent),
m_Value(value),
m_LowerBound(lowerBound),
m_UpperBound(upperBound),
m_CutLocation(location),
m_nHarpPoints(nHarpPoints)
{
}


CTogaSectionCutDlgEx::CTogaSectionCutDlgEx(CWnd* pParent /*=NULL*/)
: CDialog(CTogaSectionCutDlgEx::IDD, pParent),
m_Value(0.0),
m_LowerBound(0.0),
m_UpperBound(0.0),
m_nHarpPoints(0),
m_CutLocation(CTxDOTOptionalDesignGirderViewPage::Center)
{
	//{{AFX_DATA_INIT(CTogaSectionCutDlgEx)
	m_CutIndex = INVALID_INDEX;
	//}}AFX_DATA_INIT
}

void CTogaSectionCutDlgEx::GetBounds(Float64* plowerBound, Float64* pupperBound)
{
   *plowerBound=m_LowerBound;
   *pupperBound=m_UpperBound;
}

void CTogaSectionCutDlgEx::SetBounds(Float64 lowerBound, Float64 upperBound)
{ 
   m_LowerBound = lowerBound;
   m_UpperBound = upperBound;
}


void CTogaSectionCutDlgEx::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTogaSectionCutDlgEx) 
	//}}AFX_DATA_MAP
	DDX_Radio(pDX, IDC_LEFT_END, (int&)m_CutIndex);
	DDX_Text(pDX, IDC_VALUE, m_Value);
   DDX_UnitValueAndTag( pDX, IDC_VALUE, IDC_VALUE_UNITS, m_Value, pDisplayUnits->GetSpanLengthUnit() );

   if ( pDX->m_bSaveAndValidate && m_CutIndex == IDC_USER_CUT )
      DDV_UnitValueRange(pDX, IDC_VALUE, m_Value, m_LowerBound, m_UpperBound, pDisplayUnits->GetSpanLengthUnit() );

   if (!pDX->m_bSaveAndValidate)
   {
      CStatic* pprompt = (CStatic*)GetDlgItem(IDC_USER_CUT);
      ASSERT(pprompt); 

      CString str;
      std::_tstring tag;
      Float64 lower, upper;
      lower = ::ConvertFromSysUnits(m_LowerBound, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      upper = ::ConvertFromSysUnits(m_UpperBound, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      tag = pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag();
      str.Format(_T("Enter a Value Between %g and %g %s"),lower, upper, tag.c_str());
      pprompt->SetWindowText(str);
   }
}

BEGIN_MESSAGE_MAP(CTogaSectionCutDlgEx, CDialog)
	//{{AFX_MSG_MAP(CTogaSectionCutDlgEx)
	ON_BN_CLICKED(IDC_GIRDER_MIDDLE, OnGirderMiddle)
	ON_BN_CLICKED(IDC_LEFT_END, OnLeftEnd)
	ON_BN_CLICKED(IDC_LEFT_HARP, OnLeftHarp)
	ON_BN_CLICKED(IDC_RIGHT_END, OnRightEnd)
	ON_BN_CLICKED(IDC_RIGHT_HARP, OnRightHarp)
	ON_BN_CLICKED(IDC_USER_CUT, OnUserCut)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTogaSectionCutDlgEx message handlers

BOOL CTogaSectionCutDlgEx::OnInitDialog() 
{
   if ( m_nHarpPoints == 0 )
   {
      GetDlgItem(IDC_LEFT_HARP)->EnableWindow(FALSE);
      GetDlgItem(IDC_RIGHT_HARP)->EnableWindow(FALSE);
   }

   m_CutIndex = (int)m_CutLocation;
   
   CDialog::OnInitDialog();

	Update();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CTogaSectionCutDlgEx::Update()
{
   CWnd* pEdit = GetDlgItem(IDC_VALUE);
   CWnd* pUnit = GetDlgItem(IDC_VALUE_UNITS);

   BOOL bEnable = (m_CutLocation == CTxDOTOptionalDesignGirderViewPage::UserInput) ? TRUE : FALSE;
   pEdit->EnableWindow( bEnable );
   pUnit->EnableWindow( bEnable );
}

void CTogaSectionCutDlgEx::SetValue(Float64 value)
{
   m_Value=value;
}

Float64 CTogaSectionCutDlgEx::GetValue()const
{
   return m_Value;
}

void CTogaSectionCutDlgEx::SetCutLocation(CTxDOTOptionalDesignGirderViewPage::CutLocation location)
{
   m_CutLocation=location;
}

CTxDOTOptionalDesignGirderViewPage::CutLocation CTogaSectionCutDlgEx::GetCutLocation() const 
{
   return m_CutLocation;
}

void CTogaSectionCutDlgEx::OnGirderMiddle() 
{
   m_CutLocation = CTxDOTOptionalDesignGirderViewPage::Center;
   Update();
}

void CTogaSectionCutDlgEx::OnLeftEnd() 
{
   m_CutLocation = CTxDOTOptionalDesignGirderViewPage::LeftEnd;
   Update();
}

void CTogaSectionCutDlgEx::OnLeftHarp() 
{
   m_CutLocation = CTxDOTOptionalDesignGirderViewPage::LeftHarp;
   Update();
}

void CTogaSectionCutDlgEx::OnRightEnd() 
{
   m_CutLocation = CTxDOTOptionalDesignGirderViewPage::RightEnd;
   Update();
}

void CTogaSectionCutDlgEx::OnRightHarp() 
{
   m_CutLocation = CTxDOTOptionalDesignGirderViewPage::RightHarp;
   Update();
}

void CTogaSectionCutDlgEx::OnUserCut() 
{
   m_CutLocation = CTxDOTOptionalDesignGirderViewPage::UserInput;
   Update();
}

void CTogaSectionCutDlgEx::OnHelp() 
{
  ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_VIEW_SETTINGS );
}
