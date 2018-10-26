///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
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

// LibEditorListView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "LibEditorListView.h"
#include "LibraryEditorView.h"
#include <psglib\ISupportLibraryManager.h>
#include <LibraryFw\LibraryManager.h>

#include <psgLib\ISupportIcon.h>


#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

// menu stuff
const int CMENU_BASE=100; // command id's will be numbered 100, 101, 102, etc for each menu item
const int MAX_CMENUS=100; // this is limiting, but it is MFC's fault.
const int CMENU_MAX = CMENU_BASE+MAX_CMENUS;


/////////////////////////////////////////////////////////////////////////////
// CLibEditorListView

IMPLEMENT_DYNCREATE(CLibEditorListView, CListView)

CLibEditorListView::CLibEditorListView():
m_LibIndex(-1),
m_pMenu(0),
m_ItemSelected(-1)
{
}

CLibEditorListView::~CLibEditorListView()
{
}


BEGIN_MESSAGE_MAP(CLibEditorListView, CListView)
	//{{AFX_MSG_MAP(CLibEditorListView)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	//}}AFX_MSG_MAP
    ON_COMMAND_RANGE(CMENU_BASE, CMENU_MAX, OnCmenuSelected)
END_MESSAGE_MAP()


void CLibEditorListView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();
	
   // load up image lists
   CListCtrl& rlist = this->GetListCtrl( );

   m_NormalEntryImages.Create(32,32,TRUE,12,6);
   rlist.SetImageList(&m_NormalEntryImages,LVSIL_NORMAL);

   m_SmallEntryImages.Create(32,32,TRUE,12,6);
   rlist.SetImageList(&m_SmallEntryImages,LVSIL_SMALL);
	
}

/////////////////////////////////////////////////////////////////////////////
// CLibEditorListView drawing

void CLibEditorListView::OnDraw(CDC* pDC)
{
}

/////////////////////////////////////////////////////////////////////////////
// CLibEditorListView diagnostics

#ifdef _DEBUG
void CLibEditorListView::AssertValid() const
{
	CListView::AssertValid();
}

void CLibEditorListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLibEditorListView message handlers
void CLibEditorListView::OnLibrarySelected(int libnum, const CString& name)
{
   if (libnum==-1)
   {
      // user selected a library manager node - display a blank list
      CListCtrl& rlist = this->GetListCtrl( );
      rlist.DeleteAllItems();
      m_LibName = "Library Manager";
      m_LibIndex = -1;
   }
   else
   {
      m_LibName = name;
      m_LibIndex = libnum;
      this->RedrawAllEntries();
   }
}

void CLibEditorListView::RedrawAllEntries()
{
   ASSERT(!m_LibName.IsEmpty());

   // dont do anything if a library manager is selected
   if (IsLibrarySelected())
   {
      // remove all entries
      CListCtrl& rlist = this->GetListCtrl( );
      rlist.DeleteAllItems();

      // cycle through all library managers and add all entries
      // get names of all entries and update list control
      CDocument* pDoc = GetDocument();
      libISupportLibraryManager* pLibMgr = dynamic_cast<libISupportLibraryManager*>(pDoc);
      int num_managers = pLibMgr->GetNumberOfLibraryManagers();
      ASSERT(num_managers);

      // first we need to determine the total number of images in our imagelist
      int num_entries=0;
      int ilm = 0;
      for (ilm=0; ilm<num_managers; ilm++)
      {
         libLibraryManager* plm = pLibMgr->GetLibraryManager(ilm);
         ASSERT(plm!=0);

         const libILibrary* plib = plm->GetLibrary(m_LibName);
         ASSERT(plib!=0);

         int cnt = plib->GetCount();
         num_entries += cnt;
      }

      CImageList* images = rlist.GetImageList(LVSIL_NORMAL);
      images->SetImageCount(num_entries);
      images = rlist.GetImageList(LVSIL_SMALL);
      images->SetImageCount(num_entries);
      
      for (ilm=0; ilm<num_managers; ilm++)
      {
         libLibraryManager* plm = pLibMgr->GetLibraryManager(ilm);
         ASSERT(plm!=0);

         const libILibrary* plib = plm->GetLibrary(m_LibName);
         ASSERT(plib!=0);

         if ( !plib->IsDepreciated() )
         {
            int i=0;
            libKeyListType key_list;
            plib->KeyList(key_list);
            for (libKeyListIterator kit = key_list.begin(); kit != key_list.end(); kit++)
            {
               const std::string& name = *kit;
               const libLibraryEntry* pentry = plib->GetEntry(name.c_str());
               CHECK(pentry);
               int st = InsertEntryToList(pentry, plib, i);
               CHECK(st!=-1);
               i++;
            }
         }
      }
   }
}

