// ParabolicDuctDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "ParabolicDuctDlg.h"


void DDX_DuctGeometry(CDataExchange* pDX,CParabolicDuctGrid& grid,CParabolicDuctGeometry& ductGeometry)
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

// CParabolicDuctDlg dialog

IMPLEMENT_DYNAMIC(CParabolicDuctDlg, CDialog)

CParabolicDuctDlg::CParabolicDuctDlg(CEditGirderlineDlg* pGdrDlg,CWnd* pParent /*=NULL*/)
	: CDialog(CParabolicDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
}

CParabolicDuctDlg::~CParabolicDuctDlg()
{
}

void CParabolicDuctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_DuctGeometry(pDX,m_Grid,m_DuctGeometry);
}


BEGIN_MESSAGE_MAP(CParabolicDuctDlg, CDialog)
END_MESSAGE_MAP()


// CParabolicDuctDlg message handlers

BOOL CParabolicDuctDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_POINT_GRID,this);
   m_Grid.CustomInit(this);

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void CParabolicDuctDlg::OnDuctChanged()
{
   m_pGirderlineDlg->OnDuctChanged();
}
