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

#if !defined(AFX_GIRDERNAMEGRID_H_INCLUDED_)
#define AFX_GIRDERNAMEGRID_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderNameGrid.h : header file
//

#include <PgsExt\GirderTypes.h>
#include <PgsExt\SpanData.h>

/////////////////////////////////////////////////////////////////////////////
// CGirderNameGrid window

class CGirderNameGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CGirderNameGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderNameGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderNameGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderNameGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg void OnExpand();
   afx_msg void OnJoin();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit(const CSpanData* pSpanData);
   void FillGrid();
   void OnGirderFamilyChanged(LPCTSTR strGirderFamily);

   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

   void Enable(BOOL bEnable);

   void UseSameGirderName(bool bSame);
   void UseSameNumGirders(bool bSame);

   CGirderTypes m_GirderTypes;

private:
   BOOL m_bEnabled;
   bool m_bSameGirderName;
   bool m_bSameNumGirders;

   CString m_GirderList;
   CString m_strGirderFamilyName;
   typedef std::pair<GirderIndexType,GirderIndexType> UserData;

   virtual BOOL OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState);

   CString m_strTempGirderName;
   virtual BOOL OnStartEditing(ROWCOL nRow,ROWCOL nCol);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);

   void UpdateGirderFamilyList(LPCTSTR strGirderFamily);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERNAMEGRID_H_INCLUDED_)
