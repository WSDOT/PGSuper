///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// SpanGirderLayoutPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "GirderLayoutPage.h"
#include "SpanDetailsDlg.h"
#include "SelectItemDlg.h"
#include "ResolveGirderSpacingDlg.h"
#include "PGSuperColors.h"
#include "Utilities.h"

#include <PGSuperUnits.h>


#include <PgsExt\BridgeDescription2.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanGirderLayoutPage property page

IMPLEMENT_DYNCREATE(CSpanGirderLayoutPage, CPropertyPage)

CSpanGirderLayoutPage::CSpanGirderLayoutPage() : CPropertyPage(CSpanGirderLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CSpanGirderLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_MinGirderCount = 2;
}

CSpanGirderLayoutPage::~CSpanGirderLayoutPage()
{
}

void CSpanGirderLayoutPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanGirderLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_Control(pDX, IDC_NUMGDR_SPIN,         m_NumGdrSpinner);
   DDX_Control(pDX, IDC_GDR_SPC_TYPE_COMBO,  m_cbGirderSpacingType);

   DDX_Text( pDX, IDC_NUMGDR, m_nGirders );
   DDV_MinMaxLongLong(pDX, m_nGirders, m_MinGirderCount, MAX_GIRDERS_PER_SPAN );

   DDX_CBItemData(pDX, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::metStart]);
   DDX_CBItemData(pDX, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_GirderSpacingMeasure[pgsTypes::metEnd]);

   DDV_SpacingGrid(pDX,IDC_PREV_PIER_SPACING_GRID,&m_SpacingGrid[pgsTypes::metStart]);
   DDV_SpacingGrid(pDX,IDC_NEXT_PIER_SPACING_GRID,&m_SpacingGrid[pgsTypes::metEnd]);

   if (IsTopWidthSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()))
   {
      DDV_TopWidthGrid(pDX, IDC_TOP_WIDTH_GRID, &m_TopWidthGrid);
   }

   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER, m_RefGirderIdx[pgsTypes::metStart]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER, m_RefGirderIdx[pgsTypes::metEnd]);

   DDX_OffsetAndTag(pDX, IDC_PREV_REF_GIRDER_OFFSET,IDC_PREV_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit());
   DDX_OffsetAndTag(pDX, IDC_NEXT_REF_GIRDER_OFFSET,IDC_NEXT_REF_GIRDER_OFFSET_UNIT, m_RefGirderOffset[pgsTypes::metEnd], pDisplayUnits->GetXSectionDimUnit());

   DDX_CBItemData(pDX, IDC_PREV_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::metStart]);
   DDX_CBItemData(pDX, IDC_NEXT_REF_GIRDER_OFFSET_TYPE, m_RefGirderOffsetType[pgsTypes::metEnd]);

   if ( pDX->m_bSaveAndValidate && !::IsBridgeSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) && ::IsJointSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) )
   {
      // this is a special case for joint spacing
      if ( m_RefGirderIdx[pgsTypes::metStart] != m_RefGirderIdx[pgsTypes::metEnd] || 
         !IsEqual(m_RefGirderOffset[pgsTypes::metStart],m_RefGirderOffset[pgsTypes::metEnd]) || 
         m_RefGirderOffsetType[pgsTypes::metStart] != m_RefGirderOffsetType[pgsTypes::metEnd] )
      {
         pDX->PrepareCtrl(IDC_PREV_REF_GIRDER);
         AfxMessageBox(_T("Girders must be located the same at both ends of the span"));
         pDX->Fail();
      }
   }

   // Now that all the data has been extracted from the controls, there are a few items that
   // are stored in class data members. Assign these items to the bridge model in the parent dialog
   if ( pDX->m_bSaveAndValidate )
   {
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);
         pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)end;

         CGirderSpacing2* pSpacing = pParent->m_pSpanData->GetPier(end)->GetGirderSpacing(face);
         pSpacing->SetRefGirder(m_RefGirderIdx[end]);
         pSpacing->SetRefGirderOffset(m_RefGirderOffset[end]);
         pSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType[end]);

         pgsTypes::MeasurementType     mt;
         pgsTypes::MeasurementLocation ml;
         UnhashGirderSpacing(m_GirderSpacingMeasure[end],&ml,&mt);
         pSpacing->SetMeasurementType(mt);
         pSpacing->SetMeasurementLocation(ml);
      }

      pgsTypes::SupportedBeamSpacing spacingType = pParent->m_BridgeDesc.GetGirderSpacingType();
      if (spacingType == pgsTypes::sbsConstantAdjacent && !pParent->m_BridgeDesc.UseSameGirderForEntireBridge())
      {
         // all girders must have compatible spacing, that is, all selected girder types
         // must be able to be made to have the same width
         std::vector<std::_tstring> vGirderNames;
         GirderIndexType nGirders = pParent->m_pGirderGroup->GetGirderCount();
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            const CSplicedGirderData* pGirder = pParent->m_pGirderGroup->GetGirder(gdrIdx);
            vGirderNames.push_back(pGirder->GetGirderName());
         }

         GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
         if ( !pIBridgeDesc->AreGirdersCompatible(pParent->m_BridgeDesc,vGirderNames))
         {
            AfxMessageBox(_T("Girders do not have compatible dimensions."));
            pDX->Fail();
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CSpanGirderLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanGirderLayoutPage)
	ON_NOTIFY(UDN_DELTAPOS, IDC_NUMGDR_SPIN, OnNumGirdersChanged)
   ON_BN_CLICKED(IDC_COPY_TO_END,OnCopySpacingToEnd)
   ON_BN_CLICKED(IDC_COPY_TO_START,OnCopySpacingToStart)
	ON_CBN_SELCHANGE(IDC_PREV_PIER_GIRDER_SPACING_MEASURE, OnPrevPierGirderSpacingMeasureChanged)
	ON_CBN_SELCHANGE(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, OnNextPierGirderSpacingMeasureChanged)
   ON_CBN_SELCHANGE(IDC_GDR_SPC_TYPE_COMBO,OnChangeSameGirderSpacing)
   ON_CBN_SELCHANGE(IDC_GDR_NAME_COMBO,OnChangeSameGirderType)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_CBN_SELCHANGE(IDC_NGDRS_COMBO, &CSpanGirderLayoutPage::OnCbnSelchangeNgdrsCombo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanGirderLayoutPage message handlers
void CSpanGirderLayoutPage::Init(CSpanDetailsDlg* pParent)
{
   // This dialog page is only used for PGSuper documents (Precast Girder Bridges)
   // The span and the girder group must have the same piers
   ATLASSERT(EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)));

   Float64 skew_angle = 0; // dummy value (on purpose!) correct value will be set on OnInitialDialog

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);
      pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)end;

      CPierData2* pPier = pParent->m_pSpanData->GetPier(end);
      CGirderSpacing2* pSpacing        = pPier->GetGirderSpacing(face);
      pgsTypes::MeasurementLocation ml = pSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mt = pSpacing->GetMeasurementType();
      m_GirderSpacingMeasure[end]      = HashGirderSpacing(ml,mt);
      m_CacheGirderSpacingMeasure[end] = m_GirderSpacingMeasure[end];

      m_RefGirderIdx[end]        = pSpacing->GetRefGirder();
      m_RefGirderOffset[end]     = pSpacing->GetRefGirderOffset();
      m_RefGirderOffsetType[end] = pSpacing->GetRefGirderOffsetType();

      m_CacheRefGirderIdx[end]        = m_RefGirderIdx[end];
      m_CacheRefGirderOffset[end]     = m_RefGirderOffset[end];
      m_CacheRefGirderOffsetType[end] = m_RefGirderOffsetType[end];

      m_GirderSpacingCache[end] = *pSpacing;
      m_GirderSpacingCache[end].SetPier(nullptr);
      m_GirderSpacingCache[end].SetTemporarySupport(nullptr);

      m_SpacingGrid[end].InitializeGridData( pSpacing,
                                             pParent->m_pGirderGroup,
                                             face,
                                             pPier->GetIndex(),
                                             skew_angle,
                                             pPier->IsAbutment(),
                                             pParent->m_BridgeDesc.GetDeckDescription()->GetDeckType());
   }

   m_nGirders = pParent->m_pGirderGroup->GetGirderCount();
}

