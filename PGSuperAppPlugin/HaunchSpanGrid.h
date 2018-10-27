///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#pragma once;

#include <PgsExt\BridgeDescription2.h>

// Local data structures for SlabOffset at pier
typedef struct SlabOffsetBearingData
{
   enum PDType {pdAhead, // Ahead bearing line
                pdCL,    // Centerline bearing line (continuous interior piers only)
                pdBack} ; // Back bearing line
   PDType                 m_PDType;
   std::vector< Float64 > m_AsForGirders; // A for each girder at bearing line if SlabOffsetType is sotPier,
                                          // Then A for all girders is in slot[0]

   PierIndexType  m_PierIndex;  // pier and group associated with this bearing line
   GroupIDType    m_pGroupIndex;
} SlabOffsetBearingData;

typedef std::vector<SlabOffsetBearingData> SlabOffsetBearingDataVec;
typedef SlabOffsetBearingDataVec::iterator SlabOffsetBearingDataIter;
typedef SlabOffsetBearingDataVec::const_iterator SlabOffsetBearingDataConstIter;

// Local data structures for Assumed Excess Camber at spans
typedef struct AssExcessCamberSpanData
{
   std::vector< Float64 > m_AssExcessCambersForGirders; // Assumed Excess Camber for each girder at in span if 
                                          // AssExcessCamberType is aecGirder,
                                          // If aecSpan value for all girders is in slot[0]

   SpanIndexType  m_SpanIndex;
} AssExcessCamberSpanData;

typedef std::vector<AssExcessCamberSpanData> AssExcessCamberSpanDataVec;
typedef AssExcessCamberSpanDataVec::iterator AssExcessCamberSpanDataIter;
typedef AssExcessCamberSpanDataVec::const_iterator AssExcessCamberSpanDataConstIter;


class HaunchInputData
{
public:
   // Slab Offsets
   pgsTypes::SlabOffsetType m_SlabOffsetType;

   // Different Data for each layout type
   Float64 m_SingleSlabOffset;
   SlabOffsetBearingDataVec m_BearingsSlabOffset; // for both sotPier and sotGirder. if sotPier, A is in m_AsForGirders[0]

   // Assumed Excess Cambers
   pgsTypes::AssExcessCamberType m_AssExcessCamberType;

   // Different Data for each layout type
   Float64 m_SingleAssExcessCamber;
   AssExcessCamberSpanDataVec m_AssExcessCamberSpans; // for both aecSpan and aecGirder. if span, val is in m_AsForGirders[0]

   // General
   GirderIndexType m_MaxGirdersPerSpan;
};

#define _AHEADSTR _T("Ahead")
#define _BACKSTR  _T("Back")
#define _CLSTR    _T("C.L.")

#pragma once
// HaunchSpanGrid.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CHaunchSpanGrid window

class CHaunchSpanGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CHaunchSpanGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHaunchSpanGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHaunchSpanGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHaunchSpanGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);

public:
   // custom init for grid
   void CustomInit();

   void FillGrid(const HaunchInputData& haunchData);
   void GetData(Float64 minA, CString& minValError, HaunchInputData* pData, CDataExchange* pDX);

private:

   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

   const unitmgtLengthData* M_pCompUnit;

   std::vector<GirderIndexType> m_GirderCounts; // save number of girders per bearing line
};
