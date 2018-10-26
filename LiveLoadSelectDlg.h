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

#if !defined(AFX_LIVELOADSELECTDLG_H__97235DB6_5EFA_4B45_A738_A8EE50F13602__INCLUDED_)
#define AFX_LIVELOADSELECTDLG_H__97235DB6_5EFA_4B45_A738_A8EE50F13602__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LiveLoadSelectDlg.h : header file
//
#include "resource.h"
#include <IFace\Project.h>

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadSelectDlg dialog

class CLiveLoadSelectDlg : public CDialog
{
// Construction
public:
   CLiveLoadSelectDlg(std::vector< std::_tstring>& allNames, std::vector< std::_tstring>& dsgnNames,
                      std::vector< std::_tstring>& fatigueNames,
                      std::vector< std::_tstring>& str2Names, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLiveLoadSelectDlg)
	enum { IDD = IDD_SELECT_LIVE_LOAD };
	//}}AFX_DATA
	CCheckListBox 	m_ctlDesignLL;
	CCheckListBox 	m_ctlFatigueLL;
	CCheckListBox	m_ctlPermitLL;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLiveLoadSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLiveLoadSelectDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void SetPedestrianComboText(int iCombo, int iStatic);

public:
   std::vector< std::_tstring>& m_AllNames;
   std::vector< std::_tstring>& m_DesignNames;
   std::vector< std::_tstring>& m_FatigueNames;
   std::vector< std::_tstring>& m_PermitNames;
   double m_DesignTruckImpact;
   double m_DesignLaneImpact;
   double m_FatigueTruckImpact;
   double m_FatigueLaneImpact;
   double m_PermitTruckImpact;
   double m_PermitLaneImpact;

   ILiveLoads::PedestrianLoadApplicationType m_DesignPedesType;
   ILiveLoads::PedestrianLoadApplicationType m_FatiguePedesType;
   ILiveLoads::PedestrianLoadApplicationType m_PermitPedesType;

   bool m_bHasPedestrianLoad;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIVELOADSELECTDLG_H__97235DB6_5EFA_4B45_A738_A8EE50F13602__INCLUDED_)
