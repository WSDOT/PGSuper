///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LoadFactorsDlg.h"
#include <LRFD\VersionMgr.h>

#include <IFace\Project.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CLoadFactorsDlg dialog

IMPLEMENT_DYNAMIC(CLoadFactorsDlg, CDialog)

CLoadFactorsDlg::CLoadFactorsDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CLoadFactorsDlg::IDD, pParent)
{

}

CLoadFactorsDlg::~CLoadFactorsDlg()
{
}

void CLoadFactorsDlg::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDialog::DoDataExchange(pDX);

   DDX_Text(pDX,IDC_SERVICE_I_DC,DCmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_DC,DCmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_DW,DWmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_DW,DWmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_LLIM,LLIMmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_LLIM,LLIMmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_CR,CRmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_CR,CRmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_SH,SHmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_SH,SHmax[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_PS,PSmin[pgsTypes::ServiceI]);
   DDX_Text(pDX,IDC_SERVICE_I_PS,PSmax[pgsTypes::ServiceI]);

   DDX_Text(pDX,IDC_SERVICE_III_DC,DCmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_DC,DCmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_DW,DWmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_DW,DWmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_LLIM,LLIMmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_LLIM,LLIMmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_CR,CRmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_CR,CRmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_SH,SHmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_SH,SHmax[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_PS,PSmin[pgsTypes::ServiceIII]);
   DDX_Text(pDX,IDC_SERVICE_III_PS,PSmax[pgsTypes::ServiceIII]);

   DDX_Text(pDX,IDC_STRENGTH_I_DC_MIN,DCmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_DC_MAX,DCmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_DW_MIN,DWmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_DW_MAX,DWmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_LLIM,LLIMmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_LLIM,LLIMmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_CR_MIN,CRmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_CR_MAX,CRmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_SH_MIN,SHmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_SH_MAX,SHmax[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_PS,PSmin[pgsTypes::StrengthI]);
   DDX_Text(pDX,IDC_STRENGTH_I_PS,PSmax[pgsTypes::StrengthI]);

   DDX_Text(pDX,IDC_STRENGTH_II_DC_MIN,DCmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_DC_MAX,DCmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_DW_MIN,DWmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_DW_MAX,DWmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_LLIM,LLIMmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_LLIM,LLIMmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_CR_MIN,CRmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_CR_MAX,CRmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_SH_MIN,SHmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_SH_MAX,SHmax[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_PS,PSmin[pgsTypes::StrengthII]);
   DDX_Text(pDX,IDC_STRENGTH_II_PS,PSmax[pgsTypes::StrengthII]);

   DDX_Text(pDX,IDC_FATIGUE_I_DC,DCmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_DC,DCmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_DW,DWmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_DW,DWmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_LLIM,LLIMmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_LLIM,LLIMmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_CR,CRmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_CR,CRmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_SH,SHmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_SH,SHmax[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_PS,PSmin[pgsTypes::FatigueI]);
   DDX_Text(pDX,IDC_FATIGUE_I_PS,PSmax[pgsTypes::FatigueI]);

   DDX_Text(pDX,IDC_SERVICE_IA_DC,DCmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_DC,DCmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_DW,DWmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_DW,DWmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_LLIM,LLIMmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_LLIM,LLIMmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_CR,CRmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_CR,CRmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_SH,SHmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_SH,SHmax[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_PS,PSmin[pgsTypes::ServiceIA]);
   DDX_Text(pDX,IDC_SERVICE_IA_PS,PSmax[pgsTypes::ServiceIA]);

   if ( pDX->m_bSaveAndValidate )
   {
      // since LRFD doesn't have a load case for relaxation, and relaxation is most closely related to creep
      // copy the creep factor
      int n = sizeof(CRmax)/sizeof(CRmax[0]);
      for ( int i = 0; i < n; i++ )
      {
         REmax[i] = CRmax[i];
         REmin[i] = CRmin[i];
      }

      for (int i = 0; i < nLimitStates; i++)
      {
         pgsTypes::LimitState limitState = (pgsTypes::LimitState)i;
         m_LoadFactors.SetDC(limitState, DCmin[limitState], DCmax[limitState]);
         m_LoadFactors.SetDW(limitState, DWmin[limitState], DWmax[limitState]);
         m_LoadFactors.SetCR(limitState, CRmin[limitState], CRmax[limitState]);
         m_LoadFactors.SetSH(limitState, SHmin[limitState], SHmax[limitState]);
         m_LoadFactors.SetRE(limitState, REmin[limitState], REmax[limitState]);
         m_LoadFactors.SetPS(limitState, PSmin[limitState], PSmax[limitState]);
         m_LoadFactors.SetLLIM(limitState, LLIMmin[limitState], LLIMmax[limitState]);
      }
   }
}


BEGIN_MESSAGE_MAP(CLoadFactorsDlg, CDialog)
   ON_BN_CLICKED(ID_HELP, &CLoadFactorsDlg::OnHelp)
END_MESSAGE_MAP()


// CLoadFactorsDlg message handlers

BOOL CLoadFactorsDlg::OnInitDialog()
{
   for (int i = 0; i < nLimitStates; i++)
   {
      pgsTypes::LimitState limitState = (pgsTypes::LimitState)i;
      m_LoadFactors.GetDC(limitState, &DCmin[limitState], &DCmax[limitState]);
      m_LoadFactors.GetDW(limitState, &DWmin[limitState], &DWmax[limitState]);
      m_LoadFactors.GetCR(limitState, &CRmin[limitState], &CRmax[limitState]);
      m_LoadFactors.GetSH(limitState, &SHmin[limitState], &SHmax[limitState]);
      m_LoadFactors.GetRE(limitState, &REmin[limitState], &REmax[limitState]);
      m_LoadFactors.GetPS(limitState, &PSmin[limitState], &PSmax[limitState]);
      m_LoadFactors.GetLLIM(limitState, &LLIMmin[limitState], &LLIMmax[limitState]);
   }

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here
   int swServiceIA, swFatigueI;
   if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
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
   GetDlgItem(IDC_SERVICE_IA_PLUS)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_CR)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_CR_LABEL)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_SH)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_SH_LABEL)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_PS)->ShowWindow(swServiceIA);
   GetDlgItem(IDC_SERVICE_IA_PS_LABEL)->ShowWindow(swServiceIA);

   GetDlgItem(IDC_FATIGUE_I_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DC)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DC_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DW)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_DW_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_LLIM)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_LLIM_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_PLUS)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_CR)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_CR_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_SH)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_SH_LABEL)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_PS)->ShowWindow(swFatigueI);
   GetDlgItem(IDC_FATIGUE_I_PS_LABEL)->ShowWindow(swFatigueI);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_SERVICE_I_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_I_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_SERVICE_III_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_STRENGTH_I_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PLUS2)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR_MIN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR_MIN_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR_MAX)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR_MAX_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH_MIN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH_MIN_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH_MAX)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH_MAX_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_STRENGTH_II_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_PLUS2)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_CR_MIN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_CR_MIN_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_CR_MAX)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_CR_MAX_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_SH_MIN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_SH_MIN_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_SH_MAX)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_SH_MAX_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_II_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_FATIGUE_I_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FATIGUE_I_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FATIGUE_I_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FATIGUE_I_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FATIGUE_I_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FATIGUE_I_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FATIGUE_I_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_SERVICE_IA_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_IA_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_IA_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_IA_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_IA_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_IA_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_IA_PS_LABEL)->ShowWindow(SW_HIDE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoadFactorsDlg::OnHelp()
{
   // TODO: Add your control notification handler code here
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_LOAD_FACTORS);
}
