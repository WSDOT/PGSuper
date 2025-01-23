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

#if !defined(AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
#define AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cfSplitChildFrame.h : header file
// 
#include "SplitChildFrm.h"
#include "SectionCutDrawStrategy.h"
#include "pgsExt\PointLoadData.h"
#include <PgsExt/GirderGroupData.h>
#include <EAF\EAFViewControllerFactory.h>
#include <MfcTools\WideDropDownComboBox.h>
#include <IFace/Project.h>
#include <IFace/Selection.h>
#include <DManip/ToolPalette.h>

class CGirderModelElevationView;
class CGirderModelSectionView;
class CGirderViewPrintJob;

/// Custom version of CToolPalette that allows the use of a the CWideDropDownComboBox
class CMyToolPalette : public CToolPalette
{
public:
   virtual void DoDataExchange(CDataExchange* pDX);

protected:
   CWideDropDownComboBox m_cbEvents;
};


/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame frame

class CGirderModelChildFrame : public CSplitChildFrame, public iCutLocation, public CEAFViewControllerFactory
{
	DECLARE_DYNCREATE(CGirderModelChildFrame)

   friend CGirderViewPrintJob;
protected:
	CGirderModelChildFrame();           // protected constructor used by dynamic creation

// Attributes
public:
// Operations
   // Let our views tell us about updates
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

   void SyncWithBridgeModelView(bool bSync);
   bool SyncWithBridgeModelView() const;

   void SelectGirder(const CGirderKey& girderKey, bool bDoUpdate);
   const CGirderKey& GetSelection() const;

   EventIndexType GetEvent() const {return m_EventIndex;}
   bool SetEvent(EventIndexType eventIdx);

   void ShowStrands(bool bShow);
   bool ShowStrands() const;

   void ShowStrandCG(bool bShow);
   bool ShowStrandCG() const;

   void ShowCG(bool bShow);
   bool ShowCG() const;

   void ShowSectionProperties(bool bShow);
   bool ShowSectionProperties() const;

   void ShowDimensions(bool bShow);
   bool ShowDimensions() const;

   void ShowLongitudinalReinforcement(bool bShow);
   bool ShowLongitudinalReinforcement() const;

   void ShowTransverseReinforcement(bool bShow);
   bool ShowTransverseReinforcement() const;

   void ShowLoads(bool bShow);
   bool ShowLoads() const;

   void Schematic(bool bSchematic);
   bool Schematic() const;


   // iCutLocation
   virtual Float64 GetCurrentCutLocation() override;
   virtual void CutAt(Float64 Xgp) override;
   virtual void CutAtNext() override;
   virtual void CutAtPrev() override;
   virtual void ShowCutDlg() override;
   virtual void GetCutRange(Float64* pMin, Float64* pMax) override;

   pgsPointOfInterest GetCutLocation();

   // CEAFViewControllerFactory
protected:
   virtual void CreateViewController(IEAFViewController** ppController) override;

protected:
   void DoSyncWithBridgeModelView(bool bSync);

