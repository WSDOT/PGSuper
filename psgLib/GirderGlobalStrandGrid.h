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

#if !defined(AFX_GIRDERGLOBALSTRANDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
#define AFX_GIRDERGLOBALSTRANDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderGlobalStrandGrid.h : header file
//
#include <GeomModel/GeomModel.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <Units\Measure.h>

class CGirderGlobalStrandGrid;
class CStrandGenerationDlg;

// define a pure virtual class that clients of this grid can inherit from
class CGirderGlobalStrandGridClient
{
public:
   // capture event fired from grid that allows deletion of rows
   virtual void OnEnableDelete(bool canDelete)=0;
   virtual bool DoUseHarpedGrid()=0;
   virtual pgsTypes::AdjustableStrandType GetAdjustableStrandType()=0;
   virtual void UpdateStrandStatus(Uint16 ns, Uint16 ndb, Uint16 nh)=0; 
};

/////////////////////////////////////////////////////////////////////////////
// CGirderGlobalStrandGrid window

class CGirderGlobalStrandGrid : public CGXGridWnd
{
public:
   friend CGirderMainSheet;

//
   struct GlobalStrandGridEntry
   {
      GirderLibraryEntry::psStrandType m_Type;
      Float64 m_X;
      Float64 m_Y;

      bool m_CanDebond; // only for straight strands now

      Float64 m_Hend_X;
      Float64 m_Hend_Y;

      GlobalStrandGridEntry():m_X(0.0),m_Y(0.0),m_Type(GirderLibraryEntry::stStraight),m_CanDebond(false),m_Hend_X(0.0),m_Hend_Y(0.0)
      {;}

      bool operator==(const GlobalStrandGridEntry& rOther) const
      {
         return m_Type==rOther.m_Type && 
                m_X==rOther.m_X && m_Y==rOther.m_Y &&
                m_CanDebond==rOther.m_CanDebond &&
                m_Hend_X==rOther.m_Hend_X && m_Hend_Y==rOther.m_Hend_Y;
      }

      bool operator!=(const GlobalStrandGridEntry& rOther) const
      {
         return !(*this==rOther);
      }

   };
   typedef std::vector<GlobalStrandGridEntry> EntryCollectionType;
   typedef EntryCollectionType::iterator EntryIteratorType;

// Construction
public:
	CGirderGlobalStrandGrid(CGirderGlobalStrandGridClient* pClient);

// Attributes
public:

// Operations
public:
   void ReverseHarpedStrandOrder();
   void GenerateStrandPositions();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderGlobalStrandGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderGlobalStrandGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGirderGlobalStrandGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnEditrow();
	afx_msg void OnEditInsertrow();
	afx_msg void OnEditAppendrow();
	afx_msg void OnEditRemoverows();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual BOOL OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnLButtonDblClkRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt);
   virtual BOOL OnGridKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

public:
   // custom stuff for grid
   void CustomInit();
   // append row to end of grid
   void AppendSelectedRow();
   void InsertSelectedRow();
   void RemoveSelectedRow();
   // edit selected row
   void EditSelectedRow();
   // move rows up or down
   void MoveUpSelectedRow();
   void MoveDownSelectedRow();
   // fill grid with data
   void FillGrid(EntryCollectionType& entries);
   void RedrawGrid();

   // check box for use harped grid was changed
   void OnChangeUseHarpedGrid();

   // Web strand type changed
   void OnChangeWebStrandType();
private:
   ROWCOL Appendrow();
   void SelectRow(ROWCOL nRow);
   ROWCOL GetSelectedRow();
   void Removerows();

   void OnChangeStrandData();

   CGirderGlobalStrandGridClient* m_pClient;

private:
   // data and methods associated with strand entries
   EntryCollectionType m_Entries;

   // get entry index associated with grid row
   IndexType GetRowEntry(ROWCOL nRow);
   //  return number of rows required for entire collection
   ROWCOL GetRowsForEntries();

   bool EditEntry(ROWCOL row, GlobalStrandGridEntry& entry, bool isNewEntry);
   // fill at the starting row - return num rows filled
   ROWCOL FillRowsWithEntry(ROWCOL row, GlobalStrandGridEntry& entry, bool useHarped, pgsTypes::AdjustableStrandType asType, COLORREF color);


   void AppendEntry(GlobalStrandGridEntry& entry);
   void GenerateStraightStrands(CStrandGenerationDlg& dlg);
   void GenerateHarpedStrands(CStrandGenerationDlg& dlg);
   void DeleteAllStraightStrands();
   void DeleteAllHarpedStrands();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERGLOBALSTRANDGRID_H__8D165F54_32B9_11D2_9D40_00609710E6CE__INCLUDED_)
