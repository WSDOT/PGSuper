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

// AbutmentConnectionsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "AbutmentConnectionsPage.h"
#include "PierDetailsDlg.h"
#include "PGSuperColors.h"

#include "HtmlHelp\HelpTopics.hh"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAbutmentConnectionsPage property page

IMPLEMENT_DYNCREATE(CAbutmentConnectionsPage, CPropertyPage)

CAbutmentConnectionsPage::CAbutmentConnectionsPage() : CPropertyPage(CAbutmentConnectionsPage::IDD)
{
	//{{AFX_DATA_INIT(CAbutmentConnectionsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_WhiteBrush.CreateSolidBrush(METAFILE_BACKGROUND_COLOR);
   m_DiaphragmLoadLocation = 0;
}

CAbutmentConnectionsPage::~CAbutmentConnectionsPage()
{
}

void CAbutmentConnectionsPage::Init(CPierData2* pPier)
{
   m_pPier = pPier;
   m_PierIdx = pPier->GetIndex();
}

void CAbutmentConnectionsPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAbutmentConnectionsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   // Setup controls
   DDX_Control(pDX,IDC_BOUNDARY_CONDITIONS,m_cbBoundaryCondition);
   DDX_Control(pDX,IDC_DIAPHRAGM_OFFSET,m_DiaphragmLoadLocationEdit);

   // Set the schematic connection image
   CString image_name = GetImageName(m_BoundaryConditionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,image_name, _T("Metafile") );

   // Boundary Conditions
   DDX_CBItemData(pDX,IDC_BOUNDARY_CONDITIONS,m_BoundaryConditionType);

   // Connection Dimensions
   DDX_UnitValueAndTag(pDX,IDC_BEARING_OFFSET, IDC_BEARING_OFFSET_T,  m_BearingOffset,  pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_END_DISTANCE,   IDC_END_DISTANCE_T,    m_EndDistance,    pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_SUPPORT_WIDTH,  IDC_SUPPORT_WIDTH_T,   m_SupportWidth,   pDisplayUnits->GetComponentDimUnit());

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   // Validate connection dimensions
   if ( pDX->m_bSaveAndValidate )
   {
      DDV_UnitValueZeroOrMore(pDX, IDC_END_DISTANCE, m_EndDistance, pDisplayUnits->GetComponentDimUnit() );

      // if end distance is measured from the datum line end distance cannot be greater than
      // the bearing offset
      if ( (m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierAlongGirder ||
            m_EndDistanceMeasurementType == ConnectionLibraryEntry::FromPierNormalToPier)
            &&
            (m_BearingOffset < m_EndDistance)
         )
      {
         pDX->PrepareEditCtrl(IDC_END_DISTANCE);
         AfxMessageBox(_T("End Distance must be less than or equal to the Bearing Offset"),MB_OK | MB_ICONINFORMATION);
         pDX->Fail();
      }
   }

   // Diaphragm
   DDX_UnitValueAndTag(pDX,IDC_DIAPHRAGM_HEIGHT,IDC_DIAPHRAGM_HEIGHT_T,m_DiaphragmHeight,pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX,IDC_DIAPHRAGM_WIDTH, IDC_DIAPHRAGM_WIDTH_T, m_DiaphragmWidth, pDisplayUnits->GetComponentDimUnit());
   DDX_CBItemData(pDX,IDC_DIAPHRAGM_LOAD,m_DiaphragmLoadType);
   DDX_Tag( pDX, IDC_DIAPHRAGM_OFFSET_UNITS, pDisplayUnits->GetComponentDimUnit() );
   if ( !pDX->m_bSaveAndValidate || (pDX->m_bSaveAndValidate && m_DiaphragmLoadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation) )
   {
      // data always goes into the control and only comes out if the load type is at a specified location
      DDX_UnitValue( pDX, IDC_DIAPHRAGM_OFFSET, m_DiaphragmLoadLocation, pDisplayUnits->GetComponentDimUnit() );
   }

   if ( pDX->m_bSaveAndValidate )
   {
      // all of the data has been extracted from the dialog controls and it has been validated
      // set the values on the actual pier object
      m_pPier->SetBoundaryConditionType(m_BoundaryConditionType);

      // copy data to both sides of the abutment... this makes the default values more meaningful when we add an adjacent span
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::PierFaceType face = pgsTypes::PierFaceType(i);
         m_pPier->SetBearingOffset(    face,m_BearingOffset,m_BearingOffsetMeasurementType);
         m_pPier->SetGirderEndDistance(face,m_EndDistance,  m_EndDistanceMeasurementType);
         m_pPier->SetSupportWidth(     face,m_SupportWidth);

         m_pPier->SetDiaphragmHeight(      face,m_DiaphragmHeight);
         m_pPier->SetDiaphragmWidth(       face,m_DiaphragmWidth);
         m_pPier->SetDiaphragmLoadType(    face,m_DiaphragmLoadType);
         m_pPier->SetDiaphragmLoadLocation(face,m_DiaphragmLoadLocation);
      }
   }
}


