///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "HaunchSpanGrid.h"
#include "HaunchSame4BridgeDlg.h"
#include "HaunchSpanBySpanDlg.h"
#include "HaunchByGirderDlg.h"
#include "FilletSame4BridgeDlg.h"
#include "FilletSpanBySpanDlg.h"
#include "FilletByGirderDlg.h"
#include <PgsExt\HaunchShapeComboBox.h>

class CBridgeDescription2;

inline CString SlabOffsetTypeAsString(pgsTypes::SlabOffsetType type)
{
   if (type == pgsTypes::sotBridge)
   {
      return _T("Define single slab offset for entire Bridge");
   }
   else if (type == pgsTypes::sotPier)
   {
      return _T("Define unique slab offsets per Pier");
   }
   else if (type == pgsTypes::sotGirder)
   {
      return _T("Define unique slab offsets per Girder");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad haunch type");
   }
}

inline CString FilletTypeAsString(pgsTypes::FilletType type)
{
   if (type == pgsTypes::fttBridge)
   {
      return _T("Define single Fillet for entire Bridge");
   }
   else if (type == pgsTypes::fttSpan)
   {
      return _T("Define unique Fillets per Span");
   }
   else if (type == pgsTypes::fttGirder)
   {
      return _T("Define unique Fillets per Girder");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad fillet type");
   }
}

// CEditHaunchDlg dialog

class CEditHaunchDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditHaunchDlg)

public:
   // constructor - holds on to bridge description while dialog is active
	CEditHaunchDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent = NULL);
	virtual ~CEditHaunchDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_HAUNCH };

// embedded dialogs for different haunch layouts
   CHaunchSame4BridgeDlg m_HaunchSame4BridgeDlg;
   CHaunchSpanBySpanDlg  m_HaunchSpanBySpanDlg;
   CHaunchByGirderDlg    m_HaunchByGirderDlg;

// embedded dialogs for different Fillet layouts
   CFilletSame4BridgeDlg m_FilletSame4BridgeDlg;
   CFilletSpanBySpanDlg  m_FilletSpanBySpanDlg;
   CFilletByGirderDlg    m_FilletByGirderDlg;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangeAType();
   afx_msg void OnCbnSelchangeFilletType();

   // Force to another type than that in bridge descripton. Must be called before DoModal
   void ForceToSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType);
   void ForceToFilletType(pgsTypes::FilletType filletType);

   // Change bridge description to our haunch data
   void ModifyBridgeDescr(CBridgeDescription2* pBridgeDesc);

private:
   const CBridgeDescription2* m_pBridgeDesc;

   // Data for haunch input
   HaunchInputData m_HaunchInputData;

   pgsTypes::HaunchShapeType m_HaunchShape;
   CHaunchShapeComboBox m_cbHaunchShape;

   bool m_WasSlabOffsetTypeForced;
   pgsTypes::SlabOffsetType m_ForcedSlabOffsetType;
   bool m_WasFilletTypeForced;
   pgsTypes::FilletType m_ForcedFilletType;
   bool m_WasDataIntialized;

   void InitializeData();
public:
   afx_msg void OnBnClickedHelp();
};