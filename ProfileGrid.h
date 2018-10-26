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

#if !defined(AFX_PROFILEGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_PROFILEGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProfileGrid.h : header file
//

#include <IFace\Project.h>

/////////////////////////////////////////////////////////////////////////////
// CProfileGrid window

class CProfileGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CProfileGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProfileGrid)
	//}}AFX_VIRTUAL

   void SortCurves();
   void SetCurveData(std::vector<VertCurveData>& curves);
   bool GetCurveData(std::vector<VertCurveData>& curves);

// Implementation
public:
	virtual ~CProfileGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CProfileGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   // custom stuff for grid
   void CustomInit();
   void AppendRow();
   void RemoveRows();

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // get data for a row
   bool GetRowData(ROWCOL nRow,Float64* pStation,Float64* pGrade,Float64* pL1,Float64* pL2);
   void SetRowData(ROWCOL nRow,VertCurveData& data);

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   void InitRowData(ROWCOL row);
//   bool EnableItemDelete();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROFILEGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
