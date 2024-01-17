///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <PgsExt\SegmentPTData.h>

/////////////////////////////////////////////////////////////////////////////
// CSegmentTendonGrid window

class CSegmentTendonGrid : public CGXGridWnd
{
// Construction
public:
	CSegmentTendonGrid();

// Attributes
public:

// Operations
public:
   void OnStrandChanged();
   void OnInstallationTypeChanged();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSegmentTendonGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSegmentTendonGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSegmentTendonGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol) override;
   virtual void OnClickedButtonRowCol(ROWCOL nHitRow, ROWCOL nHitCol) override;
   virtual void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey) override;
   virtual void OnModifyCell(ROWCOL nRow, ROWCOL nCol) override;
   virtual BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol) override;

   void UpdateNumStrandsList(ROWCOL nRow);

public:
   // custom stuff for grid
   void CustomInit(const CPrecastSegmentData* pSegment);

   void OnAddRow();
   void OnRemoveSelectedRows();

   // Updates the grid data. If pDX is nullptr, it gets the
   // row data from the grid, however the data is not validated.
   void UpdateData(CDataExchange* pDX,CSegmentPTData* pTendons);

protected:
   Float64 m_SegmentLength;

   ROWCOL nDuctTypeCol;
   ROWCOL nDuctShapeCol;
   ROWCOL nStrandsCol;
   ROWCOL nPjackCheckCol;
   ROWCOL nPjackCol;
   ROWCOL nPjackUserCol;
   ROWCOL nJackEndCol;
   ROWCOL nLeftEndYCol;
   ROWCOL nLeftEndDatumCol;
   ROWCOL nMiddleYCol;
   ROWCOL nMiddleDatumCol;
   ROWCOL nRightEndYCol;
   ROWCOL nRightEndDatumCol;

   void FillGrid(const CSegmentPTData* pTendons);

   // appends a new row and initializes it with duct
   void AppendRow(const CSegmentDuctData& duct);

   // Appends a new row to the grid. Returns the index of the new row
   ROWCOL AppendRow(); 

   // Sets the style attributes of a row
   void SetRowStyle(ROWCOL nRow);

   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   CSegmentDuctData GetDuctRow(ROWCOL nRow);

   void UpdateDuctPoints(ROWCOL nRow);

   CString GetDuctName(ROWCOL nRow);
   StrandIndexType GetStrandCount(ROWCOL nRow);
   void GetPjack(ROWCOL nRow, CSegmentDuctData* pDuct);
   pgsTypes::JackingEndType GetJackingEnd(ROWCOL nRow);
   void GetDuctPoints(ROWCOL nRow, CSegmentDuctData* pDuct);
   CSegmentDuctData::GeometryType GetTendonShape(ROWCOL nRow);

   virtual UINT Validate(ROWCOL nRow, CSegmentDuctData& duct);
   virtual void ShowValidationError(ROWCOL nRow,UINT iError);

   void OnCalcPjack(ROWCOL nRow);
   void UpdateMaxPjack(ROWCOL nRow);
   bool ComputePjackMax(ROWCOL row);

   void RefreshRowHeading(ROWCOL rFrom, ROWCOL rTo);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

