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

#pragma once

// BridgePlanView.h : header file
//
//
#include "BridgeViewPane.h"
#include <map>

class CSelection;

/////////////////////////////////////////////////////////////////////////////
// CBridgePlanView view

class CBridgePlanView : public CBridgeViewPane
{
protected:
	CBridgePlanView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBridgePlanView)

   struct GirderDisplayObjectInfo
   {
      GirderDisplayObjectInfo(const CGirderKey& girderKey,IDType listID): m_GirderKey(girderKey), DisplayListID(listID) {}
      CGirderKey m_GirderKey;
      IDType DisplayListID;
   };

   struct SegmentDisplayObjectInfo
   {
      SegmentDisplayObjectInfo(const CSegmentKey& SegmentKey,IDType listID): m_SegmentKey(SegmentKey), DisplayListID(listID) {}
      CSegmentKey m_SegmentKey;
      IDType DisplayListID;
   };

   struct PierDisplayObjectInfo
   {
      PierDisplayObjectInfo(PierIndexType pierIdx,IDType listID): PierIdx(pierIdx), DisplayListID(listID) {}
      PierIndexType PierIdx;
      IDType DisplayListID;
   };

   struct TemporarySupportDisplayObjectInfo
   {
      TemporarySupportDisplayObjectInfo(SupportIndexType tsIdx, IDType listID) : tsIdx(tsIdx), DisplayListID(listID) {}
      SupportIndexType tsIdx;
      IDType DisplayListID;
   };

   struct SpanDisplayObjectInfo
   {
      SpanDisplayObjectInfo(SpanIndexType spanIdx,IDType listID): SpanIdx(spanIdx), DisplayListID(listID) {}
      SpanIndexType SpanIdx;
      IDType DisplayListID;
   };

   struct DeckDisplayObjectInfo
   {
      DeckDisplayObjectInfo(IDType deckID,IDType listID): ID(deckID), DisplayListID(listID) {}
      IDType ID;
      IDType DisplayListID;
   };

   // maps span girder hash values to a girder display object id
   std::map<CGirderKey,IDType> m_GirderIDs;
   IDType m_NextGirderID;

   std::map<CSegmentKey,IDType> m_SegmentIDs;
   IDType m_NextSegmentID;

   std::map<CSegmentKey,IDType> m_ClosureJointIDs;
   IDType m_NextClosureJointID;

// Attributes
public:

// Operations
public:
   bool GetSelectedSpan(SpanIndexType* pSpanIdx);
   void SelectSpan(SpanIndexType spanIdx,bool bSelect);
   bool GetSelectedPier(PierIndexType* pPierIdx);
   void SelectPier(PierIndexType pierIdx,bool bSelect);
   bool GetSelectedGirder(CGirderKey* pGirderKey);
   void SelectGirder(const CGirderKey& girderKey,bool bSelect);
   bool GetSelectedSegment(CSegmentKey* pSegmentKey);
   void SelectSegment(const CSegmentKey& segmentKey,bool bSelect);
   bool GetSelectedClosureJoint(CSegmentKey* pClosureKey);
   void SelectClosureJoint(const CSegmentKey& closureKey,bool bSelect);
   void SelectDeck(bool bSelect);
   void SelectAlignment(bool bSelect);
   bool GetSelectedTemporarySupport(SupportIndexType* ptsIdx);
   void SelectTemporarySupport(SupportIDType tsID,bool bSelect);
   void ClearSelection();
   bool IsDeckSelected();
   bool IsAlignmentSelected();

   void GetGroupRange(GroupIndexType* pStartGroupIdx,GroupIndexType* pEndGroupIdx);
   void SetGroupRange(GroupIndexType startGroupIdx,GroupIndexType endGroupIdx,bool bUpdate = true);

   virtual DROPEFFECT CanDrop(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point);
   virtual void OnDropped(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBridgePlanView)
	public:
	virtual void OnInitialUpdate();
	protected:
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
   virtual void HandleLButtonDblClk(UINT nFlags, CPoint logPoint);
   virtual void HandleContextMenu(CWnd* pWnd,CPoint logPoint);

	//{{AFX_MSG(CBridgePlanView)
	afx_msg void OnEditDeck();
	afx_msg void OnViewSettings();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
   afx_msg void OnZoom();
   afx_msg void OnScaleToFit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual void BuildDisplayLists();
   virtual void UpdateDrawingScale();
   virtual void UpdateDisplayObjects();

   void SetModelToWorldSpacingMapping();

   void BuildTitleDisplayObjects();
   void BuildAlignmentDisplayObjects();
   void BuildPierDisplayObjects();
   void BuildTemporarySupportDisplayObjects();
   void BuildClosureJointDisplayObjects();
   void BuildSegmentDisplayObjects();
   void BuildLongitudinalJointDisplayObject();
   void BuildGirderDisplayObjects();
   void BuildSpanDisplayObjects();
   void BuildSlabDisplayObjects();
   void BuildSectionCutDisplayObjects();
   void BuildNorthArrowDisplayObjects();
   void BuildDiaphragmDisplayObjects();

   void UpdateSectionCut();
   void UpdateSectionCut(iPointDisplayObject* pntDO,BOOL bRedraw);

   void UpdateSegmentTooltips();
   void UpdateClosureJointTooltips();

   std::_tstring GetConnectionString(const CPierData2* pPier);
   std::_tstring GetFullConnectionString(const CPierData2* pPier);
   std::_tstring GetConnectionString(const CTemporarySupportData* pTS);
   std::_tstring GetFullConnectionString(const CTemporarySupportData* pTS);

   void Select(const CSelection* pSelection);

   // Range of groups that will be displayed
   SpanIndexType m_StartGroupIdx;
   SpanIndexType m_EndGroupIdx;
};
