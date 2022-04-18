///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// BridgeDescFramingPage2.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescFramingPage.h"
#include "SpanLengthDlg.h"
#include "BridgeDescDlg.h"

#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingPage property page

IMPLEMENT_DYNCREATE(CBridgeDescFramingPage, CPropertyPage)

CBridgeDescFramingPage::CBridgeDescFramingPage() : 
CPropertyPage(CBridgeDescFramingPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescFramingPage)
	//}}AFX_DATA_INIT
}

CBridgeDescFramingPage::~CBridgeDescFramingPage()
{
}

void CBridgeDescFramingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   //{{AFX_DATA_MAP(CBridgeDescFramingPage)
   DDX_Control(pDX, IDC_ORIENTATIONFMT, m_OrientationFormat);
	DDX_Control(pDX, IDC_STATIONFMT, m_StationFormat);
   DDX_CBEnum(pDX, IDC_START_CB, m_DisplayStartSupportType);
   DDX_CBEnum(pDX, IDC_END_CB, m_DisplayEndSupportType);
   DDX_Text(pDX, IDC_STARTPIERNO, m_StartingPierNumber);
   DDV_GreaterThanZero(pDX,IDC_STARTPIERNO,(Float64)m_StartingPierNumber);
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      // Save pier numbering basis
      pParent->m_BridgeDesc.SetPierDisplaySettings(m_DisplayStartSupportType, m_DisplayEndSupportType, m_StartingPierNumber);

      DDV_GXGridWnd(pDX, &m_Grid);
      // why validate grid and then not get the data out of it?
      // pParent->m_BridgeDesc is kept continuously up to date with grid

      PierIndexType nPiers = pParent->m_BridgeDesc.GetPierCount();
      for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
      {
         const CPierData2* pPier = pParent->m_BridgeDesc.GetPier(pierIdx);
         if (pPier->IsBoundaryPier())
         {
            pgsTypes::BoundaryConditionType bc = pPier->GetBoundaryConditionType();
            std::vector<pgsTypes::BoundaryConditionType> boundary_conditions = pParent->m_BridgeDesc.GetBoundaryConditionTypes(pierIdx);
            auto found = std::find(std::cbegin(boundary_conditions), std::cend(boundary_conditions), bc);
            if (found == std::cend(boundary_conditions))
            {
               bool bIsAbument = pierIdx == 0 || pierIdx == nPiers - 1;
               CString strMsg;
               strMsg.Format(_T("The boundary conditions for %s are invalid.\r\nPress the Edit button in the grid to update the boundary conditions."), 
                            pgsPierLabel::CreatePierLabel(bIsAbument, pierIdx, m_DisplayStartSupportType, m_DisplayEndSupportType, m_StartingPierNumber).c_str());
               AfxMessageBox(strMsg, MB_ICONERROR | MB_OK);
               pDX->PrepareCtrl(IDC_PIER_GRID);
               pDX->Fail();
            }
         }
         else
         {
            pgsTypes::PierSegmentConnectionType connection = pPier->GetSegmentConnectionType();
            std::vector<pgsTypes::PierSegmentConnectionType> connections = pParent->m_BridgeDesc.GetPierSegmentConnectionTypes(pierIdx);
            auto found = std::find(std::cbegin(connections), std::cend(connections), connection);
            if (found == std::cend(connections))
            {
               CString strMsg;
               strMsg.Format(_T("The connection type at Pier %d is invalid.\r\nPress the Edit button in the grid to update the connection type."), pierIdx+m_StartingPierNumber);
               AfxMessageBox(strMsg, MB_ICONERROR | MB_OK);
               pDX->PrepareCtrl(IDC_PIER_GRID);
               pDX->Fail();
            }
         }
      }
   }
   else
   {
      m_Grid.FillGrid(pParent->m_BridgeDesc);
   }
}


BEGIN_MESSAGE_MAP(CBridgeDescFramingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescFramingPage)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_COMMAND(IDC_ADD_PIER, OnAddPier)
   ON_COMMAND(IDC_REMOVE_PIER, OnRemovePier)
   ON_COMMAND(IDC_ADD_TEMP_SUPPORT, OnAddTemporarySupport)
   ON_COMMAND(IDC_REMOVE_TEMP_SUPPORT, OnRemoveTemporarySupport)
   ON_COMMAND(IDC_LAYOUT, OnLayoutBySpanLengths)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_ORIENT_PIERS, &CBridgeDescFramingPage::OnOrientPiers)
   ON_CBN_SELCHANGE(IDC_START_CB, &CBridgeDescFramingPage::OnCbnSelchangeStartCb)
   ON_CBN_SELCHANGE(IDC_END_CB, &CBridgeDescFramingPage::OnCbnSelchangeEndCb)
   ON_EN_UPDATE(IDC_STARTPIERNO, &CBridgeDescFramingPage::OnEnUpdateStartpierno)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingPage message handlers

void CBridgeDescFramingPage::EnableInsertSpanBtn(BOOL bEnable)
{
   GetDlgItem(IDC_ADD_PIER)->EnableWindow(bEnable);
}