void CSpanGirderLayoutPage::GetPierSkewAngles(Float64& skew1,Float64& skew2)
{
   CSpanDetailsDlg* pParent   = (CSpanDetailsDlg*)GetParent();
   const CPierData2* pPrevPier = pParent->m_pSpanData->GetPrevPier();
   const CPierData2* pNextPier = pParent->m_pSpanData->GetNextPier();

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   Float64 skew_angle_1;
   pBridge->GetSkewAngle(pPrevPier->GetStation(),pPrevPier->GetOrientation(),&skew_angle_1);

   Float64 skew_angle_2;
   pBridge->GetSkewAngle(pNextPier->GetStation(),pNextPier->GetOrientation(),&skew_angle_2);

   skew1 = skew_angle_1;
   skew2 = skew_angle_2;
}

BOOL CSpanGirderLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   Float64 skew_angle_1, skew_angle_2;
   GetPierSkewAngles(skew_angle_1,skew_angle_2);

   // set up the girder name grid
	m_GirderNameGrid.SubclassDlgItem(IDC_GIRDERGRID, this);
   m_GirderNameGrid.CustomInit(pParent->m_pGirderGroup);
   m_GirderTypeCache = pParent->m_pGirderGroup->GetGirderTypeGroups();


   // set up the previous pier girder spacing input
   m_SpacingGrid[pgsTypes::metStart].SubclassDlgItem(IDC_PREV_PIER_SPACING_GRID,this);
   m_SpacingGrid[pgsTypes::metStart].CustomInit();
   m_SpacingGrid[pgsTypes::metStart].SetPierSkewAngle(skew_angle_1);


   // set up the next pier girder spacing input
   m_SpacingGrid[pgsTypes::metEnd].SubclassDlgItem(IDC_NEXT_PIER_SPACING_GRID,this);
   m_SpacingGrid[pgsTypes::metEnd].CustomInit();
   m_SpacingGrid[pgsTypes::metEnd].SetPierSkewAngle(skew_angle_2);

   if (IsTopWidthSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()))
   {
      m_TopWidthGrid.SubclassDlgItem(IDC_TOP_WIDTH_GRID, this);
      m_TopWidthGrid.CustomInit(pParent->m_pGirderGroup);
      m_TopWidthCache = pParent->m_pGirderGroup->GetGirderTopWidthGroups();
   }

   // Fill up the combo boxes for how girder spacing is measured
   Float64 offset;
   ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
   pParent->m_pPrevPier->GetBearingOffset(pgsTypes::Ahead, &offset,&start_measure,true);
   pParent->m_pNextPier->GetBearingOffset(pgsTypes::Back,  &offset,&end_measure,true);
   FillGirderSpacingMeasurementComboBox(IDC_PREV_PIER_GIRDER_SPACING_MEASURE,pgsTypes::metStart,start_measure);
   FillGirderSpacingMeasurementComboBox(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE,pgsTypes::metEnd,  end_measure);

   // Fill up combo boxes for locating the reference girder
   FillRefGirderComboBox(pgsTypes::metStart);
   FillRefGirderComboBox(pgsTypes::metEnd);

   FillRefGirderOffsetTypeComboBox(pgsTypes::metStart);
   FillRefGirderOffsetTypeComboBox(pgsTypes::metEnd);

   CPropertyPage::OnInitDialog();

   bool bUseSameNumGirders = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups();
   CComboBox* pcbNG  = (CComboBox*)GetDlgItem(IDC_NGDRS_COMBO);
   pcbNG->AddString(_T("The same number of girders are used in all spans"));
   pcbNG->AddString(_T("The number of girders is defined span by span"));
   pcbNG->SetCurSel(bUseSameNumGirders ? 0:1);

   bool bUseSameGirderType = pParent->m_BridgeDesc.UseSameGirderForEntireBridge();
   CComboBox* pcbGName  = (CComboBox*)GetDlgItem(IDC_GDR_NAME_COMBO);
   pcbGName->AddString(_T("This type of girder is used in all spans"));
   pcbGName->AddString(_T("Girder types are defined span by span"));
   pcbGName->SetCurSel(bUseSameGirderType ? 0:1);

   // Initialize the spinner control for # of girder lines
   m_MinGirderCount = GetMinGirderCount();
   m_NumGdrSpinner.SetRange(short(m_MinGirderCount),MAX_GIRDERS_PER_SPAN);

   // restrict the spinner to only go one girder at a timeToggle
   UDACCEL accel;
   accel.nInc = 1;
   accel.nSec = 0;
   m_NumGdrSpinner.SetAccel(0,&accel);

   pgsTypes::SupportedBeamSpacing spacingType = pParent->m_BridgeDesc.GetGirderSpacingType();

   if ( spacingType == pgsTypes::sbsUniform )
   {
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("The same girder spacing is used in all spans")), (DWORD_PTR)(pgsTypes::sbsUniform));
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("Girder spacing is defined span by span")), (DWORD_PTR)(pgsTypes::sbsConstantAdjacent));
      m_cbGirderSpacingType.SetCurSel(0);
   }
   else if ( spacingType == pgsTypes::sbsUniformAdjacent )
   {
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("The same joint spacing is used in all spans")), (DWORD_PTR)(pgsTypes::sbsUniformAdjacent));
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("Joint spacing is defined span by span")), (DWORD_PTR)(pgsTypes::sbsGeneralAdjacent));
      m_cbGirderSpacingType.SetCurSel(0);
   }
   else if ( spacingType == pgsTypes::sbsConstantAdjacent )
   {
      CString note;
      note.Format(_T("The same girder spacing must be used for the entire bridge for %s girders"), pParent->m_BridgeDesc.GetGirderFamilyName());
      m_cbGirderSpacingType.AddString(note);
      m_cbGirderSpacingType.SetCurSel(0);
      m_cbGirderSpacingType.EnableWindow(FALSE);
   }
   else if ( spacingType == pgsTypes::sbsGeneral )
   {
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("The same girder spacing is used in all spans")), (DWORD_PTR)(pgsTypes::sbsUniform));
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("Girder spacing is defined span by span")), (DWORD_PTR)(pgsTypes::sbsGeneral));
      m_cbGirderSpacingType.SetCurSel(1);
   }
   else if ( spacingType == pgsTypes::sbsGeneralAdjacent )
   {
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("The same joint spacing is used in all spans")), (DWORD_PTR)(pgsTypes::sbsConstantAdjacent));
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("Joint spacing is defined span by span")), (DWORD_PTR)(pgsTypes::sbsConstantAdjacent));
      m_cbGirderSpacingType.SetCurSel(1);
   }
   else if (spacingType == pgsTypes::sbsUniformAdjacentWithTopWidth || spacingType == pgsTypes::sbsGeneralAdjacentWithTopWidth)
   {
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("The same top flange width and joint spacing is used in all spans")), (DWORD_PTR)(pgsTypes::sbsUniformAdjacentWithTopWidth));
      m_cbGirderSpacingType.SetItemData(m_cbGirderSpacingType.AddString(_T("Top flange width and joint spacing is defined span by span")), (DWORD_PTR)(pgsTypes::sbsGeneralAdjacentWithTopWidth));
      m_cbGirderSpacingType.SetCurSel(spacingType == pgsTypes::sbsUniformAdjacentWithTopWidth ? 0 : 1);
   }
   else
   {
      ATLASSERT(false); // is there a new spacing type???
   }

   if ( IsGirderSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) )
   {
      GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
      GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Girder Spacing"));
   }
   else
   {
      GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
      GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->SetWindowText(_T("Joint Spacing"));
   }
  
   UpdateChildWindowState();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanGirderLayoutPage::OnNumGirdersChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

   int nGirders = pNMUpDown->iPos + pNMUpDown->iDelta;
   if ( nGirders < int(m_MinGirderCount) )
   {
      // rolling past the bottom... going back up to MAX_GIRDERS_PER_SPAN
      AddGirders( GirderIndexType(MAX_GIRDERS_PER_SPAN - pNMUpDown->iPos) );
   }
   else if ( MAX_GIRDERS_PER_SPAN < nGirders )
   {
      // rolling past the top... going back down to minimum number of girders
      RemoveGirders( GirderIndexType(pNMUpDown->iPos - m_MinGirderCount) );
   }
   else
   {
	   if ( pNMUpDown->iDelta < 0 )
      {
         RemoveGirders(GirderIndexType(-pNMUpDown->iDelta));
      }
      else
      {
         AddGirders(GirderIndexType(pNMUpDown->iDelta));
      }
   }

   // Update the girder spacing cache so that it is consistent with the current
   // number of girders
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   m_GirderSpacingCache[pgsTypes::metStart] = *(pParent->m_pGirderGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead));
   m_GirderSpacingCache[pgsTypes::metEnd]   = *(pParent->m_pGirderGroup->GetPier(pgsTypes::metEnd  )->GetGirderSpacing(pgsTypes::Back ));

   m_GirderSpacingCache[pgsTypes::metStart].SetPier(nullptr);
   m_GirderSpacingCache[pgsTypes::metStart].SetTemporarySupport(nullptr);
   m_GirderSpacingCache[pgsTypes::metEnd].SetPier(nullptr);
   m_GirderSpacingCache[pgsTypes::metEnd].SetTemporarySupport(nullptr);

   FillRefGirderComboBox(pgsTypes::metStart);
   FillRefGirderComboBox(pgsTypes::metEnd);

   *pResult = 0;
}

