///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
// CTOGAGirderSelectStrandsDlg dialog

// NOTE, This dialog and corresponding grid control are nearly identical to the dialog in the 
//       main PGsuper project.  Any bug fixes made on this dialogue and grid control should also be made
//       in the PGsuper project.

#pragma once

#include <PsgLib\SpecLibraryEntry.h>
#include "TOGAStrandFillGrid.h"
#include <GraphicsLib\GraphicsLib.h>
#include <IFace\Bridge.h>
#include "afxwin.h"


class CTOGAGirderSelectStrandsDlg : public CDialog
{
   friend CTOGAStrandFillGrid;

	DECLARE_DYNAMIC(CTOGAGirderSelectStrandsDlg)

public:
	CTOGAGirderSelectStrandsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTOGAGirderSelectStrandsDlg();

// Dialog Data
	enum { IDD = IDD_GIRDER_SELECT_STRANDS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
   virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog() override;

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

   void OnNumStrandsChanged();
   void UpdatePicture();

   // intialize and retreive data before/after domodal
   void InitializeData(SpanIndexType span, GirderIndexType girder, 
                       const CDirectStrandFillCollection& directFilledStraightStrands,
                       const std::vector<CDebondData>& straightDebond,
                       const SpecLibraryEntry* pSpecEntry,const GirderLibraryEntry* pGdrEntry, Float64 maxDebondLength);

   bool GetData(CDirectStrandFillCollection& directFilledStraightStrands,
                std::vector<CDebondData>& straightDebond); // return true if data changed

private:
   CDirectStrandFillCollection m_DirectFilledStraightStrands;

   // data for dialog resizing
   int m_BottomOffset; // distance from bottom of picture to bottom of dialog
   int m_RightOffset;  // distance from right edit of picture to right edge of dialog
   int m_Row1Offset;   // distance from bottom of picture to top of first row of strand data
   int m_Row2Offset;   // distance from bottom of picture to top of second row of strand data

   bool m_CanDebondStrands; // are there any debondable strands in this girder?
   std::vector<CDebondData> m_StraightDebond;
   Float64 m_MaxDebondLength;

   BOOL m_bSymmetricDebond; // always true in TOGA

   const GirderLibraryEntry* m_pGdrEntry;
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
private:
   CTOGAStrandFillGrid m_Grid;
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
   void DrawStrands(CDC* pDC,grlibPointMapper& mapper);
   StrandIndexType DrawStrand(CDC* pDC, grlibPointMapper& Mapper, Float64 x, Float64 y, StrandIndexType index, bool isFilled, ROWCOL gridRow);

   void AddClickRect(CRect rect, ROWCOL nRow);

   void UpdateStrandInfo();

public:
   afx_msg void OnBnClickedShowNumbers();
   afx_msg void OnCbnSelchangeComboViewloc();
   afx_msg void OnHelp();
   afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
