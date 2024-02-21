///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// GirderSegmentTendonsPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentTendonsPage.h"
#include "GirderSegmentDlg.h"

#include <GenericBridge\Helpers.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\Intervals.h>

#include <Materials/PsStrand.h>
#include <LRFD\StrandPool.h>

#include <PgsExt\CustomDDX.h>

#include "PGSuperColors.h"
#include <PgsExt\DesignConfigUtil.h>

#include "GirderDescDlg.h" // for ReconcileDebonding

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CGirderSegmentTendonsPage dialog

IMPLEMENT_DYNAMIC(CGirderSegmentTendonsPage, CPropertyPage)


CGirderSegmentTendonsPage::CGirderSegmentTendonsPage()
	: CPropertyPage(CGirderSegmentTendonsPage::IDD)
{
   m_pSegment = 0;
}

CGirderSegmentTendonsPage::~CGirderSegmentTendonsPage()
{
}

void CGirderSegmentTendonsPage::Init(CPrecastSegmentData* pSegment)
{
   m_pSegment = pSegment;
   m_Strands = m_pSegment->Strands;
   m_Tendons = m_pSegment->Tendons;

   m_pGrid = std::make_unique<CSegmentTendonGrid>();
}

CPrecastSegmentData* CGirderSegmentTendonsPage::GetSegment()
{
   return m_pSegment;
}

pgsTypes::StrandInstallationType CGirderSegmentTendonsPage::GetInstallationType()
{
   CComboBox* pList = (CComboBox*)GetDlgItem(IDC_INSTALLATION_TYPE);
   int idx = pList->GetCurSel();
   pgsTypes::StrandInstallationType installationType = (pgsTypes::StrandInstallationType)(pList->GetItemData(idx));
   return installationType;
}

void CGirderSegmentTendonsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   //{{AFX_DATA_MAP(CGirderSegmentTendonsPage)
   //}}AFX_DATA_MAP

   DDX_CBEnum(pDX, IDC_DUCT_TYPE, m_Tendons.DuctType);
   DDX_CBItemData(pDX, IDC_INSTALLATION_TYPE, m_Tendons.InstallationType);
   DDX_CBItemData(pDX, IDC_INSTALLATION_TIME, m_Tendons.InstallationEvent);
   
   DDX_Strand(pDX, IDC_STRAND, &(m_Tendons.m_pStrand));

   m_pGrid->UpdateData(pDX, &m_Tendons);

   if (pDX->m_bSaveAndValidate)
   {
      m_pSegment->Tendons = m_Tendons;
   }
}


BEGIN_MESSAGE_MAP(CGirderSegmentTendonsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderSegmentTendonsPage)
	ON_COMMAND(ID_HELP, OnHelp)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_ADD, &CGirderSegmentTendonsPage::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &CGirderSegmentTendonsPage::OnBnClickedRemove)
   ON_CBN_SELCHANGE(IDC_STRAND, OnStrandTypeChanged)
   ON_CBN_SELCHANGE(IDC_INSTALLATION_TYPE, OnInstallationTypeChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderSegmentTendonsPage message handlers

BOOL CGirderSegmentTendonsPage::OnInitDialog() 
{
   m_pGrid->SubclassDlgItem(IDC_GRID, this);
   m_pGrid->CustomInit(m_pSegment);

   m_DrawStrands.SubclassDlgItem(IDC_DRAW_STRANDS,this);
   //m_DrawStrands.CustomInit(m_pSegment,&m_Strands,&m_Tendons); // we will do this in OnSetActive

   UpdateStrandList(IDC_STRAND);

   UpdateDuctMaterialList();
   UpdateInstallationMethodList();
   UpdateInstallationTimeList();

   // Set the OK button as the default button
   SendMessage (DM_SETDEFID, IDOK);

   CPropertyPage::OnInitDialog();

   EnableToolTips(TRUE);
   EnableRemoveButton(FALSE); // start off with the button disabled... it will get enabled when a row in the grid is selected

   
   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderSegmentTendonsPage::UpdateSectionDepth()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(this->m_pSegment->GetSegmentKey(), POI_0L | POI_5L | POI_10L | POI_RELEASED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 3);

   GET_IFACE2(pBroker, IShapes, pShapes);
   std::array<CComPtr<IShape>, 3> shape;
   std::array<CComPtr<IRect2d>, 3> bounding_box;
   std::array<UINT, 3> nIDC{ IDC_HG_START,IDC_HG_MIDDLE,IDC_HG_END };
   for (int i = 0; i < 3; i++)
   {
	   const pgsPointOfInterest& poi(vPoi[i]);
       pShapes->GetSegmentShape(m_pSegment, poi.GetDistFromStart(), (i == 0 ? pgsTypes::sbRight : pgsTypes::sbLeft), &shape[i]);
      shape[i]->get_BoundingBox(&bounding_box[i]);
      Float64 Hg;
      bounding_box[i]->get_Height(&Hg);
      GetDlgItem(nIDC[i])->SetWindowText(::FormatDimension(Hg, pDisplayUnits->GetComponentDimUnit()));
   }
}

const WBFL::Materials::PsStrand* CGirderSegmentTendonsPage::GetStrand()
{
   CComboBox* pList = (CComboBox*)GetDlgItem(IDC_STRAND);
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();

   int cursel = pList->GetCurSel();
   Int64 key = (Int64)pList->GetItemData(cursel);
   return pPool->GetStrand(key);
}

void CGirderSegmentTendonsPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_SEGMENT_TENDON_INPUT);
}