void CSpanGirderLayoutPage::AddGirders(GirderIndexType nGirders)
{
   m_nGirders += nGirders;

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pParent->m_pGirderGroup->AddGirders(nGirders);

   m_GirderNameGrid.UpdateGrid();
   m_SpacingGrid[pgsTypes::metStart].UpdateGrid();
   m_SpacingGrid[pgsTypes::metEnd].UpdateGrid();
   m_TopWidthGrid.UpdateGrid();

   UpdateGirderSpacingState();
   UpdateGirderTopWidthState();
}

void CSpanGirderLayoutPage::RemoveGirders(GirderIndexType nGirders)
{
   m_nGirders -= nGirders;

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   pParent->m_pGirderGroup->RemoveGirders(nGirders);

   m_GirderNameGrid.UpdateGrid();
   m_SpacingGrid[pgsTypes::metStart].UpdateGrid();
   m_SpacingGrid[pgsTypes::metEnd].UpdateGrid();
   m_TopWidthGrid.UpdateGrid();

   UpdateGirderSpacingState();
   UpdateGirderTopWidthState();
}

void CSpanGirderLayoutPage::FillGirderSpacingMeasurementComboBox(int nIDC, pgsTypes::MemberEndType end,ConnectionLibraryEntry::BearingOffsetMeasurementType bearingMeasure)
{
   CComboBox* pSpacingType = (CComboBox*)GetDlgItem(nIDC);
   pSpacingType->ResetContent();

   int idx = pSpacingType->AddString(IsAbutment(end) ? _T("Measured at and along abutment line") : _T("Measured at and along the pier line"));
   DWORD item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::AlongItem);
   pSpacingType->SetItemData(idx,item_data);
   
   idx = pSpacingType->AddString(IsAbutment(end) ? _T("Measured normal to alignment at abutment line") : _T("Measured normal to alignment at pier line"));
   item_data = HashGirderSpacing(pgsTypes::AtPierLine,pgsTypes::NormalToItem);
   pSpacingType->SetItemData(idx,item_data);
   
   if (bearingMeasure != ConnectionLibraryEntry::AlongGirder)
   {
      idx = pSpacingType->AddString(_T("Measured at and along the CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::AlongItem);
      pSpacingType->SetItemData(idx,item_data);

      idx = pSpacingType->AddString(_T("Measured normal to alignment at CL bearing"));
      item_data = HashGirderSpacing(pgsTypes::AtCenterlineBearing,pgsTypes::NormalToItem);
      pSpacingType->SetItemData(idx,item_data);
   }
}

void CSpanGirderLayoutPage::FillRefGirderOffsetTypeComboBox(pgsTypes::MemberEndType end)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(end == pgsTypes::metStart ? IDC_PREV_REF_GIRDER_OFFSET_TYPE : IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   int idx = pCB->AddString(_T("Alignment"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtAlignment);

   idx = pCB->AddString(_T("Bridge Line"));
   pCB->SetItemData(idx,(DWORD)pgsTypes::omtBridge);
}

void CSpanGirderLayoutPage::FillRefGirderComboBox(pgsTypes::MemberEndType end)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(end == pgsTypes::metStart ? IDC_PREV_REF_GIRDER : IDC_NEXT_REF_GIRDER);
   int curSel = pCB->GetCurSel();
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Center of Girders"));
   pCB->SetItemData(idx,INVALID_INDEX);

   for ( GirderIndexType i = 0; i < m_nGirders; i++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(i));
      idx = pCB->AddString(str);
      pCB->SetItemData(idx,(DWORD)i);
   }

   if ( pCB->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
   {
      pCB->SetCurSel(0);
   }
}

void CSpanGirderLayoutPage::OnPrevPierGirderSpacingMeasureChanged() 
{
   CComboBox* pPrevCB = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   int cursel = pPrevCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pPrevCB->GetItemData(cursel),&ml,&mt);

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   CGirderSpacing2* pStartSpacing = pParent->m_pPrevPier->GetGirderSpacing(pgsTypes::Ahead);
   pStartSpacing->SetMeasurementType(mt);
   pStartSpacing->SetMeasurementLocation(ml);

   m_SpacingGrid[pgsTypes::metStart].UpdateGrid();
}

