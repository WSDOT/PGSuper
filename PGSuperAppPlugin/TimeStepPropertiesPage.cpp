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

// TimeStepPropertiesPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "TimeStepPropertiesPage.h"
#include <EAF\EAFDocument.h>


// CTimeStepPropertiesPage dialog

IMPLEMENT_DYNAMIC(CTimeStepPropertiesPage, CPropertyPage)

CTimeStepPropertiesPage::CTimeStepPropertiesPage()
	: CPropertyPage(CTimeStepPropertiesPage::IDD)
   , m_bIgnoreCreepEffects(false)
   , m_bIgnoreShrinkageEffects(false)
   , m_bIgnoreRelaxationEffects(false)
{

}

CTimeStepPropertiesPage::~CTimeStepPropertiesPage()
{
}

void CTimeStepPropertiesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_Check_Bool(pDX,IDC_IGNORE_CREEP,m_bIgnoreCreepEffects);
   DDX_Check_Bool(pDX,IDC_IGNORE_SHRINKAGE,m_bIgnoreShrinkageEffects);
   DDX_Check_Bool(pDX,IDC_IGNORE_RELAXATION,m_bIgnoreRelaxationEffects);
}


BEGIN_MESSAGE_MAP(CTimeStepPropertiesPage, CPropertyPage)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CTimeStepPropertiesPage message handlers

void CTimeStepPropertiesPage::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_TIMESTEP_PROPERTIES);
}
