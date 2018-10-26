///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// GirderSelectStrandsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "GirderSelectStrandsDlg.h"
#include <WBFLGenericBridgeTools.h>

#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\DesignConfigUtil.h>
#include "PGSuperColors.h"
#include "PGSuperUIUtil.h"

#define BORDER 7

// Utility functions
inline void UpdateHarpedOffsetLabel(CWnd* pwnd, HarpedStrandOffsetType type, bool areHarpedStraight)
{
   CString msg;
   CString mss(areHarpedStraight ? _T("A-S") : _T("HS"));
   switch(type)
   {
      case hsoCGFROMTOP:
         msg.Format(_T("%s CG to Girder Top"), mss);
         break;
      case hsoCGFROMBOTTOM:
         msg.Format(_T("%s CG to Girder Bottom"), mss);
         break;
      case hsoTOP2TOP:
         msg.Format(_T("Top-Most %s to Girder Top"), mss);
         break;
      case hsoTOP2BOTTOM:
         msg.Format(_T("Top-Most %s to Girder Bottom"), mss);
         break;
      case hsoBOTTOM2BOTTOM:
         msg.Format(_T("Bottom-Most %s to Girder Bottom"), mss);
         break;
      case hsoECCENTRICITY:
         msg.Format(_T("Eccentricity of %s Group"), mss);
         break;
      default:
         msg = _T("Unknown Adjustment type");
         ATLASSERT(false);
   }

   pwnd->SetWindowText(msg);
}


// CGirderSelectStrandsDlg dialog

IMPLEMENT_DYNAMIC(CGirderSelectStrandsDlg, CPropertySheet)

CGirderSelectStrandsDlg::CGirderSelectStrandsDlg(CWnd* pParent,UINT iSelectPage)
	: CPropertySheet(_T("Select Strands"),pParent,iSelectPage)
{
   Init();
}

CGirderSelectStrandsDlg::~CGirderSelectStrandsDlg()
{
}

BEGIN_MESSAGE_MAP(CGirderSelectStrandsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CGirderSelectStrandsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CGirderSelectStrandsDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_SelectStrandsPage.m_psp.dwFlags  |= PSP_HASHELP;

   AddPage(&m_SelectStrandsPage);
}