BEGIN_MESSAGE_MAP(CAbutmentConnectionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAbutmentConnectionsPage)
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE, OnEndDistanceMeasureChanged)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnBearingOffsetMeasureChanged)
   ON_CBN_SELCHANGE(IDC_BOUNDARY_CONDITIONS, OnBoundaryConditionChanged)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_DIAPHRAGM_LOAD, &CAbutmentConnectionsPage::OnDiaphragmLoadTypeChanged)
   ON_BN_CLICKED(IDC_COPY, &CAbutmentConnectionsPage::OnCopyFromLibrary)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbutmentConnectionsPage message handlers

BOOL CAbutmentConnectionsPage::OnInitDialog() 
{
   ATLASSERT(m_pPier->IsAbutment());

   m_BoundaryConditionType = m_pPier->GetBoundaryConditionType();

   pgsTypes::PierFaceType face = (m_pPier->GetPrevSpan() == NULL ? pgsTypes::Ahead : pgsTypes::Back);
   m_pPier->GetBearingOffset(face,&m_BearingOffset,&m_BearingOffsetMeasurementType);
   m_pPier->GetGirderEndDistance(face,&m_EndDistance,&m_EndDistanceMeasurementType);
   m_SupportWidth = m_pPier->GetSupportWidth(face);

   m_DiaphragmHeight       = m_pPier->GetDiaphragmHeight(face);
   m_DiaphragmWidth        = m_pPier->GetDiaphragmWidth(face);
   m_DiaphragmLoadType     = m_pPier->GetDiaphragmLoadType(face);
   m_DiaphragmLoadLocation = m_pPier->GetDiaphragmLoadLocation(face);

   InitializeComboBoxes();

   CPropertyPage::OnInitDialog();

   FillBoundaryConditionComboBox(); // must do this after OnInitDialog

   GetDlgItem(IDC_DIAPHRAGM_OFFSET_LABEL)->SetWindowText(_T("Distance from Abutment Line to C.G. of Diaphragm"));


   if ( m_pPier->GetIndex() == 0 )
   {
      m_cbBoundaryCondition.SetPierType(PIERTYPE_START);
   }
   else
   {
      m_cbBoundaryCondition.SetPierType(PIERTYPE_END);
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAbutmentConnectionsPage::InitializeComboBoxes()
{
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();
   FillDiaphragmLoadComboBox();
}

void CAbutmentConnectionsPage::FillBoundaryConditionComboBox()
{
   std::vector<pgsTypes::BoundaryConditionType> connections( m_pPier->GetBridgeDescription()->GetBoundaryConditionTypes(m_PierIdx) );
   m_cbBoundaryCondition.Initialize(m_pPier->IsBoundaryPier(),connections);

   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_BOUNDARY_CONDITIONS,m_BoundaryConditionType);
}

void CAbutmentConnectionsPage::FillDiaphragmLoadComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DIAPHRAGM_LOAD);
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm over CL Bearing")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtBearingCenterline));
   pCB->SetItemData( pCB->AddString(_T("Apply weight of diaphragm to girder")), DWORD_PTR(ConnectionLibraryEntry::ApplyAtSpecifiedLocation));
   pCB->SetItemData( pCB->AddString(_T("Ignore diaphragm weight")), DWORD_PTR(ConnectionLibraryEntry::DontApply));
}

void CAbutmentConnectionsPage::OnHelp() 
{
#pragma Reminder("UPDATE: need correct help topic id")
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_CONNECTIONS );
}

void CAbutmentConnectionsPage::OnEndDistanceMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CAbutmentConnectionsPage::OnBearingOffsetMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CAbutmentConnectionsPage::OnBoundaryConditionChanged()
{
   if ( m_pPier->HasCantilever() )
   {
      GetDlgItem(IDC_COPY)->EnableWindow(FALSE);
      GetDlgItem(IDC_BEARING_OFFSET_MEASURE)->EnableWindow(FALSE);
      GetDlgItem(IDC_END_DISTANCE_MEASURE)->EnableWindow(FALSE);

      GetDlgItem(IDC_LABEL)->EnableWindow(FALSE);

      GetDlgItem(IDC_BEARING_OFFSET)->EnableWindow(FALSE);
      GetDlgItem(IDC_BEARING_OFFSET_T)->EnableWindow(FALSE);
      GetDlgItem(IDC_END_DISTANCE)->EnableWindow(FALSE);
      GetDlgItem(IDC_END_DISTANCE_T)->EnableWindow(FALSE);
      ((CComboBox*)GetDlgItem(IDC_DIAPHRAGM_LOAD))->SetCurSel(0);
      GetDlgItem(IDC_DIAPHRAGM_LOAD)->EnableWindow(FALSE);
   }

   OnDiaphragmLoadTypeChanged();

   UpdateConnectionPicture();
}

void CAbutmentConnectionsPage::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType ems = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType bms = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);
   curSel = pcbConnectionType->GetCurSel();
   pgsTypes::BoundaryConditionType connectionType = (pgsTypes::BoundaryConditionType)pcbConnectionType->GetItemData(curSel);

   CString image_name = GetImageName(connectionType,bms,ems);

	m_ConnectionPicture.SetImage(image_name, _T("Metafile") );
}

