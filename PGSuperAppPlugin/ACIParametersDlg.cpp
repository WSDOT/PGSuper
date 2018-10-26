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
   m_t2 = ::ConvertToSysUnits(28.0,unitMeasure::Day);
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
   strLabel.Format(_T("Concrete Strength at t = %s, Time of Initial Loading"),::FormatDimension(m_t1,pDisplayUnits->GetLongTimeUnit()));
   GetDlgItem(IDC_T1)->SetWindowText(strLabel);

   UpdateParameters();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

// CACIParametersDlg message handlers
void CACIParametersDlg::UpdateParameters()
{
   UpdateData();

   matACI209Concrete::ComputeParameters(m_fc1,m_t1,m_fc2,m_t2,&m_A,&m_B);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strResult;
   strResult.Format(_T("A = %s, B = %4.2f"),::FormatDimension(m_A,pDisplayUnits->GetLongTimeUnit()),m_B);
   GetDlgItem(IDC_RESULT)->SetWindowText(strResult);
}
