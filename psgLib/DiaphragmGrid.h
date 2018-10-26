///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#if !defined(AFX_DIAPHRAMGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_DIAPHRAMGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DiaphramGrid.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmGrid window

class CDiaphragmGrid : public CGXGridWnd
{
// Construction
public:
	CDiaphragmGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiaphragmGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDiaphragmGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDiaphragmGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   BOOL OnLButtonDblClkRowCol(ROWCOL nRow,ROWCOL nCol,UINT nFlags,CPoint pt);

   GirderLibraryEntry::DiaphragmLayoutRules m_Rules;

   void SetRowData(ROWCOL nRow,GirderLibraryEntry::DiaphragmLayoutRule& rule);
   void AppendRow();

public:
   // custom stuff for grid
   void CustomInit();

   void RemoveRows();
   void AddRow();

   void SetRules(const GirderLibraryEntry::DiaphragmLayoutRules& rules);
   void GetRules(GirderLibraryEntry::DiaphragmLayoutRules& rules);
   GirderLibraryEntry::DiaphragmLayoutRule* GetRule(ROWCOL row);
   void SetRule(ROWCOL row,GirderLibraryEntry::DiaphragmLayoutRule& rule);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIAPHRAMGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
