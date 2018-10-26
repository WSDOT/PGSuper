///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void DDX_ColumnGrid(CDataExchange* pDX,CColumnLayoutGrid& grid,CPierData2* pPier)
{
   if ( pDX->m_bSaveAndValidate )
   {
      grid.GetColumnData(*pPier);
   }
   else
   {
      grid.SetColumnData(*pPier);
   }
}

void DDV_ColumnGrid(CDataExchange* pDX,CColumnLayoutGrid& grid)
{
   if ( pDX->m_bSaveAndValidate )
   {
      if ( grid.GetRowCount() == 0 )
      {
         AfxMessageBox(_T("The pier must have at least one column"),MB_OK | MB_ICONEXCLAMATION);
         pDX->Fail();
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage property page

IMPLEMENT_DYNCREATE(CPierLayoutPage, CPropertyPage)

CPierLayoutPage::CPierLayoutPage() : CPropertyPage(CPierLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CPierLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   
   
   // only using the fixed option (no pinned at base of column,
   // it leads to unstable models before continuity is achieved)
   m_cbColumnFixity.SetFixityTypes(COLUMN_FIXITY_FIXED); 
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
   DDX_Control(pDX, IDC_FIXITY,       m_cbColumnFixity);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_MetaFileStatic(pDX, IDC_PIER_LAYOUT, m_LayoutPicture,_T("PIERLAYOUT"), _T("Metafile") );

   // Pier Model
   DDX_CBItemData(pDX,IDC_PIER_MODEL_TYPE,m_PierModelType);

   CConcreteMaterial& concrete = m_pPier->GetConcrete();
   DDX_UnitValueAndTag(pDX,IDC_FC,IDC_FC_UNIT,concrete.Fc,pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX,IDC_EC,IDC_EC_UNIT,concrete.Ec,pDisplayUnits->GetModEUnit());
   DDX_Check_Bool(pDX,IDC_EC_LABEL,concrete.bUserEc);

   DDX_UnitValueAndTag(pDX,IDC_H1,IDC_H1_UNIT,m_XBeamHeight[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_H2,IDC_H2_UNIT,m_XBeamTaperHeight[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X1,IDC_X1_UNIT,m_XBeamTaperLength[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X2,IDC_X2_UNIT,m_XBeamEndSlopeOffset[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_H3,IDC_H3_UNIT,m_XBeamHeight[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_H4,IDC_H4_UNIT,m_XBeamTaperHeight[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X3,IDC_X3_UNIT,m_XBeamTaperLength[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X4,IDC_X4_UNIT,m_XBeamEndSlopeOffset[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_W,IDC_W_UNIT,m_XBeamWidth,pDisplayUnits->GetSpanLengthUnit() );

   DDX_CBIndex(pDX,IDC_REFCOLUMN,m_RefColumnIdx);
   DDX_OffsetAndTag(pDX,IDC_REFCOLUMN_OFFSET,IDC_REFCOLUMN_OFFSET_UNIT,m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit() );
   DDX_CBItemData(pDX,IDC_REFCOLUMN_MEASUREMENT,m_TransverseOffsetMeasurement);

   DDX_UnitValueAndTag(pDX,IDC_X5,IDC_X5_UNIT,m_XBeamOverhang[pgsTypes::pstLeft], pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X6,IDC_X6_UNIT,m_XBeamOverhang[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );

   DDX_CBItemData(pDX,IDC_FIXITY,m_ColumnFixity);

   if ( m_PierModelType == pgsTypes::pmtPhysical )
   {
      DDV_ColumnGrid(pDX,m_ColumnLayoutGrid);
      DDX_ColumnGrid(pDX,m_ColumnLayoutGrid,m_pPier);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      CColumnData::ColumnHeightMeasurementType measure;
      DDX_CBItemData(pDX,IDC_HEIGHT_MEASURE,measure);
      ColumnIndexType nColumns = m_pPier->GetColumnCount();
      for ( ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++ )
      {
         CColumnData column = m_pPier->GetColumnData(colIdx);
         column.SetColumnHeightMeasurementType(measure);
         m_pPier->SetColumnData(colIdx,column);
      }
   }
   else
   {
      CColumnData::ColumnHeightMeasurementType measure = m_pPier->GetColumnData(0).GetColumnHeightMeasurementType();
      DDX_CBItemData(pDX,IDC_HEIGHT_MEASURE,measure);
   }

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
            m_pPier->SetXBeamDimensions(side,m_XBeamHeight[side],m_XBeamTaperHeight[side],m_XBeamTaperLength[side],m_XBeamEndSlopeOffset[side]);
            m_pPier->SetXBeamOverhang(side,m_XBeamOverhang[side]);
         }
         m_pPier->SetXBeamWidth(m_XBeamWidth);

         m_pPier->SetColumnFixity(m_ColumnFixity);

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
	ON_COMMAND(ID_HELP, OnHelp)
   ON_CBN_SELCHANGE(IDC_HEIGHT_MEASURE, OnHeightMeasureChanged)
   ON_BN_CLICKED(IDC_ADD_COLUMN, &CPierLayoutPage::OnAddColumn)
   ON_BN_CLICKED(IDC_REMOVE_COLUMN, &CPierLayoutPage::OnRemoveColumns)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage message handlers

BOOL CPierLayoutPage::OnInitDialog() 
{
   m_ColumnLayoutGrid.SubclassDlgItem(IDC_COLUMN_GRID, this);
   m_ColumnLayoutGrid.CustomInit();

   m_PierModelType = m_pPier->GetPierModelType();

   m_pPier->GetTransverseOffset(&m_RefColumnIdx,&m_TransverseOffset,&m_TransverseOffsetMeasurement);
   m_XBeamWidth = m_pPier->GetXBeamWidth();

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::PierSideType side = (pgsTypes::PierSideType)i;
      m_pPier->GetXBeamDimensions(side,&m_XBeamHeight[side],&m_XBeamTaperHeight[side],&m_XBeamTaperLength[side],&m_XBeamEndSlopeOffset[side]);
      m_XBeamOverhang[side] = m_pPier->GetXBeamOverhang(side);
   }

   m_ColumnFixity = m_pPier->GetColumnFixity();

   FillTransverseLocationComboBox();
   FillRefColumnComboBox(m_pPier->GetColumnCount());
   FillHeightMeasureComboBox();
   FillPierModelTypeComboBox();

   CPropertyPage::OnInitDialog();

   UpdateConcreteTypeLabel();
   if ( m_strUserEc == _T("") )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
   OnUserEc();

   OnPierModelTypeChanged();

   OnHeightMeasureChanged();

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
   CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_REFCOLUMN_MEASUREMENT);
   pcbMeasure->ResetContent();
   int idx = pcbMeasure->AddString(_T("from the Alignment"));
   pcbMeasure->SetItemData(idx,(DWORD_PTR)pgsTypes::omtAlignment);
   idx = pcbMeasure->AddString(_T("from the Bridgeline"));
   pcbMeasure->SetItemData(idx,(DWORD_PTR)pgsTypes::omtBridge);
}

void CPierLayoutPage::FillRefColumnComboBox(ColumnIndexType nColumns)
{
   CComboBox* pcbRefColumn = (CComboBox*)GetDlgItem(IDC_REFCOLUMN);
   int curSel = pcbRefColumn->GetCurSel();
   pcbRefColumn->ResetContent();
   if (nColumns == INVALID_INDEX)
   {
      nColumns = (ColumnIndexType)m_ColumnLayoutGrid.GetRowCount();
   }

   for ( ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++ )
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

void CPierLayoutPage::OnHeightMeasureChanged()
{
   CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
   int curSel = pcbHeightMeasure->GetCurSel();
   CColumnData::ColumnHeightMeasurementType measure = (CColumnData::ColumnHeightMeasurementType)(pcbHeightMeasure->GetItemData(curSel));
   m_ColumnLayoutGrid.SetHeightMeasurementType(measure);
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
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PIERDETAILS_LAYOUT );
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
   
   int nShow = (m_PierModelType == pgsTypes::pmtIdealized ? SW_HIDE : SW_SHOW);

   // enable/disable all the controls, except pier model type selector
   CWnd* pWnd = pcbPierModel->GetNextWindow(GW_HWNDNEXT);
   while ( pWnd )
   {
      int nID = pWnd->GetDlgCtrlID();
      if ( nID != IDC_PIER_MODEL_LABEL && nID != IDC_PIER_MODEL_TYPE )
      {
         if ( nID == IDC_EC_LABEL )
         {
            m_ctrlEcCheck.ShowWindow(nShow);
         }
         else
         {
            pWnd->ShowWindow(nShow);
         }
      }
      pWnd = pWnd->GetNextWindow(GW_HWNDNEXT);
   }
}

void CPierLayoutPage::OnAddColumn()
{
   m_ColumnLayoutGrid.AddColumn();
   FillRefColumnComboBox();
}

void CPierLayoutPage::OnRemoveColumns()
{
   m_ColumnLayoutGrid.RemoveSelectedColumns();
   FillRefColumnComboBox();
}
