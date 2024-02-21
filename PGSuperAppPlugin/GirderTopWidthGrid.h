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

class CGirderGroupData;
class CGirderTopWidthGrid;

void DDV_TopWidthGrid(CDataExchange* pDX,int nIDC, CGirderTopWidthGrid* pGrid);

/////////////////////////////////////////////////////////////////////////////
// CGirderTopWidthGrid window

class CGirderTopWidthGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
   CGirderTopWidthGrid();

// Attributes
public:

// Operations
public:
   void Enable(BOOL bEnable);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderTopWidthGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderTopWidthGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderTopWidthGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg void OnExpand();
   afx_msg void OnJoin();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   // custom stuff for grid
   void CustomInit(CGirderGroupData* pGirderGroup);
   void UpdateGrid();

   BOOL ValidateGirderTopWidth();

private:
   BOOL m_bEnabled;

   CGirderGroupData* m_pGirderGroup;

   std::vector<Float64> m_MinGirderTopWidth[2];
   std::vector<Float64> m_MaxGirderTopWidth[2];

   pgsTypes::TopWidthType GetTopWidthTypeFromCell(ROWCOL col);

   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   virtual BOOL OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState);
   virtual void OnModifyCell(ROWCOL nRow, ROWCOL nCol) override;
   BOOL OnEndEditing(ROWCOL nRow, ROWCOL nCol);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
