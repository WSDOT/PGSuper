///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#pragma once;

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MultiGirderSelectGrid.h : header file
//
#include <WBFLTypes.h>

// Container for spans and girders on/off settings
typedef std::vector<bool> GirderOnVector;
typedef std::vector<GirderOnVector> SpanGirderOnCollection; // spans containing girders
typedef SpanGirderOnCollection::iterator SpanGirderOnIterator;

/////////////////////////////////////////////////////////////////////////////
// CMultiGirderSelectGrid window

class CMultiGirderSelectGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CMultiGirderSelectGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiGirderSelectGrid)
	//}}AFX_VIRTUAL

	GRID_API virtual void OnDrawItem(CDC *pDC, ROWCOL nRow, ROWCOL nCol, const CRect& rectDraw, const CGXStyle& style);

// Implementation
public:
	virtual ~CMultiGirderSelectGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMultiGirderSelectGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   afx_msg UINT OnGetDlgCode();

   // virtual overrides for grid
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   BOOL ProcessKeys(CWnd* pSender, UINT nMessage, UINT nChar, UINT nRepCnt, UINT flags);
   void OnChangedSelection(const CGXRange* pRange, BOOL, BOOL);

public:
   // custom stuff for grid
   void CustomInit(const SpanGirderOnCollection& spanGirderOnCollection, std::_tstring(*pGetGirderLabel)(GirderIndexType));

   bool GetCellValue(ROWCOL nRow, ROWCOL nCol);
   void SetAllValues(bool val);

   // Vector of girders turned on (checked)
   std::vector<SpanGirderHashType> GetData();

private:
   CGXRangeList m_SelectedRange;
};

