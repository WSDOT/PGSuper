///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// DiaphragmLayoutDlg.cpp : implementation file
//
#include "stdafx.h"
#include <psgLib\psgLib.h>
#include <psgLib\DiaphragmLayoutDlg.h>

#include <Units\sysUnits.h>
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmLayoutDlg dialog


CDiaphragmLayoutDlg::CDiaphragmLayoutDlg(libUnitsMode::Mode mode, bool allowEditing,
                                         CWnd* pParent /*=NULL*/)
	: CDialog(CDiaphragmLayoutDlg::IDD, pParent),
     m_UnitLength(unitMeasure::Meter),
     m_AllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CDiaphragmLayoutDlg)
	m_Name = _T("");
	//}}AFX_DATA_INIT

   if (mode==libUnitsMode::UNITS_SI)
   {
      m_UnitLength = unitMeasure::Meter;
      m_UnitString = "(m)";
   }
   else
   {
      m_UnitLength = unitMeasure::Feet;
      m_UnitString = "(ft)";
   }
}

void CDiaphragmLayoutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiaphragmLayoutDlg)
	DDX_Text(pDX, IDC_NAME, m_Name);
	//}}AFX_DATA_MAP

   // validation routine for CGXGridWnd controls
	DDV_GXGridWnd(pDX, &m_wndGrid);

   // get data from grid
   if (pDX->m_bSaveAndValidate)
   {
      DiaphragmLayoutEntry::DiaphragmLayout dl;
      m_DiaphragmLayoutVec.clear();
      ROWCOL nrows = m_wndGrid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         if (GetRowData(i,&dl))
         {
            // values are in display units - must convert to system
            dl.EndOfRange = ::ConvertToSysUnits(dl.EndOfRange, m_UnitLength);
            m_DiaphragmLayoutVec.push_back(dl);
         }
         else
         {
            CString msg;
            msg.Format("Invalid data in row %d, please correct input",i);
            ::AfxMessageBox(msg, MB_OK|MB_ICONEXCLAMATION );
            m_wndGrid.SetFocus();
            pDX->Fail();
         }
      }

      // check to make sure that span lengths increase
      Float64 preval=0;
      int size = m_DiaphragmLayoutVec.size();
      for ( i=0; i<size; i++)
      {
         Float64 er = m_DiaphragmLayoutVec[i].EndOfRange;
         if (er <= preval)
         {
            ::AfxMessageBox("Span Length Values Must Increase with Rule #", MB_OK|MB_ICONEXCLAMATION );
            m_wndGrid.SetFocus();
            pDX->Fail();
         }
         preval = er;
      }

   }
}

bool CDiaphragmLayoutDlg::GetRowData(ROWCOL nRow, DiaphragmLayoutEntry::DiaphragmLayout* pdl)
{
   CString s = m_wndGrid.GetCellValue(nRow, 2);
   Float64 d = atof(s);
   if (s.IsEmpty() || (d==0.0 && s[0]!='0'))
      return false;
   ASSERT(d>=0);
   pdl->EndOfRange = d;

   s = m_wndGrid.GetCellValue(nRow, 3);
   Int32 i = atoi(s);
   if (s.IsEmpty() || (i==0 && s[0]!='0'))
      return false;
   ASSERT(i>=0);
   pdl->NumberOfDiaphragms = i;

   return true;
}


BEGIN_MESSAGE_MAP(CDiaphragmLayoutDlg, CDialog)
	//{{AFX_MSG_MAP(CDiaphragmLayoutDlg)
	ON_WM_NCACTIVATE()
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoverows)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmLayoutDlg message handlers

BOOL CDiaphragmLayoutDlg::OnInitDialog() 
{

	CDialog::OnInitDialog();

//Please refer to the MFC documentation on SubclassDlgItem for 
//information on this call. This makes sure that our C++ grid 
//window class subclasses the window that is created with the 
//User Control.
	m_wndGrid.SubclassDlgItem(IDC_DIAPHRAGM_GRID, this);

   // set up the grid
   m_wndGrid.CustomInit();

   // fill with initial data
   m_wndGrid.GetParam()->SetLockReadOnly(FALSE);
   int size = m_DiaphragmLayoutVec.size();
   if (size>0)
   {
      // size grid
      for (int i=0; i<size; i++)
	      m_wndGrid.Insertrow();

      // fill grid
      int nRow=1;
      Float64 endr = 0;
      Float64 end;
      for (DiaphragmLayoutEntry::DiaphragmLayoutVec::iterator it = m_DiaphragmLayoutVec.begin(); it!=m_DiaphragmLayoutVec.end(); it++)
      {
         m_wndGrid.SetValueRange(CGXRange(nRow, 1), endr);
         // unit conversion
         end = ::ConvertFromSysUnits((*it).EndOfRange, m_UnitLength);
         m_wndGrid.SetValueRange(CGXRange(nRow, 2), end);
         m_wndGrid.SetValueRange(CGXRange(nRow, 3), (*it).NumberOfDiaphragms);
         endr = (*it).EndOfRange;
         nRow++;
      }
   }
   else
   {
	   m_wndGrid.Insertrow();
   }
   m_wndGrid.GetParam()->SetLockReadOnly(TRUE);

   // update statics
   m_wndGrid.RescanRows();

   // no grid rows selected, so can't delete any
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);

   // disable OK button if editing not allowed
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      CString head;
      GetWindowText(head);
      head += " (Read Only)";
      SetWindowText(head);
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CDiaphragmLayoutDlg::OnNcActivate(BOOL bActive)
{
	if (GXDiscardNcActivate())
		return TRUE;

	return CDialog::OnNcActivate(bActive);
}

void CDiaphragmLayoutDlg::OnRemoverows() 
{
	m_wndGrid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CDiaphragmLayoutDlg::OnInsertrow() 
{
	m_wndGrid.Insertrow();
}

void CDiaphragmLayoutDlg::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

LRESULT CDiaphragmLayoutDlg::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIAPHRAGM_LAYOUT_DIALOG );
   return TRUE;
}
