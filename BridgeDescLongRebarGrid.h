///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#if !defined(AFX_LongSteelGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_LongSteelGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LongSteelGrid.h : header file
//
#include <PgsExt\LongitudinalRebarData.h>

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLongRebarGrid window

class CGirderDescLongRebarGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CGirderDescLongRebarGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescLongRebarGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderDescLongRebarGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderDescLongRebarGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnEditInsertrow();
	afx_msg void OnEditRemoverows();
	afx_msg void OnUpdateEditRemoverows(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);
   BOOL OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   void OnModifyCell(ROWCOL nRow,ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit();
   // insert a row above the currently selected cell or at the top if no selection
   void Insertrow();
   void Removerows();
   void Appendrow();

   // fill grid with data
   void FillGrid(const CLongitudinalRebarData& rebarData);
   bool GetRebarData(CLongitudinalRebarData* pRebarData);

private:
   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   // get data for a row
   bool GetRowData(ROWCOL nRow, CLongitudinalRebarData::RebarRow* plsi);

   // changes formating of cell when measurement type changes
   pgsTypes::RebarLayoutType GetLayout(ROWCOL nRow);
   void OnLayoutTypeChanged(ROWCOL nRow);

   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   matRebar::Size GetBarSize(ROWCOL row);
   void EnableCell(ROWCOL nRow, ROWCOL nCol, BOOL bEnable);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LongSteelGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
