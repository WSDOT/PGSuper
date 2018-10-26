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

// BridgeDescRatingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ShearSteelPage2.h"
#include "HtmlHelp\HelpTopics.hh"
#include <IFace\Tools.h>
#include <IFace\Project.h>

// CBridgeDescRatingPage dialog

BEGIN_MESSAGE_MAP(CShearSteelPage2, CShearSteelPage)
	//{{AFX_MSG_MAP(CShearSteelPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CShearSteelPage2::OnInitDialog() 
{
   m_AllowRestoreDefaults = true; // enable restoring library defaults

   return CShearSteelPage::OnInitDialog();
}

void CShearSteelPage2::OnHelp() 
{
   UINT helpID = IDH_GIRDERWIZ_SHEARDESC;

   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, helpID );
}

void CShearSteelPage2::DoRestoreDefaults() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // get shear information from library
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGird = pLib->GetGirderEntry( m_CurGrdName.c_str());
   ASSERT(pGird!=0);

   // update data member
   m_ShearData.CopyGirderEntryData(*pGird);
}

