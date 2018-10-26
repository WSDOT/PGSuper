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

#if !defined(AFX_BRIDGEDESCPRESTRESSPAGE_H__8118E252_586C_11D2_8ED3_006097DF3C68__INCLUDED_)
#define AFX_BRIDGEDESCPRESTRESSPAGE_H__8118E252_586C_11D2_8ED3_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// BridgeDescPrestressPage.h : header file
//

#include "PGSuperAppPlugin\resource.h"
#include <PgsExt\GirderData.h>

#if !defined INCLUDED_MATERIAL_PSSTRAND_H_
#include <Material\PsStrand.h>
#endif

#if !defined INCLUDED_PSGLIB_GIRDERLIBRARYENTRY_H_
#include <PsgLib\GirderLibraryEntry.h>
#endif

#include <WBFLGenericBridgeTools.h>

class CGirderDescDlg;
struct IStrandGeometry;
struct IEAFDisplayUnits;

/////////////////////////////////////////////////////////////////////////////
// CGirderDescPrestressPage dialog

class CGirderDescPrestressPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGirderDescPrestressPage)

// Construction
public:
	CGirderDescPrestressPage();
	~CGirderDescPrestressPage();

// Dialog Data
	//{{AFX_DATA(CGirderDescPrestressPage)
	enum { IDD = IDD_GIRDERDESC_PRESTRESS };

	StrandIndexType m_StrandSizeIdx;
	StrandIndexType m_TempStrandSizeIdx;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGirderDescPrestressPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   Float64 GetMaxPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType); // allowable by spec
   Float64 GetUltPjack(StrandIndexType nStrands,pgsTypes::StrandType strandType); // breaking strength

private:
   StrandIndexType GetStraightStrandCount();
   StrandIndexType GetHarpedStrandCount();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGirderDescPrestressPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnNumStraightStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNumHarpedStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNumTempStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateHsPjEdit();
	afx_msg void OnUpdateSsPjEdit();
	afx_msg void OnUpdateTempPjEdit();
	afx_msg void OnHelp();
	afx_msg void OnSelchangeHpComboHp();
	afx_msg void OnSelchangeHpComboEnd();
	afx_msg void OnSelchangeStrandInputType();
	afx_msg void OnDropdownHpComboHp();
	afx_msg void OnDropdownHpComboEnd();
   afx_msg void OnStrandTypeChanged();
   afx_msg void OnTempStrandTypeChanged();
   afx_msg void OnBnClickedEditStrandFill();
	//}}AFX_MSG
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()

   void UpdatePjackEdits();
   void UpdatePjackEdit( UINT nCheckBox  );
   void UpdatePjackEditEx(StrandIndexType nStrands, UINT nCheckBox  );
   void ConvertPJackFromNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, IEAFDisplayUnits* pDisplayUnits);
   void ConvertPJackToNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, IEAFDisplayUnits* pDisplayUnits);

   void UpdateEndRangeLength(HarpedStrandOffsetType measureType, StrandIndexType Nh);
   void UpdateHpRangeLength(HarpedStrandOffsetType measureType, StrandIndexType Nh);
   void UpdateStraightHarped(StrandIndexType Ns, StrandIndexType Nh);
   void UpdateHarpedOffsets(StrandIndexType Nh);

   void InitHarpStrandOffsetMeasureComboBox(CComboBox* pCB);

   void HideControls(int key, CStrandData::PermanentStrandType numPermStrandsType);
   void HideEndOffsetControls(BOOL hide);
   void HideHpOffsetControls(BOOL hide);
   void DisappearHpOffsetControls();
   void ShowHideNumStrandControls(CStrandData::PermanentStrandType numPermStrandsType);
   void UpdateStrandControls();


   StrandIndexType StrandSpinnerInc(IStrandGeometry* pStrands, pgsTypes::StrandType type,StrandIndexType currNum, bool bAdd );
   StrandIndexType PermStrandSpinnerInc(IStrandGeometry* pStrands, StrandIndexType currNum, bool bAdd );

   HarpedStrandOffsetType m_OldHpMeasureType;
   HarpedStrandOffsetType m_OldEndMeasureType;

   bool m_AllowHpAdjustment;
   bool m_AllowEndAdjustment;

   bool m_bAreHarpedStrandsForcedStraight;
   CStrandData::PermanentStrandType m_CurrNumPermStrandsType; 

   void UpdateStrandList(UINT nIDC);

   CString m_strTip;
public:

   ConfigStrandFillVector ComputeStraightStrandFillVector(StrandIndexType Ns);
   ConfigStrandFillVector ComputeHarpedStrandFillVector();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRIDGEDESCPRESTRESSPAGE_H__8118E252_586C_11D2_8ED3_006097DF3C68__INCLUDED_)
