///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
// ClosureJointStirrupsPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperPluginApp.h"
#include "ClosureJointStirrupsPage.h"
#include "ClosureJointDlg.h"

#include <EAF\EAFDisplayUnits.h>

#include <LRFD\RebarPool.h>


//////////////////////////////////////////////////////////////////////////////////////
// NOTE
// The implementation of this class is very similar to CGirderDescShearPage
// Changes made to this class are probably needed there as well.
//////////////////////////////////////////////////////////////////////////////////////

// CClosureJointStirrupsPage dialog

IMPLEMENT_DYNAMIC(CClosureJointStirrupsPage, CShearSteelPage2)

CClosureJointStirrupsPage::CClosureJointStirrupsPage()
{

}

CClosureJointStirrupsPage::~CClosureJointStirrupsPage()
{
}


BEGIN_MESSAGE_MAP(CClosureJointStirrupsPage, CShearSteelPage2)
END_MESSAGE_MAP()


// CClosureJointStirrupsPage message handlers

BOOL CClosureJointStirrupsPage::OnInitDialog()
{
   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   m_ShearData = pParent->m_ClosureJoint.GetStirrups();

   CShearSteelPage2::OnInitDialog();
   
   EnableClosureJointMode();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosureJointStirrupsPage::DoDataExchange(CDataExchange* pDX)
{
   CShearSteelPage2::DoDataExchange(pDX);
   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_ClosureJoint.SetStirrups(m_ShearData);
   }
}

void CClosureJointStirrupsPage::GetLastZoneName(CString& strSymmetric, CString& strEnd)
{
   strSymmetric = _T("CL closure");
   strEnd = _T("end closure");
}
