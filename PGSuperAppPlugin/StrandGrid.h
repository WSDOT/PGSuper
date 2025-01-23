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

#include <PgsExt\GirderData.h>
#include <PsgLib\GirderLibraryEntry.h>

class CUserData;
interface IEAFDisplayUnits;

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
   virtual int GetColWidth(ROWCOL nCol) override;
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol) override;
   virtual void OnClickedButtonRowCol(ROWCOL nHitRow, ROWCOL nHitCol) override;
   virtual void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey) override;
   virtual BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol) override;

   // returns the number of columns used to specialize grid
   virtual CString GetRowLabelHeading() const = 0;
   virtual ROWCOL GetSpecializedColCount() const = 0;
   virtual ROWCOL InitSpecializedColumns(ROWCOL col, IEAFDisplayUnits* pDisplayUnits) = 0;
   virtual ROWCOL SetSpecializedColumnStyles(ROWCOL nRow, ROWCOL col) = 0;
   virtual ROWCOL GetSpecializedColumnValues(ROWCOL nRow, ROWCOL col, CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits) = 0;
   virtual ROWCOL AppendSpecializedColumnValues(ROWCOL nRow, ROWCOL col, const CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits) = 0;
   virtual pgsTypes::StrandDefinitionType GetStrandDefinitionType() = 0;

public:
   // custom stuff for grid
   void CustomInit(const CPrecastSegmentData* pSegment);

   void OnAddRow();
   void OnRemoveSelectedRows();

   // Updates the strand grid data. If pDX is nullptr, it gets the
   // strand row data from the grid, however the data is not validated.
   void UpdateStrandData(CDataExchange* pDX,CStrandData* pStrands);

protected:
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

   virtual UINT Validate(ROWCOL nRow,CStrandRow& strandRow);
   virtual void ShowValidationError(ROWCOL nRow,UINT iError);

   ROWCOL GetStrandTypeCol();
   ROWCOL GetHarpStrandStartCol();
   ROWCOL GetHarpStrandEndCol();
   ROWCOL GetLeftExtendCheckCol();
   ROWCOL GetRightExtendCheckCol();
   ROWCOL GetLeftDebondCheckCol();
   ROWCOL GetLeftDebondLengthCol();
   ROWCOL GetRightDebondCheckCol();
   ROWCOL GetRightDebondLengthCol();
};

/////////////////////////////////////////////////////////////////////////////

class CRowStrandGrid : public CStrandGrid
{
public:

protected:
   virtual CString GetRowLabelHeading() const override { return _T("Row"); }
   virtual ROWCOL GetSpecializedColCount() const override { return 3; } // S1, S2, and # strands
   virtual ROWCOL InitSpecializedColumns(ROWCOL col, IEAFDisplayUnits* pDisplayUnits) override;
   virtual ROWCOL SetSpecializedColumnStyles(ROWCOL nRow, ROWCOL col) override;
   virtual ROWCOL GetSpecializedColumnValues(ROWCOL nRow, ROWCOL col, CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits) override;
   virtual ROWCOL AppendSpecializedColumnValues(ROWCOL nRow, ROWCOL col, const CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits) override;
   virtual pgsTypes::StrandDefinitionType GetStrandDefinitionType() override { return pgsTypes::sdtDirectRowInput;  }

   virtual UINT Validate(ROWCOL nRow, CStrandRow& strandRow) override;
   virtual void ShowValidationError(ROWCOL nRow, UINT iError) override;
};

/////////////////////////////////////////////////////////////////////////////

class CPointStrandGrid : public CStrandGrid
{
public:

protected:
   virtual CString GetRowLabelHeading() const override { return _T("Strand"); }
   virtual ROWCOL GetSpecializedColCount() const override { return 1; } // Z
   virtual ROWCOL InitSpecializedColumns(ROWCOL col, IEAFDisplayUnits* pDisplayUnits) override;
   virtual ROWCOL SetSpecializedColumnStyles(ROWCOL nRow, ROWCOL col) override;
   virtual ROWCOL GetSpecializedColumnValues(ROWCOL nRow, ROWCOL col, CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits) override;
   virtual ROWCOL AppendSpecializedColumnValues(ROWCOL nRow, ROWCOL col, const CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits) override;
   virtual pgsTypes::StrandDefinitionType GetStrandDefinitionType() override { return pgsTypes::sdtDirectStrandInput; }

   virtual UINT Validate(ROWCOL nRow, CStrandRow& strandRow) override;
   virtual void ShowValidationError(ROWCOL nRow, UINT iError) override;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

