///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined(AFX_LLDFGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_LLDFGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LLDFGrid.h : header file
//

#include <PgsExt\BridgeDescription.h>

/////////////////////////////////////////////////////////////////////////////
// CLLDFGrid window

class CLLDFGrid : public CGXGridWnd
{
// Construction
public:
	CLLDFGrid();

// Attributes
public:

// Operations
public:
   void Enable(BOOL bEnable);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLLDFGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLLDFGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLLDFGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   BOOL m_bEnabled;

   // virtual overrides for grid
   int GetColWidth(ROWCOL nCol);

private:
   void AddPierRow(pgsTypes::LimitState ls,const CPierData* pPier);
   void AddSpanRow(pgsTypes::LimitState ls,const CSpanData* pSpan);

   void GetPierRow(pgsTypes::LimitState ls,CPierData* pPier);
   void GetSpanRow(pgsTypes::LimitState ls,CSpanData* pSpan);

public:
   // custom stuff for grid
   void CustomInit();

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // fill grid with data
   void FillGrid(pgsTypes::LimitState ls,const CBridgeDescription* pBridgeDesc);
   void GetData(pgsTypes::LimitState ls,CBridgeDescription* pBridgeDesc);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LLDFGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
