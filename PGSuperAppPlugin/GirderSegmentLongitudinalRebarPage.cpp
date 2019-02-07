///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// GirderSegmentLongitudinalRebarPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "GirderSegmentLongitudinalRebarPage.h"
#include "GirderSegmentDlg.h"
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentLongitudinalRebarPage property page

IMPLEMENT_DYNCREATE(CGirderSegmentLongitudinalRebarPage, CPropertyPage)

CGirderSegmentLongitudinalRebarPage::CGirderSegmentLongitudinalRebarPage() : CPropertyPage(CGirderSegmentLongitudinalRebarPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderSegmentLongitudinalRebarPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderSegmentLongitudinalRebarPage::~CGirderSegmentLongitudinalRebarPage()
{
}

void CGirderSegmentLongitudinalRebarPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderSegmentLongitudinalRebarPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDV_GXGridWnd(pDX, &m_Grid);

   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_cbRebar);

   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,pSegment->LongitudinalRebarData.BarType,pSegment->LongitudinalRebarData.BarGrade);

   // longitudinal steel information from grid and store it
   if (pDX->m_bSaveAndValidate)
   {
      m_Grid.GetRebarData(&pSegment->LongitudinalRebarData);
   }
   else
   {
      m_Grid.FillGrid(pSegment->LongitudinalRebarData);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      CString strMsg;
      CComPtr<IPoint2d> point;
      point.CoCreateInstance(CLSID_Point2d);
      int rowIdx = 1;
      for (const auto& row : pSegment->LongitudinalRebarData.RebarRows)
      {
         if (row.Cover < 0)
         {
            strMsg.Format(_T("The cover for row %d must be greater than zero."),rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         if (row.NumberOfBars == INVALID_INDEX)
         {
            strMsg.Format(_T("The number of bars in row %d must be greater than zero."),rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         if ( 1 < row.NumberOfBars && row.BarSpacing < 0)
         {
            strMsg.Format(_T("The bar spacing in row %d must be greater than zero."),rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         // make sure bars are inside of girder - use shape symmetry
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         for ( int i = 0; i < 4; i++ )
         {
            pgsTypes::SegmentZoneType zone = (pgsTypes::SegmentZoneType)i;

            Float64 Lzone, height, tbf;
            pSegment->GetVariationParameters(zone,false,&Lzone,&height,&tbf);

            gpPoint2d testpnt;
            testpnt.X() = row.BarSpacing * (row.NumberOfBars-1)/2.;
            if (row.Face == pgsTypes::TopFace)
            {
               testpnt.Y() = -row.Cover;
            }
            else
            {
               testpnt.Y() = -(height-row.Cover);
            }

            point->Move(testpnt.X(),testpnt.Y());

            const GirderLibraryEntry* pGdrEntry = pSegment->GetGirder()->GetGirderLibraryEntry();
            CComPtr<IBeamFactory> factory;
            pGdrEntry->GetBeamFactory(&factory);

            CComPtr<IGirderSection> gdrSection;
            factory->CreateGirderSection(pBroker,INVALID_ID,pGdrEntry->GetDimensions(),height,tbf,&gdrSection);

            CComQIPtr<IShape> shape(gdrSection);

            VARIANT_BOOL bPointInShape;
            shape->PointInShape( point,&bPointInShape );
            if ( bPointInShape == VARIANT_FALSE )
            {
               strMsg.Format(_T("One or more of the bars in row %d are outside of the girder section."),rowIdx);
               AfxMessageBox(strMsg);
               pDX->Fail();
            }
         }

         rowIdx++;
      }
   }
}


BEGIN_MESSAGE_MAP(CGirderSegmentLongitudinalRebarPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentLongitudinalRebarPage)
	ON_BN_CLICKED(IDC_RESTORE_DEFAULTS1, OnRestoreDefaults)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_BN_CLICKED(IDC_APPEND_ROW, OnAppendRow)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentLongitudinalRebarPage message handlers
void CGirderSegmentLongitudinalRebarPage::RestoreToLibraryDefaults()
{
   CGirderSegmentDlg* pParent = (CGirderSegmentDlg*)GetParent();
   const GirderLibraryEntry* pGirderEntry = pParent->m_Girder.GetGirderLibraryEntry();
   CPrecastSegmentData* pSegment = pParent->m_Girder.GetSegment(pParent->m_SegmentKey.segmentIndex);

   // update data member
   pSegment->LongitudinalRebarData.CopyGirderEntryData(pGirderEntry);
}

void CGirderSegmentLongitudinalRebarPage::OnRestoreDefaults() 
{
	RestoreToLibraryDefaults();
   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

void CGirderSegmentLongitudinalRebarPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CGirderSegmentLongitudinalRebarPage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_LONG_GRID, this);
   m_Grid.CustomInit();

   // Currently, precast segments don't have default longitudinal reinforcement
   // Hide the button in the UI
   GetDlgItem(IDC_RESTORE_DEFAULTS)->ShowWindow(SW_HIDE);

   CPropertyPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderSegmentLongitudinalRebarPage::OnInsertrow() 
{
   m_Grid.Insertrow();
}

void CGirderSegmentLongitudinalRebarPage::OnAppendRow() 
{
   m_Grid.Appendrow();
}

void CGirderSegmentLongitudinalRebarPage::OnRemoveRows() 
{
	m_Grid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CGirderSegmentLongitudinalRebarPage::OnHelp() 
{
	EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_LONGIT_REBAR );
}