   void RefreshGirderLabeling();

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderModelChildFrame)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CMDIFrameWnd* pParentWnd = nullptr,
				CCreateContext* pContext = nullptr);
   virtual BOOL OnCmdMsg(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CGirderModelChildFrame();

	// Generated message map functions
	//{{AFX_MSG(CGirderModelChildFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintDirect();
	afx_msg void OnSelectEvent();
	afx_msg void OnAddPointload();
	afx_msg void OnAddDistributedLoad();
	afx_msg void OnAddMoment();
   afx_msg void OnSync();
   afx_msg void OnSetFocus(CWnd* pOldWnd);
   afx_msg void OnUpdateDesignGirderDirect(CCmdUI* pCmdUI);
   afx_msg void OnUpdateDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI);
   afx_msg void OnDesignGirderDirect();
   afx_msg void OnDesignGirderDirectHoldSlabOffset();
   afx_msg void OnUpdateStrandsButton(CCmdUI* pCmdUI);
   afx_msg void OnStrandsButton();
   afx_msg void OnUpdateStrandsCGButton(CCmdUI* pCmdUI);
   afx_msg void OnStrandsCGButton();
   afx_msg void OnUpdateDimensionsButton(CCmdUI* pCmdUI);
   afx_msg void OnDimensionsButton();
   afx_msg void OnUpdatePropertiesButton(CCmdUI* pCmdUI);
   afx_msg void OnPropertiesButton();
   afx_msg void OnUpdateLongitudinalReinforcementButton(CCmdUI* pCmdUI);
   afx_msg void OnLongitudinalReinforcementButton();
   afx_msg void OnUpdateStirrupsButton(CCmdUI* pCmdUI);
   afx_msg void OnStirrupsButton();
   afx_msg void OnUpdateUserLoadsButton(CCmdUI* pCmdUI);
   afx_msg void OnUserLoadsButton();
   afx_msg void OnUpdateSchematicButton(CCmdUI* pCmdUI);
   afx_msg void OnSchematicButton();
   afx_msg void OnUpdateSectionCGButton(CCmdUI* pCmdUI);
   afx_msg void OnSectionCGButton();
   //}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   // update views - refreshes frame and views
   void UpdateViews();
   // update bar - set dialog bar content and update views
   void UpdateBar();

   virtual CRuntimeClass* GetLowerPaneClass() const;
   virtual Float64 GetTopFrameFraction() const;
   pgsPointOfInterest GetCutPointOfInterest(Float64 X);
   void UpdateCutLocation(const pgsPointOfInterest& poi);
   void OnUpdateFrameTitle(BOOL bAddToTitle);
   
   virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

   CGirderModelElevationView* GetGirderModelElevationView() const;
   CGirderModelSectionView*   GetGirderModelSectionView() const;

private:
   void OnGirderChanged();
   void OnGroupChanged();
   void OnSectionCut();
   void DoFilePrint(bool direct);
   void FillEventComboBox();

   CMyToolPalette m_SettingsBar;

   // view variables
   Float64 ConvertToGirderlineCoordinate(Float64 Xgl) const;
   Float64 ConvertFromGirderlineCoordinate(Float64 Xgl) const;
   void UpdateCutRange();
   pgsPointOfInterest m_cutPoi;
   pgsPointOfInterest m_minPoi;
   pgsPointOfInterest m_maxPoi;
   bool m_bFirstCut{ true };
   
   EventIndexType m_EventIndex; 

   CGirderKey m_GirderKey;
   bool m_bIsAfterFirstUpdate;

   template <class T>
   void InitLoad(T& load) const
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

      if (SyncWithBridgeModelView())
      {
         GET_IFACE2(pBroker, ISelection, pSelection);
         CSelection selection = pSelection->GetSelection();
         ATLASSERT(selection.Type == CSelection::Girder);
         ATLASSERT(selection.GroupIdx != ALL_GROUPS);
         auto group = pIBridgeDesc->GetGirderGroup(selection.GroupIdx);
         load.m_SpanKey.spanIndex = group->GetPier(pgsTypes::metStart)->GetSpan(pgsTypes::Ahead)->GetIndex();
         load.m_SpanKey.girderIndex = selection.GirderIdx;
      }
      else
      {
         SpanIndexType spanIdx;
         if (m_GirderKey.groupIndex == ALL_GROUPS)
         {
            spanIdx = ALL_SPANS;
         }
         else
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex);
            spanIdx = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan()->GetIndex();
         }

         load.m_SpanKey.spanIndex = spanIdx;
         load.m_SpanKey.girderIndex = m_GirderKey.girderIndex;
      }

      EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
      if (m_EventIndex == liveLoadEventIdx)
      {
         load.m_LoadCase = UserLoads::LL_IM;
      }
      else
      {
         load.m_LoadCase = UserLoads::DC;
      }
   }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERMODELCHILDFRAME_H__19F76E39_6848_11D2_9D7B_00609710E6CE__INCLUDED_)