void CSpanGirderLayoutPage::OnNextPierGirderSpacingMeasureChanged() 
{
   CComboBox* pNextCB = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   int cursel = pNextCB->GetCurSel();

   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(pNextCB->GetItemData(cursel),&ml,&mt);

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   CGirderSpacing2* pEndSpacing = pParent->m_pNextPier->GetGirderSpacing(pgsTypes::Back);
   pEndSpacing->SetMeasurementType(mt);
   pEndSpacing->SetMeasurementLocation(ml);

   m_SpacingGrid[pgsTypes::metEnd].UpdateGrid();
}

void CSpanGirderLayoutPage::GirderTypeChanged()
{
   m_SpacingGrid[pgsTypes::metStart].UpdateGrid();
   m_SpacingGrid[pgsTypes::metEnd].UpdateGrid();
}

GirderIndexType CSpanGirderLayoutPage::GetMinGirderCount()
{
   CComPtr<IBeamFactory> factory;
   GetBeamFactory(&factory);
   return factory->GetMinimumBeamCount();
}

void CSpanGirderLayoutPage::OnChangeSameGirderSpacing()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   if (pParent->m_BridgeDesc.GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent)
   {
      return;
   }

   // changing from uniform to general, or general to uniform spacing
   pgsTypes::SupportedBeamSpacing oldSpacingType = pParent->m_BridgeDesc.GetGirderSpacingType();

   pgsTypes::SupportedBeamSpacing spacingType = ToggleGirderSpacingType(oldSpacingType);

   if ( IsBridgeSpacing(spacingType) && spacingType != pgsTypes::sbsConstantAdjacent )
   {
      // we are going from general to uniform spacing
      // if the grid has more than one spacing, we need to ask the user which one is to be
      // used for the entire bridge

      CComboBox* pcbStartOfSpanSpacingDatum = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
      CComboBox* pcbEndOfSpanSpacingDatum   = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
      m_CacheGirderSpacingMeasure[pgsTypes::metStart] = (DWORD)pcbStartOfSpanSpacingDatum->GetItemData( pcbStartOfSpanSpacingDatum->GetCurSel() );
      m_CacheGirderSpacingMeasure[pgsTypes::metEnd]   = (DWORD)pcbEndOfSpanSpacingDatum->GetItemData( pcbEndOfSpanSpacingDatum->GetCurSel() );

      // determine if there is more than one unique spacing value
      std::set<Float64> spacings;
      Float64 bridgeSpacing = 0;
      long datum = 0;
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);
         pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)end;

         CGirderSpacing2* pGirderSpacing = pParent->m_pSpanData->GetPier(end)->GetGirderSpacing(face);
         m_GirderSpacingCache[end] = *pGirderSpacing; // update the cache before changing values
         m_GirderSpacingCache[end].SetPier(nullptr);
         m_GirderSpacingCache[end].SetTemporarySupport(nullptr);

         GroupIndexType nSpacingGroups = pGirderSpacing->GetSpacingGroupCount();
         for ( GroupIndexType spaIdx = 0; spaIdx < nSpacingGroups;spaIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            Float64 space;
            pGirderSpacing->GetSpacingGroup(spaIdx,&firstGdrIdx,&lastGdrIdx,&space);
            spacings.insert( space );
         }
      }

      if ( 1 < spacings.size() || (m_CacheGirderSpacingMeasure[pgsTypes::metStart] != m_CacheGirderSpacingMeasure[pgsTypes::metEnd]))
      {
         // there is more than one unique girder spacing... which one do we want to use
         // for the entire bridge???
         CComPtr<IBroker> broker;
         EAFGetBroker(&broker);
         GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);

         CResolveGirderSpacingDlg dlg;
         CString strItems;
         std::set<Float64>::iterator begin(spacings.begin());
         std::set<Float64>::iterator iter(begin);
         std::set<Float64>::iterator end(spacings.end());
         for ( ; iter != end; iter++ )
         {
            Float64 spacing = *iter;

            CString strItem;
            if (IsGirderSpacing(oldSpacingType))
            {
               strItem.Format(_T("%s"), FormatDimension(spacing, pDisplayUnits->GetXSectionDimUnit(), true));
            }
            else
            {
               strItem.Format(_T("%s"), FormatDimension(spacing, pDisplayUnits->GetComponentDimUnit(), true));
            }

            if (iter != begin)
            {
               strItems += _T("\n");
            }

            strItems += strItem;
         }

         // check connections to see what spacing locations we can use
         Float64 offset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType start_measure, end_measure;
         pParent->m_pPrevPier->GetBearingOffset(pgsTypes::Ahead,&offset,&start_measure,true);
         pParent->m_pNextPier->GetBearingOffset(pgsTypes::Back, &offset,&end_measure,true);
         dlg.m_RestrictSpacing = start_measure==ConnectionLibraryEntry::AlongGirder || end_measure==ConnectionLibraryEntry::AlongGirder;

         dlg.m_strSpacings = strItems;
         dlg.m_MeasurementDatum = 0;

         if ( dlg.DoModal() == IDOK )
         {
            iter = begin;
            for (int i = 0; i < dlg.m_ItemIdx; i++)
            {
               iter++;
            }

            bridgeSpacing = *iter;

            datum = dlg.m_MeasurementDatum;
         }
         else
         {
            // use pressed Cancel button... cancel the entire operation
            UpdateChildWindowState();
            return;
         }
      }
      else
      {
         // there is only one unique spacing value.. get it
         CGirderSpacing2* pGirderSpacing = pParent->m_pSpanData->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
         bridgeSpacing = pGirderSpacing->GetGirderSpacing(0);
         datum = m_CacheGirderSpacingMeasure[pgsTypes::metStart];
      }

      pParent->m_BridgeDesc.SetGirderSpacing(bridgeSpacing);

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, datum);
      DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, datum);


      ////////////
      std::vector<CGirderTopWidthGroup> topWidths;
      CGirderTopWidthGroup bridgeTopWidth;
      m_TopWidthCache = pParent->m_pGirderGroup->GetGirderTopWidthGroups();
      GroupIndexType nTopWidthGroups = pParent->m_pGirderGroup->GetGirderTopWidthGroupCount();
      for (GroupIndexType grpIdx = 0; grpIdx < nTopWidthGroups; grpIdx++)
      {
         topWidths.push_back(pParent->m_pGirderGroup->GetGirderTopWidthGroup(grpIdx));
      }
      auto iter = std::unique(std::begin(topWidths), std::end(topWidths), [](const auto& a, const auto& b) 
      { 
         bool bSameType = a.type == b.type;
         bool bSameLeft = IsEqual(a.left[pgsTypes::metStart], b.left[pgsTypes::metStart]) && IsEqual(a.left[pgsTypes::metEnd], b.left[pgsTypes::metEnd]);
         bool bSameRight(a.type != pgsTypes::twtAsymmetric ? true : IsEqual(a.right[pgsTypes::metStart], b.right[pgsTypes::metStart]) && IsEqual(a.right[pgsTypes::metEnd], b.right[pgsTypes::metEnd]));
         return bSameType && bSameLeft && bSameRight;
      }
      );
      topWidths.erase(iter, std::end(topWidths));

      if (1 < topWidths.size())
      {
         // there is more than one top widths... which one do we want to use for the entire bridge?
         CComPtr<IBroker> broker;
         EAFGetBroker(&broker);
         GET_IFACE2(broker, IEAFDisplayUnits, pDisplayUnits);

         CComPtr<IBeamFactory> factory;
         GetBeamFactory(&factory);

         CResolveGirderSpacingDlg dlg;
         CString strItems;
         auto begin(topWidths.begin());
         auto iter(begin);
         auto end(topWidths.end());
         for (; iter != end; iter++)
         {
            const auto& topWidth(*iter);

            CString strItem;
            if (topWidth.type == pgsTypes::twtAsymmetric)
            {
               if (factory->CanTopWidthVary())
               {
                  strItem.Format(_T("Left %s//Right %s\nLeft %s//Right %s"), FormatDimension(topWidth.left[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit(), true), FormatDimension(topWidth.right[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit(), true), FormatDimension(topWidth.left[pgsTypes::metEnd], pDisplayUnits->GetXSectionDimUnit(), true), FormatDimension(topWidth.right[pgsTypes::metEnd], pDisplayUnits->GetXSectionDimUnit(), true));
               }
               else
               {
                  strItem.Format(_T("Left %s//Right %s"), FormatDimension(topWidth.left[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit(), true), FormatDimension(topWidth.right[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit(), true));
               }
            }
            else
            {
               if (factory->CanTopWidthVary())
               {
                  strItem.Format(_T("%s\n%s"), FormatDimension(topWidth.left[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit(), true), FormatDimension(topWidth.left[pgsTypes::metEnd], pDisplayUnits->GetXSectionDimUnit(), true));
               }
               else
               {
                  strItem.Format(_T("%s"), FormatDimension(topWidth.left[pgsTypes::metStart], pDisplayUnits->GetXSectionDimUnit(), true));
               }
            }

            if (iter != begin)
            {
               strItems += _T("\n");
            }

            strItems += strItem;
         }

         int result = AfxChoose(_T("Multiple Top Flange Widths"), _T("There are multiple top widths. Select one to use for all girders in the bridge."), strItems, 0, TRUE);
         if ( result != -1)
         {
            iter = begin;
            iter += result;
            bridgeTopWidth = *iter;
         }
         else
         {
            // use pressed Cancel button... cancel the entire operation
            UpdateChildWindowState();
            return;
         }
      }
      else
      {
         // there is only one unique top width value.. get it
         pParent->m_pGirderGroup->GetGirder(0)->GetTopWidth(&bridgeTopWidth.type, &bridgeTopWidth.left[pgsTypes::metStart], &bridgeTopWidth.right[pgsTypes::metStart], &bridgeTopWidth.left[pgsTypes::metEnd], &bridgeTopWidth.right[pgsTypes::metEnd]);
      }

      pParent->m_BridgeDesc.SetGirderTopWidth(bridgeTopWidth.type, bridgeTopWidth.left[pgsTypes::metStart], bridgeTopWidth.right[pgsTypes::metStart]);
   }
   else
   {
      // restore the girder spacing from the cached values
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);
         pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)end;

         pParent->m_pSpanData->GetPier(end)->SetGirderSpacing(face,m_GirderSpacingCache[end]);
      }

      CDataExchange dx(this,FALSE);
      DDX_CBItemData(&dx, IDC_PREV_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::metStart]);
      DDX_CBItemData(&dx, IDC_NEXT_PIER_GIRDER_SPACING_MEASURE, m_CacheGirderSpacingMeasure[pgsTypes::metEnd]);

      if (IsTopWidthSpacing(spacingType))
      {
         pParent->m_pGirderGroup->SetGirderTopWidthGroups(m_TopWidthCache);
      }
   }

   pParent->m_BridgeDesc.SetGirderSpacingType(spacingType);
   m_SpacingGrid[pgsTypes::metStart].UpdateGrid();
   m_SpacingGrid[pgsTypes::metEnd].UpdateGrid();

   if (IsTopWidthSpacing(spacingType))
   {
      m_TopWidthGrid.UpdateGrid();
   }

   UpdateChildWindowState();
}

