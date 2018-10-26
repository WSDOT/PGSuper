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

#if !defined(AFX_GIRDERSPACINGGRID_H_INCLUDED_)
#define AFX_GIRDERSPACINGGRID_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderSpacingGrid.h : header file
//

#include <PgsExt\GirderSpacing.h>
#include <PgsExt\SpanData.h>

class CGirderSpacingGrid;

struct CGirderSpacingGridData
{
   pgsTypes::PierFaceType m_PierFace;
   CGirderTypes   m_GirderTypes;
   CGirderSpacing m_GirderSpacing;
};

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
   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);
   void SetGirderCount(GirderIndexType nGirders);

   void Enable(BOOL bEnable);
   void SetLinkedGrid(CGirderSpacingGrid* pLinkedGrid);
   bool IsLinked(bool bLinked);

   CGirderSpacingGridData GetGirderSpacingData();
   void SetGirderSpacingData(const CGirderSpacingGridData& gridData);

   void SetPierSkewAngle(Float64 skewAngle);

   void SetMeasurementType(pgsTypes::MeasurementType mt);
   void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);

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
   void Init(pgsTypes::SupportedBeamSpacing girderSpacingType,bool bSharedGirderCount,const CGirderSpacing* pGirderSpacing,const CGirderTypes* pGirderTypes,pgsTypes::PierFaceType pierFace,PierIndexType pierIdx,Float64 skewAngle,bool bAbutment);
   void CustomInit(pgsTypes::SupportedBeamSpacing girderSpacingType,bool bSharedGirderCount,const CGirderSpacing* pGirderSpacing,const CGirderTypes* pGirderTypes,pgsTypes::PierFaceType pierFace,PierIndexType pierIdx,Float64 skewAngle,bool bAbutment);
   void CustomInit();
   void FillGrid(const CGirderSpacing* pGirderSpacing);
   void FillGrid();

   void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing girderSpacingType);
   void SharedGirderCount(bool bShare);

   BOOL ValidateGirderSpacing();

private:
   BOOL m_bEnabled;
   PierIndexType m_PierIdx;
   bool m_bAbutment;
   Float64 m_PierSkewAngle;
   
   CGirderSpacingGridData m_GridData;

   std::vector<Float64> m_MinGirderSpacing;
   std::vector<Float64> m_MaxGirderSpacing;

   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;

   bool m_bSharedGirderCount;

   CGirderSpacingGrid* m_pLinkedGrid;
   bool m_bLinked;

   typedef std::pair<GirderIndexType,GirderIndexType> UserData;

   virtual BOOL OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState);
   BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);
   BOOL OnEndEditing(ROWCOL nRow,ROWCOL nCol);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERSPACINGGRID_H_INCLUDED_)
