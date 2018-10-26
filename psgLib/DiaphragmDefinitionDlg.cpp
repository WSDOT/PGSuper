///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmDefinitionDlg dialog


CDiaphragmDefinitionDlg::CDiaphragmDefinitionDlg(const GirderLibraryEntry& entry,const GirderLibraryEntry::DiaphragmLayoutRule& rule,CWnd* pParent /*=NULL*/)
	: CDialog(CDiaphragmDefinitionDlg::IDD, pParent), m_Entry(entry) ,m_Rule(rule)
{
	//{{AFX_DATA_INIT(CDiaphragmDefinitionDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_Entry.GetBeamFactory(&m_pBeamFactory);

   CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedFactory(m_pBeamFactory);
   m_bSplicedGirder = (splicedFactory == NULL ? false : true);
}


void CDiaphragmDefinitionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiaphragmDefinitionDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_String(pDX,IDC_DESCRIPTION,m_Rule.Description);
   DDX_UnitValueAndTag(pDX,IDC_SPAN1,IDC_SPAN_UNIT, m_Rule.MinSpan, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX,IDC_SPAN2,IDC_SPAN_UNIT, m_Rule.MaxSpan, pDisplayUnits->SpanLength);
   DDX_UnitValueAndTag(pDX,IDC_H,IDC_H_UNIT, m_Rule.Height, pDisplayUnits->ComponentDim );
   DDX_UnitValueAndTag(pDX,IDC_THICKNESS,IDC_THICKNESS_UNIT, m_Rule.Thickness, pDisplayUnits->ComponentDim );
   DDX_CBItemData(pDX,IDC_METHOD,m_Rule.Method);
   DDX_CBItemData(pDX,IDC_TYPE,m_Rule.Type);
   DDX_CBItemData(pDX,IDC_CONSTRUCTION,m_Rule.Construction);
   DDX_CBItemData(pDX,IDC_MEASUREMENT_TYPE,m_Rule.MeasureType);
   DDX_CBItemData(pDX,IDC_MEASURED_FROM,m_Rule.MeasureLocation);

   if ( m_Rule.Type == GirderLibraryEntry::dtInternal )
   {
      DDX_UnitValueAndTag(pDX,IDC_WEIGHT,IDC_WEIGHT_UNIT, m_Rule.Weight, pDisplayUnits->GeneralForce );
   }
   else
   {
      DDX_UnitValueAndTag(pDX,IDC_WEIGHT,IDC_WEIGHT_UNIT, m_Rule.Weight, pDisplayUnits->ForcePerLength );
   }

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
	ON_CBN_SELCHANGE(IDC_MEASUREMENT_TYPE, OnMeasurementTypeChanged)
	ON_CBN_SELCHANGE(IDC_METHOD, OnMethodChanged)
   ON_CBN_SELCHANGE(IDC_CONSTRUCTION, OnConstructionTypeChanged)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDHELP, &CDiaphragmDefinitionDlg::OnBnClickedHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmDefinitionDlg message handlers
