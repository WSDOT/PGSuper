///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// DiaphragmDefinitionDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "DiaphragmDefinitionDlg.h"
#include <MFCTools\MFCTools.h>
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmDefinitionDlg dialog


CDiaphragmDefinitionDlg::CDiaphragmDefinitionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDiaphragmDefinitionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDiaphragmDefinitionDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDiaphragmDefinitionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiaphragmDefinitionDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
   CEAFApp* pApp;
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      pApp = (CEAFApp*)AfxGetApp();
   }
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_String(pDX,IDC_DESCRIPTION,m_Rule.Description);
   DDX_UnitValueAndTag(pDX,IDC_SPAN1,IDC_SPAN_UNIT, m_Rule.MinSpan, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX,IDC_SPAN2,IDC_SPAN_UNIT, m_Rule.MaxSpan, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX,IDC_H,IDC_H_UNIT, m_Rule.Height, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX,IDC_THICKNESS,IDC_THICKNESS_UNIT, m_Rule.Thickness, pDisplayUnits->ComponentDim );
   DDX_CBItemData(pDX,IDC_TYPE,m_Rule.Type);
   DDX_CBItemData(pDX,IDC_CONSTRUCTION,m_Rule.Construction);
   DDX_CBItemData(pDX,IDC_MEASUREMENT_TYPE,m_Rule.MeasureType);
   DDX_CBItemData(pDX,IDC_MEASURED_FROM,m_Rule.MeasureLocation);
   

   if ( m_Rule.MeasureType == GirderLibraryEntry::mtFractionOfSpanLength || m_Rule.MeasureType == GirderLibraryEntry::mtFractionOfGirderLength)
   {
   	DDX_Text(pDX, IDC_LOCATION, m_Rule.Location );
      DDV_Range( pDX, mfcDDV::LE, mfcDDV::GE, m_Rule.Location, 0.00, 0.5 );
   }
   else
   {
      DDX_UnitValueAndTag(pDX,IDC_LOCATION,IDC_LOCATION_UNIT,m_Rule.Location, pDisplayUnits->SpanLength );
      DDV_UnitValueZeroOrMore(pDX, IDC_LOCATION,m_Rule.Location, pDisplayUnits->SpanLength );
   }
}


BEGIN_MESSAGE_MAP(CDiaphragmDefinitionDlg, CDialog)
	//{{AFX_MSG_MAP(CDiaphragmDefinitionDlg)
	ON_CBN_SELCHANGE(IDC_TYPE, OnDiaphragmTypeChanged)
	ON_CBN_SELCHANGE(IDC_MEASUREMENT_TYPE, OnMeasurementTypeChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmDefinitionDlg message handlers

void CDiaphragmDefinitionDlg::OnDiaphragmTypeChanged() 
{
	// TODO: Add your control notification handler code here
   CComboBox* pcbType = (CComboBox*)GetDlgItem(IDC_TYPE);	
   CComboBox* pcbConstruction = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION);	

   int cursel = pcbConstruction->GetCurSel();
   GirderLibraryEntry::ConstructionType constructionType = GirderLibraryEntry::ctBridgeSite;
   if ( cursel != CB_ERR )
   {
      constructionType = (GirderLibraryEntry::ConstructionType)pcbConstruction->GetItemData(cursel);
   }

   pcbConstruction->ResetContent();

   cursel = pcbType->GetCurSel();
   if ( cursel == CB_ERR )
   {
      pcbType->SetCurSel(0);
      cursel = 0;
   }

   if ( cursel != CB_ERR )
   {
      if ( cursel == 0 )
      {
         // external diaphragm
         int idx = pcbConstruction->AddString("Bridge Site");
         pcbConstruction->SetItemData(idx,(DWORD)GirderLibraryEntry::ctBridgeSite);
         pcbConstruction->SetCurSel(idx);
      }
      else
      {
         // internal diaphragm
         int idx = pcbConstruction->AddString("Casting Yard");
         pcbConstruction->SetItemData(idx,GirderLibraryEntry::ctCastingYard);
         
         if ( constructionType == GirderLibraryEntry::ctCastingYard )
            pcbConstruction->SetCurSel(idx);

         idx = pcbConstruction->AddString("Bridge Site");
         pcbConstruction->SetItemData(idx,GirderLibraryEntry::ctBridgeSite);

         if ( constructionType == GirderLibraryEntry::ctBridgeSite )
            pcbConstruction->SetCurSel(idx);
      }
   }
}

