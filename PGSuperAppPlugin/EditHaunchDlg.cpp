///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// EditHaunchDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "EditHaunchDlg.h"

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include "PGSuperUnits.h"

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNAMIC(CEditHaunchDlg, CDialog)

CEditHaunchDlg::CEditHaunchDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent /*=NULL*/)
	: CDialog(CEditHaunchDlg::IDD, pParent),
   m_pBridgeDesc(pBridgeDesc),
   m_Fillet(0.0),
   m_bShowFillet(true),
   m_HaunchShape(pgsTypes::hsSquare),
   m_WasSlabOffsetTypeForced(false),
   m_WasDataIntialized(false)
{
   m_HaunchInputData.m_SlabOffsetType = pgsTypes::sotBridge;
}

CEditHaunchDlg::~CEditHaunchDlg()
{
}

void CEditHaunchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );

   DDX_Control(pDX,IDC_HAUNCH_SHAPE,m_cbHaunchShape);
   DDX_CBItemData(pDX, IDC_HAUNCH_SHAPE, m_HaunchShape);

   // fillet
   DDX_UnitValueAndTag( pDX, IDC_FILLET, IDC_FILLET_UNITS, m_Fillet, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_FILLET,m_Fillet, pDisplayUnits->GetComponentDimUnit() );

   DDX_CBItemData(pDX, IDC_A_TYPE, m_HaunchInputData.m_SlabOffsetType);

   if (pDX->m_bSaveAndValidate)
   {
      // Get min A value and build error message for too small of A
      Float64 minA = m_Fillet + m_pBridgeDesc->GetDeckDescription()->GrossDepth;
      cmpdim.SetValue(minA);
      CString strMinValError;
      strMinValError.Format(_T("Slab Offset value must be greater or equal to slab depth + fillet (%.4f %s)"), cmpdim.GetValue(true), cmpdim.GetUnitTag().c_str() );

      switch(m_HaunchInputData.m_SlabOffsetType)
      {
      case pgsTypes::sotBridge:
         this->m_HaunchInputData = m_HaunchSame4BridgeDlg.DownloadData(minA,strMinValError,pDX);
         break;
      case pgsTypes::sotPier:
         this->m_HaunchInputData = m_HaunchSpanBySpanDlg.DownloadData(minA,strMinValError,pDX);
         break;
      case pgsTypes::sotGirder:
         this->m_HaunchInputData = m_HaunchByGirderDlg.DownloadData(minA,strMinValError,this->m_HaunchInputData,pDX);
         break;
      default:
         ATLASSERT(0);
         break;
      };
   }
   else
   {
      // Set data values in embedded dialogs
      m_HaunchSame4BridgeDlg.m_ADim = m_HaunchInputData.m_SingleHaunch;
      m_HaunchSame4BridgeDlg.UpdateData(FALSE);

      m_HaunchSpanBySpanDlg.UploadData(this->m_HaunchInputData);

      m_HaunchByGirderDlg.UploadData(this->m_HaunchInputData);
   }
}


