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
#pragma once


// MultiSelectGrid.h : header file
//
#include <PGSuperAll.h>
#include <PsgLib\Keys.h>

#if defined _NOGRID
#include <NoGrid.h>
#else
#include <grid\gxall.h>
#endif

// Container for spans and girders on/off settings
typedef std::vector<bool> ItemOnVector;
typedef std::vector<ItemOnVector> GroupItemOnCollection; // groups containing items
typedef GroupItemOnCollection::iterator GroupItemOnIterator;

/////////////////////////////////////////////////////////////////////////////
// CMultiGirderSelectGrid window



class CMultiSelectGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()

// Construction
public:
	CMultiSelectGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiSelectGrid)
	//}}AFX_VIRTUAL

	virtual void OnDrawItem(CDC *pDC, ROWCOL nRow, ROWCOL nCol, const CRect& rectDraw, const CGXStyle& style) override;

// Implementation
public:
	virtual ~CMultiSelectGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMultiSelectGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   afx_msg UINT OnGetDlgCode();

   // virtual overrides for grid
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt) override;
   BOOL ProcessKeys(CWnd* pSender, UINT nMessage, UINT nChar, UINT nRepCnt, UINT flags) override;
   void OnChangedSelection(const CGXRange* pRange, BOOL, BOOL) override;

public:
   void Enable(bool bEnable);

   bool GetCellValue(ROWCOL nRow, ROWCOL nCol);
   void SetAllValues(bool val);

private:
   CGXRangeList m_SelectedRange;
};

