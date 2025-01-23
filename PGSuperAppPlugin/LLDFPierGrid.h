///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_LLDFPierGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_LLDFPierGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LLDFPierGrid.h : header file
//

#include <PgsExt\BridgeDescription2.h>
#include "LLDFGrid.h"

// local data structures for lldfs
// TRICKY: Order must be same as grid columns
struct PierLLDF
{
   Float64 pgNMService;
   Float64 pgNMFatigue;

   PierLLDF():
      pgNMService(0), pgNMFatigue(0)
   {;}
};

/////////////////////////////////////////////////////////////////////////////
// CLLDFPierGrid window

class CLLDFPierGrid : public CEnableGrid
{
// Construction
public:
	CLLDFPierGrid();

// Attributes
public:

// Operations
public:
   void Enable(BOOL bEnable);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLLDFPierGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLLDFPierGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLLDFPierGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg LRESULT ChangeTabName( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL m_bEnabled;
   PierIndexType m_PierIdx;

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual BOOL OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   virtual BOOL OnEndEditing(ROWCOL nRow, ROWCOL nCol);
   virtual BOOL PasteTextRowCol(ROWCOL nRow, ROWCOL nCol, const CString& str, UINT nFlags, const CGXStyle* pOldStyle);
   virtual BOOL ProcessKeys(CWnd* pSender, UINT nMessage, UINT nChar, UINT nRepCnt, UINT flags);

private:
   void AddGirderRow(GirderIndexType gdr, const CPierData2* pPier);
   void GetGirderRow(GirderIndexType gdr, CPierData2* pPier);

public:
   // custom stuff for grid
   void CustomInit(PierIndexType pierIdx);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // Fill lldfs for a girder with a constant
   void SetGirderLLDF(GirderIndexType gdr, Float64 value );

   // Fill lldfs for a girder with computed values
   void SetGirderLLDF(GirderIndexType gdr, const PierLLDF& rlldf );

   // fill grid with data
   void FillGrid(const CBridgeDescription2* pBridgeDesc);
   void GetData(CBridgeDescription2* pBridgeDesc);
   afx_msg void OnEditCopy();
   afx_msg void OnEditPaste();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LLDFPierGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
