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

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_MetaFileStatic(pDX, IDC_PIER_LAYOUT, m_LayoutPicture,_T("PIERLAYOUT"), _T("Metafile") );
   DDX_Control(pDX, IDC_S, m_SpacingControl);

   // Pier Model
   DDX_CBItemData(pDX,IDC_PIER_MODEL_TYPE,m_PierModelType);

   DDX_UnitValueAndTag(pDX,IDC_EC,IDC_EC_UNIT,m_Ec,pDisplayUnits->GetModEUnit());

   // Transverse location of the pier
   DDX_UnitValueAndTag(pDX,IDC_X5,IDC_X5_UNIT,m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit() );
   DDX_CBItemData(pDX,IDC_X5_MEASUREMENT,m_TransverseOffsetMeasurement);

   DDX_UnitValueAndTag(pDX,IDC_H1,IDC_H1_UNIT,m_XBeamHeight[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_H2,IDC_H2_UNIT,m_XBeamTaperHeight[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X1,IDC_X1_UNIT,m_XBeamTaperLength[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_H3,IDC_H3_UNIT,m_XBeamHeight[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_H4,IDC_H4_UNIT,m_XBeamTaperHeight[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X2,IDC_X2_UNIT,m_XBeamTaperLength[pgsTypes::pstRight],pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_W,IDC_W_UNIT,m_XBeamWidth,pDisplayUnits->GetSpanLengthUnit() );

   DDX_UnitValueAndTag(pDX,IDC_X3,IDC_X3_UNIT,m_XBeamOverhang[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );
   DDX_UnitValueAndTag(pDX,IDC_X4,IDC_X4_UNIT,m_XBeamOverhang[pgsTypes::pstLeft],pDisplayUnits->GetSpanLengthUnit() );

   DDX_Text(pDX,IDC_COLUMN_COUNT,m_nColumns);
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

         if ( 1 < m_nColumns )
         {
            m_pPier->SetColumnCount(m_nColumns);
            CColumnData columnData = m_pPier->GetColumnData(0);
            columnData.SetColumnHeight(m_ColumnHeight,m_ColumnHeightMeasurementType);
            columnData.SetColumnShape(m_ColumnShape);
            columnData.SetColumnDimensions(m_B,m_D);
            for ( ColumnIndexType colIdx = 0; colIdx < m_nColumns; colIdx++ )
            {
               m_pPier->SetColumnData(colIdx,columnData);
               if ( 1 < colIdx )
               {
                  SpacingIndexType spaceIdx = (SpacingIndexType)(colIdx-1);
                  m_pPier->SetColumnSpacing(spaceIdx,m_ColumnSpacing);
               }
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
   m_Ec = m_pPier->GetModE();

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

   OnPierModelTypeChanged();
   OnColumnShapeChanged();
   UpdateColumnSpacingControls();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
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
         else
         {
            pWnd->EnableWindow(bEnable);
         }
      }
      pWnd = pWnd->GetNextWindow(GW_HWNDNEXT);
   }

   UpdateColumnSpacingControls(); // the blanket enable/disable messes up the column spacing controls... fix it 
}

void CPierLayoutPage::OnColumnShapeChanged()
{
   CComboBox* pcbColumnShape = (CComboBox*)GetDlgItem(IDC_COLUMN_SHAPE);
   int curSel = pcbColumnShape->GetCurSel();
   CColumnData::ColumnShapeType shapeType = (CColumnData::ColumnShapeType)pcbColumnShape->GetItemData(curSel);
   if ( shapeType == CColumnData::cstCircle )
   {
      GetDlgItem(IDC_B_LABEL)->SetWindowText(_T("R"));
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
