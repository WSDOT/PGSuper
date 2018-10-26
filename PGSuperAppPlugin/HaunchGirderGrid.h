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


#pragma once
// HaunchGirderGrid.h : header file
//
#include "HaunchSpanGrid.h"


/////////////////////////////////////////////////////////////////////////////
// CHaunchGirderGrid window

class CHaunchGirderGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CHaunchGirderGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHaunchGirderGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHaunchGirderGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHaunchGirderGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);

public:
   // custom init for grid
   void CustomInit(SpanIndexType numSpans, GirderIndexType maxGirdersPerSpan);

   void FillGrid(const HaunchInputData& haunchData);
   HaunchInputData GetData(Float64 minA, CString& minValError, const HaunchInputData& origData, CDataExchange* pDX);


private:


   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   HaunchPair GetHpAtCells(SpanIndexType span, GirderIndexType gdr, CDataExchange* pDX);

   const unitmgtLengthData* m_pCompUnit;
};
