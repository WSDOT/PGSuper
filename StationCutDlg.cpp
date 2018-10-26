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

// StationCutDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "StationCutDlg.h"
#include <MfcTools\CogoDDX.h>
#include <UnitMgt\UnitMgt.h>
#include <ostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStationCutDlg dialog

CStationCutDlg::CStationCutDlg(Float64 value, Float64 lowerBound, Float64 upperBound, 
                               bool bSIUnits, CWnd* pParent):
CDialog(CStationCutDlg::IDD, pParent),
m_bSIUnits(bSIUnits),
m_Value(value),
m_LowerBound(lowerBound),
m_UpperBound(upperBound)
{
}


CStationCutDlg::CStationCutDlg(CWnd* pParent /*=NULL*/)
: CDialog(CStationCutDlg::IDD, pParent),
m_Value(0.0),
m_LowerBound(0.0),
m_UpperBound(0.0),
m_bSIUnits(true)
{
	//{{AFX_DATA_INIT(CStationCutDlg)
	//}}AFX_DATA_INIT
}

void CStationCutDlg::GetBounds(Float64* plowerBound, Float64* pupperBound)
{
   *plowerBound=m_LowerBound;
   *pupperBound=m_UpperBound;
}

void CStationCutDlg::SetBounds(Float64 lowerBound, Float64 upperBound)
{ 
   m_LowerBound = lowerBound;
   m_UpperBound = upperBound;
}


void CStationCutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStationCutDlg) 
	DDX_Station(pDX,IDC_VALUE,m_Value, m_bSIUnits,unitMeasure::Feet,unitMeasure::Meter);

	DDV_MinMaxDouble(pDX, m_Value, m_LowerBound, m_UpperBound);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStationCutDlg, CDialog)
	//{{AFX_MSG_MAP(CStationCutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStationCutDlg message handlers

BOOL CStationCutDlg::OnInitDialog() 
{
   USES_CONVERSION;

	CDialog::OnInitDialog();

   CStatic* pprompt = (CStatic*)GetDlgItem(IDC_STATION_RANGE);

   CComPtr<IStation> station;
   station.CoCreateInstance(CLSID_Station);
   station->put_Value(::ConvertFromSysUnits(m_LowerBound, m_bSIUnits ? unitMeasure::Meter : unitMeasure::Feet) );

   CComBSTR bstrLowerBound;
   station->AsString(m_bSIUnits ? umSI : umUS, &bstrLowerBound);

   station->put_Value(::ConvertFromSysUnits(m_UpperBound, m_bSIUnits ? unitMeasure::Meter : unitMeasure::Feet) );
   CComBSTR bstrUpperBound;
   station->AsString(m_bSIUnits ? umSI : umUS, &bstrUpperBound);


   CString strLabel;
   strLabel.Format(_T("Enter a station between %s and %s"),OLE2T(bstrLowerBound),OLE2T(bstrUpperBound));
   pprompt->SetWindowText(strLabel);

   
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStationCutDlg::SetValue(Float64 value)
{
   m_Value=value;
}

Float64 CStationCutDlg::GetValue()const
{
   return m_Value;
}

