// LinearDuctDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LinearDuctDlg.h"
#include "SplicedGirderDescDlg.h"

void DDX_DuctGeometry(CDataExchange* pDX,CLinearDuctGrid& grid,CLinearDuctGeometry& ductGeometry)
{
   CLinearDuctGeometry::MeasurementType measurementType = ductGeometry.GetMeasurementType();
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_CBEnum(pDX,IDC_LOCATION,measurementType);
      ductGeometry.SetMeasurementType(measurementType);
      grid.GetData(ductGeometry);
   }
   else
   {
      DDX_CBEnum(pDX,IDC_LOCATION,measurementType);
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
   ON_BN_CLICKED(ID_HELP,&CLinearDuctDlg::OnHelp)
   ON_CBN_SELCHANGE(IDC_LOCATION, &CLinearDuctDlg::OnMeasurementTypeChanged )
END_MESSAGE_MAP()


// CLinearDuctDlg message handlers

BOOL CLinearDuctDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_POINT_GRID,this);
   m_Grid.CustomInit(this);

   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_LOCATION);
   pcbLocation->AddString(_T("left end of girder"));
   pcbLocation->AddString(_T("the previous point"));

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
   // tells the calling dialog that the duct changed
   // so that it can update the graphics
   m_pGirderlineDlg->OnDuctChanged();
}

void CLinearDuctDlg::OnMeasurementTypeChanged()
{
   m_Grid.SetMeasurementType(GetMeasurementType());
}

void CLinearDuctDlg::OnHelp()
{
#pragma Reminder("IMPLEMENT: OnHelp")
   AfxMessageBox(_T("Implement Help Topic"));
}

CLinearDuctGeometry::MeasurementType CLinearDuctDlg::GetMeasurementType()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_LOCATION);
   CLinearDuctGeometry::MeasurementType mt = (CLinearDuctGeometry::MeasurementType)pcbLocation->GetCurSel();
   return mt;
}

void CLinearDuctDlg::EnableDeleteBtn(BOOL bEnable)
{
   GetDlgItem(IDC_DELETE)->EnableWindow(bEnable);
}

const CGirderKey& CLinearDuctDlg::GetGirderKey() const
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)m_pGirderlineDlg->GetParent();
   return pParent->GetGirderKey();
}
