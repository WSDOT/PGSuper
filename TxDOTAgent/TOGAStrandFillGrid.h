///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

class CTOGAGirderSelectStrandsDlg;
class CUserData;

/////////////////////////////////////////////////////////////////////////////
// CTOGAStrandFillGrid window

class CTOGAStrandFillGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CTOGAStrandFillGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTOGAStrandFillGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTOGAStrandFillGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTOGAStrandFillGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol) override;
//   virtual BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt) override;
   virtual void OnClickedButtonRowCol(ROWCOL nHitRow, ROWCOL nHitCol) override;

public:
   // custom stuff for grid
   void CustomInit(CTOGAGirderSelectStrandsDlg* pParent, const GirderLibraryEntry* pGdrEntry);

   // call this before getting data used by the grid
   bool UpdateData(bool doCheckData);

   // toogle fill
   void ToggleFill(ROWCOL rowNo);

   bool GetDebondInfo(StrandIndexType straightStrandGridIdx, Float64* pleftDebond, Float64* prightDebond);

private:
   void FillGrid();

   bool IsPermStrandFilled(GirderLibraryEntry::psStrandType strandType, StrandIndexType localIdx);

   // insert a row above the currently selected cell or at the top if no selection
   void InsertRow();
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   void RemoveStrandFill(const CUserData* pUserData);
   void AddStrandFill(const CUserData* pUserData);

   void UpdateParent();

   CTOGAGirderSelectStrandsDlg* m_pParent;
   const GirderLibraryEntry* m_pGdrEntry;
   CString m_strMaxDebondLength;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