void CSpanGirderLayoutPage::OnChangeSameGirderType()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   bool bUseSameGirderType = pParent->m_BridgeDesc.UseSameGirderForEntireBridge();
   bUseSameGirderType = !bUseSameGirderType; // toggle the current value.. will set it back on the bridge at the end of this method

   if ( bUseSameGirderType )
   {
      // cache the current grid data
      m_GirderTypeCache = pParent->m_pGirderGroup->GetGirderTypeGroups();

      // if the grid has more than one girder type, we need to ask the user which one is to be
      // used for the entire bridge
      GroupIndexType nGirderTypeGroups = pParent->m_pGirderGroup->GetGirderTypeGroupCount();
      std::_tstring strBridgeGirderName;
      if ( 1 < nGirderTypeGroups )
      {
         // there is more than one group... get all the unique girder names
         std::set<std::_tstring> girderNames;
         for ( GroupIndexType girderGroupTypeIdx = 0; girderGroupTypeIdx < nGirderTypeGroups; girderGroupTypeIdx++ )
         {
            GirderIndexType firstGdrIdx, lastGdrIdx;
            std::_tstring strGirderName;
            pParent->m_pGirderGroup->GetGirderTypeGroup(girderGroupTypeIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);
            girderNames.insert(strGirderName);
         }

         if ( 1 < girderNames.size() )
         {
            // there is more than one unique girder type... which one do we want to use
            // for the entire bridge???
            CSelectItemDlg dlg;
            dlg.m_strLabel = _T("Several different types of girders are used in this span. Select the girder that will be used for the entire bridge");
            dlg.m_strTitle = _T("Select Girder");
            dlg.m_ItemIdx = 0;

            CString strItems;
            std::set<std::_tstring>::iterator begin(girderNames.begin());
            std::set<std::_tstring>::iterator iter(begin);
            std::set<std::_tstring>::iterator end(girderNames.end());
            for ( ; iter != end; iter++ )
            {
               std::_tstring strGirderName = *iter;

               if ( iter != begin )
               {
                  strItems += _T("\n");
               }

               strItems += CString(strGirderName.c_str());
            }

            dlg.m_strItems = strItems;
            if ( dlg.DoModal() == IDOK )
            {
               iter = begin;
               for ( IndexType i = 0; i < dlg.m_ItemIdx; i++ )
               {
                  iter++;
               }

               strBridgeGirderName = *iter;
            }
            else
            {
               // use pressed Cancel button... cancel the entire operation
               UpdateChildWindowState();
               return;
            }
         }
         else
         {
            strBridgeGirderName = pParent->m_pGirderGroup->GetGirderName(0);
         }
      }
      else
      {
         strBridgeGirderName = pParent->m_pGirderGroup->GetGirderName(0);
      }

      // set the girder name value at the bridge level
      pParent->m_BridgeDesc.SetGirderName(strBridgeGirderName.c_str());
   }
   else
   {
      // restore the cached values
      pParent->m_pGirderGroup->SetGirderTypeGroups(m_GirderTypeCache);
   }

   pParent->m_BridgeDesc.UseSameGirderForEntireBridge(bUseSameGirderType);
   m_GirderNameGrid.UpdateGrid();

   UpdateChildWindowState();
}

