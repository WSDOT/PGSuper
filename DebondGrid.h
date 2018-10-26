///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// DebondGrid.h : header file
//

#include <PgsExt\DebondData.h>
#include <PgsExt\Keys.h>
#include <PgsExt\PrecastSegmentData.h>


interface IDebondGridParent
{
   virtual LPCTSTR GetGirderName() = 0;
   virtual void OnChange() = 0;
   virtual const CSegmentKey& GetSegmentKey() = 0;
   virtual ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondGrid window

class CGirderDescDebondGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CGirderDescDebondGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescDebondGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderDescDebondGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderDescDebondGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);

   void OnClickedButtonRowCol(ROWCOL nHitRow,ROWCOL nHitCol);

public:
   // custom stuff for grid
   void CustomInit(bool bSymmetricDebond);

   // insert a row above the currently selected cell or at the top if no selection
   void InsertRow();

   void FillGrid(const CPrecastSegmentData& segment);
   void GetData(CPrecastSegmentData& segment);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   StrandIndexType GetNumDebondedStrands();
   StrandIndexType GetNumExtendedStrands(pgsTypes::MemberEndType endType);

   void CanDebond(bool bCanDebond,bool bSymmetricDebond);

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   Float64 GetLeftDebondLength(ROWCOL row);
   Float64 GetRightDebondLength(ROWCOL row);
   Float64 GetDebondLength(ROWCOL row,ROWCOL col);

   ROWCOL GetRow(GridIndexType gridIdx);

   bool m_bSymmetricDebond;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
