///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DebondGrid.h : header file
//

#include <PgsExt\GirderData.h>
#include <PsgLib\GirderLibraryEntry.h>

class CGirderSelectStrandsPage;
class CUserData;

/////////////////////////////////////////////////////////////////////////////
// CStrandFillGrid window

class CStrandFillGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CStrandFillGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStrandFillGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStrandFillGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStrandFillGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
//   virtual BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual void OnClickedButtonRowCol(ROWCOL nHitRow, ROWCOL nHitCol);

public:
   // custom stuff for grid
   void CustomInit(CGirderSelectStrandsPage* pParent, const GirderLibraryEntry* pGdrEntry);

   // call this before getting data used by the grid
   bool UpdateData(bool doCheckData);

   // strand stuff for client
   void SymmetricDebond(BOOL bSymmetricDebond);

   // toogle fill
   void ToggleFill(ROWCOL rowNo);

   bool GetDebondInfo(StrandIndexType straightStrandGridIdx, Float64* pleftDebond, Float64* prightDebond);

   bool IsStrandExtended(StrandIndexType strandIdx,pgsTypes::MemberEndType endType);

private:
   void FillGrid();

   bool IsPermStrandFilled(GirderLibraryEntry::psStrandType strandType, StrandIndexType localIdx);
   bool IsTempStrandFilled(StrandIndexType localIdx);

   // insert a row above the currently selected cell or at the top if no selection
   void InsertRow();
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   void RemoveStrandFill(const CUserData* pUserData);
   void AddStrandFill(const CUserData* pUserData);

   void UpdateParent();

   CGirderSelectStrandsPage* m_pParent;
   const GirderLibraryEntry* m_pGdrEntry;
   CString m_strMaxDebondLength;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

