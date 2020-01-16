///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <PgsExt\CastDeckActivity.h>

interface IBridge;
interface IEAFDisplayUnits;

// DeckRegionGrid.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeckRegionGrid window

class CDeckRegionGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
   CDeckRegionGrid();

public:
   // custom stuff for grid
   void CustomInit();

   void Enable(BOOL bEnable);

   void GetData(CCastDeckActivity& activity);
   void SetData(const CCastDeckActivity& activity);

   void GetPierUsage(PierIndexType pierIdx, IBridge* pBridge, BOOL* pbUseBack, BOOL* pbUseAhead);
   
   // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeckRegionGrid)
	//}}AFX_VIRTUAL
   
   virtual BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);

// Implementation
public:
	virtual ~CDeckRegionGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDeckRegionGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CCastingRegion GetCastingRegion(ROWCOL row, IEAFDisplayUnits* pDisplayUnits);
   CCastingRegion::RegionType GetRegionType(ROWCOL row);
   CCastingRegion GetSpanData(ROWCOL row);
   CCastingRegion GetPierData(ROWCOL row, IEAFDisplayUnits* pDisplayUnits);

   ROWCOL GetPierRow(PierIndexType pierIdx);
   ROWCOL GetSpanRow(SpanIndexType spanIdx);

   void SetRegionData(const CCastingRegion& region, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits);
   void SetPierData(const CCastingRegion& region, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits);
   void SetPierData(PierIndexType pierIdx, BOOL bUseBack, Float64 Xback, BOOL bUseAhead, Float64 Xahead, IndexType sequenceIdx, IEAFDisplayUnits* pDisplayUnits);
   void SetSpanData(const CCastingRegion& region, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits);
   void SetSpanData(SpanIndexType spanIdx,Float64 L,IndexType sequenceIdx,IEAFDisplayUnits* pDisplayUnits);
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);
};
