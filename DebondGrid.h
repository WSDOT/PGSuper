///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#if !defined(AFX_BRIDGEDEBONDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEDEBONDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DebondGrid.h : header file
//

#include <PgsExt\GirderData.h>


/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondGrid window

class CGirderDescDebondGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CGirderDescDebondGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescDebondGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderDescDebondGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderDescDebondGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);

   void OnModifyCell(ROWCOL nRow,ROWCOL nCol);
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);

public:
   // custom stuff for grid
   void CustomInit(bool bSymmetricDebond);

   // insert a row above the currently selected cell or at the top if no selection
   void InsertRow();
   void DoRemoveRows();

   void FillGrid(const std::vector<CDebondInfo>& debondInfo);
   void GetData(std::vector<CDebondInfo>& debondInfo);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   StrandIndexType GetNumDebondedStrands();

   void SymmetricDebond(bool bSymmetricDebond);

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   void UpdateStrandLists();
   CString GetStrandList(ROWCOL nRow);

   Float64 GetLeftDebondLength(ROWCOL row);
   Float64 GetRightDebondLength(ROWCOL row);
   Float64 GetDebondLength(ROWCOL row,ROWCOL col);

   bool m_bSymmetricDebond;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDEBONDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
