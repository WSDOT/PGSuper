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

#include "resource.h"
#include <PsgLib\GirderData.h>

#include <Materials/PsStrand.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <WBFLGenericBridgeTools.h>

#include <MfcTools\WideDropDownComboBox.h>

class IStrandGeometry;
class IEAFDisplayUnits;

class CGirderDescDlg;

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

   std::array<Int64, 3> m_StrandKey{ -1,-1,-1 };
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
   CWideDropDownComboBox m_cbStraight;
   CWideDropDownComboBox m_cbHarped;
   CWideDropDownComboBox m_cbTemporary;

	// Generated message map functions
	//{{AFX_MSG(CGirderDescPrestressPage)
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	afx_msg void OnNumStraightStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNumHarpedStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNumTempStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateHsPjEdit();
	afx_msg void OnUpdateSsPjEdit();
	afx_msg void OnUpdateTempPjEdit();
	afx_msg void OnHelp();
	afx_msg void OnSelchangeHpComboHp();
	afx_msg void OnSelchangeHpComboEnd();
	afx_msg void OnStrandInputTypeChanged();
	afx_msg void OnDropdownHpComboHp();
	afx_msg void OnDropdownHpComboEnd();
   afx_msg void OnStraightStrandTypeChanged();
   afx_msg void OnHarpedStrandTypeChanged();
   afx_msg void OnTempStrandTypeChanged();
   afx_msg void OnBnClickedEditStrandFill();
	//}}AFX_MSG
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()

   void UpdateStrandTypes();

   void OnStrandTypeChanged(int nIDC, pgsTypes::StrandType strandType);

   void UpdatePjackEdits();
   void UpdatePjackEdit( UINT nCheckBox  );
   void UpdatePjackEditEx(StrandIndexType nStrands, UINT nCheckBox  );
   void ConvertPJackFromNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
   void ConvertPJackToNumPerm(StrandIndexType numStraight, StrandIndexType numHarped, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

   void UpdateEndRangeLength(HarpedStrandOffsetType measureType, StrandIndexType Nh);
   void UpdateHpRangeLength(HarpedStrandOffsetType measureType, StrandIndexType Nh);
   void UpdateStraightHarped(StrandIndexType Ns, StrandIndexType Nh);
   void UpdateHarpedOffsets(StrandIndexType Nh);

   void InitHarpStrandOffsetMeasureComboBox(CComboBox* pCB);

   void HideControls(int key, pgsTypes::StrandDefinitionType numPermStrandsType);
   void DisableEndOffsetControls(BOOL disable);
   void ShowEndOffsetControls(BOOL show);
   void DisableHpOffsetControls(BOOL disable);
   void ShowHpOffsetControls(BOOL show);
   void ShowHideNumStrandControls(pgsTypes::StrandDefinitionType numPermStrandsType);
   void UpdateStrandControls();
   void UpdateAdjustableStrandControls();
   void ShowOffsetControlGroup(BOOL show);

   StrandIndexType StrandSpinnerInc(std::shared_ptr<IStrandGeometry> pStrands, pgsTypes::StrandType type,StrandIndexType currNum, bool bAdd );
   StrandIndexType PermStrandSpinnerInc(std::shared_ptr<IStrandGeometry> pStrands, StrandIndexType currNum, bool bAdd );

   void GetStrandCount(StrandIndexType* pNs, StrandIndexType* pNh, StrandIndexType* pNt, StrandIndexType* pNp);

   Float64 m_HgStart;
   Float64 m_HgHp1;
   Float64 m_HgHp2;
   Float64 m_HgEnd;

   HarpedStrandOffsetType m_OldHpMeasureType;
   HarpedStrandOffsetType m_OldEndMeasureType;

   bool m_AllowHpAdjustment;
   bool m_AllowEndAdjustment;

   pgsTypes::AdjustableStrandType m_LibraryAdjustableStrandType; // in girder library
   pgsTypes::StrandDefinitionType m_CurrStrandDefinitionType; 

   void UpdateStrandList(UINT nIDC);

   void EditDirectSelect();
   void EditDirectRowInput();
   void EditDirectStrandInput();

   CString m_strTip;
public:

   ConfigStrandFillVector ComputeStraightStrandFillVector(StrandIndexType Ns);
   ConfigStrandFillVector ComputeHarpedStrandFillVector();
   afx_msg void OnCbnSelchangeAdjustableCombo();
   afx_msg void OnEpoxyChanged();
};
