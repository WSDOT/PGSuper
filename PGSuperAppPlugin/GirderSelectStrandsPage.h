///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <PsgLib\SpecLibraryEntry.h>
#include "StrandFillGrid.h"
#include <GraphicsLib\GraphicsLib.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include "afxwin.h"

// CGirderSelectStrandsPage dialog

class CGirderSelectStrandsPage : public CPropertyPage
{
   friend CStrandFillGrid;

	DECLARE_DYNAMIC(CGirderSelectStrandsPage)

public:
	CGirderSelectStrandsPage();
	virtual ~CGirderSelectStrandsPage();

// Dialog Data
	enum { IDD = IDD_GIRDER_SELECT_STRANDS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();

   afx_msg void OnBnClickedCheckSymm();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnUpdateHsPjEdit();
	afx_msg void OnUpdateSsPjEdit();
	afx_msg void OnUpdateTempPjEdit();

   void OnNumStrandsChanged();
   void UpdatePicture();

   // intialize data before domodal
   void InitializeData(const CSegmentKey& segmentKey, CStrandData* pStrands, 
                       const SpecLibraryEntry* pSpecEntry,const GirderLibraryEntry* pGdrEntry, bool allowEndAdjustment, bool allowHpAdjustment,
                       HarpedStrandOffsetType endMeasureType, HarpedStrandOffsetType hpMeasureType, 
                       Float64 hpOffsetAtStart, Float64 hpOffsetAtHp1, Float64 hpOffsetAtHp2, Float64 hpOffsetAtEnd,
                       Float64 maxDebondLength,
                       Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd);

private:
   // data for dialog resizing
   int m_BottomOffset; // distance from bottom of picture to bottom of dialog
   int m_RightOffset;  // distance from right edit of picture to right edge of dialog
   int m_Row1Offset;   // distance from bottom of picture to top of first row of strand data
   int m_Row2Offset;   // distance from bottom of picture to top of second row of strand data

   pgsTypes::AdjustableStrandType m_AdjustableStrandType;

   bool m_bAllowHpAdjustment;
   bool m_bAllowEndAdjustment;
   HarpedStrandOffsetType m_HsoEndMeasurement;
   HarpedStrandOffsetType m_HsoHpMeasurement;
   Float64 m_HpOffsetAtEnd[2];
   Float64 m_HpOffsetAtHp[2];
   Float64 m_HgEnd[2];
   Float64 m_HgHp[2];

   bool m_bCanExtendStrands; // can strands be extended
   std::vector<StrandIndexType> m_ExtendedStrands[2]; // index is pgsTypes::MemberEndType

   bool m_bCanDebondStrands; // are there any debondable strands in this girder?
   std::vector<CDebondData> m_StraightDebond;
   BOOL m_bSymmetricDebond;
   Float64 m_MaxDebondLength;

   const GirderLibraryEntry* m_pGdrEntry;
   CSegmentKey m_SegmentKey;
   CStrandData* m_pStrands;

private:
   CStrandFillGrid m_Grid;
   int             m_IsMidSpan;
   BOOL            m_DrawNumbers;
   Float64         m_Radius;

   // tracking for mouse clicks - row is row in grid
   std::vector< std::pair<CRect, ROWCOL> > m_StrandLocations;

   CToolTipCtrl* m_pToolTip;

   // don't allow dialog to be smaller than initial size
   bool m_bFirstSize;
   CSize m_FirstSize;

private:
   void DrawShape(CDC* pDC,IShape* shape,grlibPointMapper& mapper);
   void DrawStrands(CDC* pDC,grlibPointMapper& mapper, IStrandMover* strand_mover, Float64 absol_end_offset, Float64 absol_hp_offset);
   StrandIndexType DrawStrand(CDC* pDC, grlibPointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index, bool isFilled, ROWCOL gridRow);

   void AddClickRect(CRect rect, ROWCOL nRow);

   gpRect2d ComputeStrandBounds(IStrandMover* strand_mover, Float64 absol_end_offset, Float64 absol_hp_offset);

   void UpdateStrandAdjustments();
   void ShowHarpedAdjustmentControls(BOOL show, bool AreHarpStraight);
   void ShowHarpedHpAdjustmentControls(BOOL show);
   void ShowHarpedEndAdjustmentControls(BOOL show, bool AreHarpStraight);
   void EnableHarpedHpAdjustmentControls(BOOL enable);
   void EnableHarpedEndAdjustmentControls(BOOL enable);

   void UpdateStrandInfo();

   Float64 GetMaxPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType); // allowable by spec
   Float64 GetUltPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType); // breaking strength
   void UpdatePjackEdit( UINT nCheckBox  );
   void UpdatePjackEditEx(StrandIndexType nStrands, UINT nCheckBox  );


public:
   afx_msg void OnBnClickedShowNumbers();
   afx_msg void OnCbnSelchangeComboViewloc();
   afx_msg void OnCbnSelchangeHarpEndCb();
   afx_msg void OnCbnSelchangeHarpHpCb();
   afx_msg void OnHelp();
   afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