BOOL CSpanGirderLayoutPage::OnSetActive() 
{
   UpdateChildWindowState();
	BOOL bResult = CPropertyPage::OnSetActive();

   return bResult;
}

void CSpanGirderLayoutPage::UpdateChildWindowState()
{
   UpdateGirderNumState();
   UpdateGirderTypeState();
   UpdateGirderSpacingState();
   UpdateGirderTopWidthState();
}

void CSpanGirderLayoutPage::UpdateGirderTypeState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   bool bUseSameGirderType = pParent->m_BridgeDesc.UseSameGirderForEntireBridge();

   m_GirderNameGrid.Enable(bUseSameGirderType ? FALSE:TRUE);
}

void CSpanGirderLayoutPage::UpdateGirderNumState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   BOOL bEnable = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups() ? FALSE:TRUE;

   GetDlgItem(IDC_NUMGDR_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_NUMGDR_SPIN)->EnableWindow(bEnable);
   GetDlgItem(IDC_NUMGDR)->EnableWindow(bEnable);
}

void CSpanGirderLayoutPage::UpdateGirderSpacingState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   BOOL bPrevEnable = m_SpacingGrid[pgsTypes::metStart].InputSpacing();
   BOOL bNextEnable = m_SpacingGrid[pgsTypes::metEnd].InputSpacing();

   if (m_nGirders == 1 || IsBridgeSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()))
   {
      // if there is only 1 girder or we are inputting spacing for the whole bridge
      // (not span by span) then disable the input controls
      bPrevEnable = FALSE;
      bNextEnable = FALSE;
   }

   // Prev Pier
   GetDlgItem(IDC_PREV_PIER_LABEL)->EnableWindow(bPrevEnable);
   GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_LABEL)->EnableWindow(bPrevEnable);
   GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(bPrevEnable);
   m_SpacingGrid[pgsTypes::metStart].Enable(bPrevEnable);

   // Next Pier
   GetDlgItem(IDC_NEXT_PIER_LABEL)->EnableWindow(bNextEnable);
   GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_LABEL)->EnableWindow(bNextEnable);
   GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE)->EnableWindow(bNextEnable);
   m_SpacingGrid[pgsTypes::metEnd].Enable(bNextEnable);

   if (pParent->m_BridgeDesc.GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent)
   {
      GetDlgItem(IDC_GDR_SPC_TYPE_COMBO)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_GDR_SPC_TYPE_COMBO)->EnableWindow(m_nGirders == 1 ? FALSE : TRUE);
   }


   BOOL bEnable = IsSpanSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) ? TRUE : FALSE;

   GetDlgItem(IDC_COPY_TO_END)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_FROM)->EnableWindow(bEnable);
   GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);

   GetDlgItem(IDC_COPY_TO_START)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_UNIT)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_FROM)->EnableWindow(bEnable);
   GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE)->EnableWindow(bEnable);
}

