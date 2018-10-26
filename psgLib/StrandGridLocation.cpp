///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

// StrandGridLocation.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "StrandGridLocation.h"
#include <MfcTools\CustomDDX.h>
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int ENDBOX_CTRLS[] = {IDC_HS_BOX, IDC_HS_TXT, IDC_HS_XT, IDC_HS_YT, IDC_GEX, IDC_GEY, IDC_GEX_T, IDC_GEY_T, -1};


/////////////////////////////////////////////////////////////////////////////
// CStrandGridLocation dialog

void CStrandGridLocation::SetEntry(const CGirderGlobalStrandGrid::GlobalStrandGridEntry& Entry, bool UseHarpedGrid, bool UseHarpedWebStrands)
{
   // we don't store an actual entry, just the data
   m_StrandType = (int)Entry.m_Type;
   m_HpX = Entry.m_X;
   m_HpY = Entry.m_Y;

   m_UseHarpedGrid = UseHarpedGrid;
   m_UseHarpedWebStrands = UseHarpedWebStrands;

   if (Entry.m_Type == GirderLibraryEntry::stHarped && m_UseHarpedGrid)
   {
      m_EndX = Entry.m_Hend_X;
      m_EndY = Entry.m_Hend_Y;
   }
   else
   {
      m_EndX = 0.0;
      m_EndY = 0.0;
   }

   if (Entry.m_Type==GirderLibraryEntry::stStraight && Entry.m_CanDebond)
   {
      m_AllowDebonding = TRUE;
   }
   else
   {
      m_AllowDebonding = FALSE;
   }
}

CGirderGlobalStrandGrid::GlobalStrandGridEntry CStrandGridLocation::GetEntry()
{
   CGirderGlobalStrandGrid::GlobalStrandGridEntry entry;

   if (m_StrandType==0)
   {
      entry.m_Type = GirderLibraryEntry::stStraight;
      entry.m_CanDebond = m_AllowDebonding==0 ? false : true;
   }
   else if (m_StrandType==1)
      entry.m_Type = GirderLibraryEntry::stHarped;
   else
      ATLASSERT(0);

   entry.m_X = m_HpX;
   entry.m_Y = m_HpY;

   if (entry.m_Type == GirderLibraryEntry::stHarped && m_UseHarpedGrid)
   {
      entry.m_Hend_X = m_EndX;
      entry.m_Hend_Y = m_EndY;
   }
   else
   {
      entry.m_Hend_X = 0.0;
      entry.m_Hend_Y = 0.0;
   }

   return entry;
}


CStrandGridLocation::CStrandGridLocation(CWnd* pParent /*=NULL*/)
	: CDialog(CStrandGridLocation::IDD, pParent), m_Row(-1)
{
	//{{AFX_DATA_INIT(CStrandGridLocation)
	m_StrandType = -1;
	m_UnitString = _T("");
	m_HpX = 0.0;
	m_HpY = 0.0;
	m_EndX = 0.0;
	m_EndY = 0.0;
	m_AllowDebonding = FALSE;
	//}}AFX_DATA_INIT
}


void CStrandGridLocation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStrandGridLocation)
	DDX_CBIndex(pDX, IDC_STRAND_TYPE, m_StrandType);
	DDX_Text(pDX, IDC_HPX_T, m_UnitString);
	DDX_Check(pDX, IDC_ALLOW_DEBONDING, m_AllowDebonding);
	//}}AFX_DATA_MAP

	DDX_Text(pDX, IDC_HPX, m_HpX);
	DDV_NonNegativeDouble(pDX, IDC_HPX, m_HpX);
	DDX_Text(pDX, IDC_HPY, m_HpY);
	DDV_NonNegativeDouble(pDX, IDC_HPY, m_HpY);

   if (m_StrandType==1 && m_UseHarpedGrid)
   {
	   DDX_Text(pDX, IDC_GEX, m_EndX);
	   DDV_NonNegativeDouble(pDX, IDC_GEX, m_EndX);
	   DDX_Text(pDX, IDC_GEY, m_EndY);
	   DDV_NonNegativeDouble(pDX, IDC_GEY, m_EndY);
   }

	DDX_Text(pDX, IDC_HPY_T, m_UnitString);
	DDX_Text(pDX, IDC_GEX_T, m_UnitString);
	DDX_Text(pDX, IDC_GEY_T, m_UnitString);

}


