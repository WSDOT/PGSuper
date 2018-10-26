///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#if !defined(AFX_LiveLoadAxleGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_LiveLoadAxleGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LiveLoadAxleGrid.h : header file
//
#include <PsgLib\LiveLoadLibraryEntry.h>

class CLiveLoadDlg;
/////////////////////////////////////////////////////////////////////////////
// CLiveLoadAxleGrid window

class CLiveLoadAxleGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CLiveLoadAxleGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLiveLoadAxleGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLiveLoadAxleGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLiveLoadAxleGrid)
	afx_msg void OnUpdateEditRemoverows(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
public:
   // custom stuff for grid
   void CustomInit();
   void ResetGrid();

   void UploadData(CDataExchange* pDX, CLiveLoadDlg* dlg);
   void DownloadData(CDataExchange* pDX, CLiveLoadDlg* dlg);

   // add and remove rows
   void Removerows();
   void Appendrow();
   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   void Enable(BOOL bEnable);

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   enum SpacingType {stNone, stFixed, stVariable} ;
   SpacingType ParseAxleRow(ROWCOL nRow, CDataExchange* pDX, Float64* pWeight, 
                            Float64* pSpacingMin, Float64* pSpacingMax);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LiveLoadAxleGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
