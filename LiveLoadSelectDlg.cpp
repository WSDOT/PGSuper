///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// LiveLoadSelectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "LiveLoadSelectDlg.h"
#include <..\htmlhelp\helptopics.hh>
#include <MfcTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadSelectDlg dialog


CLiveLoadSelectDlg::CLiveLoadSelectDlg(std::vector< std::string>& allNames, std::vector< std::string>& dsgnNames,
                                       std::vector< std::string>& fatigueNames,std::vector< std::string>& str2Names,CWnd* pParent /*=NULL*/)
	: CDialog(CLiveLoadSelectDlg::IDD, pParent), 
     m_AllNames(allNames),
     m_DesignNames(dsgnNames),
     m_FatigueNames(fatigueNames),
     m_PermitNames(str2Names)
{
	//{{AFX_DATA_INIT(CLiveLoadSelectDlg)
	//}}AFX_DATA_INIT
   m_DesignTruckImpact = 0;
   m_DesignLaneImpact = 0;
   m_FatigueTruckImpact = 0;
   m_FatigueLaneImpact = 0;
   m_PermitTruckImpact = 0;
   m_PermitLaneImpact = 0;
}


void CLiveLoadSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLiveLoadSelectDlg)
	DDX_Control(pDX, IDC_DESIGN_LL_LIST, m_ctlDesignLL);
	DDX_Control(pDX, IDC_FATIGUE_LL_LIST, m_ctlFatigueLL);
	DDX_Control(pDX, IDC_STRENGTH_LL_LIST, m_ctlPermitLL);
	//}}AFX_DATA_MAP

   DDX_Percentage(pDX,IDC_DESIGN_TRUCK_IMPACT,  m_DesignTruckImpact);
   DDX_Percentage(pDX,IDC_DESIGN_LANE_IMPACT,   m_DesignLaneImpact);
   DDX_Percentage(pDX,IDC_FATIGUE_TRUCK_IMPACT, m_FatigueTruckImpact);
   DDX_Percentage(pDX,IDC_FATIGUE_LANE_IMPACT,  m_FatigueLaneImpact);
   DDX_Percentage(pDX,IDC_PERMIT_TRUCK_IMPACT,  m_PermitTruckImpact);
   DDX_Percentage(pDX,IDC_PERMIT_LANE_IMPACT,   m_PermitLaneImpact);

   if (pDX->m_bSaveAndValidate)
   {
      m_DesignNames.clear(); // reuse vector 
      int cnt = m_ctlDesignLL.GetCount();
      int icnt;
      for (icnt=0; icnt<cnt; icnt++)
      {
         int chk = m_ctlDesignLL.GetCheck(icnt);
         if (chk==1)
         {
            CString str;
            m_ctlDesignLL.GetText(icnt, str);
            m_DesignNames.push_back( std::string(str));
         }
      }

      // selected fatigue names
      m_FatigueNames.clear(); // reuse vector 
      cnt = m_ctlFatigueLL.GetCount();
      for (icnt=0; icnt<cnt; icnt++)
      {
         int chk = m_ctlFatigueLL.GetCheck(icnt);
         if (chk==1)
         {
            CString str;
            m_ctlFatigueLL.GetText(icnt, str);
            m_FatigueNames.push_back( std::string(str));
         }
      }

      // selected permit names
      m_PermitNames.clear(); // reuse vector 
      cnt = m_ctlPermitLL.GetCount();
      for (icnt=0; icnt<cnt; icnt++)
      {
         int chk = m_ctlPermitLL.GetCheck(icnt);
         if (chk==1)
         {
            CString str;
            m_ctlPermitLL.GetText(icnt, str);
            m_PermitNames.push_back( std::string(str));
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CLiveLoadSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CLiveLoadSelectDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadSelectDlg message handlers

void CLiveLoadSelectDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SELECT_LIVELOAD);
}

BOOL CLiveLoadSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   m_ctlDesignLL.SetCheckStyle( BS_AUTOCHECKBOX );
   m_ctlFatigueLL.SetCheckStyle( BS_AUTOCHECKBOX );
   m_ctlPermitLL.SetCheckStyle( BS_AUTOCHECKBOX );

   for (std::vector< std::string>::iterator it=m_AllNames.begin(); it!=m_AllNames.end(); it++)
   {
      const char* str = it->c_str();
      m_ctlDesignLL.AddString(str);
      m_ctlFatigueLL.AddString(str);
      m_ctlPermitLL.AddString(str);
   }


   // Set the check marks for the various loads
   // design
   std::vector< std::string>::iterator iter;
   for ( iter = m_DesignNames.begin(); iter != m_DesignNames.end(); iter++)
   {
      const char* str = iter->c_str();
      int idx = m_ctlDesignLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlDesignLL.SetCheck( idx, 1 );
      }
   }

   // fatigue
   for (iter = m_FatigueNames.begin(); iter != m_FatigueNames.end(); iter++)
   {
      const char* str = iter->c_str();
      int idx = m_ctlFatigueLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlFatigueLL.SetCheck( idx, 1 );
      }
   }

   // permit
   for (iter = m_PermitNames.begin(); iter != m_PermitNames.end(); iter++)
   {
      const char* str = iter->c_str();
      int idx = m_ctlPermitLL.FindString(-1, str);
      if (idx != LB_ERR)
      {
         m_ctlPermitLL.SetCheck( idx, 1 );
      }
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