void CSpanGirderLayoutPage::UpdateGirderTopWidthState()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if (IsTopWidthSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()))
   {
      BOOL bEnable = IsSpanSpacing(pParent->m_BridgeDesc.GetGirderSpacingType()) ? TRUE : FALSE;
      GetDlgItem(IDC_TOP_WIDTH_GRID_LABEL)->EnableWindow(bEnable);
      GetDlgItem(IDC_TOP_WIDTH_GRID_LABEL)->ShowWindow(SW_SHOW);
      m_TopWidthGrid.ShowWindow(SW_SHOW);
      m_TopWidthGrid.Enable(bEnable);
   }
   else
   {
      GetDlgItem(IDC_TOP_WIDTH_GRID_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_TOP_WIDTH_GRID)->ShowWindow(SW_HIDE);
      //m_TopWidthGrid.ShowWindow(SW_HIDE);
   }
   
}

void CSpanGirderLayoutPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_SPANDETAILS_GIRDERSPACING );
}

bool CSpanGirderLayoutPage::IsAbutment(pgsTypes::MemberEndType end)
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   return pParent->m_pSpanData->GetPier(end)->IsAbutment();
}

void CSpanGirderLayoutPage::OnCopySpacingToEnd()
{
   // Copy girder spacing from the start of the span to the end of the span
   // Prev Pier, Ahead side --> Next Pier, Back side
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   // Girder spacing measurement type
   CComboBox* pcbBackMeasure  = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   CComboBox* pcbAheadMeasure = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   pcbBackMeasure->SetCurSel(pcbAheadMeasure->GetCurSel());

   // Spacing Grid
   CGirderSpacing2* pSpacing = pParent->m_pSpanData->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
   pParent->m_pSpanData->GetPier(pgsTypes::metEnd)->SetGirderSpacing(pgsTypes::Back,*pSpacing);
   m_SpacingGrid[pgsTypes::metEnd].UpdateGrid();

   // Spacing Location

   // reference girder index
   CComboBox* pcbBackRefGirder  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER);
   CComboBox* pcbAheadRefGirder = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER);
   pcbAheadRefGirder->SetCurSel(pcbBackRefGirder->GetCurSel());

   // reference girder offset
   CWnd* pwndBackRefGirderOffset  = GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET);
   CWnd* pwndAheadRefGirderOffset = GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET);
   CString strText;
   pwndBackRefGirderOffset->GetWindowText(strText);
   pwndAheadRefGirderOffset->SetWindowText(strText);

   // reference girder offset type
   CComboBox* pcbBackRefGirderOffsetType  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE);
   CComboBox* pcbAheadRefGirderOffsetType = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   pcbAheadRefGirderOffsetType->SetCurSel(pcbBackRefGirderOffsetType->GetCurSel());
}