BOOL CDiaphragmDefinitionDlg::OnInitDialog() 
{
   // Method combo box
   CComboBox* pcbMethod = (CComboBox*)GetDlgItem(IDC_METHOD);
   int idx = pcbMethod->AddString(_T("Compute weight of diaphragm based on Width, Height, and Girder Spacing"));
   pcbMethod->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::dwmCompute);
   idx = pcbMethod->AddString(_T("Input diaphragm weight"));
   pcbMethod->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::dwmInput);

   // Construction type
   CComboBox* pcbConstruction = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION);
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes = m_pBeamFactory->GetSupportedDiaphragms();
   pgsTypes::SupportedDiaphragmTypes::iterator iter(diaphragmTypes.begin());
   pgsTypes::SupportedDiaphragmTypes::iterator end(diaphragmTypes.end());
   for ( ; iter != end; iter++ )
   {
      pgsTypes::DiaphragmType diaphragmType = *iter;
      int idx;
      switch(diaphragmType)
      {
      case pgsTypes::dtPrecast:
         idx = pcbConstruction->AddString(_T("Precast"));
         pcbConstruction->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::ctCastingYard);
         if ( m_Rule.Construction == GirderLibraryEntry::ctCastingYard )
         {
            pcbConstruction->SetCurSel(idx);
         }
         break;
      case pgsTypes::dtCastInPlace:
         idx = pcbConstruction->AddString(_T("Cast-in-Place"));
         pcbConstruction->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::ctBridgeSite);
         if ( m_Rule.Construction == GirderLibraryEntry::ctBridgeSite )
         {
            pcbConstruction->SetCurSel(idx);
         }
         break;
      default:
         ATLASSERT(false);
      }
   }
   
   OnConstructionTypeChanged();

   CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	OnMeasurementTypeChanged();
   OnMethodChanged();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDiaphragmDefinitionDlg::FillMeasurementDatumComboBox()
{
   // Measured from combo box
   CComboBox* pcbDatum = (CComboBox*)GetDlgItem(IDC_MEASURED_FROM);
   int curSel = pcbDatum->GetCurSel();
   pcbDatum->ResetContent();

   CComboBox* pcbConstruction = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION);	
   GirderLibraryEntry::ConstructionType constructionType = (GirderLibraryEntry::ConstructionType)(pcbConstruction->GetItemData(pcbConstruction->GetCurSel()));

   int idx;
   if ( constructionType == GirderLibraryEntry::ctBridgeSite )
   {
      idx = pcbDatum->AddString(_T("centerline of bearing"));
      pcbDatum->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mlBearing);

      idx = pcbDatum->AddString(_T("end of girder"));
      pcbDatum->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mlEndOfGirder);

      if ( m_bSplicedGirder )
      {
         idx = pcbDatum->AddString(_T("centerline of span"));
      }
      else
      {
         idx = pcbDatum->AddString(_T("centerline of girder"));
      }
      pcbDatum->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mlCenterlineOfGirder);
   }
   else
   {
      // if the diaphragm is precast, its location can be measured from the end or CL of the segment
      if ( m_bSplicedGirder )
      {
         idx = pcbDatum->AddString(_T("end of segment"));
      }
      else
      {
         idx = pcbDatum->AddString(_T("end of girder"));
      }
      pcbDatum->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mlEndOfGirder);

      if ( m_bSplicedGirder )
      {
         idx = pcbDatum->AddString(_T("centerline of segment"));
      }
      else
      {
         idx = pcbDatum->AddString(_T("centerline of girder"));
      }
      pcbDatum->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mlCenterlineOfGirder);
   }

   if ( pcbDatum->SetCurSel(curSel) == CB_ERR )
   {
      pcbDatum->SetCurSel(0);
   }
}

void CDiaphragmDefinitionDlg::FillMeasurementTypeComboBox()
{
   // Measurement type combo box
   CComboBox* pcbMeasurementType = (CComboBox*)GetDlgItem(IDC_MEASUREMENT_TYPE);
   CComboBox* pcbConstruction = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION);	
   int curSel = pcbConstruction->GetCurSel();
   ASSERT(curSel != CB_ERR);
   GirderLibraryEntry::ConstructionType constructionType = (GirderLibraryEntry::ConstructionType)(pcbConstruction->GetItemData(curSel));

   curSel = pcbMeasurementType->GetCurSel();
   pcbMeasurementType->ResetContent();

   int idx;

   idx = pcbMeasurementType->AddString(_T("fraction of the span length"));
   pcbMeasurementType->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mtFractionOfSpanLength);

   if ( m_bSplicedGirder )
   {
      idx = pcbMeasurementType->AddString(_T("fraction of the segment length"));
   }
   else
   {
      idx = pcbMeasurementType->AddString(_T("fraction of the girder length"));
   }
   pcbMeasurementType->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mtFractionOfGirderLength);

   // diaphragm can always be located as a fixed distance
   idx = pcbMeasurementType->AddString(_T("fixed distance"));
   pcbMeasurementType->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::mtAbsoluteDistance);

   if ( pcbMeasurementType->SetCurSel(curSel) == CB_ERR )
   {
      pcbMeasurementType->SetCurSel(0);
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
      pWnd->SetWindowText(_T("(0.0-0.5]"));
   }
   else
   {
      CDataExchange dx(this,FALSE);
      CEAFApp* pApp = EAFGetApp();
      const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();
      DDX_Tag(&dx,IDC_LOCATION_UNIT,pDisplayUnits->SpanLength);
   }
}

