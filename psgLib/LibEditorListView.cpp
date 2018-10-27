///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <psglib\LibEditorListView.h>
#include <psglib\LibraryEditorView.h>
#include <psglib\ISupportLibraryManager.h>
#include <LibraryFw\LibraryManager.h>

#include <psgLib\ISupportIcon.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CLibEditorListView

IMPLEMENT_DYNCREATE(CLibEditorListView, CListView)

CLibEditorListView::CLibEditorListView():
m_LibIndex(-1),
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
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndEditLabel)
	//}}AFX_MSG_MAP
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
   AFX_MANAGE_STATE(AfxGetAppModuleState());
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
   if (libnum == -1)
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
      CollectionIndexType num_managers = pLibMgr->GetNumberOfLibraryManagers();
      ASSERT(num_managers);

      // first we need to determine the total number of images in our imagelist
      CollectionIndexType num_entries=0;
      CollectionIndexType ilm = 0;
      for (ilm = 0; ilm < num_managers; ilm++)
      {
         libLibraryManager* plm = pLibMgr->GetLibraryManager(ilm);
         ASSERT(plm!=0);

         const libILibrary* plib = plm->GetLibrary(m_LibName);
         ASSERT(plib!=0);

         CollectionIndexType cnt = plib->GetCount();
         num_entries += cnt;
      }

      CImageList* images = rlist.GetImageList(LVSIL_NORMAL);
      images->SetImageCount((UINT)num_entries);
      images = rlist.GetImageList(LVSIL_SMALL);
      images->SetImageCount((UINT)num_entries);
      
      for (ilm = 0; ilm < num_managers; ilm++)
      {
         libLibraryManager* plm = pLibMgr->GetLibraryManager(ilm);
         ASSERT(plm!=0);

         const libILibrary* plib = plm->GetLibrary(m_LibName);
         ASSERT(plib!=0);

         if ( !plib->IsDepreciated() )
         {
            int i = 0;
            libKeyListType key_list;
            plib->KeyList(key_list);
            libKeyListIterator iter(key_list.begin());
            libKeyListIterator iterEnd(key_list.end());
            for ( ; iter != iterEnd; iter++ )
            {
               const std::_tstring& name = *iter;
               const libLibraryEntry* pEntry = plib->GetEntry(name.c_str());
               ATLASSERT(pEntry);
               int st = InsertEntryToList(pEntry, plib, i);
               ATLASSERT(st!=-1);
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
   ASSERT(plib_man != nullptr);

   libILibrary* plib = plib_man->GetLibrary(m_LibName);
   ASSERT(plib != nullptr);

   std::_tstring name = plib->GetUniqueEntryName();
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

      CollectionIndexType n = plib->GetCount();
      const libLibraryEntry* pentry = plib->GetEntry(name.c_str());
      CollectionIndexType it = InsertEntryToList(pentry, plib, ni-1);
      if (it != -1)
      {
         m_ItemSelected = (int)it;
         rlist.EditLabel((int)it);
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
   {
      this->EditEntry(plib,entry_name);
   }

	CListView::OnLButtonDblClk(nFlags, point);
}

void CLibEditorListView::OnRButtonDown(UINT nFlags, CPoint point) 
{
   // dont do anything if a library manager is selected
   if (IsLibrarySelected())
   {
      CListCtrl& rlist = this->GetListCtrl();

      CMenu menu;
	   if (!(menu.CreatePopupMenu()))
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
         UINT dodup = plib->IsCopyingEnabled(entry_name) ? MF_ENABLED|MF_STRING : MF_GRAYED|MF_STRING;

         menu.AppendMenu( dodel,                  IDM_DELETE_ENTRY, _T("Delete") );
         menu.AppendMenu( dodup,                  IDM_DUPLICATE_ENTRY, _T("Duplicate") );
         menu.AppendMenu( MF_STRING | MF_ENABLED, IDM_EDIT_ENTRY, _T("Edit") );
         menu.AppendMenu( dodel,                  IDM_RENAME_ENTRY, _T("Rename") );

         rlist.SetItemState( m_ItemSelected,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);   
      }
      else
      {
         menu.AppendMenu( MF_STRING | MF_ENABLED, IDM_ADD_ENTRY, _T("Add New Item") );
      }
	   CPoint pt;
      GetCursorPos(&pt);
      menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, GetParentFrame() );
   }
}

bool CLibEditorListView::EditEntry(libILibrary* plib, LPCTSTR entryName)
{
   ASSERT(plib);
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgrDoc = dynamic_cast<libISupportLibraryManager*>(pDoc);

   libILibrary::EntryEditOutcome eo = plib->EditEntry(entryName);
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
      AfxMessageBox(_T("The new name for the entry was invalid - Keeping the original name"));
      break;

   case libILibrary::NotFound:
      ASSERT(0); // this should never happen
      break;
   }

   return false;
}