void CSpanGirderLayoutPage::OnCopySpacingToStart()
{
   // Copy girder spacing from the end of the span to the start of the span
   // Next Pier, Back side --> Prev Pier, Ahead side
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   // Girder spacing measurement type
   CComboBox* pcbBackMeasure  = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_GIRDER_SPACING_MEASURE);
   CComboBox* pcbAheadMeasure = (CComboBox*)GetDlgItem(IDC_PREV_PIER_GIRDER_SPACING_MEASURE);
   pcbAheadMeasure->SetCurSel(pcbBackMeasure->GetCurSel());

   // Spacing Grid
   CGirderSpacing2* pSpacing = pParent->m_pSpanData->GetPier(pgsTypes::metEnd)->GetGirderSpacing(pgsTypes::Back);
   pParent->m_pSpanData->GetPier(pgsTypes::metStart)->SetGirderSpacing(pgsTypes::Ahead,*pSpacing);
   m_SpacingGrid[pgsTypes::metStart].UpdateGrid();

   // Spacing Location

   // reference girder index
   CComboBox* pcbBackRefGirder  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER);
   CComboBox* pcbAheadRefGirder = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER);
   pcbBackRefGirder->SetCurSel(pcbAheadRefGirder->GetCurSel());

   // reference girder offset
   CWnd* pwndBackRefGirderOffset  = GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET);
   CWnd* pwndAheadRefGirderOffset = GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET);
   CString strText;
   pwndAheadRefGirderOffset->GetWindowText(strText);
   pwndBackRefGirderOffset->SetWindowText(strText);

   // reference girder offset type
   CComboBox* pcbBackRefGirderOffsetType  = (CComboBox*)GetDlgItem(IDC_PREV_REF_GIRDER_OFFSET_TYPE);
   CComboBox* pcbAheadRefGirderOffsetType = (CComboBox*)GetDlgItem(IDC_NEXT_REF_GIRDER_OFFSET_TYPE);
   pcbBackRefGirderOffsetType->SetCurSel(pcbAheadRefGirderOffsetType->GetCurSel());
}

void CSpanGirderLayoutPage::OnCbnSelchangeNgdrsCombo()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   bool bUseSameNumGirders = pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups();
   pParent->m_BridgeDesc.UseSameNumberOfGirdersInAllGroups(!bUseSameNumGirders);
   pParent->m_BridgeDesc.SetGirderCount(m_nGirders);

   UpdateChildWindowState();
}

void CSpanGirderLayoutPage::GetBeamFactory(IBeamFactory** ppFactory)
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   const CGirderGroupData* pGroup = pParent->m_pGirderGroup;

   // It is ok to use girder 0 here because all the girders within the group
   // are of the same family. All the girders in the span will have the
   // same factory
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(0);

   pGdrEntry->GetBeamFactory(ppFactory);
}