bool CLibEditorListView::AddNewEntry()
{
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgr = dynamic_cast<libISupportLibraryManager*>(pDoc);
   libLibraryManager* plib_man = pLibMgr->GetTargetLibraryManager();
   ASSERT(plib_man!=0);

   libILibrary* plib = plib_man->GetLibrary(m_LibName);
   ASSERT(plib!=0);

   std::string name = plib->GetUniqueEntryName();
   if (plib->NewEntry(name.c_str()))
   {
      // add a single place for a new entry on our image lists
      CListCtrl& rlist = this->GetListCtrl( );
      CImageList* images = rlist.GetImageList(LVSIL_NORMAL);
      int ni = images->GetImageCount();  // quick way to get count of all entries (multiple manager scenario)
      ni++;
      images->SetImageCount(ni);

      images = rlist.GetImageList(LVSIL_SMALL);
      images->SetImageCount(ni);

      int n = plib->GetCount();
      const libLibraryEntry* pentry = plib->GetEntry(name.c_str());
      int it = InsertEntryToList(pentry, plib, ni-1);
      if (it!=-1)
      {
         m_ItemSelected = it;
         rlist.EditLabel(it);
         pDoc->SetModifiedFlag(true);
         return true;
      }
   }

   return false;
}

void CLibEditorListView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
   CListCtrl& rlist = this->GetListCtrl();

   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
      this->EditEntry(plib,entry_name);

	CListView::OnLButtonDblClk(nFlags, point);
}

// right button menus
enum rbmenu { RB_DELETE, RB_DUPLICATE, RB_EDIT, RB_RENAME, RB_ADDNEW};

void CLibEditorListView::OnRButtonDown(UINT nFlags, CPoint point) 
{
   // dont do anything if a library manager is selected
   if (IsLibrarySelected())
   {
      CListCtrl& rlist = this->GetListCtrl();

      m_pMenu = new CMenu();
      ASSERT(m_pMenu);
	   if (!(m_pMenu->CreatePopupMenu()))
	   {
		   MessageBox(_T("Could not create CMenu"));
	   }

	   int idx;
	   LV_HITTESTINFO lvH;
	   lvH.pt.x = point.x;
      lvH.pt.y = point.y;	   
      idx = rlist.HitTest(&lvH);
      m_ItemSelected = idx;

      if ( idx != -1)
      {
         CString entry_name = rlist.GetItemText(m_ItemSelected,0);
         libILibrary* plib = (libILibrary*)rlist.GetItemData(m_ItemSelected);
         ASSERT(plib);
         UINT dodel = plib->IsEditingEnabled(entry_name) ? MF_ENABLED|MF_STRING : MF_GRAYED|MF_STRING;

         m_pMenu->AppendMenu( dodel,                  CMENU_BASE+RB_DELETE, "Delete" );
         m_pMenu->AppendMenu( MF_STRING | MF_ENABLED, CMENU_BASE+RB_DUPLICATE, "Duplicate" );
         m_pMenu->AppendMenu( MF_STRING | MF_ENABLED, CMENU_BASE+RB_EDIT, "Edit" );
         m_pMenu->AppendMenu( dodel,                  CMENU_BASE+RB_RENAME, "Rename" );

         rlist.SetItemState( m_ItemSelected,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);   
      }
      else
      {
         m_pMenu->AppendMenu( MF_STRING | MF_ENABLED, CMENU_BASE+RB_ADDNEW, "Add New Item" );
      }
	   CPoint pt;
      GetCursorPos(&pt);
      this->SetForegroundWindow();
      m_pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this );
      // send a null message to keep popup from hogging messages. Refer to KB article
      // Article ID: Q135788 for more details
      ::PostMessage(this->m_hWnd, WM_NULL, 0, 0);
   }
}

