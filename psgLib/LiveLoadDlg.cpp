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

// LiveLoadDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "LiveLoadDlg.h"
#include <MfcTools\CustomDDX.h>
#include <psgLib\LiveLoadLibraryEntry.h>
#include <..\htmlhelp\helptopics.hh>

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDlg dialog


CLiveLoadDlg::CLiveLoadDlg(bool allowEditing, CWnd* pParent /*=NULL*/)
	: CDialog(CLiveLoadDlg::IDD, pParent),
   m_AllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CLiveLoadDlg)
	m_EntryName = _T("");
	m_LaneLoad = 0.0;
   m_LaneLoadSpanLength = 0;
	m_IsNotional = FALSE;
	//}}AFX_DATA_INIT

   m_VariableAxleIndex = FIXED_AXLE_TRUCK;

   m_UsageType = LiveLoadLibraryEntry::llaEntireStructure;
}

void CLiveLoadDlg::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLiveLoadDlg)
	DDX_Text(pDX, IDC_ENTRY_NAME, m_EntryName);
	DDX_Text(pDX, IDC_LANE_LOAD, m_LaneLoad);
	DDX_Check(pDX, IDC_CHECK_NOTIONAL, m_IsNotional);
	//}}AFX_DATA_MAP
	DDX_CBItemData(pDX, IDC_CONFIG_TYPE, m_ConfigType);
	DDX_CBItemData(pDX, IDC_USAGE, m_UsageType);

   DDX_UnitValueAndTag( pDX, IDC_LANE_LOAD,  IDC_LANE_LOAD_UNITS, m_LaneLoad, pDisplayUnits->ForcePerLength);
   DDV_UnitValueZeroOrMore( pDX, IDC_LANE_LOAD,m_LaneLoad, pDisplayUnits->ForcePerLength );

   DDX_UnitValueAndTag( pDX, IDC_BRIDGE_LENGTH,  IDC_BRIDGE_LENGTH_UNITS, m_LaneLoadSpanLength, pDisplayUnits->SpanLength );
   DDV_UnitValueZeroOrMore( pDX, IDC_BRIDGE_LENGTH, m_LaneLoadSpanLength, pDisplayUnits->SpanLength );

   DDV_GXGridWnd(pDX, &m_Grid);
   if (pDX->m_bSaveAndValidate)
   {
      m_Grid.DownloadData(pDX,this);

      // data checking
      if (m_ConfigType!=LiveLoadLibraryEntry::lcTruckOnly)
      {
         // lane load must be >0
         if (m_LaneLoad<=0.0)
         {
            pDX->PrepareEditCtrl(IDC_LANE_LOAD);
            ::AfxMessageBox("Error - Lane load must be a positive number",MB_OK | MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }

      if (m_ConfigType != LiveLoadLibraryEntry::lcLaneOnly)
      {
         if (m_Axles.empty())
         {
            pDX->PrepareCtrl(IDC_AXLES_GRID);
            ::AfxMessageBox("Error - The truck must have at least one axle",MB_OK | MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
   }
   else
   {
      m_Grid.UploadData(pDX,this);
   }
}

void CLiveLoadDlg::InitData(LiveLoadLibraryEntry* entry)
{
}

BEGIN_MESSAGE_MAP(CLiveLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CLiveLoadDlg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_CBN_SELCHANGE(IDC_CONFIG_TYPE, OnSelchangeConfigType)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDlg message handlers
LRESULT CLiveLoadDlg::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_LIVELOAD_DIALOG );
   return TRUE;
}

BOOL CLiveLoadDlg::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_AXLES_GRID, this);
   m_Grid.CustomInit();

   // Fill Load Type combo box
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CONFIG_TYPE);
   int idx = pCB->AddString("Truck Only");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::lcTruckOnly);

   idx = pCB->AddString("Lane Load Only");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::lcLaneOnly);

   idx = pCB->AddString("Sum of Lane Load and Truck");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::lcTruckPlusLane);

   idx = pCB->AddString("Envelope of Lane Load and Truck");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::lcTruckLaneEnvelope);

   // Fill Usage combo box
   pCB = (CComboBox*)GetDlgItem(IDC_USAGE);
   idx = pCB->AddString("Use for all actions at all locations");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::llaEntireStructure);

   idx = pCB->AddString("Use only for negative moments between points of contraflexure and interior pier reactions");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::llaContraflexure);

   idx = pCB->AddString("Use only for negative moments and interior pier reactions");
   pCB->SetItemData(idx,LiveLoadLibraryEntry::llaNegMomentAndInteriorPierReaction);


	CDialog::OnInitDialog();
	
	OnEnableDelete(false);
   UpdateConfig();	

   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += " - ";
   head += m_EntryName;
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += " (Read Only)";
   }
   SetWindowText(head);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLiveLoadDlg::OnAdd() 
{
	m_Grid.Appendrow();
}

void CLiveLoadDlg::OnDelete() 
{
   m_Grid.Removerows();
}

void CLiveLoadDlg::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_DELETE);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete?TRUE:FALSE);
}

void CLiveLoadDlg::OnSelchangeConfigType() 
{
   UpdateConfig();	
}

void CLiveLoadDlg::UpdateConfig()
{
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_CONFIG_TYPE );
   int cur_sel = pList->GetCurSel();

   CWnd* planelabel = GetDlgItem(IDC_LANE_LOAD_LABEL);
   CWnd* plane  = GetDlgItem(IDC_LANE_LOAD);
   CWnd* planeunit  = GetDlgItem(IDC_LANE_LOAD_UNITS);
   CWnd* pbridgelengthlabel = GetDlgItem(IDC_BRIDGE_LENGTH_LABEL);
   CWnd* pbridgelength = GetDlgItem(IDC_BRIDGE_LENGTH);
   CWnd* pbridgelengthunit = GetDlgItem(IDC_BRIDGE_LENGTH_UNITS);
   CWnd* pnotl  = GetDlgItem(IDC_CHECK_NOTIONAL);
   //CWnd* paxles = GetDlgItem(IDC_AXLES_GRID);
   CWnd* pdel   = GetDlgItem(IDC_DELETE);
   CWnd* padd   = GetDlgItem(IDC_ADD);

   bool do_live(true), do_lane(true);

   LiveLoadLibraryEntry::LiveLoadConfigurationType type;
   type = (LiveLoadLibraryEntry::LiveLoadConfigurationType)pList->GetItemData(cur_sel);
   switch(type)
   {
      case LiveLoadLibraryEntry::lcTruckOnly:
         do_lane = false;
         break;
      case LiveLoadLibraryEntry::lcLaneOnly:
         do_live = false;
         break;
      case LiveLoadLibraryEntry::lcTruckPlusLane:
      case LiveLoadLibraryEntry::lcTruckLaneEnvelope:
         break;
      default:
         ATLASSERT(false);
   };

   planelabel->EnableWindow(do_lane?TRUE:FALSE);
   plane->EnableWindow(do_lane?TRUE:FALSE);
   planeunit->EnableWindow(do_lane?TRUE:FALSE);
   pbridgelengthlabel->EnableWindow(do_lane?TRUE:FALSE);
   pbridgelength->EnableWindow(do_lane?TRUE:FALSE);
   pbridgelengthunit->EnableWindow(do_lane?TRUE:FALSE);
   pnotl->EnableWindow(do_live?TRUE:FALSE);
   //paxles->EnableWindow(do_live?TRUE:FALSE);
   pdel->EnableWindow(do_live?TRUE:FALSE);
   padd->EnableWindow(do_live?TRUE:FALSE);

   this->m_Grid.Enable(do_live?TRUE:FALSE);
}
