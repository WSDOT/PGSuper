///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// LibraryEntryConflict.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "LibraryEntryConflict.h"
#include "RenameLibraryEntry.h"
#include "..\htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLibraryEntryConflict dialog


CLibraryEntryConflict::CLibraryEntryConflict(const std::_tstring& entryName, const std::_tstring& libName, 
                                             const std::vector<std::_tstring>& keylists, bool isImported, CWnd* pParent)
	: CDialog(CLibraryEntryConflict::IDD, pParent),
   m_KeyList(keylists),
   m_EntryName(entryName.c_str()),
   m_LibName(libName.c_str()),
   m_IsImported(isImported)
{
	//{{AFX_DATA_INIT(CLibraryEntryConflict)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CLibraryEntryConflict::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLibraryEntryConflict)
	DDX_Control(pDX, IDC_OVERWRITE, m_Overwrite);
	DDX_Control(pDX, IDC_CONFLICT_BOTTOM, m_ConflictBottom);
	DDX_Control(pDX, IDC_CONFLICT_TOP, m_ConflictTop);
	DDX_Control(pDX, IDC_ENTRY_TEXT, m_EntryText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLibraryEntryConflict, CDialog)
	//{{AFX_MSG_MAP(CLibraryEntryConflict)
	ON_BN_CLICKED(IDC_RENAME_ENTRY, OnRenameEntry)
	ON_BN_CLICKED(IDC_OVERWRITE, OnOverwrite)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryEntryConflict message handlers

void CLibraryEntryConflict::OnRenameEntry() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CRenameLibraryEntry dlg;
   bool done=false;
   while (!done)
   {
      INT_PTR st = dlg.DoModal();
      if (st==IDOK)
      {
         if (dlg.m_EntryName.IsEmpty())
         {
            ::AfxMessageBox(_T("Please type in a valid entry name"),MB_OK|MB_ICONEXCLAMATION);
         }
         else
         {
            // user typed in a name. make sure it's not in the keylist
            bool in_list = false;
            for (std::vector<std::_tstring>::const_iterator it=m_KeyList.begin(); it!=m_KeyList.end(); it++)
            {
               CString list_memb(it->c_str());
               if (list_memb.CompareNoCase(dlg.m_EntryName)==0)
               {
                  in_list = true;
                  // user typed in a name that's already in list. bark at him 
                  // and ask him type type in another one.
                  CString temp;
                  temp.Format(_T("The name %s is already in the library."),dlg.m_EntryName);
                  ::AfxMessageBox(temp,MB_OK|MB_ICONEXCLAMATION);
                  break;
               }
            }

            if (!in_list)
            {
	            m_OutCome = Rename;
               m_NewName = dlg.m_EntryName;
               CDialog::EndDialog(0);
               done = true;
            }
         }
      }
      else
         done=true;
   }
}

void CLibraryEntryConflict::OnOverwrite() 
{
	m_OutCome = OverWrite;
   CDialog::EndDialog(0);
}

BOOL CLibraryEntryConflict::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString text;
   text.Format(_T("The entry name is %s in the %s"),m_EntryName,m_LibName);
   m_EntryText.SetWindowText(text);

   if (m_IsImported)
   {
      CString s;
      VERIFY(s.LoadString(IDS_CONFLICT_BOTTOM_IMPORT));
      m_ConflictBottom.SetWindowText(s);
      VERIFY(s.LoadString(IDS_CONFLICT_TOP_IMPORT));
      m_ConflictTop.SetWindowText(s);
      m_Overwrite.SetWindowText(_T("Skip"));
   }
   else
   {
      CString s;
      VERIFY(s.LoadString(IDS_CONFLICT_BOTTOM_MASTER));
      m_ConflictBottom.SetWindowText(s);
      VERIFY(s.LoadString(IDS_CONFLICT_TOP_MASTER));
      m_ConflictTop.SetWindowText(s);
      m_Overwrite.SetWindowText(_T("Overwrite"));
   }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLibraryEntryConflict::OnHelp() 
{
   if (m_IsImported)
      ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_LIBIMPORTENTRYCONFLICT  );
   else
      ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_LIBENTRYCONFLICT );
}
