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
#include <PsgLib\PierPointData.h>

// PierPointGrid.h : header file


/////////////////////////////////////////////////////////////////////////////
// CPierPointGrid window

class CPierPointGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CPierPointGrid();
	virtual ~CPierPointGrid();

// Attributes
public:

// Operations
public:
   void AddPierPoint();
   void RemoveSelectedPierPoints();

   void GetPierPointData(CPierData2& pier);
   void SetPierPointData(const CPierData2& pier);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPierPointGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
   void CustomInit();

   static void DDX_PierPointGrid(CDataExchange* pDX, CPierPointGrid& grid, CPierData2* pPier);
   static void DDV_PierPointGrid(CDataExchange* pDX, CPierPointGrid& grid);

	// Generated message map functions
protected:
	//{{AFX_MSG(CPierPointGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   
   virtual void OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);

private:
   void AddPierPoint(const CPierPointData& pierPoint);
   void SetPierPointData(ROWCOL row,const CPierPointData& pierPoint);
   void GetPierPointData(ROWCOL row,CPierPointData* pierPoint);
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