void CAbutmentConnectionsPage::FillBearingOffsetComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Normal to Abutment Line"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::NormalToPier));

   idx = pCB->AddString(_T("Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::AlongGirder));
}

void CAbutmentConnectionsPage::FillEndDistanceComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Measured from CL Bearing, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingAlongGirder));

   idx = pCB->AddString(_T("Measured from and Normal to CL Bearing"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingNormalToPier));

   idx = pCB->AddString(_T("Measured from Abutment Line, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierAlongGirder));

   idx = pCB->AddString(_T("Measured from and Normal to Abutment Line"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierNormalToPier));
}

CString CAbutmentConnectionsPage::GetImageName(pgsTypes::BoundaryConditionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CSpanData2* pPrevSpan = m_pPier->GetPrevSpan();
   CSpanData2* pNextSpan = m_pPier->GetNextSpan();
   const int StartPier = PIERTYPE_START;
   const int IntPier   = PIERTYPE_INTERMEDIATE;
   const int EndPier   = PIERTYPE_END;
   int pierType = (pPrevSpan != NULL && pNextSpan != NULL ? IntPier : (pPrevSpan == NULL ? StartPier : EndPier));

   CString strName;
   if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGGDRFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGGDRFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGGDRFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGNORMALFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGGDRFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGGDRFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGGDRFROMPIER");
         }
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGGDR_ENDALONGNORMALFROMPIER");
         }
         break;
      }
   }
   else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
         }
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
         }
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         if ( pierType == StartPier )
         {
            strName = _T("SA_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         }
         else if ( pierType == EndPier )
         {
            strName = _T("EA_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         }
         else
         {
            strName = _T("CLPIER_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
         }
         break;
      }
   }

   return strName;
}

void CAbutmentConnectionsPage::OnDiaphragmLoadTypeChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DIAPHRAGM_LOAD);
   int cursel = pCB->GetCurSel();
   ConnectionLibraryEntry::DiaphragmLoadType loadType = (ConnectionLibraryEntry::DiaphragmLoadType)pCB->GetItemData(cursel);

   BOOL bEnable = (loadType == ConnectionLibraryEntry::ApplyAtSpecifiedLocation) ? TRUE : FALSE;

   GetDlgItem(IDC_DIAPHRAGM_OFFSET_LABEL)->EnableWindow(bEnable);
   m_DiaphragmLoadLocationEdit.EnableWindow(bEnable);
   GetDlgItem(IDC_DIAPHRAGM_OFFSET_UNITS)->EnableWindow(bEnable);
}

void CAbutmentConnectionsPage::OnCopyFromLibrary()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames);
   std::vector<std::_tstring> names;
   pLibNames->EnumGdrConnectionNames(&names);

   if ( names.size() == 0 )
   {
      AfxMessageBox(_T("There are not any connections defined in the library"));
   }
   else
   {
	   CString strNames;
	   std::vector<std::_tstring>::iterator iter = names.begin();
	   strNames = (*iter).c_str();
       iter++;
	   for ( ; iter != names.end(); iter++ )
	   {
	      strNames += _T("\n");
	      CString strName( (*iter).c_str() );
	      strNames += strName;
	   }
	
	   CString strMsg(_T("Select a pre-defined connection from the list. Connection dimensions will be copied to both sides of intermediate piers."));
	   int result = AfxChoose(_T("Copy Connection Data from Library"),strMsg,strNames,0,TRUE);
	   if ( 0 <= result )
	   {
	      CDataExchange dx(this,TRUE);
	      DoDataExchange(&dx);
	
	      std::_tstring name = names[result];
		
	      GET_IFACE2(pBroker,ILibrary,pLib);
	      const ConnectionLibraryEntry* pEntry = pLib->GetConnectionEntry(name.c_str());
	
         m_BearingOffset = pEntry->GetGirderBearingOffset();
         m_EndDistance   = pEntry->GetGirderEndDistance();
         m_SupportWidth  = pEntry->GetSupportWidth();
   
         m_DiaphragmHeight       = pEntry->GetDiaphragmHeight();
         m_DiaphragmWidth        = pEntry->GetDiaphragmWidth();
         m_DiaphragmLoadType     = pEntry->GetDiaphragmLoadType();
         m_DiaphragmLoadLocation = pEntry->GetDiaphragmLoadLocation();

         m_EndDistanceMeasurementType   = pEntry->GetEndDistanceMeasurementType();
	      m_BearingOffsetMeasurementType = pEntry->GetBearingOffsetMeasurementType();
	
	      dx.m_bSaveAndValidate = FALSE;
	      DoDataExchange(&dx);

         OnBoundaryConditionChanged();
         OnDiaphragmLoadTypeChanged();
	   }
   }
}

BOOL CAbutmentConnectionsPage::OnSetActive()
{
   BOOL bResult = CPropertyPage::OnSetActive();

   // re-get the boundary condition. it could have changed if
   // this page is in the span editing dialog and a cantilever was added
   m_BoundaryConditionType = m_pPier->GetBoundaryConditionType();
   FillBoundaryConditionComboBox();
   OnBoundaryConditionChanged();

   return bResult;
}
