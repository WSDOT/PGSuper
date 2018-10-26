///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#if !defined(AFX_GIRDERSTRANDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_GIRDERSTRANDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderStrandGrid.h : header file
//
#include <GeometricPrimitives\GeometricPrimitives.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <Units\Measure.h>

class CGirderStrandGrid;

// define a pure virtual class that clients of this grid can inherit from
class CGirderStrandGridClient
{
public:
   // capture event fired from grid that allows deletion of rows
   virtual void OnEnableDelete(const CGirderStrandGrid* pgrid, bool canDelete)=0;
};

/////////////////////////////////////////////////////////////////////////////
// CGirderStrandGrid window

class CGirderStrandGrid : public CGXGridWnd
{
// Construction
public:
	CGirderStrandGrid(CGirderStrandGridClient* pClient);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderStrandGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderStrandGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderStrandGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnEditInsertrow();
	afx_msg void OnEditAppendrow();
	afx_msg void OnEditRemoverows();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual BOOL OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit();
   // insert a row above the currently selected cell or at the top if no selection
   void Insertrow();
   // append row to end of grid
   void Appendrow();
   void Removerows();
   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   // get data for a row
   bool GetRowData(ROWCOL nRow, Float64* pX,Float64* pY);
   // fill grid with data
   void FillGrid(IPoint2dCollection* points);

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   CGirderStrandGridClient* m_pClient;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERSTRANDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
