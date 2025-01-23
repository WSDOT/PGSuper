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

#if !defined(AFX_DESIGNGIRDERDLG_H__051156F4_8FA6_11D2_889E_006097C68A9C__INCLUDED_)
#define AFX_DESIGNGIRDERDLG_H__051156F4_8FA6_11D2_889E_006097C68A9C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DesignGirderDlg.h : header file
//

interface IBroker;
#include "resource.h"
#include "MultiGirderSelectDlg.h"
#include <PGSuperTypes.h>
#include <IFace\Artifact.h>
/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg dialog

class CDesignGirderDlg : public CDialog
{
// Construction
public:
	CDesignGirderDlg(const CGirderKey& girderKey, IBroker* pBroker, arSlabOffsetDesignType haunchDesignRequest, CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDesignGirderDlg)
	enum { IDD = IDD_DESIGN_GIRDER };
   CGirderKey m_GirderKey;
   //}}AFX_DATA

   static void LoadSettings(arSlabOffsetDesignType haunchDesignRequest, bool& bDesignFlexure, arSlabOffsetDesignType& haunchDesignType, arConcreteDesignType& concreteDesignType, arShearDesignType& shearDesignType);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDesignGirderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// return design options
public:
   std::vector<CGirderKey> m_GirderKeys;

// Implementation
private:
   IBroker* m_pBroker;

   // Request made by calling function for haunch design. This may be altered if design cannot be done (e.g., no deck bridge)
   arSlabOffsetDesignType m_HaunchDesignRequest;
   BOOL m_bCanEnableHaunchDesign; // if true the haunch design option can be enabled

   void UpdateGirderComboBox(SpanIndexType spanIdx);
   void UpdateDesignHaunchCtrl();
   void UpdateConcreteDesignCtrl();

   void SaveSettings();

   BOOL DesignForFlexure();
   BOOL DesignHaunch();
   BOOL PreserveConcreteStrength();
   BOOL DesignForShear();
   BOOL DesignWithCurrentStirrups();

protected:
	// Generated message map functions
	//{{AFX_MSG(CDesignGirderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnSpanChanged();
	afx_msg void OnDesignFlexure();
   afx_msg void OnDestroy();
   afx_msg void OnBnClickedSelectGirders();
   afx_msg void OnBnClickedRadio();
   afx_msg void OnBnClickedDesignShear();
   //}}AFX_MSG
   afx_msg BOOL OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

   CString m_strToolTip;
   int m_DesignRadioNum;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DESIGNGIRDERDLG_H__051156F4_8FA6_11D2_889E_006097C68A9C__INCLUDED_)
