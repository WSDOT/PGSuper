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
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "PierLayoutPage.h"
#include "PierDetailsDlg.h"
#include <MfcTools\MfcTools.h>

#include "HtmlHelp\HelpTopics.hh"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\BridgeDescription.h>

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

   m_MovePierOption = pgsTypes::MoveBridge;

   HRESULT hr = m_objStation.CoCreateInstance(CLSID_Station);
   ASSERT(SUCCEEDED(hr));
}

CPierLayoutPage::~CPierLayoutPage()
{
}

void CPierLayoutPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPierLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   DDX_Station(pDX,IDC_STATION,m_Station,pDisplayUnits->GetStationFormat());

   DDX_CBItemData(pDX,IDC_MOVE_PIER,m_MovePierOption);

   DDX_String(pDX,IDC_ORIENTATION,m_strOrientation);

   if ( pDX->m_bSaveAndValidate )
   {
      pDX->PrepareEditCtrl(IDC_ORIENTATION);
      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 skewAngle;
      bool bSuccess = pBridge->GetSkewAngle(m_Station,m_strOrientation.c_str(),&skewAngle);
      if ( !bSuccess )
      {
         AfxMessageBox(_T("Invalid pier orientation"));
         pDX->Fail();
      }
      else if ( bSuccess && IsLT(fabs(skewAngle),0.0) || IsGE(MAX_SKEW_ANGLE,fabs(skewAngle)) )
      {
         AfxMessageBox(_T("Pier skew must be less than 88°\r\nPier skew is measured from the alignment normal"));
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CPierLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierLayoutPage)
	ON_EN_CHANGE(IDC_STATION, OnChangeStation)
	ON_EN_KILLFOCUS(IDC_STATION, OnKillfocusStation)
	ON_CBN_SETFOCUS(IDC_MOVE_PIER, OnSetfocusMovePier)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierLayoutPage message handlers

BOOL CPierLayoutPage::OnInitDialog() 
{
   // move options are not available until the station changes
   CComboBox* pOptions = (CComboBox*)GetDlgItem(IDC_MOVE_PIER);
   pOptions->EnableWindow(FALSE);

	CPropertyPage::OnInitDialog();

   UpdateMoveOptionList();

   
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();

   CString strPierType(pParent->m_pPrevSpan == NULL || pParent->m_pNextSpan == NULL ? _T("Abutment") : _T("Pier"));

   CString strGroupLabel;
   strGroupLabel.Format(_T("%s Line"),strPierType);
   GetDlgItem(IDC_LINE_GROUP)->SetWindowText(strGroupLabel);

   CString strPierLabel;
   strPierLabel.Format(_T("%s %d"),strPierType,LABEL_PIER(m_PierIdx));
   GetDlgItem(IDC_PIER_LABEL)->SetWindowText(strPierLabel);

   CString strStationLocation;
   strStationLocation.Format(_T("Station and Orientation defines the %s Line"),strPierType);
   GetDlgItem(IDC_STATION_LOCATION_LABEL)->SetWindowText(strStationLocation);
	
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   CString fmt;
   fmt.LoadString( IS_SI_UNITS(pDisplayUnits) ? IDS_DLG_STATIONFMT_SI : IDS_DLG_STATIONFMT_US );
   GetDlgItem(IDC_STATION_FORMAT)->SetWindowText( fmt );

   fmt.LoadString( IDS_DLG_ORIENTATIONFMT );
   GetDlgItem(IDC_ORIENTATION_FORMAT)->SetWindowText( fmt );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierLayoutPage::Init(const CPierData* pPier)
{
   m_PierIdx = pPier->GetPierIndex();

   const CBridgeDescription* pBridgeDesc = pPier->GetBridgeDescription();
   m_nSpans = pBridgeDesc->GetSpanCount();

   m_Station = pPier->GetStation();
   m_FromStation = m_Station; // keep a copy of this for the "move" note

   if ( m_PierIdx != 0 )
      m_PrevPierStation = pBridgeDesc->GetPier(m_PierIdx-1)->GetStation();
   else
      m_PrevPierStation = -DBL_MAX;

   if ( m_PierIdx != m_nSpans )
      m_NextPierStation = pBridgeDesc->GetPier(m_PierIdx+1)->GetStation();
   else
      m_NextPierStation = -DBL_MAX;

   m_strOrientation = pPier->GetOrientation();
}

void CPierLayoutPage::OnChangeStation() 
{
   GetDlgItem(IDC_MOVE_PIER)->EnableWindow(TRUE);

   UpdateMoveOptionList();
}

BOOL CPierLayoutPage::IsValidStation(Float64* pStation)
{
   BOOL bResult = TRUE;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   UnitModeType unitMode = pDisplayUnits->GetStationFormat().GetUnitOfMeasure() == unitStationFormat::Feet ? umUS : umSI;
   const unitLength& displayUnit = (unitMode == umUS ? unitMeasure::Feet : unitMeasure::Meter);

   CWnd* pWnd = GetDlgItem(IDC_STATION);
   
   int cLength = pWnd->GetWindowTextLength() + 1;
   cLength = (cLength == 1 ? 32 : cLength );
   LPTSTR lpszBuffer = new TCHAR[cLength];

   pWnd->GetWindowText(lpszBuffer,cLength);
   HRESULT hr = m_objStation->FromString(CComBSTR(lpszBuffer),unitMode);
   if ( SUCCEEDED(hr) )
   {
      m_objStation->get_Value(pStation);
      *pStation = ::ConvertToSysUnits( *pStation, displayUnit );
      bResult = TRUE;
   }
   else
   {
      bResult = FALSE;
   }

   delete[] lpszBuffer;
   return bResult;
}

void CPierLayoutPage::UpdateMoveOptionList()
{
   Float64 toStation;
   BOOL bIsValid = IsValidStation(&toStation);
   if ( bIsValid == FALSE )
   {
      CComboBox* pOptions = (CComboBox*)GetDlgItem(IDC_MOVE_PIER);
      pOptions->EnableWindow(FALSE);

      CEdit* pEdit = (CEdit*)GetDlgItem(IDC_STATION);
      pEdit->SetSel(-1,0);
      return;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // get the current selection
   CComboBox* pOptions = (CComboBox*)GetDlgItem(IDC_MOVE_PIER);
   int curSel = _cpp_max(pOptions->GetCurSel(),0);

   pOptions->ResetContent();

   CString strName = (m_PierIdx == 0 || m_PierIdx == m_nSpans ? _T("Abutment") : _T("Pier"));


   CWnd* pMove = GetDlgItem(IDC_MOVE_LABEL);
   CString strMove;
   strMove.Format(_T("Move %s %d from %s to %s"),
                  strName,
                  LABEL_PIER(m_PierIdx),
                  FormatStation(pDisplayUnits->GetStationFormat(),m_FromStation),
                  FormatStation(pDisplayUnits->GetStationFormat(),toStation)
                  );
   pMove->SetWindowText(strMove);

   CString strOptions[4];
   pgsTypes::MovePierOption options[4];
   strOptions[0].Format(_T("Move bridge retaining all span lengths"));
   options[0] = pgsTypes::MoveBridge;

   int nOptions = 1;

   if ( 1 < m_nSpans &&  // must have more than one span... moving an interior pier
        m_PierIdx != 0 && m_PierIdx != m_nSpans &&  // can't be first or last pier
        m_PrevPierStation < toStation && toStation < m_NextPierStation ) // can't move pier beyond adjacent piers
   {
      options[nOptions] = pgsTypes::AdjustAdjacentSpans;
      strOptions[nOptions++].Format(_T("Adjust the length of Spans %d and %d"),
                                    m_PierIdx,m_PierIdx+1);
   }

   if ( m_PierIdx == 0 && toStation < m_NextPierStation )
   {
      // adjust length of first span only
      options[nOptions] = pgsTypes::AdjustNextSpan;
      if ( m_nSpans == 1 )
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d by moving %s %d"),
                                       LABEL_SPAN(m_PierIdx),strName,LABEL_SPAN(m_PierIdx));
      }
      else
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx));
      }
   }
   else if ( m_PierIdx == m_nSpans && m_PrevPierStation < toStation )
   {
      // adjust length of last span only
      options[nOptions] = pgsTypes::AdjustPrevSpan;
      if ( m_nSpans == 1 )
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d by moving %s %d"),
                                       LABEL_SPAN(m_PierIdx-1),strName,LABEL_SPAN(m_PierIdx));
      }
      else
      {
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx-1));
      }
   }
   else if ( 0 < m_PierIdx && m_PierIdx < m_nSpans )
   {
      if ( m_PrevPierStation < toStation )
      {
         // adjust length of previous span only
         options[nOptions] = pgsTypes::AdjustPrevSpan;
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx-1));
      }

      if ( toStation < m_NextPierStation )
      {
         // adjust length of next span only
         options[nOptions] = pgsTypes::AdjustNextSpan;
         strOptions[nOptions++].Format(_T("Adjust the length of Span %d, retain length of all other spans"),
                                       LABEL_SPAN(m_PierIdx));
      }
   }

   for ( int i = 0; i < nOptions; i++ )
   {
      int idx = pOptions->AddString(strOptions[i]); 
      pOptions->SetItemData(idx,(DWORD)options[i]);
   }
   pOptions->SetCurSel(curSel);
}

void CPierLayoutPage::OnKillfocusStation() 
{
   UpdateMoveOptionList();
}

void CPierLayoutPage::OnSetfocusMovePier() 
{
   UpdateMoveOptionList();
}

void CPierLayoutPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_GENERAL );
}

HBRUSH CPierLayoutPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hbr = CPropertyPage::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_STATION )
   {
      Float64 toStation;
      if ( IsValidStation(&toStation) )
      {
         pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
      }
      else
      {
         pDC->SetTextColor(RED);
      }
   }

   return hbr;
}