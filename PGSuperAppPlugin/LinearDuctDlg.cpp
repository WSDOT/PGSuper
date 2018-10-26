// LinearDuctDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LinearDuctDlg.h"

void DDX_DuctGeometry(CDataExchange* pDX,CLinearDuctGrid& grid,CLinearDuctGeometry& ductGeometry)
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

// CLinearDuctDlg dialog

IMPLEMENT_DYNAMIC(CLinearDuctDlg, CDialog)

CLinearDuctDlg::CLinearDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CWnd* pParent /*=NULL*/)
	: CDialog(CLinearDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
}

CLinearDuctDlg::~CLinearDuctDlg()
{
}

void CLinearDuctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_DuctGeometry(pDX,m_Grid,m_DuctGeometry);
}


BEGIN_MESSAGE_MAP(CLinearDuctDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &CLinearDuctDlg::OnAddPoint)
   ON_BN_CLICKED(IDC_DELETE, &CLinearDuctDlg::OnDeletePoint)
END_MESSAGE_MAP()


// CLinearDuctDlg message handlers

BOOL CLinearDuctDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_POINT_GRID,this);
   m_Grid.CustomInit(this);

   CDialog::OnInitDialog();

   EnableDeleteBtn(FALSE);

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLinearDuctDlg::OnAddPoint()
{
   m_Grid.AddPoint();
}

void CLinearDuctDlg::OnDeletePoint()
{
   m_Grid.DeletePoint();
}

void CLinearDuctDlg::OnDuctChanged()
{
   m_pGirderlineDlg->OnDuctChanged();
}

void CLinearDuctDlg::EnableDeleteBtn(BOOL bEnable)
{
   GetDlgItem(IDC_DELETE)->EnableWindow(bEnable);
}