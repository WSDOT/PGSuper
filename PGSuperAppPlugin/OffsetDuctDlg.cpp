// OffsetDuctDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "OffsetDuctDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

COffsetDuctDlg::COffsetDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,DuctIndexType ductIdx,CWnd* pParent /*=nullptr*/)
	: CDialog(COffsetDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
   m_DuctIdx = ductIdx;
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
   CString strTitle;
   strTitle.Format(_T("Offset Duct - Duct %d"), LABEL_DUCT(m_DuctIdx));
   SetWindowText(strTitle);

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