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

#include <PsgLib\PTData.h>

// OffsetDuctGrid.h : header file
//

class COffsetDuctGridCallback
{
public:
   virtual void OnDuctChanged() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// COffsetDuctGrid window

class COffsetDuctGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	COffsetDuctGrid();

public:
   // custom stuff for grid
   void CustomInit(COffsetDuctGridCallback* pCallback);

   void AddPoint();
   void DeletePoint();
   
   COffsetDuctGeometry GetData();
   void SetData(const COffsetDuctGeometry& ductGeometry);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COffsetDuctGrid)
	//}}AFX_VIRTUAL
   
   BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);

// Implementation
public:
	virtual ~COffsetDuctGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(COffsetDuctGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   COffsetDuctGridCallback* m_pCallback;

   void FillRow(ROWCOL row,Float64 distance,Float64 offset);
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
};