void CGirderSegmentTendonsPage::UpdateDuctMaterialList()
{
   // LRFD 5.4.6.1
   CComboBox* pcbDuctType = (CComboBox*)GetDlgItem(IDC_DUCT_TYPE);
   pcbDuctType->AddString(_T("Galvanized ferrous metal"));
   pcbDuctType->AddString(_T("Polyethylene"));
   pcbDuctType->AddString(_T("Formed in concrete with removable cores"));
}

void CGirderSegmentTendonsPage::UpdateInstallationMethodList()
{
   // LRFD 5.4.6.2
   CComboBox* pcbInstallType = (CComboBox*)GetDlgItem(IDC_INSTALLATION_TYPE);
   int idx = pcbInstallType->AddString(_T("Push"));
   pcbInstallType->SetItemData(idx, (DWORD_PTR)pgsTypes::sitPush);
   idx = pcbInstallType->AddString(_T("Pull"));
   pcbInstallType->SetItemData(idx, (DWORD_PTR)pgsTypes::sitPull);
}

void CGirderSegmentTendonsPage::UpdateInstallationTimeList()
{
   CComboBox* pcbInstallationTime = (CComboBox*)GetDlgItem(IDC_INSTALLATION_TIME);
   pcbInstallationTime->SetItemData(pcbInstallationTime->AddString(_T("Immediately after release")), (DWORD_PTR)pgsTypes::sptetRelease);
   pcbInstallationTime->SetItemData(pcbInstallationTime->AddString(_T("At beginning of storage")), (DWORD_PTR)pgsTypes::sptetStorage);
   pcbInstallationTime->SetItemData(pcbInstallationTime->AddString(_T("Immediately before hauling")), (DWORD_PTR)pgsTypes::sptetHauling);
}

void CGirderSegmentTendonsPage::UpdateStrandList(UINT nIDC)
{
   CComboBox* pList = (CComboBox*)GetDlgItem(nIDC);
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();

   // capture the current selection, if any
   int cur_sel = pList->GetCurSel();
   Int64 cur_key = (Int64)pList->GetItemData( cur_sel );
   // remove the coating flag from the current key
   WBFL::System::Flags<Int64>::Clear(&cur_key,+WBFL::Materials::PsStrand::Coating::None);
   WBFL::System::Flags<Int64>::Clear(&cur_key,+WBFL::Materials::PsStrand::Coating::GritEpoxy);

   WBFL::Materials::PsStrand::Coating coating = WBFL::Materials::PsStrand::Coating::None;
   WBFL::System::Flags<Int64>::Set(&cur_key,+coating); // add the coating flag for the strand type we are changing to

   pList->ResetContent();

   int sel_count = 0;  // Keep count of the number of strings added to the combo box
   int new_cur_sel = -1; // This will be in index of the string we want to select.
   for (int i = 0; i < 3; i++)
   {
      WBFL::Materials::PsStrand::Grade grade = (i == 0 ? WBFL::Materials::PsStrand::Grade::Gr1725 :
                                                i == 1 ? WBFL::Materials::PsStrand::Grade::Gr1860 : WBFL::Materials::PsStrand::Grade::Gr2070);
      for ( int j = 0; j < 2; j++ )
      {
         WBFL::Materials::PsStrand::Type type = (j == 0 ? WBFL::Materials::PsStrand::Type::LowRelaxation : WBFL::Materials::PsStrand::Type::StressRelieved);

         WBFL::LRFD::StrandIter iter(grade,type,coating);

         for ( iter.Begin(); iter; iter.Next() )
         {
            const auto* pStrand = iter.GetCurrentStrand();
            int idx = pList->AddString( pStrand->GetName().c_str() );
               
            auto key = pPool->GetStrandKey( pStrand );
            pList->SetItemData( idx, key );

            if ( key == cur_key )
            {
               new_cur_sel = sel_count;
            }

            sel_count++;
         }
      }
   }

   // Attempt to re-select the strand.
   if ( 0 <= new_cur_sel )
   {
      pList->SetCurSel( new_cur_sel );
   }
   else
   {
      pList->SetCurSel( pList->GetCount()-1 );
   }
}

void CGirderSegmentTendonsPage::OnStrandTypeChanged() 
{
   m_pGrid->OnStrandChanged();
}

void CGirderSegmentTendonsPage::OnInstallationTypeChanged()
{
   m_pGrid->OnInstallationTypeChanged();
}

BOOL CGirderSegmentTendonsPage::OnSetActive() 
{
   // make sure segment geometry is up to date with what ever has been changed during
   // this editing session
   m_Strands = m_pSegment->Strands;
   m_Tendons = m_pSegment->Tendons;
   m_DrawStrands.CustomInit(m_pSegment,&m_Strands,&m_Tendons);

   OnChange();

   return CPropertyPage::OnSetActive();
}

BOOL CGirderSegmentTendonsPage::OnKillActive()
{
   //this->SetFocus();  // prevents artifacts from grid list controls (not sure why)

   return CPropertyPage::OnKillActive();
}

void CGirderSegmentTendonsPage::OnBnClickedAdd()
{
   m_pGrid->OnAddRow();
}

void CGirderSegmentTendonsPage::OnBnClickedRemove()
{
   m_pGrid->OnRemoveSelectedRows();
}

void CGirderSegmentTendonsPage::EnableRemoveButton(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE)->EnableWindow(bEnable);
}

void CGirderSegmentTendonsPage::OnChange()
{
   m_pGrid->UpdateData(nullptr, &m_Tendons);
   m_DrawStrands.Invalidate();
   m_DrawStrands.UpdateWindow(); 
   UpdateSectionDepth();
}

