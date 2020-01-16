///////////////////////////////////////////////////////////////////////
// Library Editor - Editor for WBFL Library Services
// Copyright © 1999-2020  Washington State Department of Transportation
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

// LibraryView.cpp : implementation of the CLibraryEditorView class
//

#include "stdafx.h"
#include <psglib\LibraryEditorView.h>
#include <PsgLib\ISupportLibraryManager.h>
#include <LibraryFw\LibraryManager.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

#if defined _BUILD_LIBEDITOR_
#include "resource.h" // resource file for library editor
#else
#include "..\PGSuperAppPlugin\resource.h" // resource file for PGSuper
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define color_mask RGB(255,255,255)

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorView

IMPLEMENT_DYNCREATE(CLibraryEditorView, CTreeView)

BEGIN_MESSAGE_MAP(CLibraryEditorView, CTreeView)
	//{{AFX_MSG_MAP(CLibraryEditorView)
	ON_WM_CREATE()
   ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorView construction/destruction

CLibraryEditorView::CLibraryEditorView():
m_pListView(0)
{
	// TODO: add construction code here

}

CLibraryEditorView::~CLibraryEditorView()
{
}

BOOL CLibraryEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorView drawing

void CLibraryEditorView::OnDraw(CDC* pDC)
{
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorView printing

BOOL CLibraryEditorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CLibraryEditorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CLibraryEditorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorView diagnostics

#ifdef _DEBUG
void CLibraryEditorView::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
	CTreeView::AssertValid();
}

void CLibraryEditorView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLibraryEditorView message handlers

int CLibraryEditorView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   lpCreateStruct->style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;

	if (CTreeView::OnCreate(lpCreateStruct) == -1)
   {
		return -1;
   }

   return 0;
}

void CLibraryEditorView::OnInitialUpdate() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CTreeView::OnInitialUpdate();

   // get library manager(s)
   ASSERT(m_pListView);
   CDocument* pDoc = GetDocument();
   libISupportLibraryManager* pLibMgrDoc = dynamic_cast<libISupportLibraryManager*>(pDoc);
   CollectionIndexType num_managers = pLibMgrDoc->GetNumberOfLibraryManagers();
   ASSERT(num_managers);
   if (num_managers==0)
   {
      return;
   }

   // create and load up image list for tree control and list view
   // load two icons for each library. a normal and a selected
   m_LibraryImages.Create(16,16,TRUE,20,10);
   m_StateImages.Create(32,32,TRUE,3,1);

   // load names and images into tree control
	CTreeCtrl& tree = GetTreeCtrl();
   tree.SetImageList(&m_LibraryImages,LVSIL_NORMAL);

   CWinApp* the_app = AfxGetApp( );
   HICON icon = the_app->LoadIcon(IDI_LIBRARY_MANAGER);
   VERIFY(icon);  // special icon for root nodes
   int st = m_LibraryImages.Add(icon);
   ASSERT(st!=-1);
   if (st==-1)
   {
      return;
   }

   // closed folder icon
   icon = the_app->LoadIcon(IDI_FOLDER);
   VERIFY(icon);  
   st = m_LibraryImages.Add(icon);
   ASSERT(st!=-1);
   if (st==-1) 
   {
      return;
   }

   // open/selected folder icon
   icon = the_app->LoadIcon(IDI_OPEN_FOLDER);
   VERIFY(icon);  
   st = m_LibraryImages.Add(icon);
   ASSERT(st!=-1);
   if (st==-1)
   {
      return;
   }

   int last_icon=2; // running count of icons

   // insert the first library manager in the list into the tree
   // NOTE: the assumption here is that all of the library managers in the 
   //       list contain the same library types. This may not fit all 
   //       applications of the editor
   int ui=0;
   libLibraryManager* pMan = pLibMgrDoc->GetLibraryManager(ui);
   ASSERT(pMan);
   std::_tstring name = _T("Libraries"); // pMan->GetName();

   HTREEITEM hitm = tree.InsertItem(name.c_str(),0,0);
   ASSERT(hitm);
   // library managers have an item number of -1
   tree.SetItemData(hitm, -1);
   // add libraries to tree
   this->InsertLibraryManager(1, 2, ui, pMan, tree, hitm, &last_icon);

   // load state images for list control
   // NOTE: this order is important and is kept track of by the IconSequence
   CBitmap bmin_use;
   st = bmin_use.LoadBitmap(IDB_IN_USE);  // in use icon
   ASSERT(st);
   st = m_StateImages.Add(&bmin_use, color_mask);
   ASSERT(st!=-1);

   CBitmap bmr_o;
   st = bmr_o.LoadBitmap(IDB_READ_ONLY);  // read-only icon
   ASSERT(st);
   st = m_StateImages.Add(&bmr_o, color_mask);
   ASSERT(st!=-1);

   CBitmap bm_ro_iu;
   st = bm_ro_iu.LoadBitmap(IDB_IN_USE_AND_READ_ONLY);  // both in use and read-only
   ASSERT(st);
   st = m_StateImages.Add(&bm_ro_iu, color_mask);
   ASSERT(st!=-1);

   // Make the tree expanded
   CTreeCtrl& ctrl = GetTreeCtrl();
   ctrl.Expand(ctrl.GetRootItem(),TVE_EXPAND);

   // load image list into list control
   CListCtrl& list = m_pListView->GetListCtrl( );
   list.SetImageList(&m_StateImages,LVSIL_STATE); // state images
}


void CLibraryEditorView::InsertLibraryManager(Uint32 ilib_man, Uint32 ilib_man_sel, 
                                              int man_num, libLibraryManager* pMan, CTreeCtrl& tree, HTREEITEM hParent, int* lastIcon)
{
   CEAFApp* papp = EAFGetApp();
   CollectionIndexType nlibs = pMan->GetLibraryCount();
   if (nlibs==0) return;

   std::_tstring disp_name;
   for (CollectionIndexType ui=0; ui<nlibs; ui++)
   {
      // use same folder icon for all libraries
      CollectionIndexType ilib = ilib_man;
      CollectionIndexType isel = ilib_man_sel;

      if ( !pMan->IsDepreciated(ui) )
      {
         disp_name = pMan->GetLibraryDisplayName(ui);   

         HTREEITEM hitm = tree.InsertItem(disp_name.c_str(), (int)ilib, (int)isel, hParent);
         // associate library manager with library so later we can figure out what got selected
         tree.SetItemData(hitm, ui);
      }
   }
}

void CLibraryEditorView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
   // tell the entry list view to update itself.
	CTreeCtrl& tree = GetTreeCtrl();
   HTREEITEM hItem = tree.GetSelectedItem();
   CString library_name = tree.GetItemText( hItem );
   DWORD_PTR ilib       = tree.GetItemData(hItem);

   m_pListView->OnLibrarySelected((int)ilib, library_name);
	
	*pResult = 0;
}

void CLibraryEditorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if (m_pListView)
   {
      m_pListView->OnUpdate(pSender, lHint, pHint);
   }

   // Make the tree expanded
   CTreeCtrl& ctrl = GetTreeCtrl();
   ctrl.Expand(ctrl.GetRootItem(),TVE_EXPAND);
}

void CLibraryEditorView::OnDestroy()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   m_LibraryImages.DeleteImageList();
   m_StateImages.DeleteImageList();
   CTreeView::OnDestroy();
}