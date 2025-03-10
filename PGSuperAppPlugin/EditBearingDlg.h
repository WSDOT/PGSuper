///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

//#include "BearingSpanGrid.h"
#include "BearingSame4BridgeDlg.h"
#include "BearingPierByPierDlg.h"
#include "BearingGdrByGdrDlg.h"
#include "PgsExt\BearingData2.h"

class CBridgeDescription2;

inline CString BearingTypeAsString(pgsTypes::BearingType type)
{
   if (type == pgsTypes::brtBridge)
   {
      return _T("Bearing data is the same for the entire Bridge");
   }
   else if (type == pgsTypes::brtPier)
   {
      return _T("Bearing Data is unique along every Bearing Line");
   }
   else if (type == pgsTypes::brtGirder)
   {
      return _T("Bearing Data is unique at the end of every Girder");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad Bearing type");
   }
}

// simple, exception-safe class for blocking events
class SimpleMutex
{
public:
   SimpleMutex(bool& flag):
   m_Flag(flag)
   {
      m_Flag = true;
   }

   ~SimpleMutex()
   {
      m_Flag = false;
   }
private:
   bool& m_Flag;
};

// CEditBearingDlg dialog

class CEditBearingDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditBearingDlg)

public:
   // constructor - holds on to bridge description while dialog is active
	CEditBearingDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent = nullptr);
	CEditBearingDlg(const BearingInputData* pBearingData, CWnd* pParent = nullptr);
	virtual ~CEditBearingDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_BEARINGS };

// embedded dialogs for different Bearing layouts
   CBearingSame4BridgeDlg m_BearingSame4BridgeDlg;
   CBearingPierByPierDlg  m_BearingPierByPierDlg;
   CBearingGdrByGdrDlg    m_BearingGdrByGdrDlg;
   CMetaFileStatic m_Bearing;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();

   // Change bridge description to our Bearing data
   void ModifyBridgeDescr(CBridgeDescription2* pBridgeDesc);

   // Data for Bearing input
   BearingInputData m_BearingInputData;

private:
   void InitializeData(const CBridgeDescription2* pBridgeDescr);
public:
   afx_msg void OnHelp();
   afx_msg void OnCbnSelchangeBrType();
};