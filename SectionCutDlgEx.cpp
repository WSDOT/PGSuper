///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include "GirderModelChildFrame.h"
#include "SectionCutDlgEx.h"
#include <ostream>
#include <MfcTools\CustomDDx.h>
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSectionCutDlgEx dialog

CSectionCutDlgEx::CSectionCutDlgEx(long nHarpPoints,Float64 value, Float64 lowerBound, Float64 upperBound, 
                     bool isUnitsSi, CGirderModelChildFrame::CutLocation location, CWnd* pParent):
CDialog(CSectionCutDlgEx::IDD, pParent),
m_IsUnitsSi(isUnitsSi),
m_Value(value),
m_LowerBound(lowerBound),
m_UpperBound(upperBound),
m_CutLocation(location),
m_nHarpPoints(nHarpPoints)
{
}


CSectionCutDlgEx::CSectionCutDlgEx(CWnd* pParent /*=NULL*/)
: CDialog(CSectionCutDlgEx::IDD, pParent),
m_Value(0.0),
m_LowerBound(0.0),
m_UpperBound(0.0),
m_nHarpPoints(0),
m_CutLocation(CGirderModelChildFrame::Center)
{
	//{{AFX_DATA_INIT(CSectionCutDlgEx)
	m_CutIndex = -1;
	//}}AFX_DATA_INIT
}

void CSectionCutDlgEx::GetBounds(Float64* plowerBound, Float64* pupperBound)
{
   *plowerBound=m_LowerBound;
   *pupperBound=m_UpperBound;
}

void CSectionCutDlgEx::SetBounds(Float64 lowerBound, Float64 upperBound)
{ 
   m_LowerBound = lowerBound;
   m_UpperBound = upperBound;
}


void CSectionCutDlgEx::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSectionCutDlgEx) 
	//}}AFX_DATA_MAP
	DDX_Radio(pDX, IDC_LEFT_END, m_CutIndex);
	DDX_Text(pDX, IDC_VALUE, m_Value);
   DDX_UnitValueAndTag( pDX, IDC_VALUE, IDC_VALUE_UNITS, m_Value, m_IsUnitsSi, unitMeasure::Feet, unitMeasure::Meter );

   if ( pDX->m_bSaveAndValidate && m_CutIndex == IDC_USER_CUT )
      DDV_UnitValueRange(pDX, m_Value, m_LowerBound, m_UpperBound, m_IsUnitsSi, unitMeasure::Feet, unitMeasure::Meter );

   if (!pDX->m_bSaveAndValidate)
   {
      CStatic* pprompt = (CStatic*)GetDlgItem(IDC_USER_CUT);
      ASSERT(pprompt); 

      CString str;
      std::string tag;
      Float64 lower, upper;
      if (m_IsUnitsSi)
      {
         lower = ::ConvertFromSysUnits(m_LowerBound, unitMeasure::Meter);
         upper = ::ConvertFromSysUnits(m_UpperBound, unitMeasure::Meter);
         tag = unitMeasure::Meter.UnitTag();
      }
      else
      {
         lower = ::ConvertFromSysUnits(m_LowerBound, unitMeasure::Feet);
         upper = ::ConvertFromSysUnits(m_UpperBound, unitMeasure::Feet);
         tag = unitMeasure::Feet.UnitTag();
      }
      str.Format("Enter a Value Between %g and %g %s",lower, upper, tag.c_str());
      pprompt->SetWindowText(str);
   }
}

BEGIN_MESSAGE_MAP(CSectionCutDlgEx, CDialog)
	//{{AFX_MSG_MAP(CSectionCutDlgEx)
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
// CSectionCutDlgEx message handlers

BOOL CSectionCutDlgEx::OnInitDialog() 
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


void CSectionCutDlgEx::Update()
{
   CWnd* pEdit = GetDlgItem(IDC_VALUE);
   CWnd* pUnit = GetDlgItem(IDC_VALUE_UNITS);

   BOOL bEnable = (m_CutLocation == CGirderModelChildFrame::UserInput) ? TRUE : FALSE;
   pEdit->EnableWindow( bEnable );
   pUnit->EnableWindow( bEnable );
}

void CSectionCutDlgEx::SetValue(Float64 value)
{
   m_Value=value;
}

Float64 CSectionCutDlgEx::GetValue()const
{
   return m_Value;
}

void CSectionCutDlgEx::SetCutLocation(CGirderModelChildFrame::CutLocation location)
{
   m_CutLocation=location;
}

CGirderModelChildFrame::CutLocation CSectionCutDlgEx::GetCutLocation() const 
{
   return m_CutLocation;
}

void CSectionCutDlgEx::OnGirderMiddle() 
{
   m_CutLocation = CGirderModelChildFrame::Center;
   Update();
}

void CSectionCutDlgEx::OnLeftEnd() 
{
   m_CutLocation = CGirderModelChildFrame::LeftEnd;
   Update();
}

void CSectionCutDlgEx::OnLeftHarp() 
{
   m_CutLocation = CGirderModelChildFrame::LeftHarp;
   Update();
}

void CSectionCutDlgEx::OnRightEnd() 
{
   m_CutLocation = CGirderModelChildFrame::RightEnd;
   Update();
}

void CSectionCutDlgEx::OnRightHarp() 
{
   m_CutLocation = CGirderModelChildFrame::RightHarp;
   Update();
}

void CSectionCutDlgEx::OnUserCut() 
{
   m_CutLocation = CGirderModelChildFrame::UserInput;
   Update();
}

void CSectionCutDlgEx::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_SECTIONCUT );
}
