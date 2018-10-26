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

// BridgeDescLongitudinalRebar.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescLongitudinalRebar.h"
#include "GirderDescDlg.h"
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <EAF\EAFDisplayUnits.h>
#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLongitudinalRebar property page

IMPLEMENT_DYNCREATE(CGirderDescLongitudinalRebar, CPropertyPage)

CGirderDescLongitudinalRebar::CGirderDescLongitudinalRebar() : CPropertyPage(CGirderDescLongitudinalRebar::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescLongitudinalRebar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderDescLongitudinalRebar::~CGirderDescLongitudinalRebar()
{
}

void CGirderDescLongitudinalRebar::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescLongitudinalRebar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDV_GXGridWnd(pDX, &m_Grid);

   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_cbRebar);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   // longitudinal steel information from grid and store it
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   if (pDX->m_bSaveAndValidate)
   {
      CLongitudinalRebarData rebarData;

      if ( !m_Grid.GetRebarData(&rebarData) )
      {
         pDX->Fail();
      }

#pragma Reminder("UPDATE: need to validate at both ends because of tapered segments")
      // Validate geometry of the bars
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(pParent->m_SegmentKey);
      GET_IFACE2(pBroker,IPointOfInterest,pPOI);
      pgsPointOfInterest poi(pPOI->GetPointOfInterest(pParent->m_SegmentKey,0.0));
      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      Float64 height = pSectProp->GetHg(releaseIntervalIdx,poi);
      GET_IFACE2(pBroker,IShapes,pShapes);
      CComPtr<IShape> shape;
      pShapes->GetSegmentShape(releaseIntervalIdx,poi,false,pgsTypes::scGirder,&shape);

      CString strMsg;
      CComPtr<IPoint2d> point;
      point.CoCreateInstance(CLSID_Point2d);
      int rowIdx = 1;
      BOOST_FOREACH(CLongitudinalRebarData::RebarRow& row,rebarData.RebarRows)
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
         VARIANT_BOOL bPointInShape;
         shape->PointInShape( point,&bPointInShape );
         if ( bPointInShape == VARIANT_FALSE )
         {
            strMsg.Format(_T("One or more of the bars in row %d are outside of the girder section."),rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         rowIdx++;
      }

      DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,rebarData.BarType,rebarData.BarGrade);

      pParent->m_pSegment->LongitudinalRebarData = rebarData;
   }
   else
   {
      DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,pParent->m_pSegment->LongitudinalRebarData.BarType,pParent->m_pSegment->LongitudinalRebarData.BarGrade);

      m_Grid.FillGrid(pParent->m_pSegment->LongitudinalRebarData);
   }
}


BEGIN_MESSAGE_MAP(CGirderDescLongitudinalRebar, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescLongitudinalRebar)
	ON_BN_CLICKED(IDC_RESTORE_DEFAULTS1, OnRestoreDefaults)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_BN_CLICKED(IDC_APPEND_ROW, OnAppendRow)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescLongitudinalRebar message handlers

void CGirderDescLongitudinalRebar::RestoreToLibraryDefaults(CLongitudinalRebarData* pLongData)
{
   // get shear information from library
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( m_CurGrdName.c_str());
   ASSERT(pGirderEntry != 0);

   // update data 
   pLongData->CopyGirderEntryData(*pGirderEntry);
}

void CGirderDescLongitudinalRebar::GetRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade)
{
   m_cbRebar.GetMaterial(pType,pGrade);
}

void CGirderDescLongitudinalRebar::OnRestoreDefaults() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

	RestoreToLibraryDefaults(&(pParent->m_pSegment->LongitudinalRebarData));
   // update data in page and redraw
   VERIFY(UpdateData(FALSE));
}

void CGirderDescLongitudinalRebar::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CGirderDescLongitudinalRebar::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_LONG_GRID, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDescLongitudinalRebar::OnInsertrow() 
{
   m_Grid.Insertrow();
}

void CGirderDescLongitudinalRebar::OnAppendRow() 
{
   m_Grid.Appendrow();
}

void CGirderDescLongitudinalRebar::OnRemoveRows() 
{
	m_Grid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CGirderDescLongitudinalRebar::OnHelp() 
{
	EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_LONGIT_REBAR );
}
