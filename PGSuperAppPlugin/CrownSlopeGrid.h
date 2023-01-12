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

#if !defined(AFX_CrownSlopeGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_CrownSlopeGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CrownSlopeGrid.h : header file
//

#include <IFace\Project.h>

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopeGrid window

class CCrownSlopeGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CCrownSlopeGrid();
	CCrownSlopeGrid(RoadwaySectionData* pData); 

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCrownSlopeGrid)
	//}}AFX_VIRTUAL

   bool SortCrossSections(bool updateGrid);
   void InitRoadwaySectionData(bool updateHeader);
   bool UpdateRoadwaySectionData();

// Implementation
public:
	virtual ~CCrownSlopeGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCrownSlopeGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   // custom stuff for grid
   void CustomInit();
   bool AppendRow();
   void RemoveRows();
   bool IsRowSelected();
   bool IsGridEmpty();
   bool IsGridDataValid(ROWCOL* pBadRow);

   void UpdateGridSizeAndHeaders(const RoadwaySectionData& data);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   // get data for a row
   bool GetRowData(ROWCOL nRow,RoadwaySectionTemplate& data);
   void SetRowData(ROWCOL nRow,const RoadwaySectionTemplate& data);

   // virtual overrides for grid
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol) override;
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol) override;
   virtual void OnMovedCurrentCell(ROWCOL nRow, ROWCOL nCol) override;

private:
   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);
   void InitRowData(ROWCOL row);

   std::set<ROWCOL> m_LengthCols;
   std::set<ROWCOL> m_SlopeCols;

   RoadwaySectionData* m_pRoadwaySectionData;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CrownSlopeGrid_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
