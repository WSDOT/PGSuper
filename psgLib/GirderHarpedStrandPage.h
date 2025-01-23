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

#if !defined(AFX_GIRDERHARPEDSTRANDPAGE_H__CE0B8E33_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
#define AFX_GIRDERHARPEDSTRANDPAGE_H__CE0B8E33_312C_11D2_9D3E_00609710E6CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GirderStrandPage.h : header file

#include "GirderGlobalStrandGrid.h"
#include <Units\Measure.h>

// Names are slightly different in library than rest of program
inline LPCTSTR LOCAL_LABEL_HARP_TYPE(pgsTypes::AdjustableStrandType type)
{
   if (pgsTypes::asHarped == type)
   {
      return _T("Harped");
   }
   else  if (pgsTypes::asStraight == type)
   {
      return _T("Adj. Straight");
   }
   else if (pgsTypes::asStraightOrHarped)
   {
      return _T("Adjustable");
   }
   else
   {
      ATLASSERT(false);
      return _T("Error");
   }
}

class CGirderMainSheet;

/////////////////////////////////////////////////////////////////////////////
// CGirderPermanentStrandPage dialog

class CGirderPermanentStrandPage : public CPropertyPage, public CGirderGlobalStrandGridClient
{
   friend CGirderMainSheet;

	DECLARE_DYNCREATE(CGirderPermanentStrandPage)

// Construction
public:
	CGirderPermanentStrandPage();
	~CGirderPermanentStrandPage();

// Dialog Data
	//{{AFX_DATA(CGirderPermanentStrandPage)
	enum { IDD = IDD_GIRDER_PERMANENT_STRANDS };
	//}}AFX_DATA

private:
   CString    m_UnitString;
   CGirderGlobalStrandGrid m_MainGrid;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderPermanentStrandPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderPermanentStrandPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDelGlobalStrand();
	afx_msg void OnAddGlobalStrand();
	afx_msg void OnAppendGlobalStrand();
   afx_msg void OnEditGlobalStrand();
	afx_msg void OnClickHarpedBox();
	afx_msg void OnClickHpAdjust();
	afx_msg void OnClickEndAdjust();
   afx_msg void OnClickStraightAdjust();
   afx_msg void OnMoveUpGlobalStrand();
   afx_msg void OnMoveDownGlobalStrand();
   afx_msg void OnHelp();
	afx_msg void OnEndview();
	afx_msg void OnMidview();
   afx_msg void OnReverseHarpedStrandOrder();
   afx_msg void OnGenerateStrandPositions();
	//}}AFX_MSG
	afx_msg BOOL OnNcActivate(BOOL bActive);
	DECLARE_MESSAGE_MAP()

public:
   // capture event fired from grid that allows deletion of rows
   virtual void OnEnableDelete(bool canDelete);
   virtual bool DoUseHarpedGrid();
   virtual pgsTypes::AdjustableStrandType GetAdjustableStrandType();
   virtual void UpdateStrandStatus(Uint16 ns, Uint16 ndb, Uint16 nh); 

	void UpdateHpAdjust();
	void UpdateEndAdjust();
	void UpdateStraightAdjust();
   void UpdateAdjustCtls(BOOL enableGroup, int checkCtrl, int otherCtrls[]);

   afx_msg void OnCbnSelchangeWebStrandTypeCombo();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GIRDERHARPEDSTRANDPAGE_H__CE0B8E33_312C_11D2_9D3E_00609710E6CE__INCLUDED_)