void CDiaphragmDefinitionDlg::OnConstructionTypeChanged()
{
   CComboBox* pcbConstruction = (CComboBox*)GetDlgItem(IDC_CONSTRUCTION);	

   int cursel = pcbConstruction->GetCurSel();
   GirderLibraryEntry::ConstructionType constructionType = GirderLibraryEntry::ctBridgeSite;
   if ( cursel != CB_ERR )
   {
      constructionType = (GirderLibraryEntry::ConstructionType)pcbConstruction->GetItemData(cursel);
   }

   pgsTypes::DiaphragmType diaphragmType = ConstructionTypeToDiaphragmType(constructionType);

   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_TYPE);	
   cursel = pcbLocation->GetCurSel();
   GirderLibraryEntry::DiaphragmType currentType = GirderLibraryEntry::dtExternal;
   if (cursel != CB_ERR)
   {
      currentType = (GirderLibraryEntry::DiaphragmType)(pcbLocation->GetItemData(cursel));
   }

   pcbLocation->ResetContent();

   pgsTypes::SupportedDiaphragmLocationTypes locations = m_pBeamFactory->GetSupportedDiaphragmLocations(diaphragmType);
   ATLASSERT(0 < locations.size());
   pgsTypes::SupportedDiaphragmLocationTypes::iterator iter(locations.begin());
   pgsTypes::SupportedDiaphragmLocationTypes::iterator end(locations.end());
   for ( ; iter != end; iter++ )
   {
      pgsTypes::DiaphragmLocationType location(*iter);

      if ( location == pgsTypes::dltExternal )
      {
         int idx = pcbLocation->AddString(_T("External (between girders)"));
         pcbLocation->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::dtExternal);
         if ( currentType == GirderLibraryEntry::dtExternal )
         {
            pcbLocation->SetCurSel(idx);
         }
      }

      if ( location == pgsTypes::dltInternal )
      {
         int idx = pcbLocation->AddString(_T("Internal (between interior webs)"));
         pcbLocation->SetItemData(idx,(DWORD_PTR)GirderLibraryEntry::dtInternal);
         if ( currentType == GirderLibraryEntry::dtInternal )
         {
            pcbLocation->SetCurSel(idx);
         }
      }
   }

   if (pcbLocation->GetCurSel() == CB_ERR)
   {
      pcbLocation->SetCurSel(0);
   }

   // Update dependent UI elements
   if ( constructionType == GirderLibraryEntry::ctCastingYard )
   {
      if ( m_bSplicedGirder )
      {
         GetDlgItem(IDC_RANGE_LABEL)->SetWindowText(_T("Create diaphragms when the segment length is between"));
      }
      else
      {
         GetDlgItem(IDC_RANGE_LABEL)->SetWindowText(_T("Create diaphragms when the girder length is between"));
      }

      CComboBox* pcbMethod = (CComboBox*)GetDlgItem(IDC_METHOD);
      ChangeComboBoxString(pcbMethod,0,_T("Compute weight of diaphragm based on Width, Height, and Web Spacing"));
   }
   else
   {
      GetDlgItem(IDC_RANGE_LABEL)->SetWindowText(_T("Create a diaphragm when the span length is between"));

      CComboBox* pcbMethod = (CComboBox*)GetDlgItem(IDC_METHOD);
      ChangeComboBoxString(pcbMethod,0,_T("Compute weight of diaphragm based on Width, Height, and Girder Spacing"));
   }

   FillMeasurementTypeComboBox();
   FillMeasurementDatumComboBox();
}

void CDiaphragmDefinitionDlg::OnMethodChanged()
{
   CComboBox* pcbMethod = (CComboBox*)GetDlgItem(IDC_METHOD);
   int curSel = pcbMethod->GetCurSel();
   BOOL bEnableDimensions = (curSel == 0);
   BOOL bEnableWeight     = (curSel != 0);

   GetDlgItem(IDC_HEIGHT_LABEL)->EnableWindow(bEnableDimensions);
   GetDlgItem(IDC_H)->EnableWindow(bEnableDimensions);
   GetDlgItem(IDC_H_UNIT)->EnableWindow(bEnableDimensions);

   GetDlgItem(IDC_THICKNESS_LABEL)->EnableWindow(bEnableDimensions);
   GetDlgItem(IDC_THICKNESS)->EnableWindow(bEnableDimensions);
   GetDlgItem(IDC_THICKNESS_UNIT)->EnableWindow(bEnableDimensions);

   GetDlgItem(IDC_WEIGHT_LABEL)->EnableWindow(bEnableWeight);
   GetDlgItem(IDC_WEIGHT)->EnableWindow(bEnableWeight);
   GetDlgItem(IDC_WEIGHT_UNIT)->EnableWindow(bEnableWeight);
}

pgsTypes::DiaphragmType CDiaphragmDefinitionDlg::ConstructionTypeToDiaphragmType(GirderLibraryEntry::ConstructionType constructionType)
{
   pgsTypes::DiaphragmType diaphragmType;
   switch(constructionType)
   {
   case GirderLibraryEntry::ctCastingYard:
      diaphragmType = pgsTypes::dtPrecast;
      break;
   case GirderLibraryEntry::ctBridgeSite:
      diaphragmType = pgsTypes::dtCastInPlace;
      break;
   default:
      ATLASSERT(false);
      diaphragmType = pgsTypes::dtCastInPlace;
      break;
   }
   return diaphragmType;
}

void CDiaphragmDefinitionDlg::OnBnClickedHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(),IDH_DIAPHRAGM_LAYOUT_RULES);
}
