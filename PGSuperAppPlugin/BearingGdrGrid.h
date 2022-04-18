///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "BearingPierGrid.h"

/////////////////////////////////////////////////////////////////////////////
// CBearingGdrGrid window

class CBearingGdrGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CBearingGdrGrid();
	CBearingGdrGrid(BearingInputData* pData);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBearingGdrGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBearingGdrGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBearingGdrGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg LRESULT ChangeTabName( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);

public:
   // custom init for grid
   void CustomInit(SpanIndexType ispan);

   void FillGrid();
   void WriteBearingRow(ROWCOL row, const CBearingData2& bearingData);
   void GetData(CDataExchange* pDX);
   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

private:

   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   PierIndexType GetBackBearingIdx();

   const unitmgtLengthData* m_pCompUnit;
   BearingGridDataGetter<CBearingGdrGrid> m_DGetter;

   BearingInputData* m_pBearingInputData;

   SpanIndexType m_SpanIdx;
};