BEGIN_MESSAGE_MAP(CStrandGridLocation, CDialog)
	//{{AFX_MSG_MAP(CStrandGridLocation)
	ON_CBN_SELCHANGE(IDC_STRAND_TYPE, OnSelchangeStrandType)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStrandGridLocation message handlers


BOOL CStrandGridLocation::OnInitDialog() 
{
   CWnd* pbox = GetDlgItem(IDC_HP_BOX);
   CComboBox* pcb = (CComboBox*)GetDlgItem(IDC_STRAND_TYPE);
   ASSERT(pcb);
   pcb->AddString(_T("Straight"));
   if (m_UseHarpedWebStrands)
   {
      pcb->AddString(_T("Harped"));
      pbox->SetWindowTextW(_T("Location at Harping Points"));
   }
   else
   {
      pcb->AddString(_T("Straight-Web"));
      pbox->SetWindowTextW(_T("Location along Girder"));

      HideEndBox();
   }

	CDialog::OnInitDialog();
	
   CString strg;
   strg.Format(_T("Starting Fill Order is %d"), m_Row);
   CWnd* pdel = GetDlgItem(IDC_FILL_ORDER);
   ASSERT(pdel);
   pdel->SetWindowText(strg);

   BOOL enable = (m_StrandType==1 && m_UseHarpedGrid) ? TRUE : FALSE;
   EnableEndBox(enable);

   BOOL show =  (m_StrandType==0) ? TRUE : FALSE;
   ShowDebondCtrl(show);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStrandGridLocation::OnSelchangeStrandType() 
{
   CWnd* pdel = GetDlgItem(IDC_STRAND_TYPE);
   ASSERT(pdel);
   CString str;
   pdel->GetWindowText(str);

   if (str==_T("Harped"))
   {
      ShowDebondCtrl(FALSE);

      if (m_UseHarpedGrid)
      {
         EnableEndBox(TRUE);

         CWnd* pdel = GetDlgItem(IDC_GEX);
         ASSERT(pdel);
         CString strg;
         strg.Format(_T("%g"),(float)m_EndX);
         pdel->SetWindowText(strg);

         pdel = GetDlgItem(IDC_GEY);
         ASSERT(pdel);
         strg.Format(_T("%g"),(float)m_EndY);
         pdel->SetWindowText(strg);
      }
   }
   else if (str==_T("Straight"))
   {
      ShowDebondCtrl(TRUE);

      if (m_UseHarpedGrid)
      {
         EnableEndBox(FALSE);

         CWnd* pdel = GetDlgItem(IDC_GEX);
         ASSERT(pdel);
         CString strg; // blank string
         pdel->SetWindowText(strg);

         pdel = GetDlgItem(IDC_GEY);
         ASSERT(pdel);
         pdel->SetWindowText(strg);
      }
   }
   else if (str==_T("Straight-Web"))
   {
      ShowDebondCtrl(FALSE);

      if (m_UseHarpedGrid)
      {
         EnableEndBox(FALSE);

         CWnd* pdel = GetDlgItem(IDC_GEX);
         ASSERT(pdel);
         CString strg; // blank string
         pdel->SetWindowText(strg);

         pdel = GetDlgItem(IDC_GEY);
         ASSERT(pdel);
         pdel->SetWindowText(strg);
      }
   }
   else
      ATLASSERT(0);
	
}

void CStrandGridLocation::EnableEndBox(BOOL enable)
{

   int idx=0;
   while (ENDBOX_CTRLS[idx] != -1)
   {
      CWnd* pdel = GetDlgItem(ENDBOX_CTRLS[idx]);
      ASSERT(pdel);
      pdel->EnableWindow(enable);

      idx++;
   }
}

void CStrandGridLocation::HideEndBox()
{

   int idx=0;
   while (ENDBOX_CTRLS[idx] != -1)
   {
      CWnd* pdel = GetDlgItem(ENDBOX_CTRLS[idx]);
      ASSERT(pdel);
      pdel->ShowWindow(SW_HIDE);

      idx++;
   }
}


void CStrandGridLocation::ShowDebondCtrl(BOOL show)
{
      CWnd* pdel = GetDlgItem(IDC_ALLOW_DEBONDING);
      ASSERT(pdel);
      pdel->ShowWindow(show);
}

LRESULT CStrandGridLocation::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_HARPED_STRANDS_TAB );

   return TRUE;
}
