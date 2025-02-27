///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#if !defined(AFX_EDITLOADSVIEW_H__9FB98026_0BA0_4B0D_9317_0FF2C75EE2B6__INCLUDED_)
#define AFX_EDITLOADSVIEW_H__9FB98026_0BA0_4B0D_9317_0FF2C75EE2B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditLoadsView.h : header file
//
#include "PGSuperDoc.h"
#include <pgsExt\PointLoadData.h>
#include <pgsExt\DistributedLoadData.h>
#include <pgsExt\MomentLoadData.h>
#include <System\System.h>

/////////////////////////////////////////////////////////////////////////////
// CEditLoadsView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "resource.h"

class CEditLoadsView : public CFormView
{
public:
   enum Field { Type, Event, LoadCase, Location, Magnitude, Description };
   enum Direction {Ascending, Descending};

protected:
	CEditLoadsView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CEditLoadsView)

// Form Data
public:
	//{{AFX_DATA(CEditLoadsView)
	enum { IDD = IDD_EDIT_LOADS };
	CButton	m_StaticCtrl;
	CButton	m_HelpCtrl;
	CButton	m_AddPointCtrl;
	CButton	m_AddDistributedCtrl;
	CButton	m_AddMomentCtrl;
	CButton	m_DeleteCtrl;
	CButton	m_EditCtrl;
	CListCtrl	m_LoadsListCtrl;
	//}}AFX_DATA

// Attributes
public:
   void SortBy(Field field,Direction direction);
   IndexType GetLoadCount() const;
   std::_tstring GetFieldValue(IndexType idx, Field field) const;
   void DeleteLoad(IndexType idx);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditLoadsView)
public:
	virtual void OnInitialUpdate();
protected:
   virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
   virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = nullptr);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CEditLoadsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CEditLoadsView)
	afx_msg void OnAddPointload();
	afx_msg void OnAddMomentload();
	afx_msg void OnDeleteLoad();
	afx_msg void OnEditLoad();
	afx_msg void OnDblclkLoadsList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnClickLoadsList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnContextMenu(CWnd* pWnd,CPoint pos);
	afx_msg void OnHelp();
	//}}AFX_MSG
   afx_msg void OnHeaderClicked(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

   CComPtr<IBroker>     m_pBroker;

   bool                 m_IsInitialUpdate;
   bool                 m_FirstSizeEvent;

   void UpdatePointLoadItem(int icol, const CPointLoadData& pld);
   void UpdateDistributedLoadItem(int icol, const CDistributedLoadData& pld);
   void UpdateMomentLoadItem(int icol, const CMomentLoadData& pld);
   void EditLoad(POSITION pos);
   void InsertData();

   CSize  m_MinSize;
   CSize  m_But1Size;
   CSize  m_But2Size;
   int    m_HelpButWidth;

   int m_SortColIdx;
   bool m_bSortAscending;
   void Sort(int columnIdx,bool bReverse=true);

   int GetColumnIndex(Field field) const;

   CString GetEventName(EventIDType loadID);

public:
   afx_msg void OnDestroy();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITLOADSVIEW_H__9FB98026_0BA0_4B0D_9317_0FF2C75EE2B6__INCLUDED_)
