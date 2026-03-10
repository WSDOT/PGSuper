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

// BridgeDescRatingPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "ShearSteelPage2.h"
#include <IFace\Tools.h>
#include <IFace\Project.h>

#include "..\PsgLib\resource.h"

// CBridgeDescRatingPage dialog



BEGIN_MESSAGE_MAP(CShearSteelPage2, CShearSteelPage)
	//{{AFX_MSG_MAP(CShearSteelPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CShearSteelPage2::OnInitDialog() 
{
   m_bIsDisplayedInProject = true; // enable restoring library defaults

   return CShearSteelPage::OnInitDialog();
}

UINT CShearSteelPage2::GetHelpID()
{
   return IDH_GIRDERDETAILS_TRANSV_REBAR;
}

void CShearSteelPage2::DoRestoreDefaults() 
{
   
   auto pBroker = EAFGetBroker();

   // get shear information from library
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( m_CurGrdName.c_str());
   ASSERT(pGirderEntry != nullptr);

   // update data member
   m_ShearData.CopyGirderEntryData(pGirderEntry);
}

void CShearSteelPage2::EnableClosureJointMode()
{
   static int nID[] = 
   {
      IDC_CHECK_SPLITTING,

      IDC_SPLITTING_LABEL,
      IDC_SPLITTING_ZL,
      IDC_SPLITTING_ZL_UNIT,
      IDC_SPLITTING_BAR_SIZE,
      IDC_SPLITTING_SPACING,
      IDC_SPLITTING_SPACING_UNIT,

      IDC_CONFINE_LABEL,
      IDC_CONFINE_ZL,
      IDC_CONFINE_ZL_UNIT,
      IDC_CONFINE_BAR_SIZE,
      IDC_CONFINE_SPACING,
      IDC_CONFINE_SPACING_UNIT,

      IDC_SPLITTING_NLEGS,

      IDC_END_GROUP,
      IDC_ZL_LABEL,
      IDC_SPACING_LABEL,
      IDC_BAR_SIZE_LABEL,
      IDC_NLEGS_LABEL,

      IDC_RESTORE_DEFAULTS
   };

   int nIDs = sizeof(nID)/sizeof(int);
   for ( int i = 0; i < nIDs; i++ )
   {
      CWnd* pWnd = GetDlgItem(nID[i]);
      pWnd->ShowWindow(SW_HIDE);
   }

   // The "Confinement Bar Size" column needs to be hidded because
   // it doesn't apply to closure joints
   int nCol = m_pGrid->GetConfinementBarColumn();
	m_pGrid->GetParam()->EnableUndo(FALSE);
   m_pGrid->GetParam()->SetLockReadOnly(FALSE);
   m_pGrid->HideCols(nCol,nCol);
	m_pGrid->GetParam()->EnableUndo(TRUE);
   m_pGrid->GetParam()->SetLockReadOnly(TRUE);
}
