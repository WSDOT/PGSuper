// OffsetDuctDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "OffsetDuctDlg.h"

void DDX_DuctGeometry(CDataExchange* pDX,COffsetDuctGrid& grid,COffsetDuctGeometry& ductGeometry)
{
   if ( pDX->m_bSaveAndValidate )
   {
      ductGeometry = grid.GetData();
   }
   else
   {
      grid.SetData(ductGeometry);
   }
}

// COffsetDuctDlg dialog

IMPLEMENT_DYNAMIC(COffsetDuctDlg, CDialog)

COffsetDuctDlg::COffsetDuctDlg(CEditGirderlineDlg* pGdrDlg,CWnd* pParent /*=NULL*/)
	: CDialog(COffsetDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
}

COffsetDuctDlg::~COffsetDuctDlg()
{
}

void COffsetDuctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   if ( !pDX->m_bSaveAndValidate )
   {
      RefDuctIdx = m_DuctGeometry.RefDuctIdx;
   }

   DDX_CBIndex(pDX,IDC_REFERENCE_DUCT,(int&)RefDuctIdx);
   DDX_DuctGeometry(pDX,m_Grid,m_DuctGeometry); // when getting data from grid, RefDuctIdx gets messed up ...

   if ( pDX->m_bSaveAndValidate )
      m_DuctGeometry.RefDuctIdx = RefDuctIdx; // ... restore its value here
}


BEGIN_MESSAGE_MAP(COffsetDuctDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &COffsetDuctDlg::OnAddPoint)
   ON_BN_CLICKED(IDC_DELETE, &COffsetDuctDlg::OnDeletePoint)
END_MESSAGE_MAP()


// COffsetDuctDlg message handlers

BOOL COffsetDuctDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_GRID,this);
   m_Grid.CustomInit(this);

   int nDucts = m_pGirderlineDlg->GetDuctCount();
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_REFERENCE_DUCT);
   for ( int i = 0; i < nDucts-1; i++ )
   {
      CString str;
      str.Format(_T("%d"),i+1);
      pCB->AddString(str);
   }


   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void COffsetDuctDlg::OnDuctChanged()
{
}

void COffsetDuctDlg::OnAddPoint()
{
   m_Grid.AddPoint();
}

void COffsetDuctDlg::OnDeletePoint()
{
   m_Grid.DeletePoint();
}

void COffsetDuctDlg::EnableDeleteBtn(BOOL bEnable)
{
   GetDlgItem(IDC_DELETE)->EnableWindow(bEnable);
}