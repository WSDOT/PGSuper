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

#if !defined(AFX_BRIDGEDESCDECKREBARGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_BRIDGEDESCDECKREBARGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeDescDeckRebarGrid.h : header file
//

#include <PgsExt\DeckRebarData.h>

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckRebarGrid window

class CBridgeDescDeckRebarGrid : public CGXGridWnd
{
// Construction
public:
	CBridgeDescDeckRebarGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgeDescDeckRebarGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBridgeDescDeckRebarGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBridgeDescDeckRebarGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);
   void OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey);

   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);

public:
   // custom stuff for grid
   void CustomInit();

   void EnableMats(BOOL bEnableTop,BOOL bEnableBottom);

   void AddRow();
   void RemoveSelectedRows();

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // get data for a row
   bool GetRowData(ROWCOL nRow, CDeckRebarData::NegMomentRebarData* pRebarData);
   void PutRowData(ROWCOL nRow, const CDeckRebarData::NegMomentRebarData& rebarData);

   // fill grid with data
   void FillGrid(const std::vector<CDeckRebarData::NegMomentRebarData>& vRebarData);
   bool GetRebarData(std::vector<CDeckRebarData::NegMomentRebarData>& vRebarData);

   void UpdatePierList();

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   WBFL::Materials::Rebar::Size GetBarSize(ROWCOL row);

   void UpdateCutoff(ROWCOL nRow,const CPierData2* pPier);

   std::_tstring CreatePierLabel(const CBridgeDescription2& bridgeDescr, PierIndexType pierIdx);
   PierIndexType GetPierIndexFromString(const CBridgeDescription2& bridgeDescr, const CString& string);


   BOOL m_bEnableTopMat;
   BOOL m_bEnableBottomMat;
   IndexType m_nContinuousPiers;
   CString m_strPiers;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCDECKREBARGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
