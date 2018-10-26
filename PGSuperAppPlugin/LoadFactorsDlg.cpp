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

// LoadFactorsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LoadFactorsDlg.h"
#include <Lrfd\VersionMgr.h>

// CLoadFactorsDlg dialog

IMPLEMENT_DYNAMIC(CLoadFactorsDlg, CDialog)

CLoadFactorsDlg::CLoadFactorsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoadFactorsDlg::IDD, pParent)
{

}

CLoadFactorsDlg::~CLoadFactorsDlg()
{
}

void CLoadFactorsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   DDX_Text(pDX,IDC_SERVICE_I_DC,m_LoadFactors.DCmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_DC,m_LoadFactors.DCmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_DW,m_LoadFactors.DWmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_DW,m_LoadFactors.DWmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_LLIM,m_LoadFactors.LLIMmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_LLIM,m_LoadFactors.LLIMmax[pgsTypes::ServiceI]);

   DDX_Text(pDX,IDC_SERVICE_IA_DC,m_LoadFactors.DCmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_DC,m_LoadFactors.DCmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_DW,m_LoadFactors.DWmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_DW,m_LoadFactors.DWmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_LLIM,m_LoadFactors.LLIMmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_LLIM,m_LoadFactors.LLIMmax[pgsTypes::ServiceIA]);

   DDX_Text(pDX,IDC_SERVICE_III_DC,m_LoadFactors.DCmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_DC,m_LoadFactors.DCmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_DW,m_LoadFactors.DWmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_DW,m_LoadFactors.DWmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_LLIM,m_LoadFactors.LLIMmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_LLIM,m_LoadFactors.LLIMmax[pgsTypes::ServiceIII]);

   DDX_Text(pDX,IDC_STRENGTH_I_DC_MIN,m_LoadFactors.DCmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_DC_MAX,m_LoadFactors.DCmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_DW_MIN,m_LoadFactors.DWmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_DW_MAX,m_LoadFactors.DWmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_LLIM,m_LoadFactors.LLIMmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_LLIM,m_LoadFactors.LLIMmax[pgsTypes::StrengthI]);

   DDX_Text(pDX,IDC_STRENGTH_II_DC_MIN,m_LoadFactors.DCmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_DC_MAX,m_LoadFactors.DCmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_DW_MIN,m_LoadFactors.DWmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_DW_MAX,m_LoadFactors.DWmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_LLIM,m_LoadFactors.LLIMmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_LLIM,m_LoadFactors.LLIMmax[pgsTypes::StrengthII]);

   DDX_Text(pDX,IDC_FATIGUE_I_DC,m_LoadFactors.DCmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_DC,m_LoadFactors.DCmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_DW,m_LoadFactors.DWmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_DW,m_LoadFactors.DWmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_LLIM,m_LoadFactors.LLIMmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_LLIM,m_LoadFactors.LLIMmax[pgsTypes::FatigueI]);
}


BEGIN_MESSAGE_MAP(CLoadFactorsDlg, CDialog)
END_MESSAGE_MAP()


// CLoadFactorsDlg message handlers

BOOL CLoadFactorsDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here
   int swServiceIA, swFatigueI;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      // Fatigue I introduced 4th edition 2009
      // Hide Service IA, Show Fatigue I
      swServiceIA = SW_HIDE;
      swFatigueI  = SW_SHOW;
   }
   else
   {
      // Show Service IA, Hide Fatigue I
      swServiceIA = SW_SHOW;
      swFatigueI  = SW_HIDE;
   }


   GetDlgItem(IDC_SERVICE_IA_LABEL)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_DC)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_DC_LABEL)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_DW)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_DW_LABEL)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_LLIM)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_LLIM_LABEL)->ShowWindow(swServiceIA);

   GetDlgItem(IDC_FATIGUE_I_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DC)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DC_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DW)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DW_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_LLIM)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_LLIM_LABEL)->ShowWindow(swFatigueI);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
