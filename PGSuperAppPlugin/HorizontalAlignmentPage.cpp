///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// HorizontalAlignmentPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "HorizontalAlignmentPage.h"
#include "AlignmentDescriptionDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHorizontalAlignmentPage property page

IMPLEMENT_DYNCREATE(CHorizontalAlignmentPage, CPropertyPage)

CHorizontalAlignmentPage::CHorizontalAlignmentPage() : CPropertyPage(CHorizontalAlignmentPage::IDD)
{
	//{{AFX_DATA_INIT(CHorizontalAlignmentPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_DirFormatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   m_DirFormatter->put_CondensedFormat(VARIANT_TRUE);
   m_DirFormatter->put_BearingFormat(VARIANT_TRUE);


   m_AngleFormatter.CoCreateInstance(CLSID_AngleDisplayUnitFormatter);
   m_AngleFormatter->put_CondensedFormat(VARIANT_TRUE);
   m_AngleFormatter->put_Signed(VARIANT_FALSE);
}

CHorizontalAlignmentPage::~CHorizontalAlignmentPage()
{
}

IBroker* CHorizontalAlignmentPage::GetBroker()
{
   CAlignmentDescriptionDlg* pParent = (CAlignmentDescriptionDlg*)GetParent();
   return pParent->m_pBroker;
}

void CHorizontalAlignmentPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHorizontalAlignmentPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CComPtr<IDirection> direction;
   direction.CoCreateInstance(CLSID_Direction);


   if ( pDX->m_bSaveAndValidate )
   {
      DDX_Direction(pDX,IDC_DIRECTION,direction,m_DirFormatter);
      direction->get_Value(&m_AlignmentData.Direction);

      m_Grid.SortCurves();
      if ( !m_Grid.GetCurveData(m_AlignmentData.HorzCurves) )
      {
         AfxMessageBox(_T("Invalid curve data"));
         pDX->Fail();
      }

      std::vector<HorzCurveData>::iterator iter;
      int curveID = 1;
      for ( iter = m_AlignmentData.HorzCurves.begin(); iter != m_AlignmentData.HorzCurves.end(); iter++, curveID++ )
      {
         HorzCurveData& hc = *iter;
         if ( hc.Radius < 0 )
         {
            CString strMsg;
            strMsg.Format(_T("Curve Radius cannot be less than zero for curve # %d"),curveID);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         if ( hc.EntrySpiral < 0 || (IsZero(hc.Radius) && 0 < hc.EntrySpiral))
         {
            CString strMsg;
            strMsg.Format(_T("Invalid Entry Spiral Length for curve # %d"),curveID);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         if ( hc.ExitSpiral < 0 || (IsZero(hc.Radius) && 0 < hc.ExitSpiral) )
         {
            CString strMsg;
            strMsg.Format(_T("Invalid Exit Spiral Length for curve # %d"),curveID);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }
      }
   }
   else
   {
      direction->put_Value(m_AlignmentData.Direction);
      DDX_Direction(pDX,IDC_DIRECTION,direction,m_DirFormatter);
      m_Grid.SetCurveData(m_AlignmentData.HorzCurves);
   }

   GET_IFACE2(GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   DDX_Station(pDX,  IDC_REFSTATION, m_AlignmentData.RefStation, pDisplayUnits->GetStationFormat() );
   DDX_UnitValue(pDX,IDC_NORTHING,m_AlignmentData.yRefPoint,pDisplayUnits->GetAlignmentLengthUnit());
   DDX_UnitValue(pDX,IDC_EASTING, m_AlignmentData.xRefPoint,pDisplayUnits->GetAlignmentLengthUnit());
}

BEGIN_MESSAGE_MAP(CHorizontalAlignmentPage, CPropertyPage)
	//{{AFX_MSG_MAP(CHorizontalAlignmentPage)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_SORT, OnSort)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHorizontalAlignmentPage message handlers

void CHorizontalAlignmentPage::OnAdd() 
{
   m_Grid.AppendRow();	
}

void CHorizontalAlignmentPage::OnRemove() 
{
   m_Grid.RemoveRows();
}

void CHorizontalAlignmentPage::OnSort() 
{
   m_Grid.SortCurves();
}

BOOL CHorizontalAlignmentPage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_HCURVE_GRID, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CHorizontalAlignmentPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_ALIGNMENT_HORIZONTAL );
}
