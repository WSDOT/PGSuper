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

#include <EAF\EAFGraphControlWindow.h>

class CSegmentGraphControllerBase : public CEAFGraphControlWindow
{
public:
   CSegmentGraphControllerBase(bool bAllGroups);
   DECLARE_DYNCREATE(CSegmentGraphControllerBase);

   GroupIndexType GetGirderGroup();
   GirderIndexType GetGirder();
   SegmentIndexType GetSegment();
   const CSegmentKey& GetSegmentKey() const;
   void SelectSegment(const CSegmentKey& SegmentKey);

   void ShowGrid(bool bShow);
   bool ShowGrid() const;

   void ShowBeam(bool bShow);
   bool ShowBeam() const;

   // Sometimes we want to show the beam below our graph
   virtual bool ShowBeamBelowGraph() const = 0;

   // called by the framework when the view's OnUpdate method is called
   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

protected:

   virtual BOOL OnInitDialog() override;

   // let subclasses decide how to deal with ISelection
   virtual CSegmentKey GetAppSelectedSegment();

	//{{AFX_MSG(CSegmentGraphControllerBase)
   afx_msg void CbnOnGroupChanged();
   afx_msg void CbnOnGirderChanged();
   afx_msg void CbnOnSegmentChanged();
   afx_msg void OnShowGrid();
   afx_msg void OnShowBeam();
   //}}AFX_MSG

   // Called from CbnOnXXXChanged just before UpdateGraph is called
   virtual void OnGroupChanged();
   virtual void OnGirderChanged();
   virtual void OnSegmentChanged();


	DECLARE_MESSAGE_MAP()

   std::shared_ptr<WBFL::EAF::Broker> m_pBroker;

   void FillGroupCtrl();
   void FillGirderCtrl();
   void FillSegmentCtrl();

   void UpdateGraph();

   bool m_bAllGroups;

   // control variables
   CSegmentKey m_SegmentKey;
 
#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};

class CIntervalSegmentGraphControllerBase : public CSegmentGraphControllerBase
{
public:
   CIntervalSegmentGraphControllerBase(bool bAllGroups=true);
   DECLARE_DYNCREATE(CIntervalSegmentGraphControllerBase);

   void SetInterval(IntervalIndexType intervalIdx);
   IntervalIndexType GetInterval() const;

   // called by the framework when the view's OnUpdate method is called
   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

   // returns the first and last interval in the valid range of intervals for the graph
   virtual IntervalIndexType GetFirstInterval();
   virtual IntervalIndexType GetLastInterval();

protected:

   virtual BOOL OnInitDialog() override;

	//{{AFX_MSG(CIntervalSegmentGraphControllerBase)
   afx_msg void CbnOnIntervalChanged();
   //}}AFX_MSG

   // Called from CbnOnXXXChanged just before UpdateGraph is called
   virtual void OnIntervalChanged();

	DECLARE_MESSAGE_MAP()

   void FillIntervalCtrl();

   // control variables
   IntervalIndexType      m_IntervalIdx;
 
#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};

class CMultiIntervalSegmentGraphControllerBase : public CSegmentGraphControllerBase
{
public:
   CMultiIntervalSegmentGraphControllerBase(bool bAllGroups=true);
   DECLARE_DYNCREATE(CMultiIntervalSegmentGraphControllerBase);

   void SelectInterval(IntervalIndexType intervalIdx);
   void SelectIntervals(const std::vector<IntervalIndexType>& vIntervals);
   std::vector<IntervalIndexType> GetSelectedIntervals() const;
   IndexType GetGraphCount() const;

   // called by the framework when the view's OnUpdate method is called
   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

   // returns the first and last interval in the valid range of intervals for the graph
   virtual IntervalIndexType GetFirstInterval();
   virtual IntervalIndexType GetLastInterval();

protected:

   virtual BOOL OnInitDialog() override;

	//{{AFX_MSG(CMultiIntervalSegmentGraphControllerBase)
   afx_msg void OnIntervalsChanged();
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

   void FillIntervalCtrl();
 
#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};