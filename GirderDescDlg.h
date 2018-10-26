///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "BridgeDescGirderMaterialsPage.h"

#include <PgsExt\PrecastSegmentData.h>

// handy functions

// Changes in girder fill can make debonding invalid. This algorithm gets rid of any
// debonding of strands that don't exist
inline bool ReconcileDebonding(const ConfigStrandFillVector& fillvec, std::vector<CDebondData>& rDebond)
{
   bool didErase = false;
   StrandIndexType strsize = fillvec.size();

   std::vector<CDebondData>::iterator it=rDebond.begin();
   while ( it!=rDebond.end() )
   {
      if (it->strandTypeGridIdx > strsize || fillvec[it->strandTypeGridIdx]==0)
      {
         it = rDebond.erase(it);
         didErase = true;
      }
      else
      {
         it++;
      }
   }

   return didErase;
}

inline bool ReconcileExtendedStrands(const ConfigStrandFillVector& fillvec, std::vector<GridIndexType>& extendedStrands)
{
   bool didErase = false;
   StrandIndexType strsize = fillvec.size();

   std::vector<GridIndexType>::iterator it(extendedStrands.begin());
   while ( it != extendedStrands.end() )
   {
      GridIndexType gridIdx = *it;
      if (strsize < gridIdx || fillvec[gridIdx]==0)
      {
         it = extendedStrands.erase(it);
         didErase = true;
      }
      else
      {
         it++;
      }
   }

   return didErase;
}

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDlg

class CGirderDescDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CGirderDescDlg)

// Construction
public:
	CGirderDescDlg(const CSegmentKey& segmentKey,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
   CSegmentKey m_SegmentKey;

   CGirderDescGeneralPage       m_General;
   CGirderDescPrestressPage     m_Prestress;
   CShearSteelPage2             m_Shear;
   CGirderDescLongitudinalRebar m_LongRebar;
   CGirderDescLiftingPage       m_Lifting;
   CGirderDescDebondPage        m_Debond;

   std::_tstring m_strGirderName;
   void SetSegment(const CPrecastSegmentData& segment);
   const CPrecastSegmentData& GetSegment();

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
   void DoUpdate();

   void OnGirderTypeChanged(bool bAllowExtendedStrands,bool bIsDebonding);


protected:
   void Init();
   StrandIndexType GetStraightStrandCount();
   StrandIndexType GetHarpedStrandCount();
   void SetDebondTabName();
   ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type);

   void AddAdditionalPropertyPages(bool bAllowExtendedStrands,bool bIsDebonding);


   friend CGirderDescGeneralPage;
   friend CGirderDescLiftingPage;
   friend CGirderDescPrestressPage;
   friend CGirderDescDebondPage;
   friend CGirderDescDebondGrid;
   friend CGirderDescLongitudinalRebar;

	// Generated message map functions
	//{{AFX_MSG(CGirderDescDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
   afx_msg BOOL OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CPrecastSegmentData m_Segment;
   pgsTypes::ConditionFactorType m_ConditionFactorType;
   Float64 m_ConditionFactor;

   CButton m_CheckBox;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERDESCDLG_H__4C050873_311C_11D2_8EB6_006097DF3C68__INCLUDED_)
