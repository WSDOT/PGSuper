// LinearDuctDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LinearDuctDlg.h"
#include "SplicedGirderDescDlg.h"

#include <IFace\Bridge.h>

#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void DDX_DuctGeometry(CDataExchange* pDX,CLinearDuctGrid& grid,CLinearDuctGeometry& ductGeometry)
{
   CLinearDuctGeometry::MeasurementType measurementType = ductGeometry.GetMeasurementType();
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_CBEnum(pDX,IDC_LOCATION,measurementType);
      grid.GetData(ductGeometry);
      ductGeometry.SetMeasurementType(measurementType);
   }
   else
   {
      DDX_CBEnum(pDX,IDC_LOCATION,measurementType);
      grid.SetData(ductGeometry);
   }
}

void DDV_DuctGeometry(CDataExchange* pDX,const CGirderKey& girderKey,CLinearDuctGeometry& ductGeometry)
{
   if ( !pDX->m_bSaveAndValidate )
   {
      return;
   }

   pDX->PrepareCtrl(IDC_POINT_GRID);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(girderKey);

   CLinearDuctGeometry::MeasurementType measurementType = ductGeometry.GetMeasurementType();

   Float64 Xg = 0;
   CollectionIndexType nPoints = ductGeometry.GetPointCount();
   for ( CollectionIndexType pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      Float64 location, offset;
      CDuctGeometry::OffsetType offsetType;
      ductGeometry.GetPoint(pntIdx,&location,&offset,&offsetType);

      if ( offset <= 0.0 )
      {
         CString strMsg(_T("The tendon offset must be greater than zero."));
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         pDX->Fail();
      }


      Float64 dXg = location;
      if ( location < 0 )
      {
         // location is fractional length of girder
         dXg *= -Lg;
      }

      if ( measurementType == CLinearDuctGeometry::FromPrevious )
      {
         Xg += dXg;
      }
      else
      {
         Xg = dXg; // location was measured from start of girder
      }

      if ( Lg < Xg )
      {
         CString strMsg;
         strMsg.Format(_T("Duct point %d is beyond the end of the girder. Adjust duct point location."),(pntIdx+1));
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         pDX->Fail();
      }
   }
}

// CLinearDuctDlg dialog

IMPLEMENT_DYNAMIC(CLinearDuctDlg, CDialog)

CLinearDuctDlg::CLinearDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CWnd* pParent /*=nullptr*/)
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
   DDV_DuctGeometry(pDX,GetGirderKey(),m_DuctGeometry);
}


BEGIN_MESSAGE_MAP(CLinearDuctDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &CLinearDuctDlg::OnAddPoint)
   ON_BN_CLICKED(IDC_DELETE, &CLinearDuctDlg::OnDeletePoint)
   ON_BN_CLICKED(ID_HELP,&CLinearDuctDlg::OnHelp)
   ON_CBN_DROPDOWN(IDC_LOCATION, &CLinearDuctDlg::OnMeasurementTypeChanging )
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

void CLinearDuctDlg::OnMeasurementTypeChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOCATION);
   m_PrevMeasurmentTypeIdx = pCB->GetCurSel();
}

void CLinearDuctDlg::OnMeasurementTypeChanged()
{
   if ( UpdateData() ) // update data first to make sure the grid has valid data in it before converting it to another basis
   {
      m_Grid.SetMeasurementType(GetMeasurementType());
   }
   else
   {
      CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOCATION);
      pCB->SetCurSel(m_PrevMeasurmentTypeIdx);
   }
}

void CLinearDuctDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_LINEAR_DUCT);
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