void CLibEditorListView::DeleteEntry(libILibrary* plib, LPCTSTR entryName, bool force)
{
   ASSERT(plib);
   int st = IDYES;
   // prompt user if delete is not forced
   if (!force)
   {
      CString tmp(_T("Are you sure you want to delete \""));
      CString tmp2(entryName);
      tmp +=tmp2 + _T("\"");
      st = AfxMessageBox(tmp, MB_YESNO|MB_ICONQUESTION);
   }

   if (st==IDYES)
   {
      libILibrary::EntryRemoveOutcome ro = plib->RemoveEntry(entryName);
      switch(ro)
      {
      case libILibrary::RemReferenced:
         AfxMessageBox(_T("Entry is being used by other objects, cannot be deleted"));
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

void CLibEditorListView::DuplicateEntry(libILibrary* plib, LPCTSTR entryName)
{
   ASSERT(plib);

   // entries can only be duplicated into target library manager
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgr = dynamic_cast<libISupportLibraryManager*>(pDoc);
   libLibraryManager* plib_man = pLibMgr->GetTargetLibraryManager();
   ASSERT(plib_man!=0);

   std::_tstring lib_name = plib->GetDisplayName();
   libILibrary* ptarget_lib = plib_man->GetLibrary(lib_name.c_str());
   ASSERT(ptarget_lib);

   CString new_name = CString(entryName) + _T(" (Copy 1)");
   CString the_name = new_name;
   // make a new name for the entry
   int i=2;
   while(DoesEntryExist(the_name))
   {
      the_name.Format(_T("%s (Copy %d)"),entryName,i++);
   }

   if (plib==ptarget_lib)
   {
      // entry to be copied is in target library - do intralibrary copy
      if(!plib->CloneEntry(entryName, the_name))
      {
         ASSERT(0);
         AfxMessageBox(_T("An Error occurred trying to duplicate the entry"));
      }
   }
   else
   {
       //entry exists in other than target library - must  copy across library bounds
      std::unique_ptr<libLibraryEntry> pent(plib->CreateEntryClone(entryName));
      ASSERT(pent.get() != nullptr);
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
   {
      return plib->IsEditingEnabled(entry_name);
   }

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
      {
         this->DeleteEntry(plib, entry_name);
      }
   }
   else
   {
      ASSERT(0);
   }
}

void CLibEditorListView::DuplicateSelectedEntry()
{
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
   {
      this->DuplicateEntry(plib,entry_name);
   }
   else
   {
      ASSERT(0);
   }
}

bool CLibEditorListView::CanDuplicateEntry()
{
   CString entry_name;
   libILibrary* plib;
   if( !GetSelectedEntry(&entry_name, &plib) )
   {
      return false;
   }

   const libLibraryEntry* pEntry = plib->GetEntry(entry_name);
   if ( pEntry )
   {
      return pEntry->IsCopyingEnabled();
   }

   ATLASSERT(false); // entry should have been retrieved
   return false;
}

void CLibEditorListView::EditSelectedEntry()
{
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
   {
      this->EditEntry(plib,entry_name);
   }
   else
   {
      ASSERT(0);
   }
}

void CLibEditorListView::RenameSelectedEntry()
{
   CListCtrl& rlist = this->GetListCtrl();
   CString entry_name;
   libILibrary* plib;
   if(GetSelectedEntry(&entry_name, &plib))
   {
      rlist.SetFocus();
      rlist.EditLabel(m_ItemSelected);
   }
   else
   {
      ASSERT(0);
   }
}

BOOL CLibEditorListView::PreCreateWindow(CREATESTRUCT& cs) 
{
   // get list view mode settings from registry and set view
   CEAFApp* pApp = EAFGetApp();
   CString list_settings = pApp->GetProfileString(_T("Settings"),_T("LibView"),_T("Large Icons"));
   if (list_settings==_T("Large Icons"))
   {
      cs.style |= LVS_ICON;
   }
   else if (list_settings==_T("Report View"))
   {
      cs.style |= LVS_REPORT;
   }
   else if (list_settings==_T("List View"))
   {
      cs.style |= LVS_LIST;
   }
   else
   {
      cs.style |= LVS_SMALLICON;
   }

   // allow single selection of entries only.
   cs.style |= LVS_SINGLESEL;
   // allow edting of entry labels
   cs.style |= LVS_EDITLABELS;
	
	return CListView::PreCreateWindow(cs);
}

void CLibEditorListView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_DELETE && IsItemSelected())
   {
      DeleteSelectedEntry();
   }
   else if (nChar == VK_RETURN && IsItemSelected() )
   {
      EditSelectedEntry();
   }
   else if ( nChar == VK_F2 && IsEditableItemSelected() )
   {
      RenameSelectedEntry();
   }
	
	CListView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CLibEditorListView::OnDestroy() 
{
   // Save the list mode setting
   CString mode = _T("Small Icons");
   DWORD	dwStyle = GetWindowLong(GetSafeHwnd(), GWL_STYLE);
					
   if ((dwStyle & LVS_TYPEMASK) == LVS_ICON)
   {
      mode = _T("Large Icons");
   }
   else if ((dwStyle & LVS_TYPEMASK) == LVS_REPORT)
   {
      mode = _T("Report View");
   }
   else if ((dwStyle & LVS_TYPEMASK) == LVS_LIST)
   {
      mode = _T("List View");
   }

   CEAFApp* pApp = EAFGetApp();
   ASSERT(pApp);
   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("LibView"), mode ));
	
	CListView::OnDestroy();
}