void CBridgeDescFramingPage::EnableInsertTempSupportBtn(BOOL bEnable)
{
   GetDlgItem(IDC_ADD_TEMP_SUPPORT)->EnableWindow(bEnable);
}

void CBridgeDescFramingPage::EnableRemovePierBtn(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE_PIER)->EnableWindow(bEnable);
   if ( bEnable )
   {
      SpanIndexType spanIdx = m_Grid.GetSelectedSpan();
      PierIndexType pierIdx = m_Grid.GetSelectedPier();
      PierIndexType nPiers  = m_Grid.GetPierCount();
      SpanIndexType nSpans = nPiers - 1;

      if ( spanIdx == INVALID_INDEX && pierIdx == INVALID_INDEX )
      {
         GetDlgItem(IDC_REMOVE_PIER)->SetWindowText(_T("Remove Span/Pier"));
      }
      else
      {
         CString strLabel;
         if ( pierIdx != INVALID_INDEX && pierIdx == nPiers-1 ) // a pier is selected and it is the last pier
         {
            strLabel.Format(_T("Remove Span %d/Pier %d"), pierIdx-1+m_StartingPierNumber, pierIdx+m_StartingPierNumber);
         }
         else
         {
            if (spanIdx != INVALID_INDEX)
            {
               // a span is selected
               strLabel.Format(_T("Remove Span %d/Pier %d"), spanIdx+m_StartingPierNumber, spanIdx+1+m_StartingPierNumber);
            }
            else
            {
               // a pier is selected
               ATLASSERT(pierIdx != INVALID_INDEX);
               strLabel.Format(_T("Remove Span %d/Pier %d"), pierIdx+m_StartingPierNumber, pierIdx+m_StartingPierNumber);
            }
         }

         GetDlgItem(IDC_REMOVE_PIER)->SetWindowText(strLabel);
      }

   }
   else
   {
      GetDlgItem(IDC_REMOVE_PIER)->SetWindowText(_T("Remove Span/Pier"));
   }
}

void CBridgeDescFramingPage::EnableRemoveTemporarySupportBtn(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE_TEMP_SUPPORT)->EnableWindow(bEnable);
}

BOOL CBridgeDescFramingPage::OnInitDialog() 
{
   EnableToolTips(TRUE);

   m_Grid.SubclassDlgItem(IDC_PIER_GRID, this);
   m_Grid.CustomInit();

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   // Load pier numbering basis
   pParent->m_BridgeDesc.GetPierDisplaySettings(&m_DisplayStartSupportType, &m_DisplayEndSupportType, &m_StartingPierNumber);

   CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
   CString fmt;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   fmt.LoadString( IS_SI_UNITS(pDisplayUnits) ? IDS_DLG_STATIONFMT_SI : IDS_DLG_STATIONFMT_US );
   m_StationFormat.SetWindowText( fmt );

   fmt.LoadString( IDS_DLG_ORIENTATIONFMT );
   m_OrientationFormat.SetWindowText( fmt );

   // causes the Remove Span button to be enabled/disabled
   EnableRemovePierBtn(m_Grid.EnableRemovePierBtn());
   EnableRemoveTemporarySupportBtn(m_Grid.EnableRemoveTemporarySupportBtn());

   // I don't like using this interface because we should be a generic as possible
   // I can't think of a different way to this this right now.
   // Also, in the future, PGSuper will support temporary supports in the form of shoring towers during erection
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      CWnd* pwndAddTempSupport = GetDlgItem(IDC_ADD_TEMP_SUPPORT);
      pwndAddTempSupport->EnableWindow(FALSE);

      // Hide the temporary support buttons and move the [Layout Span Lengths] and [Orient Piers] button up
      CWnd* pwndRemoveTempSupport = GetDlgItem(IDC_REMOVE_TEMP_SUPPORT);
      CWnd* pwndLayoutSpanLengths = GetDlgItem(IDC_LAYOUT);
      CWnd* pwndOrientPiers = GetDlgItem(IDC_ORIENT_PIERS);

      CRect rAddTempSupport, rRemoveTempSupport;
      pwndAddTempSupport->GetWindowRect(&rAddTempSupport);
      pwndRemoveTempSupport->GetWindowRect(&rRemoveTempSupport);

      ScreenToClient(rAddTempSupport);
      ScreenToClient(rRemoveTempSupport);

      pwndLayoutSpanLengths->MoveWindow(rAddTempSupport);
      pwndOrientPiers->MoveWindow(rRemoveTempSupport);

      pwndAddTempSupport->ShowWindow(SW_HIDE);
      pwndRemoveTempSupport->ShowWindow(SW_HIDE);
   }

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescFramingPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_FRAMING );
}

void CBridgeDescFramingPage::OnRemovePier() 
{
   m_Grid.OnRemovePier();
   EnableRemovePierBtn(FALSE); // nothing selected
}

