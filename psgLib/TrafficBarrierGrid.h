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

#if !defined(AFX_TrafficBarrierGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_TrafficBarrierGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TrafficBarrierGrid.h : header file
//

class CTrafficBarrierDlg;

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierGrid window

class CTrafficBarrierGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CTrafficBarrierGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrafficBarrierGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTrafficBarrierGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTrafficBarrierGrid)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   virtual void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey);

public:
   // custom stuff for grid
   void CustomInit();
   void ResetGrid();

   void UploadData(CDataExchange* pDX, IPoint2dCollection* points);
   void DownloadData(CDataExchange* pDX, IPoint2dCollection* points);

   // add and remove rows
   void RemoveRows();
   void AppendRow();

   // move the selected row(s) up/down
   void MoveUp();
   void MoveDown();
   BOOL CanEnableMoveUp();
   BOOL CanEnableMoveDown();

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   void SelectRow(ROWCOL nRow);

   void ParseRow(ROWCOL nRow, CDataExchange* pDX, Float64* pX,Float64* pY);

   void SwapRows(ROWCOL row1,ROWCOL row2);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TrafficBarrierGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
