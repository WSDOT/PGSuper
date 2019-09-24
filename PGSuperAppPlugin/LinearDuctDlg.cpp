// LinearDuctDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LinearDuctDlg.h"
#include "SplicedGirderDescDlg.h"

#include <IFace\Bridge.h>

#include <EAF\EAFDocument.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void DDX_DuctGeometry(CDataExchange* pDX,CLinearDuctGrid& grid,CLinearDuctGeometry& ductGeometry)
{
   if ( pDX->m_bSaveAndValidate )
   {
      ductGeometry = grid.GetData();
#if defined _DEBUG
      CLinearDuctGeometry::MeasurementType measurementType;
      DDX_CBEnum(pDX,IDC_LOCATION,measurementType);
      ATLASSERT(measurementType == ductGeometry.GetMeasurementType());
#endif
   }
   else
   {
      CLinearDuctGeometry::MeasurementType measurementType = ductGeometry.GetMeasurementType();
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
   Float64 Xg_Last = 0;
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
         if (location < -1.0)
         {
            CString strMsg;
            strMsg.Format(_T("Enter a location between 0%% and 100%% for duct point %d"), LABEL_INDEX(pntIdx));
            AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }

         dXg *= -Lg;
      }

      if ( measurementType == CLinearDuctGeometry::FromPrevious )
      {
         Xg += dXg;
      }
      else
      {
         Xg = dXg; // location was measured from start of girder

         if (Xg < Xg_Last)
         {
            CString strMsg;
            strMsg.Format(_T("Duct point %d is before duct point %d. Adjust duct geometry."), LABEL_INDEX(pntIdx), LABEL_INDEX(pntIdx-1));
            AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }
         
         Xg_Last = Xg;
      }

      if ( ::IsLT(Lg,Xg) )
      {
         GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
         CString strMsg;
         strMsg.Format(_T("Duct point %d is beyond the end of the girder. The girder length is %s, Adjust duct geometry."), LABEL_INDEX(pntIdx), FormatDimension(Lg, pDisplayUnits->GetSpanLengthUnit()));
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         pDX->Fail();
      }
   }
}

// CLinearDuctDlg dialog

IMPLEMENT_DYNAMIC(CLinearDuctDlg, CDialog)

CLinearDuctDlg::CLinearDuctDlg(CSplicedGirderGeneralPage* pGdrDlg, CPTData* pPTData,DuctIndexType ductIdx,CWnd* pParent /*=nullptr*/)
	: CDialog(CLinearDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
   m_PTData = *pPTData;
   m_PTData.SetGirder(m_pGirderlineDlg->GetGirder(), false/*don't initialize pt data*/);
   m_DuctIdx = ductIdx;
}

CLinearDuctDlg::~CLinearDuctDlg()
{
}

void CLinearDuctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_DuctGeometry(pDX,m_Grid, m_PTData.GetDuct(m_DuctIdx)->LinearDuctGeometry);
   DDV_DuctGeometry(pDX,GetGirderKey(), m_PTData.GetDuct(m_DuctIdx)->LinearDuctGeometry);
}


BEGIN_MESSAGE_MAP(CLinearDuctDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &CLinearDuctDlg::OnAddPoint)
   ON_BN_CLICKED(IDC_DELETE, &CLinearDuctDlg::OnDeletePoint)
   ON_BN_CLICKED(ID_HELP,&CLinearDuctDlg::OnHelp)
   ON_CBN_DROPDOWN(IDC_LOCATION, &CLinearDuctDlg::OnMeasurementTypeChanging )
   ON_CBN_SELCHANGE(IDC_LOCATION, &CLinearDuctDlg::OnMeasurementTypeChanged )
   ON_BN_CLICKED(IDC_SCHEMATIC, &CLinearDuctDlg::OnSchematicButton)
END_MESSAGE_MAP()


// CLinearDuctDlg message handlers

BOOL CLinearDuctDlg::OnInitDialog()
{
   CString strTitle;
   strTitle.Format(_T("Linear Duct - Duct %d"), LABEL_DUCT(m_DuctIdx));
   SetWindowText(strTitle);

   m_Grid.SubclassDlgItem(IDC_POINT_GRID,this);
   m_Grid.CustomInit(this);
   m_Grid.SetData(m_PTData.GetDuct(m_DuctIdx)->LinearDuctGeometry);

      // subclass the schematic drawing of the tendons
   m_DrawTendons.SubclassDlgItem(IDC_TENDONS, this);
   m_DrawTendons.CustomInit(m_pGirderlineDlg->GetGirder()->GetGirderKey(), m_pGirderlineDlg->GetGirder(), &m_PTData);
   m_DrawTendons.SetDuct(m_DuctIdx);
   m_DrawTendons.SetMapMode(m_pGirderlineDlg->GetTendonControlMapMode());

   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_LOCATION);
   pcbLocation->AddString(_T("left end of girder"));
   pcbLocation->AddString(_T("the previous point"));

   CDialog::OnInitDialog();

   EnableDeleteBtn(FALSE);

   HINSTANCE hInstance = AfxGetInstanceHandle();
   CButton* pSchematicBtn = (CButton*)GetDlgItem(IDC_SCHEMATIC);
   pSchematicBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_SCHEMATIC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));

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
   m_PTData.GetDuct(m_DuctIdx)->LinearDuctGeometry = m_Grid.GetData(); // must update the PTData so the tendon control is drawing the most current data

   //m_pGirderlineDlg->OnDuctChanged();
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}

void CLinearDuctDlg::OnMeasurementTypeChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOCATION);
   m_PrevMeasurmentTypeIdx = pCB->GetCurSel();
}

void CLinearDuctDlg::OnMeasurementTypeChanged()
{
   //if ( UpdateData() ) // update data first to make sure the grid has valid data in it before converting it to another basis
   //{
      m_Grid.SetMeasurementType(GetMeasurementType());
   //}
   //else
   //{
   //   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_LOCATION);
   //   pCB->SetCurSel(m_PrevMeasurmentTypeIdx);
   //}
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

const CLinearDuctGeometry& CLinearDuctDlg::GetDuctGeometry() const
{
   return m_PTData.GetDuct(m_DuctIdx)->LinearDuctGeometry;
}

void CLinearDuctDlg::OnSchematicButton()
{
   auto mm = m_DrawTendons.GetMapMode();
   mm = (mm == grlibPointMapper::Isotropic ? grlibPointMapper::Anisotropic : grlibPointMapper::Isotropic);
   m_DrawTendons.SetMapMode(mm);
}

grlibPointMapper::MapMode CLinearDuctDlg::GetTendonControlMapMode() const
{
   return m_DrawTendons.GetMapMode();
}