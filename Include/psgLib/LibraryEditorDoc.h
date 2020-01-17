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

// LibraryEditorDoc.h : interface of the CLibraryEditorDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIBRARYEDITORDOC_H__340EC2FE_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
#define AFX_LIBRARYEDITORDOC_H__340EC2FE_20E1_11D2_9D35_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "psgLibLib.h"

#include <psglib\ISupportLibraryManager.h>
#include <psgLib\LibraryManager.h>

#include <EAF\EAFDocument.h>

class PSGLIBCLASS CLibraryEditorDoc  : public CEAFDocument , public libISupportLibraryManager
{
protected: // create from serialization only
	CLibraryEditorDoc();
	DECLARE_DYNCREATE(CLibraryEditorDoc)

// Attributes
public:

// Operations
public:
   virtual BOOL Init(); 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibraryEditorDoc)
	public:
	//}}AFX_VIRTUAL

   // CEAFDocument pure-virtual functions
   virtual HRESULT WriteTheDocument(IStructuredSave* pStrSave);
   virtual HRESULT LoadTheDocument(IStructuredLoad* pStrLoad);
   virtual HRESULT OpenDocumentRootNode(IStructuredLoad* pStrLoad);
   virtual HRESULT CloseDocumentRootNode(IStructuredLoad* pStrLoad);
   virtual HRESULT OpenDocumentRootNode(IStructuredSave* pStrSave);

   virtual CString GetRootNodeName();
   virtual BOOL GetStatusBarMessageString(UINT nID,CString& rMessage) const;
   virtual BOOL GetToolTipMessageString(UINT nID, CString& rMessage) const;

   virtual void LoadDocumentSettings();
   virtual void SaveDocumentSettings();

   virtual CString GetDocumentationRootLocation();

   void OnImport(); // import library entries (command handler)

// Implementation
public:
	virtual ~CLibraryEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   CollectionIndexType GetNumberOfLibraryManagers() const {return 1;}
   libLibraryManager* GetLibraryManager(CollectionIndexType n){ASSERT(n!=1); return &m_LibraryManager;}
   libLibraryManager* GetTargetLibraryManager(){return &m_LibraryManager;}
   psgLibraryManager* GetLibraryManager(){return &m_LibraryManager;}

public:
   void HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName );
   void HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName );

private:
   CEAFToolBar* m_pMyToolBar;
   HRESULT m_hrOpenRootNode; // keeps track of the state of BeginUnit for the root node

protected:
   psgLibraryManager m_LibraryManager;

   virtual CString GetToolbarSectionName();
   virtual void DoIntegrateWithUI(BOOL bIntegrate);

   virtual CATID GetDocumentPluginCATID();
   virtual HINSTANCE GetResourceInstance();

// Generated message map functions
protected:
	//{{AFX_MSG(CLibraryEditorDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIBRARYEDITORDOC_H__340EC2FE_20E1_11D2_9D35_00609710E6CE__INCLUDED_)
