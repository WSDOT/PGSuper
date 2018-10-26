///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// TimeStepPropertiesPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "TimeStepPropertiesPage.h"


// CTimeStepPropertiesPage dialog

IMPLEMENT_DYNAMIC(CTimeStepPropertiesPage, CPropertyPage)

CTimeStepPropertiesPage::CTimeStepPropertiesPage()
	: CPropertyPage(CTimeStepPropertiesPage::IDD)
   , m_bIgnoreTimeDependentEffects(false)
{

}

CTimeStepPropertiesPage::~CTimeStepPropertiesPage()
{
}

void CTimeStepPropertiesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_Check_Bool(pDX,IDC_TIME_DEPENDENT_EFFECTS,m_bIgnoreTimeDependentEffects);
}


BEGIN_MESSAGE_MAP(CTimeStepPropertiesPage, CPropertyPage)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CTimeStepPropertiesPage message handlers

void CTimeStepPropertiesPage::OnHelp()
{
#pragma Reminder("UPDATE: implement Help button")
   //::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_GIRDERSPACING );
}