BEGIN_MESSAGE_MAP(CEditHaunchDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_A_TYPE, &CEditHaunchDlg::OnCbnSelchangeAType)
   ON_BN_CLICKED(ID_HELP, &CEditHaunchDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditHaunchDlg message handlers

BOOL CEditHaunchDlg::OnInitDialog()
{

   // Initialize our data structure for current data
   InitializeData();

   // Embed dialogs for strand editing into current. A discription may be found at
   // http://www.codeproject.com/KB/dialog/embedded_dialog.aspx

   // Set up embedded dialogs
   {
      CWnd* pBox = GetDlgItem(IDC_A_BOX);
      pBox->ShowWindow(SW_HIDE);

      CRect boxRect;
      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      VERIFY(m_HaunchSame4BridgeDlg.Create(CHaunchSame4BridgeDlg::IDD, this));
      VERIFY(m_HaunchSame4BridgeDlg.SetWindowPos( GetDlgItem(IDC_A_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_HaunchSpanBySpanDlg.Create(CHaunchSpanBySpanDlg::IDD, this));
      VERIFY(m_HaunchSpanBySpanDlg.SetWindowPos( GetDlgItem(IDC_A_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      m_HaunchByGirderDlg.InitSize(m_HaunchInputData.m_SpanGirdersHaunch.size(), m_HaunchInputData.m_MaxGirdersPerSpan);
      VERIFY(m_HaunchByGirderDlg.Create(CHaunchByGirderDlg::IDD, this));
      VERIFY(m_HaunchByGirderDlg.SetWindowPos( GetDlgItem(IDC_A_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));
   }

   // Slab offset type combo
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_A_TYPE);
   int sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotBridge);
   sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotPier));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotPier);
   sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotGirder);

   CDialog::OnInitDialog();

   OnCbnSelchangeAType();

   // Don't show input for fillet and shape if we only have an overlay
   GetDlgItem(IDC_FILLET_LABEL)->EnableWindow(m_bShowFillet);
   GetDlgItem(IDC_FILLET)->EnableWindow(m_bShowFillet);
   GetDlgItem(IDC_FILLET_UNITS)->EnableWindow(m_bShowFillet);
   if ( !m_bShowFillet )
   {
      GetDlgItem(IDC_FILLET)->SetWindowText(_T("0.00"));
   }
   GetDlgItem(IDC_HAUNCH_SHAPE)->EnableWindow(m_bShowFillet);
   GetDlgItem(IDC_HAUNCH_SHAPE_LABEL)->EnableWindow(m_bShowFillet);

   m_cbHaunchShape.Initialize(m_HaunchShape);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_HAUNCH_SHAPE,m_HaunchShape);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditHaunchDlg::InitializeData()
{
   m_WasDataIntialized = true;

   // First get fillet
   const CDeckDescription2* pDeck = m_pBridgeDesc->GetDeckDescription();
   ATLASSERT(pDeck->DeckType!= pgsTypes::sdtNone); // should not be able to edit haunch if no deck
   m_Fillet =  pDeck->Fillet;

   m_bShowFillet = pDeck->DeckType == pgsTypes::sdtNone ? FALSE:TRUE;

   if (!m_bShowFillet)
   {
      m_HaunchShape = pgsTypes::hsSquare; // show fillet as square for overlays (the control will be disabled)
   }
   else
   {
      m_HaunchShape = pDeck->HaunchShape;
   }

   // Take data from project and fill our local data structures
   // Fill all slab offset types with default values depending on initial type
   m_HaunchInputData.m_SpansHaunch.clear();
   m_HaunchInputData.m_SpanGirdersHaunch.clear();

   if (m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge)
   {
      m_HaunchInputData.m_SlabOffsetType = pgsTypes::sotBridge;
      m_HaunchInputData.m_SingleHaunch = m_pBridgeDesc->GetSlabOffset();

      // fill pier data with same value
      FillPierData(m_HaunchInputData.m_SingleHaunch, m_HaunchInputData.m_SingleHaunch);
      FillGirderData(m_HaunchInputData.m_SingleHaunch, m_HaunchInputData.m_SingleHaunch);
   }
   else if (m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotPier || 
            m_pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotGirder)
   {
      // Pier and girder based A's are treated the same
      m_HaunchInputData.m_SlabOffsetType = m_pBridgeDesc->GetSlabOffsetType();

      GroupIndexType ngrp = m_pBridgeDesc->GetGirderGroupCount();
#ifdef _DEBUG
   SpanIndexType ns = m_pBridgeDesc->GetSpanCount();
   ATLASSERT(ns==ngrp); // Always true in PGSuper
#endif

      bool bFirst(true);
      for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
      {
         const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(igrp);
         PierIndexType startPier = igrp;
         PierIndexType endPier  = igrp+1;

         m_HaunchInputData.m_SpanGirdersHaunch.push_back( HaunchPairVec() );

         HaunchPairVec& rGirderA = m_HaunchInputData.m_SpanGirdersHaunch.back();

         GirderIndexType ng = pGroup->GetGirderCount();
         for (GirderIndexType ig=0; ig<ng; ig++)
         {
            Float64 startA = pGroup->GetSlabOffset(startPier, ig);
            Float64 endA   = pGroup->GetSlabOffset(endPier, ig);

            // For same A for entire bridge, just use span 0, girder 0
            if (bFirst)
            {
               m_HaunchInputData.m_SingleHaunch = startA;
               bFirst = false;
            }

            // For piers option, take girder 0
            if (ig==0)
            {
               // slab offsets are contained in girders and assumed to be the same across all in the same span
               m_HaunchInputData.m_SpansHaunch.push_back( HaunchPair(startA, endA) );
            }

            // For unique for girders option, fill all values
            rGirderA.push_back( HaunchPair(startA, endA) );
         }
      }
   }
   else 
   {
      ATLASSERT(0); // new type??
   }

   // See if haunch fill type was forced. If so, set it
   if (m_WasSlabOffsetTypeForced)
   {
      m_HaunchInputData.m_SlabOffsetType = m_ForcedSlabOffsetType;
   }

   // Compute max number of girders per span
   m_HaunchInputData.m_MaxGirdersPerSpan = 0;
   std::vector<HaunchPairVec>::const_iterator pvec    = m_HaunchInputData.m_SpanGirdersHaunch.begin();
   std::vector<HaunchPairVec>::const_iterator pvecend = m_HaunchInputData.m_SpanGirdersHaunch.end();
   while (pvec != pvecend)
   {
      m_HaunchInputData.m_MaxGirdersPerSpan = max((Uint32)pvec->size(), m_HaunchInputData.m_MaxGirdersPerSpan);
      pvec++;
   }
}

void CEditHaunchDlg::FillPierData(Float64 startA, Float64 endA)
{
   SpanIndexType ns = m_pBridgeDesc->GetSpanCount();
   for(SpanIndexType is=0; is<ns; is++)
   {
      m_HaunchInputData.m_SpansHaunch.push_back( HaunchPair(startA, endA) );
   }
}

void CEditHaunchDlg::FillGirderData(Float64 startA, Float64 endA)
{
   SpanIndexType ns = m_pBridgeDesc->GetSpanCount();
   for(SpanIndexType is=0; is<ns; is++)
   {
      HaunchPairVec spanAs;
      GirderIndexType ng = m_pBridgeDesc->GetSpan(is)->GetGirderCount();
      for(GirderIndexType ig=0; ig<ng; ig++)
      {
         spanAs.push_back( HaunchPair(startA, endA) );
      }

       m_HaunchInputData.m_SpanGirdersHaunch.push_back(spanAs);
   }
}

void CEditHaunchDlg::OnCbnSelchangeAType()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_A_TYPE);
   
   m_HaunchInputData.m_SlabOffsetType = (pgsTypes::SlabOffsetType)pBox->GetCurSel();

   switch(m_HaunchInputData.m_SlabOffsetType)
   {
   case pgsTypes::sotBridge:
      m_HaunchSame4BridgeDlg.ShowWindow(SW_SHOW);
      m_HaunchSpanBySpanDlg.ShowWindow(SW_HIDE);
      m_HaunchByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::sotPier:
      m_HaunchSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_HaunchSpanBySpanDlg.ShowWindow(SW_SHOW);
      m_HaunchByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::sotGirder:
      m_HaunchSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_HaunchSpanBySpanDlg.ShowWindow(SW_HIDE);
      m_HaunchByGirderDlg.ShowWindow(SW_SHOW);
      break;
   default:
      ATLASSERT(0);
      break;
   };
}