void CLibEditorListView::OnCmenuSelected(UINT id)
{
   delete m_pMenu;

   CListCtrl& rlist = this->GetListCtrl();

   int mi = id-CMENU_BASE;

   if (m_ItemSelected!=-1)
   {
      CString text = rlist.GetItemText(m_ItemSelected,0);
      libILibrary* plib = (libILibrary*)rlist.GetItemData(m_ItemSelected);

      if (mi==RB_DELETE)
      {
         if (plib->IsEditingEnabled(text))
            this->DeleteEntry(plib, text);
      }
      else if (mi==RB_DUPLICATE)
      {
         this->DuplicateEntry(plib,text);
      }
      else if (mi== RB_EDIT)
      {
         this->EditEntry(plib,text);
      }
      else if (mi== RB_RENAME)
      {
         rlist.EditLabel(m_ItemSelected);
      }
      else
         ASSERT(0);
   }
   else
   {
      if (mi==RB_ADDNEW)
      {
         this->AddNewEntry();
      }
      else
         ASSERT(0);
   }

}


bool CLibEditorListView::EditEntry(libILibrary* plib, const char* entryName)
{
   ASSERT(plib);
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgrDoc = dynamic_cast<libISupportLibraryManager*>(pDoc);

   libILibrary::EntryEditOutcome eo = plib->EditEntry(entryName, pLibMgrDoc->GetUnitsMode());
   switch(eo)
   {
   case libILibrary::Ok:
      // document was changed-update.
      pDoc->SetModifiedFlag(true);
      this->RedrawAllEntries();

      // re-select our entry
      if (m_ItemSelected!=-1)
      {
         CListCtrl& rlist = this->GetListCtrl();
         rlist.SetItemState( m_ItemSelected,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);   
      }

      return true;
   case libILibrary::RenameFailed:
      AfxMessageBox("The new name for the entry was invalid - Keeping the original name");
      break;
   case libILibrary::NotFound:
      ASSERT(0); // this should never happen
      break;
   }

   return false;
}

void CLibEditorListView::DeleteEntry(libILibrary* plib, const char* entryName, bool force)
{
   ASSERT(plib);
   int st = IDYES;
   // prompt user if delete is not forced
   if (!force)
   {
      CString tmp("Are you sure you want to delete \"");
      CString tmp2(entryName);
      tmp +=tmp2 + "\"";
      st = AfxMessageBox(tmp, MB_YESNO|MB_ICONQUESTION);
   }

   if (st==IDYES)
   {
      libILibrary::EntryRemoveOutcome ro = plib->RemoveEntry(entryName);
      switch(ro)
      {
      case libILibrary::RemReferenced:
         AfxMessageBox("Entry is being used by other objects, cannot be deleted");
         break;
      case libILibrary::NotFound:
         ASSERT(0); // this should never happen
         break;
      default:
         // document was changed-update.
         CDocument* pDoc = GetDocument();
         pDoc->SetModifiedFlag(true);
         this->RedrawAllEntries();
      }
   }
}

void CLibEditorListView::DuplicateEntry(libILibrary* plib, const char* entryName)
{
   ASSERT(plib);

   // entries can only be duplicated into target library manager
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgr = dynamic_cast<libISupportLibraryManager*>(pDoc);
   libLibraryManager* plib_man = pLibMgr->GetTargetLibraryManager();
   ASSERT(plib_man!=0);

   std::string lib_name = plib->GetDisplayName();
   libILibrary* ptarget_lib = plib_man->GetLibrary(lib_name.c_str());
   ASSERT(ptarget_lib);

   CString new_name = CString(entryName) + " (Copy 1)";
   CString the_name = new_name;
   // make a new name for the entry
   int i=2;
   while(DoesEntryExist(the_name))
   {
      the_name.Format("%s (Copy %d)",entryName,i++);
   }

   if (plib==ptarget_lib)
   {
      // entry to be copied is in target library - do intralibrary copy
      if(!plib->CloneEntry(entryName, the_name))
      {
         ASSERT(0);
         AfxMessageBox("An Error occurred trying to duplicate the entry");
      }
   }
   else
   {
       //entry exists in other than target library - must  copy across library bounds
      std::auto_ptr<libLibraryEntry> pent(plib->CreateEntryClone(entryName));
      ASSERT(pent.get()!=0);
      VERIFY(ptarget_lib->AddEntry(*pent, the_name));
   }

   // make the new entry so we can edit it
   ptarget_lib->EnableEditing(the_name,true);

   // document was changed-update.
   pDoc->SetModifiedFlag(true);
   this->RedrawAllEntries();
}

void CLibEditorListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if (IsLibrarySelected())
      this->RedrawAllEntries();
}

bool CLibEditorListView::IsItemSelected()const
{
   // a library must be selected
   if (!IsLibrarySelected())
      return false;

   // only return true if one item is selected
   CListCtrl& rlist = this->GetListCtrl();
   return (rlist.GetSelectedCount()==1);
}

bool CLibEditorListView::IsEditableItemSelected()const
{
   // the library must not be read only
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
      return plib->IsEditingEnabled(entry_name);

   return false;
}

bool CLibEditorListView::IsLibrarySelected() const
{
   // a library must be selected
   return m_LibIndex!=-1;
}

void CLibEditorListView::DeleteSelectedEntry()
{
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
   {
      if (plib->IsEditingEnabled(entry_name))
         this->DeleteEntry(plib, entry_name);
   }
   else
      ASSERT(0);
}

void CLibEditorListView::DuplicateSelectedEntry()
{
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
      this->DuplicateEntry(plib,entry_name);
   else
      ASSERT(0);
}

void CLibEditorListView::EditSelectedEntry()
{
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
      this->EditEntry(plib,entry_name);
   else
      ASSERT(0);
}

void CLibEditorListView::RenameSelectedEntry()
{
   CListCtrl& rlist = this->GetListCtrl();
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
   {
      rlist.EditLabel(m_ItemSelected);
      CDocument* pDoc = GetDocument();
      pDoc->SetModifiedFlag(true);
   }
   else
      ASSERT(0);
}

BOOL CLibEditorListView::PreCreateWindow(CREATESTRUCT& cs) 
{
   // get list view mode settings from registry and set view
   CWinApp* papp = ::AfxGetApp();
   CString list_settings = papp->GetProfileString(_T("Settings"),_T("LibView"),_T("Large Icons"));
   if (list_settings=="Large Icons")
      cs.style |= LVS_ICON;
   else if (list_settings=="Report View")
      cs.style |= LVS_REPORT;
   else if (list_settings=="List View")
      cs.style |= LVS_LIST;
   else
      cs.style |= LVS_SMALLICON;

   // allow single selection of entries only.
   cs.style |= LVS_SINGLESEL;
   // allow edting of entry labels
   cs.style |= LVS_EDITLABELS;
	
	return CListView::PreCreateWindow(cs);
}

void CLibEditorListView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_DELETE)
   {
      if (IsItemSelected())
         DeleteSelectedEntry();
   }
   else 	if (nChar == VK_RETURN)
   {
      if (IsItemSelected())
         EditSelectedEntry();
   }
   else if ( nChar == VK_F2 )
   {
      if ( IsEditableItemSelected() )
         RenameSelectedEntry();
   }

	
	CListView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CLibEditorListView::OnDestroy() 
{
   // Save the list mode setting
   CString mode = "Small Icons";
   DWORD	dwStyle = GetWindowLong(GetSafeHwnd(), GWL_STYLE);
					
   if ((dwStyle & LVS_TYPEMASK) == LVS_ICON)
      mode = "Large Icons";
   else if ((dwStyle & LVS_TYPEMASK) == LVS_REPORT)
      mode = "Report View";
   else if ((dwStyle & LVS_TYPEMASK) == LVS_LIST)
      mode = "List View";

   CWinApp* papp = ::AfxGetApp();
   ASSERT(papp);
   VERIFY(papp->WriteProfileString( _T("Settings"),_T("LibView"), mode ));
	
	CListView::OnDestroy();
}

bool CLibEditorListView::DoesEntryExist(const CString& entryName)
{
   // cycle through all library managers and see if the entry exists
   // in the named library
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgr = dynamic_cast<libISupportLibraryManager*>(pDoc);
   libLibraryManager* plib_man = pLibMgr->GetTargetLibraryManager();

   int num_managers = pLibMgr->GetNumberOfLibraryManagers();
   ASSERT(num_managers);
   for (int ilm=0; ilm<num_managers; ilm++)
   {
      libLibraryManager* plm = pLibMgr->GetLibraryManager(ilm);
      ASSERT(plm!=0);

      const libILibrary* plib = plm->GetLibrary(m_LibName);
      ASSERT(plib!=0);

      if (plib->IsEntry(entryName))
         return true;
   }
   return false;
}

