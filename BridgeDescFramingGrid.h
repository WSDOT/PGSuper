///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEDESCFRAMINGGRID_H_INCLUDED_)
#define AFX_BRIDGEDESCFRAMINGGRID_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeDescFramingGrid.h : header file
//

#include <PgsExt\BridgeDescription.h>
#include <WBFLCogo.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid window

class CBridgeDescFramingGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CBridgeDescFramingGrid();
	virtual ~CBridgeDescFramingGrid();

// Attributes
public:

// Operations
public:
   std::vector<double> GetSpanLengths();
   void SetSpanLengths(const std::vector<double>& spanLengths,PierIndexType fixedPierIdx);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescFramingGrid)
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
public:
	//{{AFX_MSG(CBridgeDescFramingGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);
   BOOL OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey);
   void OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol);
   
   BOOL CanActivateGrid(BOOL bActivate); // called by DDV_GXGridWnd
   BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit();

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // fill grid with data
   void FillGrid(const CBridgeDescription& bridgeDesc);
   void GetGridData();
   CPierData* GetPierRowData(ROWCOL nRow);

	void OnAddSpan();
	void OnRemoveSpan();
   bool EnableItemDelete();

private:
   void InsertRow();

   void FillPierRow(ROWCOL row,const CPierData& pierData);
   void FillSpanRow(ROWCOL row,const CSpanData& spanData);

   // set up styles for interior rows
   void SetPierRowStyle(ROWCOL nRow,const CPierData& pierData);
   void SetSpanRowStyle(ROWCOL nRow,const CSpanData& spanData);

   PierIndexType GetPierCount();

   void EditSpan(SpanIndexType spanIdx);
   void EditPier(PierIndexType pierIdx);
   ROWCOL GetSpanRow(SpanIndexType spanIdx);
   ROWCOL GetPierRow(PierIndexType pierIdx);
   SpanIndexType GetSpanIndex(ROWCOL nRow);
   PierIndexType GetPierIndex(ROWCOL nRow);

   CComPtr<IStation> m_objStation;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCFRAMINGGRID_H_INCLUDED_)
