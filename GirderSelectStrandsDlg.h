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

#pragma once

#include <PsgLib\SpecLibraryEntry.h>
#include "StrandFillGrid.h"
#include <GraphicsLib\GraphicsLib.h>
#include <IFace\Bridge.h>
#include "afxwin.h"

// CGirderSelectStrandsDlg dialog

class CGirderSelectStrandsDlg : public CDialog
{
   friend CStrandFillGrid;

	DECLARE_DYNAMIC(CGirderSelectStrandsDlg)

public:
	CGirderSelectStrandsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGirderSelectStrandsDlg();

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

   void OnNumStrandsChanged();
   void UpdatePicture();

   // intialize and retreive data before/after domodal
   void InitializeData(SpanIndexType span, GirderIndexType girder, const CPrestressData& rPrestress, 
                       const SpecLibraryEntry* pSpecEntry,const GirderLibraryEntry* pGdrEntry, bool allowEndAdjustment, bool allowHpAdjustment,
                       HarpedStrandOffsetType endMeasureType, HarpedStrandOffsetType hpMeasureType, Float64 hpOffsetAtEnd, Float64 hpOffsetAtHp, 
                       Float64 maxDebondLength);

   bool GetData(CPrestressData& rPrestress); // return true if data changed


private:
   DirectStrandFillCollection m_DirectFilledStraightStrands;
   DirectStrandFillCollection m_DirectFilledHarpedStrands;
   DirectStrandFillCollection m_DirectFilledTemporaryStrands;

   // data for dialog resizing
   int m_BottomOffset; // distance from bottom of picture to bottom of dialog
   int m_RightOffset;  // distance from right edit of picture to right edge of dialog
   int m_Row1Offset;   // distance from bottom of picture to top of first row of strand data
   int m_Row2Offset;   // distance from bottom of picture to top of second row of strand data

   pgsTypes::AdjustableStrandType m_AdjustableStrandType;

   bool m_AllowHpAdjustment;
   bool m_AllowEndAdjustment;
   HarpedStrandOffsetType m_HsoEndMeasurement;
   HarpedStrandOffsetType m_HsoHpMeasurement;
   Float64 m_HpOffsetAtEnd;
   Float64 m_HpOffsetAtHp;

   bool m_bCanExtendStrands; // can strands be extended
   std::vector<StrandIndexType> m_ExtendedStrands[2]; // index is pgsTypes::MemberEndType

   bool m_CanDebondStrands; // are there any debondable strands in this girder?
   std::vector<CDebondInfo> m_StraightDebond;
   BOOL m_bSymmetricDebond;
   Float64 m_MaxDebondLength;

   const GirderLibraryEntry* m_pGdrEntry;
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
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

public:
   afx_msg void OnBnClickedShowNumbers();
   afx_msg void OnCbnSelchangeComboViewloc();
   afx_msg void OnCbnSelchangeHarpEndCb();
   afx_msg void OnCbnSelchangeHarpHpCb();
   afx_msg void OnHelp();
   afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
