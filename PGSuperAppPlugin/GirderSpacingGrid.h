///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#if !defined(AFX_GIRDERSPACINGGRID_H_INCLUDED_)
#define AFX_GIRDERSPACINGGRID_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderSpacingGrid.h : header file
//

#include <PgsExt\GirderSpacing2.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\GirderGroupData.h>

class CGirderSpacingGrid;

void DDV_SpacingGrid(CDataExchange* pDX,int nIDC,CGirderSpacingGrid* pGrid);

/////////////////////////////////////////////////////////////////////////////
// CGirderSpacingGrid window

class CGirderSpacingGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CGirderSpacingGrid();

// Attributes
public:

// Operations
public:
   void Enable(BOOL bEnable);

   void SetPierSkewAngle(Float64 skewAngle);

   bool InputSpacing() const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderSpacingGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderSpacingGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderSpacingGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg void OnExpand();
   afx_msg void OnJoin();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   // custom stuff for grid
   void CustomInit();
   void InitializeGridData(CGirderSpacing2* pGirderSpacing,CGirderGroupData* pGirderGroup,pgsTypes::PierFaceType pierFace,PierIndexType pierIdx,Float64 skewAngle,bool bAbutment,pgsTypes::SupportedDeckType deckType);
   void UpdateGrid();

   BOOL ValidateGirderSpacing();

private:
   BOOL m_bEnabled;
   PierIndexType m_PierIdx;
   bool m_bAbutment;
   Float64 m_PierSkewAngle;
   pgsTypes::SupportedDeckType m_DeckType;
   
   pgsTypes::PierFaceType m_PierFace;
   CGirderGroupData* m_pGirderGroup;
   CGirderSpacing2* m_pGirderSpacing;

   std::vector<Float64> m_MinGirderSpacing;
   std::vector<Float64> m_MaxGirderSpacing;

   virtual BOOL OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState);
   BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERSPACINGGRID_H_INCLUDED_)
