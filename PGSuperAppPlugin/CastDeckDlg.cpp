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
// CastDeckDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "CastDeckDlg.h"

#include <EAF\EAFDisplayUnits.h>



// CCastDeckDlg dialog

IMPLEMENT_DYNAMIC(CCastDeckDlg, CDialog)

CCastDeckDlg::CCastDeckDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CCastDeckDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
}

CCastDeckDlg::~CCastDeckDlg()
{
}

void CCastDeckDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      // Data coming out of the dialog
      m_TimelineMgr.SetCastDeckEventByIndex(m_EventIndex,true);

      Float64 age;
      DDX_Text(pDX,IDC_AGE,age);
      DDV_GreaterThanZero(pDX,IDC_AGE,age);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity().SetConcreteAgeAtContinuity(age);
   }
   else
   {
      // Data going into the dialog
      Float64 age = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastDeckActivity().GetConcreteAgeAtContinuity();
      DDX_Text(pDX,IDC_AGE,age);
   }
}


BEGIN_MESSAGE_MAP(CCastDeckDlg, CDialog)
END_MESSAGE_MAP()


// CCastDeckDlg message handlers
