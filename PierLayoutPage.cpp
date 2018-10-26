///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// PierLayoutPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "PierLayoutPage.h"
#include "PierDetailsDlg.h"
#include <PgsExt\ConcreteDetailsDlg.h>

#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>
#include "HtmlHelp\HelpTopics.hh"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage property page

IMPLEMENT_DYNCREATE(CPierLayoutPage, CPropertyPage)

CPierLayoutPage::CPierLayoutPage() : CPropertyPage(CPierLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CPierLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPierLayoutPage::~CPierLayoutPage()
{
}

void CPierLayoutPage::Init(CPierData2* pPier)
{
   m_pPier = pPier;

   m_PierIdx = pPier->GetIndex();
}

void CPierLayoutPage::DoDataExchange(CDataExchange* pDX)
{
     CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPierConnectionsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EC,           m_ctrlEc);
	DDX_Control(pDX, IDC_EC_LABEL,     m_ctrlEcCheck);
	DDX_Control(pDX, IDC_FC,           m_ctrlFc);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_MetaFileStatic(pDX, IDC_PIER_LAYOUT, m_LayoutPicture,_T("PIERLAYOUT"), _T("Metafile") );
   DDX_Control(pDX, IDC_S, m_SpacingControl);

   // Pier Model
   DDX_CBItemData(pDX,IDC_PIER_MODEL_TYPE,m_PierModelType);

   CConcreteMaterial& concrete = m_pPier->GetConcrete();
   DDX_UnitValueAndTag(pDX,IDC_FC,IDC_FC_UNIT,concrete.Fc,pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX,IDC_EC,IDC_EC_UNIT,concrete.Ec,pDisplayUnits->GetModEUnit());
   DDX_Check_Bool(pDX,IDC_EC_LABEL,concrete.bUserEc);

   DDX_UnitValueAndTag(pDX,IDC_H1,IDC_H1_UNIT,m_XBeamHeight[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_H2,IDC_H2_UNIT,m_XBeamTaperHeight[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X1,IDC_X1_UNIT,m_XBeamTaperLength[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_H3,IDC_H3_UNIT,m_XBeamHeight[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_H4,IDC_H4_UNIT,m_XBeamTaperHeight[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X2,IDC_X2_UNIT,m_XBeamTaperLength[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_W,IDC_W_UNIT,m_XBeamWidth,pDisplayUnits->GetSpanLengthUnit() );

   DDX_Text(pDX,IDC_COLUMN_COUNT,m_nColumns);
   DDX_CBIndex(pDX,IDC_REFCOLUMN,m_RefColumnIdx);
   DDX_OffsetAndTag(pDX,IDC_X5,IDC_X5_UNIT,m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit() );
   DDX_CBItemData(pDX,IDC_X5_MEASUREMENT,m_TransverseOffsetMeasurement);

   DDX_UnitValueAndTag(pDX,IDC_X3,IDC_X3_UNIT,m_XBeamOverhang[pgsTypes::pstLeft], pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X4,IDC_X4_UNIT,m_XBeamOverhang[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_S,IDC_S_UNIT,m_ColumnSpacing,pDisplayUnits->GetSpanLengthUnit());
   DDX_UnitValueAndTag(pDX,IDC_H,IDC_H_UNIT,m_ColumnHeight,pDisplayUnits->GetSpanLengthUnit());
   DDX_CBItemData(pDX,IDC_HEIGHT_MEASURE,m_ColumnHeightMeasurementType);

   DDX_CBItemData(pDX,IDC_COLUMN_SHAPE,m_ColumnShape);
   DDX_UnitValueAndTag(pDX,IDC_B,IDC_B_UNIT,m_B,pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_D,IDC_D_UNIT,m_D,pDisplayUnits->GetSpanLengthUnit() );

   if ( pDX->m_bSaveAndValidate )
   {
      // all of the data has been extracted from the dialog controls and it has been validated
      // set the values on the actual pier object
      m_pPier->SetPierModelType(m_PierModelType);

      if ( m_PierModelType == pgsTypes::pmtPhysical )
      {
         m_pPier->SetTransverseOffset(m_RefColumnIdx,m_TransverseOffset,m_TransverseOffsetMeasurement);
         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::PierSideType side = (pgsTypes::PierSideType)i;
            m_pPier->SetXBeamDimensions(side,m_XBeamHeight[side],m_XBeamTaperHeight[side],m_XBeamTaperLength[side]);
            m_pPier->SetXBeamOverhang(side,m_XBeamOverhang[side]);
         }
         m_pPier->SetXBeamWidth(m_XBeamWidth);

         m_pPier->SetColumnCount(m_nColumns);
         CColumnData columnData = m_pPier->GetColumnData(0);
         columnData.SetColumnHeight(m_ColumnHeight,m_ColumnHeightMeasurementType);
         columnData.SetColumnShape(m_ColumnShape);
         columnData.SetColumnDimensions(m_B,m_D);
         for ( ColumnIndexType colIdx = 0; colIdx < m_nColumns; colIdx++ )
         {
            m_pPier->SetColumnData(colIdx,columnData);
            if ( 0 < colIdx )
            {
               SpacingIndexType spaceIdx = (SpacingIndexType)(colIdx-1);
               m_pPier->SetColumnSpacing(spaceIdx,m_ColumnSpacing);
            }
         }

#pragma Reminder("WOKRING HERE - need to validate overall pier geometry")
         // maybe do this on the parent property page...
         // XBeam needs to be long enough to support all girders
         // XBeam needs to be as wide as columns(? not necessarily)
      }
   }
}

BEGIN_MESSAGE_MAP(CPierLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierLayoutPage)
   ON_BN_CLICKED(IDC_EC_LABEL,OnUserEc)
	ON_EN_CHANGE(IDC_FC, OnChangeFc)
   ON_BN_CLICKED(IDC_MORE_PROPERTIES, OnMoreProperties)
   ON_CBN_SELCHANGE(IDC_PIER_MODEL_TYPE, OnPierModelTypeChanged)
   ON_CBN_SELCHANGE(IDC_COLUMN_SHAPE, OnColumnShapeChanged)
   ON_NOTIFY(UDN_DELTAPOS, IDC_COLUMN_COUNT_SPINNER, OnColumnCountChanged)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage message handlers

BOOL CPierLayoutPage::OnInitDialog() 
{
   m_PierModelType = m_pPier->GetPierModelType();

   m_pPier->GetTransverseOffset(&m_RefColumnIdx,&m_TransverseOffset,&m_TransverseOffsetMeasurement);
   m_XBeamWidth = m_pPier->GetXBeamWidth();
   m_nColumns = m_pPier->GetColumnCount();

   // all columns are the same
   const CColumnData& columnData = m_pPier->GetColumnData(0); 
   m_ColumnHeightMeasurementType = columnData.GetColumnHeightMeasurementType();
   m_ColumnHeight = columnData.GetColumnHeight();
   m_ColumnSpacing = ::ConvertToSysUnits(10,unitMeasure::Feet);

   m_ColumnShape = columnData.GetColumnShape();
   columnData.GetColumnDimensions(&m_B,&m_D);

   if ( 1 < m_nColumns )
   {
      m_ColumnSpacing = m_pPier->GetColumnSpacing(0);
   }

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierSideType side = (pgsTypes::PierSideType)i;
      m_pPier->GetXBeamDimensions(side,&m_XBeamHeight[side],&m_XBeamTaperHeight[side],&m_XBeamTaperLength[side]);
      m_XBeamOverhang[side] = m_pPier->GetXBeamOverhang(side);
   }

   FillTransverseLocationComboBox();
   FillRefColumnComboBox();
   FillHeightMeasureComboBox();
   FillColumnShapeComboBox();
   FillPierModelTypeComboBox();

   CSpinButtonCtrl* pSpinner = (CSpinButtonCtrl*)GetDlgItem(IDC_COLUMN_COUNT_SPINNER);
   pSpinner->SetRange(1,UD_MAXVAL);

   CPropertyPage::OnInitDialog();

   UpdateConcreteTypeLabel();
   if ( m_strUserEc == _T("") )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
   OnUserEc();

   OnPierModelTypeChanged();
   OnColumnShapeChanged();
   UpdateColumnSpacingControls();
   UpdateEcControls();



   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierLayoutPage::UpdateConcreteTypeLabel()
{
   CString strLabel;
   switch( m_pPier->GetConcrete().Type )
   {
   case pgsTypes::Normal:
      strLabel = _T("Normal Weight Concrete");
      break;

   case pgsTypes::AllLightweight:
      strLabel = _T("All Lightweight Concrete");
      break;

   case pgsTypes::SandLightweight:
      strLabel = _T("Sand Lightweight Concrete");
      break;

   default:
      ATLASSERT(false); // should never get here
      strLabel = _T("Concrete Type Label Error");
   }

   GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText(strLabel);
}

void CPierLayoutPage::FillTransverseLocationComboBox()
{
   CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_X5_MEASUREMENT);
   pcbMeasure->ResetContent();
   int idx = pcbMeasure->AddString(_T("from the Alignment"));
   pcbMeasure->SetItemData(idx,(DWORD_PTR)pgsTypes::omtAlignment);
   idx = pcbMeasure->AddString(_T("from the Bridgeline"));
   pcbMeasure->SetItemData(idx,(DWORD_PTR)pgsTypes::omtBridge);
}

void CPierLayoutPage::FillRefColumnComboBox()
{
   CComboBox* pcbRefColumn = (CComboBox*)GetDlgItem(IDC_REFCOLUMN);
   int curSel = pcbRefColumn->GetCurSel();
   pcbRefColumn->ResetContent();
   for ( ColumnIndexType colIdx = 0; colIdx < m_nColumns; colIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Column %d"),LABEL_COLUMN(colIdx));
      pcbRefColumn->AddString(strLabel);
   }

   if ( pcbRefColumn->SetCurSel(curSel) == CB_ERR )
   {
      pcbRefColumn->SetCurSel(0);
   }
}

void CPierLayoutPage::FillHeightMeasureComboBox()
{
   CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
   pcbHeightMeasure->ResetContent();
   int idx = pcbHeightMeasure->AddString(_T("Column Height (H)"));
   pcbHeightMeasure->SetItemData(idx,(DWORD_PTR)CColumnData::chtHeight);
   idx = pcbHeightMeasure->AddString(_T("Bottom Elevation"));
   pcbHeightMeasure->SetItemData(idx,(DWORD_PTR)CColumnData::chtBottomElevation);
}

void CPierLayoutPage::FillColumnShapeComboBox()
{
   CComboBox* pcbColumnShape = (CComboBox*)GetDlgItem(IDC_COLUMN_SHAPE);
   pcbColumnShape->ResetContent();
   int idx = pcbColumnShape->AddString(_T("Circle"));
   pcbColumnShape->SetItemData(idx,(DWORD_PTR)CColumnData::cstCircle);
   idx = pcbColumnShape->AddString(_T("Rectangle"));
   pcbColumnShape->SetItemData(idx,(DWORD_PTR)CColumnData::cstRectangle);
}

void CPierLayoutPage::FillPierModelTypeComboBox()
{
   CComboBox* pcbPierModel = (CComboBox*)GetDlgItem(IDC_PIER_MODEL_TYPE);
   pcbPierModel->ResetContent();

   int idx = pcbPierModel->AddString(_T("Idealized"));
   pcbPierModel->SetItemData(idx,(DWORD_PTR)pgsTypes::pmtIdealized);

   idx = pcbPierModel->AddString(_T("Physical"));
   pcbPierModel->SetItemData(idx,(DWORD_PTR)pgsTypes::pmtPhysical);
}

void CPierLayoutPage::OnHelp() 
{
#pragma Reminder("UPDATE: Update the help context id")
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_CONNECTIONS );
}

void CPierLayoutPage::OnUserEc()
{
   BOOL bEnable = m_ctrlEcCheck.GetCheck();

   GetDlgItem(IDC_EC_LABEL)->EnableWindow(TRUE);

   if (bEnable==FALSE)
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
      UpdateEc();
   }
   else
   {
      m_ctrlEc.SetWindowText(m_strUserEc);
   }

   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}

void CPierLayoutPage::OnMoreProperties()
{
   UpdateData(TRUE);
   CConcreteDetailsDlg dlg(true/*f'c*/,false/*don't enable Compute Time Parameters option*/);
   CConcreteMaterial& concrete = m_pPier->GetConcrete();

   dlg.m_fc28 = concrete.Fc;
   dlg.m_Ec28 = concrete.Ec;
   dlg.m_bUserEc28 = concrete.bUserEc;

   dlg.m_General.m_Type    = concrete.Type;
   dlg.m_General.m_AggSize = concrete.MaxAggregateSize;
   dlg.m_General.m_Ds      = concrete.StrengthDensity;
   dlg.m_General.m_Dw      = concrete.WeightDensity;
   dlg.m_General.m_strUserEc  = m_strUserEc;

   dlg.m_AASHTO.m_EccK1       = concrete.EcK1;
   dlg.m_AASHTO.m_EccK2       = concrete.EcK2;
   dlg.m_AASHTO.m_CreepK1     = concrete.CreepK1;
   dlg.m_AASHTO.m_CreepK2     = concrete.CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = concrete.ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = concrete.ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = concrete.bHasFct;
   dlg.m_AASHTO.m_Fct         = concrete.Fct;

   if ( dlg.DoModal() == IDOK )
   {
      concrete.Fc               = dlg.m_fc28;
      concrete.Ec               = dlg.m_Ec28;
      concrete.bUserEc          = dlg.m_bUserEc28;

      concrete.Type             = dlg.m_General.m_Type;
      concrete.MaxAggregateSize = dlg.m_General.m_AggSize;
      concrete.StrengthDensity  = dlg.m_General.m_Ds;
      concrete.WeightDensity    = dlg.m_General.m_Dw;
      concrete.EcK1             = dlg.m_AASHTO.m_EccK1;
      concrete.EcK2             = dlg.m_AASHTO.m_EccK2;
      concrete.CreepK1          = dlg.m_AASHTO.m_CreepK1;
      concrete.CreepK2          = dlg.m_AASHTO.m_CreepK2;
      concrete.ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      concrete.ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      concrete.bHasFct          = dlg.m_AASHTO.m_bHasFct;
      concrete.Fct              = dlg.m_AASHTO.m_Fct;

      m_strUserEc  = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      UpdateData(FALSE);
      OnUserEc();
      UpdateConcreteTypeLabel();
   }
}

void CPierLayoutPage::OnChangeFc() 
{
   UpdateEc();
}

void CPierLayoutPage::UpdateEc()
{
   // update modulus
   if (m_ctrlEcCheck.GetCheck() == 0)
   {
      // blank out ec
      CString strEc;
      m_ctrlEc.SetWindowText(strEc);

      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      strDensity.Format(_T("%s"),FormatDimension(m_pPier->GetConcrete().StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),m_pPier->GetConcrete().EcK1);
      strK2.Format(_T("%f"),m_pPier->GetConcrete().EcK2);

      strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CPierLayoutPage::OnPierModelTypeChanged()
{
   CComboBox* pcbPierModel = (CComboBox*)GetDlgItem(IDC_PIER_MODEL_TYPE);
   int curSel = pcbPierModel->GetCurSel();
   m_PierModelType = (pgsTypes::PierModelType)pcbPierModel->GetItemData(curSel);
   
   BOOL bEnable = (m_PierModelType == pgsTypes::pmtIdealized ? FALSE : TRUE);

   // enable/disable all the controls, except pier model type selector
   CWnd* pWnd = pcbPierModel->GetNextWindow(GW_HWNDNEXT);
   while ( pWnd )
   {
      int nID = pWnd->GetDlgCtrlID();
      if ( nID != IDC_PIER_MODEL_LABEL && nID != IDC_PIER_MODEL_TYPE )
      {
         if ( nID == IDC_S )
         {
            m_SpacingControl.EnableWindow(bEnable);
         }
         else if ( nID == IDC_EC_LABEL )
         {
            m_ctrlEcCheck.EnableWindow(bEnable);
         }
         else
         {
            pWnd->EnableWindow(bEnable);
         }
      }
      pWnd = pWnd->GetNextWindow(GW_HWNDNEXT);
   }

   UpdateColumnSpacingControls(); // the blanket enable/disable messes up the column spacing controls... fix it 
   UpdateEcControls(); // the blanket enable/disable messes up the modulus of elasticity controls... fix it
}

void CPierLayoutPage::OnColumnShapeChanged()
{
   CComboBox* pcbColumnShape = (CComboBox*)GetDlgItem(IDC_COLUMN_SHAPE);
   int curSel = pcbColumnShape->GetCurSel();
   CColumnData::ColumnShapeType shapeType = (CColumnData::ColumnShapeType)pcbColumnShape->GetItemData(curSel);
   if ( shapeType == CColumnData::cstCircle )
   {
      GetDlgItem(IDC_B_LABEL)->SetWindowText(_T("D"));
      GetDlgItem(IDC_D_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_D)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_D_UNIT)->ShowWindow(SW_HIDE);
   }
   else
   {
      GetDlgItem(IDC_B_LABEL)->SetWindowText(_T("B"));
      GetDlgItem(IDC_D_LABEL)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_D)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_D_UNIT)->ShowWindow(SW_SHOW);
   }
}

void CPierLayoutPage::OnColumnCountChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   // this is what the count will be
   int new_count = pNMUpDown->iPos + pNMUpDown->iDelta;

   m_nColumns = new_count;

   *pResult = 0;

   FillRefColumnComboBox();
   UpdateColumnSpacingControls();
}

void CPierLayoutPage::UpdateColumnSpacingControls()
{
   BOOL bEnable = (m_PierModelType == pgsTypes::pmtPhysical && 1 < m_nColumns ? TRUE : FALSE);
   GetDlgItem(IDC_S_LABEL)->EnableWindow(bEnable);
   m_SpacingControl.EnableWindow(bEnable);
   GetDlgItem(IDC_S_UNIT)->EnableWindow(bEnable);
}

void CPierLayoutPage::UpdateEcControls()
{
   BOOL bEnable = (m_PierModelType == pgsTypes::pmtPhysical && IsDlgButtonChecked(IDC_EC_LABEL)) == BST_CHECKED ? TRUE : FALSE;
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}