void CBridgeDescFramingPage::OnAddPier() 
{
   // Tricky: Need to take current status of pier labelling, set it, and stow it so proper labels are displayed
   pgsAutoPierLabel autoPierLabel; // will reset values upon destruction
   pgsPierLabel::SetPierLabelSettings(m_DisplayStartSupportType, m_DisplayEndSupportType, m_StartingPierNumber);

   m_Grid.OnAddPier();
}

void CBridgeDescFramingPage::OnLayoutBySpanLengths()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<Float64> spanLengths = m_Grid.GetSpanLengths();

   // Tricky: Need to take current status of pier labelling, set it, and stow it so proper labels are displayed
   pgsAutoPierLabel autoPierLabel; // will reset values upon destruction
   pgsPierLabel::SetPierLabelSettings(m_DisplayStartSupportType, m_DisplayEndSupportType, m_StartingPierNumber);

   CSpanLengthDlg dlg;
   dlg.m_SpanLengths = spanLengths;
   dlg.m_PierIdx = (int)(m_StartingPierNumber-1);
   if ( dlg.DoModal() == IDOK )
   {
      m_Grid.SetSpanLengths(dlg.m_SpanLengths,dlg.m_PierIdx - m_StartingPierNumber + 1);
   }
}

BOOL CBridgeDescFramingPage::OnNcActivate(BOOL bActive)
{
   // This is required when a CGXComboBox is used in the grid. It prevents
   // the dialog from losing focus and the title bar from flashing. See CGXComboBox documentation
   if ( GXDiscardNcActivate() )
   {
      return true;
   }

   return CPropertyPage::OnNcActivate(bActive);
}

void CBridgeDescFramingPage::OnAddTemporarySupport()
{
   m_Grid.OnAddTemporarySupport();
}

void CBridgeDescFramingPage::OnRemoveTemporarySupport()
{
   m_Grid.OnRemoveTemporarySupport();
   EnableRemoveTemporarySupportBtn(FALSE); // nothing selected
}

void CBridgeDescFramingPage::OnOrientPiers()
{
   CString strResult;
   BOOL bDone = FALSE;
   do
   {
      if ( AfxQuestion(_T("Orient Piers"),_T("Enter orientation of all piers and temporary supports"),_T("NORMAL"),strResult) )
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IValidate,pValidate);
         UINT result = pValidate->Orientation(strResult);
         if ( result == VALIDATE_SUCCESS )
         {
            m_Grid.SetPierOrientation(strResult);
            bDone = TRUE;
         }
         else
         {
            CString strMsg;
            strMsg.Format(_T("\"%s\" is not a validate orientation"),strResult);
            AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         }
      }
      else
      {
         bDone = TRUE; // user canceled so we are done
      }
   }
   while ( !bDone ); // keep looping until we get a valid response
}

BOOL CBridgeDescFramingPage::OnSetActive()
{
   EnableRemovePierBtn(m_Grid.EnableRemovePierBtn());
   EnableRemoveTemporarySupportBtn(m_Grid.EnableRemoveTemporarySupportBtn());

   return CPropertyPage::OnSetActive();
}

void CBridgeDescFramingPage::OnCbnSelchangeStartCb()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_START_CB);
   pgsTypes::DisplayEndSupportType shp = (pgsTypes::DisplayEndSupportType)pctrl->GetCurSel();
   ATLASSERT(shp!=CB_ERR);

   m_DisplayStartSupportType = shp;

   // update grid text
   m_Grid.FillGrid(pParent->m_BridgeDesc);
}

void CBridgeDescFramingPage::OnCbnSelchangeEndCb()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   CComboBox* pctrl = (CComboBox*)GetDlgItem(IDC_END_CB);
   pgsTypes::DisplayEndSupportType shp = (pgsTypes::DisplayEndSupportType)pctrl->GetCurSel();
   ATLASSERT(shp!=CB_ERR);

   m_DisplayEndSupportType = shp;

   // update grid text
   m_Grid.FillGrid(pParent->m_BridgeDesc);
}

CString CBridgeDescFramingPage::GetPierLabel(bool isAbutment, PierIndexType pierIdx)
{
   return pgsPierLabel::CreatePierLabel(isAbutment, pierIdx, m_DisplayStartSupportType, m_DisplayEndSupportType, m_StartingPierNumber).c_str();
}


void CBridgeDescFramingPage::OnEnUpdateStartpierno()
{
   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();

   try
   {
      CDataExchange DX(this, TRUE);
      DDX_Text(&DX, IDC_STARTPIERNO, m_StartingPierNumber);
      DDV_GreaterThanZero(&DX, IDC_STARTPIERNO, (Float64)m_StartingPierNumber);

      // update grid text
      m_Grid.FillGrid(pParent->m_BridgeDesc);
   }
   catch (...)
   {
      throw;
   }
}

void CBridgeDescFramingPage::OnCancel()
{
   // Tricky: tell grid not to validate cells if dialog is cancelling. This fixed a bug where
   //         users were forced to input valid grid data in order to cancel the dialog
   m_Grid.SetDoNotValidateCells();

   CPropertyPage::OnCancel();
}
