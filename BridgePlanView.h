///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#if !defined(AFX_BRIDGEPLANVIEW_H__E2B376C9_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
#define AFX_BRIDGEPLANVIEW_H__E2B376C9_2D38_11D2_8EB4_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgePlanView.h : header file
//
//
#include <map>
#include <DManip\DManip.h>
#include <DManipTools\DManipTools.h>
#include <PgsExt\PierData.h>
#include "BridgeModelViewChildFrame.h"

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView view

class CBridgePlanView : public CDisplayView
{
   friend CBridgeModelViewChildFrame;
protected:
	CBridgePlanView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBridgePlanView)

   struct GirderDisplayObjectInfo
   {
      GirderDisplayObjectInfo(SpanGirderHashType spanGirderHash,long listID): SpanGirderHash(spanGirderHash), DisplayListID(listID) {}
      SpanGirderHashType SpanGirderHash;
      long DisplayListID;
   };

   struct PierDisplayObjectInfo
   {
      PierDisplayObjectInfo(PierIndexType pierIdx,long listID): PierIdx(pierIdx), DisplayListID(listID) {}
      PierIndexType PierIdx;
      long DisplayListID;
   };

   struct SpanDisplayObjectInfo
   {
      SpanDisplayObjectInfo(SpanIndexType spanIdx,long listID): SpanIdx(spanIdx), DisplayListID(listID) {}
      SpanIndexType SpanIdx;
      long DisplayListID;
   };

   struct DeckDisplayObjectInfo
   {
      DeckDisplayObjectInfo(Int32 deckID,long listID): ID(deckID), DisplayListID(listID) {}
      long ID;
      long DisplayListID;
   };

   // maps span girder hash values to a girder display object id
   std::map<SpanGirderHashType,long> m_GirderIDs;
   long m_NextGirderID;

// Attributes
public:

// Operations
public:
   void DoPrint(CDC* pDC, CPrintInfo* pInfo,CRect rcDraw);

   bool GetSelectedSpan(SpanIndexType* pSpanIdx);
   void SelectSpan(SpanIndexType spanIdx,bool bSelect);
   bool GetSelectedPier(PierIndexType* pPierIdx);
   void SelectPier(PierIndexType pierIdx,bool bSelect);
   bool GetSelectedGirder(SpanIndexType* pSpanIdx,GirderIndexType* pGirderIdx);
   void SelectGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bSelect);
   void SelectDeck(bool bSelect);
   void SelectAlignment(bool bSelect);
   void ClearSelection();
   bool IsDeckSelected();
   bool IsAlignmentSelected();

   virtual DROPEFFECT CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point);
   virtual void OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgePlanView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBridgePlanView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
   virtual void HandleLButtonDown(UINT nFlags, CPoint logPoint);
	virtual void HandleLButtonDblClk(UINT nFlags, CPoint logPoint);
   virtual void HandleContextMenu(CWnd* pWnd,CPoint logPoint);

	//{{AFX_MSG(CBridgePlanView)
	afx_msg void OnEditRoadway();
	afx_msg void OnEditBridge();
	afx_msg void OnEditDeck();
	afx_msg void OnViewSettings();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
   afx_msg void OnZoom();
   afx_msg void OnScaleToFit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CBridgeModelViewChildFrame* m_pFrame;

   void UpdateDisplayObjects();
   void BuildTitleDisplayObjects();
   void BuildAlignmentDisplayObjects();
   void BuildPierDisplayObjects();
   void BuildGirderDisplayObjects();
   void BuildSpanDisplayObjects();
   void BuildSlabDisplayObjects();
   void BuildSectionCutDisplayObjects();
   void BuildNorthArrowDisplayObjects();
   void BuildDiaphragmDisplayObjects();

   void UpdateDrawingScale();
   void UpdateSectionCut();
   void UpdateSectionCut(iPointDisplayObject* pntDO,BOOL bRedraw);

   void UpdateGirderTooltips();

   void DrawFocusRect();

   std::_tstring GetConnectionString(const CPierData* pPier);

   CBridgeModelViewChildFrame* GetFrame();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEPLANVIEW_H__E2B376C9_2D38_11D2_8EB4_006097DF3C68__INCLUDED_)
