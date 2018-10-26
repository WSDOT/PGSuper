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

#if !defined(AFX_BRIDGEDESCDECKPOINTGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEDESCDECKPOINTGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeDescDeckPointGrid.h : header file
//

#include <PgsExt\DeckDescription.h>
#include <PgsExt\DeckPoint.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckPointGrid window

class CBridgeDescDeckPointGrid : public CGXGridWnd
{
// Construction
public:
	CBridgeDescDeckPointGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDeckPointGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBridgeDescDeckPointGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBridgeDescDeckPointGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);
   void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey);
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   void OnModifyCell(ROWCOL nRow,ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit();

   void FillGrid(const CDeckDescription* pDeck);
   std::vector<CDeckPoint> GetEdgePoints();

   void RemoveSelectedRows();
   void Enable(BOOL bEnable);

private:
   BOOL m_bEnabled;

   ROWCOL AddRow();
   void SetPointRowData(ROWCOL row,const CDeckPoint& point);
   void GetPointRowData(ROWCOL row,CDeckPoint* pPoint);

   void SetTransitionRowData(ROWCOL row,const CDeckPoint& point);
   void GetTransitionRowData(ROWCOL row,CDeckPoint *pPoint);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   void SelectRow(ROWCOL nRow);

   // need a station object to convert stations between text and values
   // create this object once and use it over and over
   CComPtr<IStation> m_objStation;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCDECKPOINTGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
