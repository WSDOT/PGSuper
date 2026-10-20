///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include <PsgLib\PierData2.h>

// ColumnLayoutGrid.h : header file


/////////////////////////////////////////////////////////////////////////////
// CColumnLayoutGrid window

class CColumnLayoutGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CColumnLayoutGrid();
	virtual ~CColumnLayoutGrid();

// Attributes
public:

// Operations
public:
   void AddColumn();
   void RemoveSelectedColumns();

   virtual void GetColumnData(CPierData2& pier);
   void SetColumnData(const CPierData2& pier);

   void SetHeightMeasurementType(CColumnData::ColumnHeightMeasurementType measure);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColumnLayoutGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
   virtual void CustomInit();

   static void DDX_ColumnGrid(CDataExchange* pDX, CColumnLayoutGrid& grid, CPierData2* pPier);
   static void DDV_ColumnGrid(CDataExchange* pDX, CColumnLayoutGrid& grid);

	// Generated message map functions
protected:
	//{{AFX_MSG(CColumnLayoutGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   
   virtual void OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);

private:
   void AddColumn(const CColumnData& column,Float64 S);
   void SetColumnData(ROWCOL row,const CColumnData& column,Float64 S);
   virtual void GetColumnData(ROWCOL row,CColumnData* pColumn,Float64* pS);

protected:
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   virtual int GetGridColumnCount() const { return 6; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