void CDiaphragmDefinitionDlg::OnMeasurementTypeChanged()
{
   CComboBox* pcbMeasurementType = (CComboBox*)GetDlgItem(IDC_MEASUREMENT_TYPE);
   int idx = pcbMeasurementType->GetCurSel();
   GirderLibraryEntry::MeasurementType mt = (GirderLibraryEntry::MeasurementType)(pcbMeasurementType->GetItemData(idx));

   if ( mt == GirderLibraryEntry::mtFractionOfSpanLength || mt == GirderLibraryEntry::mtFractionOfGirderLength )
   {
      CWnd* pWnd = GetDlgItem(IDC_LOCATION_UNIT);
      pWnd->SetWindowText("(0.0-0.5]");
   }
   else
   {
      CDataExchange dx(this,FALSE);
      CEAFApp* pApp;
      {
         AFX_MANAGE_STATE(AfxGetAppModuleState());
         pApp = (CEAFApp*)AfxGetApp();
      }
      const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();
      DDX_Tag(&dx,IDC_LOCATION_UNIT,pDisplayUnits->SpanLength);
   }
}

BOOL CDiaphragmDefinitionDlg::OnInitDialog() 
{
   // Diaphragm type combo box
   CComboBox* pcbDiaphragmType = (CComboBox*)GetDlgItem(IDC_TYPE);
   pcbDiaphragmType->ResetContent();
   int idx = pcbDiaphragmType->AddString("External (between girders)");
   pcbDiaphragmType->SetItemData(idx,(DWORD)GirderLibraryEntry::dtExternal);

   idx = pcbDiaphragmType->AddString("Internal (between interior webs)");
   pcbDiaphragmType->SetItemData(idx,(DWORD)GirderLibraryEntry::dtInternal);

   // put some dummy stuff here
   CComboBox* pcbConstruction = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION);
   pcbConstruction->AddString("1");
   pcbConstruction->SetItemData(0,(DWORD)GirderLibraryEntry::ctCastingYard);
   pcbConstruction->AddString("2");
   pcbConstruction->SetItemData(1,(DWORD)GirderLibraryEntry::ctBridgeSite);

   // Measurement type combo box
   CComboBox* pcbMeasurementType = (CComboBox*)GetDlgItem(IDC_MEASUREMENT_TYPE);
   idx = pcbMeasurementType->AddString("fraction of the span length");
   pcbMeasurementType->SetItemData(idx,(DWORD)GirderLibraryEntry::mtFractionOfSpanLength);

   idx = pcbMeasurementType->AddString("fraction of the girder length");
   pcbMeasurementType->SetItemData(idx,(DWORD)GirderLibraryEntry::mtFractionOfGirderLength);

   idx = pcbMeasurementType->AddString("fixed distance");
   pcbMeasurementType->SetItemData(idx,(DWORD)GirderLibraryEntry::mtAbsoluteDistance);

   // Measured from combo box
   CComboBox* pcbMeasuredFrom = (CComboBox*)GetDlgItem(IDC_MEASURED_FROM);
   idx = pcbMeasuredFrom->AddString("centerline of bearing");
   pcbMeasuredFrom->SetItemData(idx,(DWORD)GirderLibraryEntry::mlBearing);

   idx = pcbMeasuredFrom->AddString("end of girder");
   pcbMeasuredFrom->SetItemData(idx,(DWORD)GirderLibraryEntry::mlEndOfGirder);

   idx = pcbMeasuredFrom->AddString("centerline of girder");
   pcbMeasuredFrom->SetItemData(idx,(DWORD)GirderLibraryEntry::mlCenterlineOfGirder);
   

   CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

   OnDiaphragmTypeChanged();
	OnMeasurementTypeChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
