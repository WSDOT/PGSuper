///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <PgsExt\GirderData.h>
#include <PsgLib\GirderLibraryEntry.h>

class CUserData;

/////////////////////////////////////////////////////////////////////////////
// CStrandGrid window

class CStrandGrid : public CGXGridWnd
{
// Construction
public:
	CStrandGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStrandGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStrandGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStrandGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);
   virtual void OnClickedButtonRowCol(ROWCOL nHitRow, ROWCOL nHitCol);
   virtual void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey);
   virtual BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit(const CPrecastSegmentData* pSegment);

   void OnAddRow();
   void OnRemoveSelectedRows();

   // Updates the strand grid data. If pDX is NULL, it gets the
   // strand row data from the grid, however the data is not validated.
   void UpdateStrandData(CDataExchange* pDX,CStrandData* pStrands);

private:
   Float64 m_SegmentLength;

   void FillGrid(CStrandData* pStrands);

   // appends a new row and initializes it with strandRow
   void AppendRow(const CStrandRow& strandRow);

   // Appends a new row to the grid. Returns the index of the new row
   ROWCOL AppendRow(); 

   // Sets the style attributes of a row
   void SetRowStyle(ROWCOL nRow);

   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
   CStrandRow GetStrandRow(ROWCOL nRow);

   void UpdateExtendedStrandProperties(ROWCOL nRow);

   bool Validate(ROWCOL nRow,CStrandRow& strandRow);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

