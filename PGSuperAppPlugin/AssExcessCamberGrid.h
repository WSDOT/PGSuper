///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

class CSplicedGirderData;
class CGirderGroupData;

// AssExcessCamberGrid.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAssExcessCamberGrid window

class CAssExcessCamberGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CAssExcessCamberGrid();

public:
   // custom stuff for grid
   void CustomInit(CSplicedGirderData* pGirder);

   void FillGrid();
   void UpdateAssExcessCamberData();

   // pop dialog and select a single slab offset value
   bool SelectSingleValue(Float64* pValue);

   void EnableWindow(BOOL bEnable);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssExcessCamberGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAssExcessCamberGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAssExcessCamberGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   CSplicedGirderData* m_pGirder;
   CGirderGroupData* m_pGroup;
};
