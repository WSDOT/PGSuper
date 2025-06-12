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

#include <PsgLib\BridgeDescription2.h>


// HaunchBearingGrid.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CHaunchBearingGrid window

class CHaunchBearingGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CHaunchBearingGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHaunchBearingGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHaunchBearingGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHaunchBearingGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual void DoDataExchange(CDataExchange* pDX) override;

public:
   // custom init for grid
   void CustomInit();

private:
   void FillGrid();
   void GetGridData(CDataExchange* pDX);

   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   const WBFL::Units::LengthData* m_pUnit;
};
