///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <PgsExt\PTData.h>

// LinearDuctGrid.h : header file
//

class CLinearDuctGridCallback
{
public:
   virtual void OnDuctChanged() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CLinearDuctGrid window

class CLinearDuctGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CLinearDuctGrid();

public:
   // custom stuff for grid
   void CustomInit(CLinearDuctGridCallback* pCallback);

   void AddPoint();
   void DeletePoint();
   void SetMeasurementType(CLinearDuctGeometry::MeasurementType mt);
   
   void GetData(CLinearDuctGeometry& ductGeometry);
   void SetData(const CLinearDuctGeometry& ductGeometry);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLinearDuctGrid)
	//}}AFX_VIRTUAL
   
   virtual int GetColWidth(ROWCOL nCol);
   virtual BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);
   virtual void OnChangedSelection(const CGXRange* pRange,BOOL bIsDragging,BOOL bKey);


// Implementation
public:
	virtual ~CLinearDuctGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLinearDuctGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CLinearDuctGridCallback* m_pCallback;

   void FillRow(ROWCOL row,Float64 location,Float64 offset,CLinearDuctGeometry::OffsetType offsetType);
   void GetPoint(ROWCOL row,Float64* pLocation,Float64* pOffset,CLinearDuctGeometry::OffsetType* pOffsetType);

   void SetRowStyle(ROWCOL nRow);
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   void SetDeleteButtonState();
   BOOL UpdateLastRow();
};
