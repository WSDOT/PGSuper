///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// ParabolicDuctGrid.h : header file
//

class CParabolicDuctGridCallback
{
public:
   virtual void OnDuctChanged() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CParabolicDuctGrid window

class CParabolicDuctGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CParabolicDuctGrid();

public:
   // custom stuff for grid
   void CustomInit(CParabolicDuctGridCallback* pCallback);

   void AddPoint();
   void DeletePoint();
   
   CParabolicDuctGeometry GetData();
   void SetData(const CParabolicDuctGeometry& ductGeometry);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLinearDuctGrid)
	//}}AFX_VIRTUAL
   
   virtual BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual int GetColWidth(ROWCOL nCol);

// Implementation
public:
	virtual ~CParabolicDuctGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLinearDuctGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CParabolicDuctGridCallback* m_pCallback;

   const CSplicedGirderData* m_pGirder;

   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   void InsertFirstPoint(SpanIndexType spanIdx,Float64 distance,Float64 offset,CDuctGeometry::OffsetType offsetType);
   void InsertLowPoint(SpanIndexType spanIdx,Float64 distance,Float64 offset,CDuctGeometry::OffsetType offsetType);
   void InsertInflectionPoint(SpanIndexType spanIdx,Float64 distance);
   void InsertHighPoint(PierIndexType pierIdx,Float64 offset,CDuctGeometry::OffsetType offsetType);
   void InsertLastPoint(SpanIndexType spanIdx,Float64 distance,Float64 offset,CDuctGeometry::OffsetType offsetType);

   void GetRowValues(ROWCOL row,Float64* pDistance,Float64* pOffset,CDuctGeometry::OffsetType* pOffsetType);
};
