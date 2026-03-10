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

#if !defined(AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
#define AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderDescDlg.h : header file
//
#include "BridgeDescPrestressPage.h"
#include "ShearSteelPage2.h"
#include "BridgeDescLongitudinalRebar.h"
#include "BridgeDescLiftingPage.h"
#include "DebondDlg.h"
#include "GirderDescGeneralPage.h"
#include "SpanGdrDetailsBearingsPage.h"

#include <PsgLib\SplicedGirderData.h>
#include <PsgLib\BridgeDescription2.h>
#include <IFace\ExtendUI.h>
#include <EAF\MacroTxn.h>

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDlg

class CGirderDescDlg : public CPropertySheet, public IEditGirderData, public CShearSteelPageParent
{
	DECLARE_DYNAMIC(CGirderDescDlg)

// Construction
public:
	CGirderDescDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

   // IEditGirderData
   virtual const CSegmentKey& GetSegmentKey() override { return m_SegmentKey; }

   //CShearSteelPageParent
   virtual bool HasDeck() const override;
   virtual LPCTSTR GetIntentionalRougheningPrompt() const override;

// Attributes
public:
   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;
   CTimelineManager m_TimelineMgr; // copy of the timeine manager we are editing
   CGirderGroupData m_Group;
   CSplicedGirderData m_Girder; // copy of the girder we are editing (contains the segment we are editing)
   CPrecastSegmentData* m_pSegment;
   CSegmentKey m_SegmentKey; // key to the segment we are editing
   SegmentIDType m_SegmentID; // ID of the segment we are editing

   CGirderDescGeneralPage       m_General;
   CGirderDescPrestressPage     m_Prestress;
   CShearSteelPage2             m_Shear;
   CGirderDescLongitudinalRebar m_LongRebar;
   CGirderDescLiftingPage       m_Lifting;
   CGirderDescDebondPage        m_StrandExtensionandDebond;
   CSpanGdrDetailsBearingsPage m_SpanGdrDetailsBearingsPage;

   std::_tstring m_strGirderName;
   void SetSegment(const CPrecastSegmentData& segment);
   const CPrecastSegmentData* GetSegment();

   void SetConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor);
   pgsTypes::ConditionFactorType GetConditionFactorType();
   Float64 GetConditionFactor();


   bool m_bApplyToAll;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescDlg)
	public:
	virtual BOOL OnInitDialog();
   virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGirderDescDlg();
	virtual INT_PTR DoModal();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsible for deleting this object
   std::unique_ptr<WBFL::EAF::Transaction> GetExtensionPageTransaction();

   void InitialzePages();

   void OnGirderTypeChanged(bool bGridBasedStrandInput);


protected:
   void Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey);
   void CreateExtensionPages();
   void DestroyExtensionPages();

   StrandIndexType GetStraightStrandCount();
   StrandIndexType GetHarpedStrandCount();
   void SetDebondTabName();
   ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type);

   void AddAdditionalPropertyPages(bool bGridBasedStrandInput);


   WBFL::EAF::MacroTxn m_Macro;
   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>> m_ExtensionPages;
   void NotifyExtensionPages();


   friend CGirderDescGeneralPage;
   friend CGirderDescLiftingPage;
   friend CGirderDescPrestressPage;
   friend CGirderDescDebondPage;
   friend CGirderDescDebondGrid;
   friend CGirderDescLongitudinalRebar;
   friend CSpanGdrDetailsBearingsPage;

	// Generated message map functions
	//{{AFX_MSG(CGirderDescDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg BOOL OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

   pgsTypes::ConditionFactorType m_ConditionFactorType;
   Float64 m_ConditionFactor;

   pgsTypes::SupportedDeckType m_DeckType;
   bool m_bCanAssumedExcessCamberInputBeEnabled;

   CButton m_CheckBox;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