bool CLibEditorListView::DoesEntryExist(const CString& entryName)
{
   // cycle through all library managers and see if the entry exists
   // in the named library
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgr = dynamic_cast<libISupportLibraryManager*>(pDoc);
   libLibraryManager* plib_man = pLibMgr->GetTargetLibraryManager();

   CollectionIndexType num_managers = pLibMgr->GetNumberOfLibraryManagers();
   ASSERT(num_managers);
   for (CollectionIndexType ilm=0; ilm<num_managers; ilm++)
   {
      libLibraryManager* plm = pLibMgr->GetLibraryManager(ilm);
      ASSERT(plm!=0);

      const libILibrary* plib = plm->GetLibrary(m_LibName);
      ASSERT(plib!=0);

      if (plib->IsEntry(entryName))
      {
         return true;
      }
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

void CLibEditorListView::OnEndEditLabel(NMHDR* pNMHDR, LRESULT* pResult) 
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
         ::AfxMessageBox(_T("Entry names cannot be blank"),MB_OK|MB_ICONEXCLAMATION);
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
                  CDocument* pDoc = GetDocument();
                  pDoc->SetModifiedFlag(true);
            	   *pResult = TRUE;
               }
               else if ( new_plib->IsReservedName(new_name) )
               {
                  ::AfxMessageBox(new_name + _T(" is a reserved name"),MB_OK|MB_ICONEXCLAMATION);
               }
               else
               {
                  ::AfxMessageBox(_T("Invalid entry name - Keeping original"),MB_OK|MB_ICONEXCLAMATION);
               }
            }
         }
         else
         {
            ::AfxMessageBox(_T("Entry is read-only cannot rename"),MB_OK|MB_ICONEXCLAMATION);
         }
      }
      else
      {
         ASSERT(0);
      }

   }
}

int CLibEditorListView::InsertEntryToList(const libLibraryEntry* pentry, const libILibrary* plib, int i)
{
   CListCtrl& rlist = this->GetListCtrl( );

   std::_tstring entryName = pentry->GetName();
   Uint32 ref_cnt = pentry->GetRefCount();

   bool read_only = !pentry->IsEditingEnabled();

   // try to cast entry to see if it has an icon associated with it
   // if not, associate the default entry icon
   const ISupportIcon* isi = dynamic_cast<const ISupportIcon*>(pentry);
   HICON icon;
   if (isi!=nullptr)
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
   if (0 < ref_cnt && read_only)
   {
      iconnum = CLibraryEditorView::InUseAndReadOnly;
   }
   else if (0 < ref_cnt)
   {
      iconnum = CLibraryEditorView::InUse;
   }
   else if (read_only)
   {
      iconnum = CLibraryEditorView::ReadOnly;
   }
   else
   {
      iconnum = 0;
   }

    LV_ITEM  lvi;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    lvi.iItem = i;
    lvi.iSubItem = 0;

    // Set the text of the item.
    TCHAR text[256];
    ATLASSERT(entryName.size()<256);
    _tcscpy_s(text,sizeof(text)/sizeof(TCHAR), entryName.c_str());
    lvi.pszText        = text;  

    lvi.iImage         = i;
    lvi.state          = INDEXTOSTATEIMAGEMASK (iconnum);
    lvi.stateMask      = LVIS_STATEIMAGEMASK;
    lvi.lParam         = (LPARAM)plib; // entry holds pointer to its library

    return rlist.InsertItem (&lvi);
}