bool CLibEditorListView::GetSelectedEntry(CString* pentryName, libILibrary** pplib) const
{
   // return the name of the entry and the library it belongs to
   CListCtrl& rlist = this->GetListCtrl();
   int idx = rlist.GetNextItem(-1,LVNI_SELECTED);
   m_ItemSelected=idx;
   if ( idx != -1)
   {
      *pentryName = rlist.GetItemText(idx,0);
      *pplib = (libILibrary*)rlist.GetItemData(idx);
      ASSERT(*pplib!=0);
      return true;
   }
   return false;
}

void CLibEditorListView::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

   // assume the worst
	*pResult = FALSE;

   // get the new name and the old name
   if (pDispInfo->item.pszText!=0)
   {
      CString new_name(pDispInfo->item.pszText);
      new_name.TrimLeft();
      new_name.TrimRight();
      if (new_name.IsEmpty())
      {
         ::AfxMessageBox("Entry names cannot be blank",MB_OK|MB_ICONEXCLAMATION);
         return;
      }

      ASSERT(pDispInfo->item.lParam!=0);
      libILibrary* new_plib = (libILibrary*)pDispInfo->item.lParam;

      CString old_name;
      libILibrary* old_plib;
      if(GetSelectedEntry(&old_name, &old_plib))
      {
         ASSERT(old_plib==new_plib);
         if (old_plib->IsEditingEnabled(old_name))
         {
            if (old_name!=new_name)
            {
               if (new_plib->RenameEntry(old_name, new_name))
               {
            	   *pResult = TRUE;
               }
               else if ( new_plib->IsReservedName(new_name) )
               {
                  ::AfxMessageBox(new_name + " is a reserved name",MB_OK|MB_ICONEXCLAMATION);
               }
               else
               {
                  ::AfxMessageBox("Invalid entry name - Keeping original",MB_OK|MB_ICONEXCLAMATION);
               }
            }
         }
         else
         {
            ::AfxMessageBox("Entry is read-only cannot rename",MB_OK|MB_ICONEXCLAMATION);
         }
      }
      else
         ASSERT(0);

   }
}

int CLibEditorListView::InsertEntryToList(const libLibraryEntry* pentry, const libILibrary* plib, int i)
{
   CListCtrl& rlist = this->GetListCtrl( );

   std::string entryName = pentry->GetName();
   Uint32 ref_cnt = pentry->GetRefCount();
   bool read_only = !pentry->IsEditingEnabled();

   // try to cast entry to see if it has an icon associated with it
   // if not, associate the default entry icon
   const ISupportIcon* isi = dynamic_cast<const ISupportIcon*>(pentry);
   HICON icon;
   if (isi!=NULL)
   {
      icon = isi->GetIcon();
   }
   else
   {
      icon = ::LoadIcon(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDI_DEFAULT_ENTRY));
   }

   CImageList* images = rlist.GetImageList(LVSIL_NORMAL);
   images->Replace(i, icon);

   images = rlist.GetImageList(LVSIL_SMALL);
   images->Replace(i, icon);

   // need to put a wart on the entry to show that it's read-only or in use
   int iconnum;
   if (ref_cnt>0 && read_only)
      iconnum = CLibraryEditorView::InUseAndReadOnly;
   else if (ref_cnt>0)
      iconnum = CLibraryEditorView::InUse;
   else if (read_only)
      iconnum = CLibraryEditorView::ReadOnly;
   else
      iconnum = 0;

    LV_ITEM  lvi;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    lvi.iItem = i;
    lvi.iSubItem = 0;

    // Set the text of the item.
    char text[256];
    CHECK(entryName.size()<256);
    strcpy_s(text,sizeof(text), entryName.c_str());
    lvi.pszText        = text;  

    lvi.iImage         = i;
    lvi.state          = INDEXTOSTATEIMAGEMASK (iconnum);
    lvi.stateMask      = LVIS_STATEIMAGEMASK;
    lvi.lParam         = (LPARAM)plib; // entry holds pointer to its library

    return rlist.InsertItem (&lvi);
}