void CEditHaunchDlg::ForceToSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType)
{
   m_WasSlabOffsetTypeForced = true;
   m_ForcedSlabOffsetType = slabOffsetType;
   m_WasDataIntialized = false;
}

void CEditHaunchDlg::ModifyBridgeDescr(CBridgeDescription2* pBridgeDesc)
{
   if (!m_WasDataIntialized)
   {
      InitializeData();
   }

   // First Fillet
   pBridgeDesc->GetDeckDescription()->Fillet = this->m_Fillet;

   pBridgeDesc->GetDeckDescription()->HaunchShape = m_HaunchShape;

   // Haunch
   if (m_HaunchInputData.m_SlabOffsetType == pgsTypes::sotBridge)
   {
      pBridgeDesc->SetSlabOffsetType(pgsTypes::sotBridge);
      pBridgeDesc->SetSlabOffset(m_HaunchInputData.m_SingleHaunch);
   }
   else if (m_HaunchInputData.m_SlabOffsetType == pgsTypes::sotPier)
   {
      pBridgeDesc->SetSlabOffsetType(pgsTypes::sotPier);

      GroupIndexType ngrp = pBridgeDesc->GetGirderGroupCount();
      for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
      {
         PierIndexType startPier = igrp;
         PierIndexType endPier  = igrp+1;

         CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(igrp);

         HaunchPair& pair = m_HaunchInputData.m_SpansHaunch[igrp];

         GirderIndexType ng = pGroup->GetGirderCount();
         for (GirderIndexType ig=0; ig<ng; ig++)
         {
            pGroup->SetSlabOffset(startPier, ig, pair.first);
            pGroup->SetSlabOffset(endPier,   ig, pair.second);
         }
      }
   }
   else if (m_HaunchInputData.m_SlabOffsetType == pgsTypes::sotGirder)
   {
      pBridgeDesc->SetSlabOffsetType(pgsTypes::sotGirder);

      GroupIndexType ngrp = pBridgeDesc->GetGirderGroupCount();
      for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
      {
         PierIndexType startPier = igrp;
         PierIndexType endPier  = igrp+1;

         CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(igrp);

         HaunchPairVec& rpv = m_HaunchInputData.m_SpanGirdersHaunch[igrp];

         GirderIndexType ng = pGroup->GetGirderCount();
         for (GirderIndexType ig=0; ig<ng; ig++)
         {
            HaunchPair pair = rpv[ig];
            pGroup->SetSlabOffset(startPier, ig, pair.first);
            pGroup->SetSlabOffset(endPier,   ig, pair.second);
         }
      }
   }
}

void CEditHaunchDlg::OnBnClickedHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_HAUNCH);
}
