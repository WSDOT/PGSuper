///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include "ACIParametersDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <Materials/ACI209Concrete.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CACIParametersDlg dialog

IMPLEMENT_DYNAMIC(CACIParametersDlg, CDialog)

CACIParametersDlg::CACIParametersDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CACIParametersDlg::IDD, pParent)
{
   m_fc1 = WBFL::Units::ConvertToSysUnits(4.0,WBFL::Units::Measure::KSI);
   m_fc2 = WBFL::Units::ConvertToSysUnits(8.0,WBFL::Units::Measure::KSI);
   m_t1 = WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Day);
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

   Float64 t1 = WBFL::Units::ConvertFromSysUnits(m_t1,WBFL::Units::Measure::Day);
   WBFL::Materials::ACI209Concrete::ComputeParameters(m_fc1,t1,m_fc2,28,&m_A,&m_B);

   m_A = WBFL::Units::ConvertToSysUnits(m_A,WBFL::Units::Measure::Day);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strResult;
   strResult.Format(_T("a = %s, Beta = %4.2f\r\nFor use in Eq'n. 2-1"),::FormatDimension(m_A,pDisplayUnits->GetFractionalDaysUnit()),m_B);
   GetDlgItem(IDC_RESULT)->SetWindowText(strResult);
}
