///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "HaunchBearingGrid.h"
#include "HaunchByBridgeDlg.h"
#include "HaunchByBearingDlg.h"
#include "HaunchBySegmentDlg.h"
#include "AssumedExcessCamberByBridgeDlg.h"
#include "AssumedExcessCamberBySpanDlg.h"
#include "AssumedExcessCamberByGirderDlg.h"
#include <PgsExt\HaunchShapeComboBox.h>

class CBridgeDescription2;

// CEditHaunchACamberDlg dialog

class CEditHaunchACamberDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditHaunchACamberDlg)

public:
   // constructor - holds on to bridge description while dialog is active
	CEditHaunchACamberDlg(CWnd* pParent = nullptr);
	virtual ~CEditHaunchACamberDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_HAUNCH_ACAMBER};

// embedded dialogs for different haunch layouts
   CHaunchByBridgeDlg m_HaunchByBridgeDlg;
   CHaunchByBearingDlg m_HaunchByBearingDlg;
   CHaunchBySegmentDlg m_HaunchBySegmentDlg;

// embedded dialogs for different AssumedExcessCamber layouts
   CAssumedExcessCamberByBridgeDlg m_AssumedExcessCamberByBridgeDlg;
   CAssumedExcessCamberBySpanDlg  m_AssumedExcessCamberBySpanDlg;
   CAssumedExcessCamberByGirderDlg m_AssumedExcessCamberByGirderDlg;

   // returns the current selection for slab offset type
   pgsTypes::SlabOffsetType GetSlabOffsetType();

   // returns the current selection for assumed excess camber type
   pgsTypes::AssumedExcessCamberType GetAssumedExcessCamberType();

   bool m_bCanAssumedExcessCamberInputBeEnabled;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnSlabOffsetTypeChanged();
   afx_msg void OnAssumedExcessCamberTypeChanged();
   afx_msg void OnBnClickedHelp();

   CBridgeDescription2* GetBridgeDesc();


   pgsTypes::SlabOffsetType m_SlabOffsetType;
   pgsTypes::AssumedExcessCamberType m_AssumedExcessCamberType;

   void InitializeData();
};