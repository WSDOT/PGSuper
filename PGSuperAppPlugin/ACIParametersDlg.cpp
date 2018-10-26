///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// ACIParametersDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "ACIParametersDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <Material\ACI209Concrete.h>


// CACIParametersDlg dialog

IMPLEMENT_DYNAMIC(CACIParametersDlg, CDialog)

CACIParametersDlg::CACIParametersDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CACIParametersDlg::IDD, pParent)
{
   m_fc1 = ::ConvertToSysUnits(4.0,unitMeasure::KSI);
   m_fc2 = ::ConvertToSysUnits(8.0,unitMeasure::KSI);
   m_t1 = ::ConvertToSysUnits(1.0,unitMeasure::Day);
}

CACIParametersDlg::~CACIParametersDlg()
{
}

void CACIParametersDlg::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CDialog::DoDataExchange(pDX);
   DDX_UnitValueAndTag( pDX, IDC_FCI, IDC_FCI_UNIT, m_fc1 , pDisplayUnits->GetStressUnit() );
   DDX_UnitValueAndTag( pDX, IDC_FC,  IDC_FC_UNIT,  m_fc2 , pDisplayUnits->GetStressUnit() );
}


BEGIN_MESSAGE_MAP(CACIParametersDlg, CDialog)
	ON_EN_CHANGE(IDC_FCI, UpdateParameters)
	ON_EN_CHANGE(IDC_FC, UpdateParameters)
END_MESSAGE_MAP()


BOOL CACIParametersDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strLabel;
   strLabel.Format(_T("Concrete Strength at t = %s, Time of Initial Loading"),::FormatDimension(m_t1,pDisplayUnits->GetWholeDaysUnit()));
   GetDlgItem(IDC_T1)->SetWindowText(strLabel);

   UpdateParameters();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

// CACIParametersDlg message handlers
void CACIParametersDlg::UpdateParameters()
{
   UpdateData();

   Float64 t1 = ::ConvertFromSysUnits(m_t1,unitMeasure::Day);
   matACI209Concrete::ComputeParameters(m_fc1,t1,m_fc2,28,&m_A,&m_B);

   m_A = ::ConvertToSysUnits(m_A,unitMeasure::Day);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strResult;
   strResult.Format(_T("a = %s, Beta = %4.2f\r\nFor use in Eq'n. 2-1"),::FormatDimension(m_A,pDisplayUnits->GetFractionalDaysUnit()),m_B);
   GetDlgItem(IDC_RESULT)->SetWindowText(strResult);
}
