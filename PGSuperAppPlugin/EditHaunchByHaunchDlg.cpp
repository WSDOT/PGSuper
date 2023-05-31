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

// EditHaunchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "EditHaunchByHaunchDlg.h"
#include "EditHaunchDlg.h"

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include "PGSuperUnits.h"
#include "PGSuperDoc.h"
#include "Utilities.h"

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\ClosureJointData.h>

#include <IFace\Project.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline CString GetHaunchInputLocationTypeAsString(pgsTypes::HaunchInputLocationType locType,pgsTypes::HaunchLayoutType layoutType,bool bIsPGSuper)
{
   bool bForSpans = bIsPGSuper || layoutType == pgsTypes::hltAlongSpans;

   if (locType == pgsTypes::hilSame4Bridge)
   {
      return _T("Same for entire Bridge");
   }
   else if (locType == pgsTypes::hilSame4AllGirders)
   {
      return bForSpans ? _T("Same in each Span") : _T("Same in each Segment");
   }
   else if (locType == pgsTypes::hilPerEach)
   {
      return bForSpans ? _T("Unique for each Girder") : _T("Unique for each Segment");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad HaunchInputDepthType type");
   }
}

inline CString GetHaunchInputDistributionTypeAsString(pgsTypes::HaunchInputDistributionType type)
{
   if (type == pgsTypes::hidUniform)
   {
      return _T("Uniformly");
   }
   else if (type == pgsTypes::hidAtEnds)
   {
      return _T("Linearly Between Ends");
   }
   else if (type == pgsTypes::hidParabolic)
   {
      return _T("Parabolically");
   }
   else if (type == pgsTypes::hidQuarterPoints)
   {
      return _T("Linearly Between 1/4th points");
   }
   else if (type == pgsTypes::hidTenthPoints)
   {
      return _T("Linearly Between 1/10th points");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad HaunchInputDistributionType type");
   }
}


IMPLEMENT_DYNAMIC(CEditHaunchByHaunchDlg,CDialog)

CEditHaunchByHaunchDlg::CEditHaunchByHaunchDlg(CWnd* pParent /*=nullptr*/)
   : CDialog(CEditHaunchByHaunchDlg::IDD,pParent),
   m_pGrid(nullptr),m_pHaunchDirectSegmentGrid(nullptr),m_pHaunchDirectSpansGrid(nullptr),m_pHaunchDirectSameAsSpansGrid(nullptr),m_pHaunchDirectSameAsSegmentsGrid(nullptr),m_pHaunchEntireBridgeGrid(nullptr)
{
}

CEditHaunchByHaunchDlg::~CEditHaunchByHaunchDlg()
{
}

void CEditHaunchByHaunchDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      // Only assign this on creation. They will be kept up to date by event handlers
      DDX_CBItemData(pDX,IDC_HAUNCH_INPUT_TYPE,m_HaunchInputLocationType);
      DDX_CBItemData(pDX,IDC_HAUNCH_DISTRIBUTION,m_HaunchInputDistributionType);
   }

   if (m_bNeedsGroupTabs && pgsTypes::hltAlongSegments == GetHaunchLayoutType() && m_HaunchInputLocationType != pgsTypes::hilSame4Bridge)
   {
      if (m_HaunchInputLocationType == pgsTypes::hilPerEach)
      {
         CGXTabBeam& beam = m_HaunchDirectSegmentTabWnd.GetBeam();
         auto nTabs = beam.GetCount();
         for (auto i = 0; i < nTabs; i++)
         {
            auto& tabInfo = beam.GetTab(i);
            CHaunchDirectSegmentGrid* pGrid = (CHaunchDirectSegmentGrid*)tabInfo.pExtra;
            if (FALSE == pGrid->UpdateData(pDX->m_bSaveAndValidate))
            {
               pDX->Fail();
            }
         }
      }
      else
      {
         CGXTabBeam& beam = m_HaunchDirectSameAsSegmentsTabWnd.GetBeam();
         auto nTabs = beam.GetCount();
         for (auto i = 0; i < nTabs; i++)
         {
            auto& tabInfo = beam.GetTab(i);
            CHaunchDirectSameAsGrid* pGrid = (CHaunchDirectSameAsGrid*)tabInfo.pExtra;
            if (FALSE == pGrid->UpdateData(pDX->m_bSaveAndValidate))
            {
               pDX->Fail();
            }
         }
      }
   }
   else
   {
      if (FALSE == m_pGrid->UpdateData(pDX->m_bSaveAndValidate))
      {
         pDX->Fail();
      }
   }
}

BEGIN_MESSAGE_MAP(CEditHaunchByHaunchDlg,CDialog)
   ON_CBN_SELCHANGE(IDC_HAUNCH_INPUT_TYPE,&CEditHaunchByHaunchDlg::OnHaunchInputTypeChanged)
   ON_CBN_SELCHANGE(IDC_HAUNCH_DISTRIBUTION,&CEditHaunchByHaunchDlg::OnHaunchInputDistributionTypeChanged)
   ON_BN_CLICKED(ID_HELP,&CEditHaunchByHaunchDlg::OnBnClickedHelp)
END_MESSAGE_MAP()

// CEditHaunchByHaunchDlg message handlers

BOOL CEditHaunchByHaunchDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // initialize units
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pUnit = &(pDisplayUnits->GetComponentDimUnit());

   CBridgeDescription2* pBridge = GetBridgeDesc();

   auto pDeck = pBridge->GetDeckDescription();
   if (pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP)
   {
      m_DeckThickness = pDeck->GrossDepth + pDeck->PanelDepth;
   }
   else
   {
      m_DeckThickness = pDeck->GrossDepth;
   }

   // Initialize our data structure for current data
   InitializeData();

   UpdateLocationTypeControl(bIsPGSuper);

   // HaunchInputDistributionType combo
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_HAUNCH_DISTRIBUTION);
   int sqidx = pBox->AddString(GetHaunchInputDistributionTypeAsString(pgsTypes::hidUniform));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hidUniform);
   sqidx = pBox->AddString(GetHaunchInputDistributionTypeAsString(pgsTypes::hidAtEnds));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hidAtEnds);
   sqidx = pBox->AddString(GetHaunchInputDistributionTypeAsString(pgsTypes::hidParabolic));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hidParabolic);
   sqidx = pBox->AddString(GetHaunchInputDistributionTypeAsString(pgsTypes::hidQuarterPoints));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hidQuarterPoints);
   sqidx = pBox->AddString(GetHaunchInputDistributionTypeAsString(pgsTypes::hidTenthPoints));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hidTenthPoints);

   CWnd* pPlaceBox = GetDlgItem(IDC_PLACEHOLDER);
   pPlaceBox->ShowWindow(SW_HIDE);
   CRect boxRect;
   pPlaceBox->GetWindowRect(&boxRect);
   ScreenToClient(boxRect);

   m_bNeedsGroupTabs = !bIsPGSuper && pBridge->GetGirderGroupCount() > 1;

   if (!m_bNeedsGroupTabs)
   {
      m_pHaunchDirectSegmentGrid = new CHaunchDirectSegmentGrid;
      m_pHaunchDirectSegmentGrid->SubclassDlgItem(IDC_HAUNCH_DIRECT_GRID,this);
      m_pHaunchDirectSegmentGrid->SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);
      m_pHaunchDirectSegmentGrid->CustomInit(0,this);
   }
   else
   {
      // In pgsplice we have tab per group for segment grid
      m_HaunchDirectSegmentTabWnd.SubclassDlgItem(IDC_HAUNCH_DIRECT_GRID,this);
      m_HaunchDirectSegmentTabWnd.SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);

      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
      {
         CString strLabel;
         strLabel.Format(_T("Group %d"),LABEL_GROUP(grpIdx));
         auto pGrid = std::make_unique<CHaunchDirectSegmentGrid>();
         pGrid->Create(0,CRect(0,0,1,1),&m_HaunchDirectSegmentTabWnd,m_HaunchDirectSegmentTabWnd.GetNextID());
         m_HaunchDirectSegmentTabWnd.AttachWnd(pGrid.get(),strLabel);
         pGrid->CustomInit(grpIdx,this);
         pGrid.release();
      }
   }

   m_pHaunchDirectSpansGrid = new CHaunchDirectSpansGrid;
   m_pHaunchDirectSpansGrid->SubclassDlgItem(IDC_HAUNCH_DIRECT_SPAN_GRID,this);
   m_pHaunchDirectSpansGrid->SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);
   m_pHaunchDirectSpansGrid->CustomInit();

   m_pHaunchDirectSameAsSpansGrid = new CHaunchDirectSameAsGrid;
   m_pHaunchDirectSameAsSpansGrid->SubclassDlgItem(IDC_HAUNCH_DIRECT_SAMEAS_GRID,this);
   m_pHaunchDirectSameAsSpansGrid->SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);
   m_pHaunchDirectSameAsSpansGrid->CustomInit(pgsTypes::hltAlongSpans,0,this);


   if (!m_bNeedsGroupTabs)
   {
      m_pHaunchDirectSameAsSegmentsGrid = new CHaunchDirectSameAsGrid;
      m_pHaunchDirectSameAsSegmentsGrid->SubclassDlgItem(IDC_HAUNCH_DIRECT_SAMEAS_SEGMENT_GRID,this);
      m_pHaunchDirectSameAsSegmentsGrid->SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);
      m_pHaunchDirectSameAsSegmentsGrid->CustomInit(pgsTypes::hltAlongSegments,0,this);
   }
   else
   {
      // In pgsplice we have tab per group for segment grid
      m_HaunchDirectSameAsSegmentsTabWnd.SubclassDlgItem(IDC_HAUNCH_DIRECT_SAMEAS_SEGMENT_GRID,this);
      m_HaunchDirectSameAsSegmentsTabWnd.SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);

      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
      {
         CString strLabel;
         strLabel.Format(_T("Group %d"),LABEL_GROUP(grpIdx));
         auto pGrid = std::make_unique<CHaunchDirectSameAsGrid>();
         pGrid->Create(0,CRect(0,0,1,1),&m_HaunchDirectSameAsSegmentsTabWnd,m_HaunchDirectSameAsSegmentsTabWnd.GetNextID());
         m_HaunchDirectSameAsSegmentsTabWnd.AttachWnd(pGrid.get(),strLabel);
         pGrid->CustomInit(pgsTypes::hltAlongSegments,grpIdx,this);
         pGrid.release();
      }
   }

   m_pHaunchEntireBridgeGrid = new CHaunchDirectEntireBridgeGrid;
   m_pHaunchEntireBridgeGrid->SubclassDlgItem(IDC_HAUNCH_DIRECT_ENTIRE_BRIDGE_GRID,this);
   m_pHaunchEntireBridgeGrid->SetWindowPos(GetDlgItem(IDC_PLACEHOLDER),boxRect.left,boxRect.top,boxRect.Width(),boxRect.Height(),SWP_SHOWWINDOW);
   m_pHaunchEntireBridgeGrid->CustomInit();

   UpdateActiveControls();

   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditHaunchByHaunchDlg::UpdateGroupBox()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString unitag = pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();
   pgsTypes::HaunchInputDepthType inputDepthType = GetHaunchInputDepthType();
   CString strDepthType = pgsTypes::hidHaunchDirectly == inputDepthType ? _T("Haunch") : _T("Haunch+Deck");

   CString boxtag;
   if (disttype == pgsTypes::hidUniform)
   {
      if (GetHaunchInputLocationType() == pgsTypes::hilSame4Bridge)
      {
         boxtag.Format(_T("Enter Uniform %s Depth Value for Entire Bridge (%s)"),strDepthType,unitag);
      }
      else
      {
         boxtag.Format(_T("Enter Uniform %s Depth Values (%s)"),strDepthType,unitag);
      }
   }
   else
   {
      CString segspan = GetHaunchLayoutType() == pgsTypes::hltAlongSegments ? _T("Segments") : _T("Spans");
      boxtag.Format(_T("Enter %s Depth Values (%s). Locations are fractional distances along %s"),strDepthType,unitag,segspan);
   }

   CWnd* pWnd = GetDlgItem(IDC_HAUNCH_UNIT);
   pWnd->SetWindowText(boxtag);
}

void CEditHaunchByHaunchDlg::UpdateLocationTypeControl(bool bIsPGSuper)
{
   pgsTypes::HaunchLayoutType layoutType = GetHaunchLayoutType();

   // HaunchInputDepthType combo
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_HAUNCH_INPUT_TYPE);
   int curIdx = pBox->GetCurSel();

   pBox->ResetContent();
   int sqidx = pBox->AddString(GetHaunchInputLocationTypeAsString(pgsTypes::hilSame4Bridge,layoutType,bIsPGSuper));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hilSame4Bridge);
   sqidx = pBox->AddString(GetHaunchInputLocationTypeAsString(pgsTypes::hilSame4AllGirders,layoutType,bIsPGSuper));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hilSame4AllGirders);
   sqidx = pBox->AddString(GetHaunchInputLocationTypeAsString(pgsTypes::hilPerEach,layoutType,bIsPGSuper));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::hilPerEach);

   pBox->SetCurSel(curIdx == CB_ERR ? 0 : curIdx);
}

void CEditHaunchByHaunchDlg::InitializeData()
{
   const CBridgeDescription2* pBridge = GetBridgeDesc();
   const CDeckDescription2* pDeck = pBridge->GetDeckDescription();

   ATLASSERT(pDeck->GetDeckType() != pgsTypes::sdtNone); // should not be able to edit haunch if no deck

   m_HaunchInputLocationType = pBridge->GetHaunchInputLocationType();
   m_HaunchInputDistributionType = pBridge->GetHaunchInputDistributionType();
}

pgsTypes::HaunchInputLocationType CEditHaunchByHaunchDlg::GetHaunchInputLocationType()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_HAUNCH_INPUT_TYPE);
   int curSel = pBox->GetCurSel();
   pgsTypes::HaunchInputLocationType type = (pgsTypes::HaunchInputLocationType)pBox->GetItemData(curSel);
   return type;
}

pgsTypes::HaunchLayoutType CEditHaunchByHaunchDlg::GetHaunchLayoutType()
{
   CEditHaunchDlg* pPa = (CEditHaunchDlg*)GetParent();
   pgsTypes::HaunchLayoutType type = pPa->GetHaunchLayoutType();
   return type;
}

pgsTypes::HaunchInputDistributionType CEditHaunchByHaunchDlg::GetHaunchInputDistributionType()
{
   return m_HaunchInputDistributionType;
}

pgsTypes::HaunchInputDepthType CEditHaunchByHaunchDlg::GetHaunchInputDepthType()
{
   CEditHaunchDlg* pPa = (CEditHaunchDlg*)GetParent();
   pgsTypes::HaunchInputDepthType type = pPa->GetHaunchInputDepthType();
   return type;
}

void CEditHaunchByHaunchDlg::UpdateCurrentData()
{
   try
   {
      UpdateData(TRUE);
   }
   catch (...)
   {
      return;
   }
}

void CEditHaunchByHaunchDlg::UpdateActiveControls()
{
   // 
   bool bShowSpans = GetHaunchLayoutType() == pgsTypes::hltAlongSpans;

   // First hide all grids
   m_pHaunchEntireBridgeGrid->ShowWindow(SW_HIDE);
   m_pHaunchDirectSpansGrid->ShowWindow(SW_HIDE);
   m_pHaunchDirectSameAsSpansGrid->ShowWindow(SW_HIDE);

   if (m_bNeedsGroupTabs)
   {
      m_HaunchDirectSegmentTabWnd.ShowWindow(SW_HIDE);
      m_HaunchDirectSameAsSegmentsTabWnd.ShowWindow(SW_HIDE);
   }
   else
   {
      m_pHaunchDirectSegmentGrid->ShowWindow(SW_HIDE);
      m_pHaunchDirectSameAsSegmentsGrid->ShowWindow(SW_HIDE);
   }

   // Validate and show grid in context
   switch (m_HaunchInputLocationType)
   {
   case pgsTypes::hilSame4Bridge:
      m_pGrid = m_pHaunchEntireBridgeGrid;
      m_pGrid->ShowWindow(SW_SHOW);
      m_pHaunchEntireBridgeGrid->InvalidateGrid();
      break;

   case  pgsTypes::hilSame4AllGirders:
      if (bShowSpans)
      {
         m_pGrid = m_pHaunchDirectSameAsSpansGrid;
         m_pGrid->ShowWindow(SW_SHOW);
         m_pHaunchDirectSameAsSpansGrid->InvalidateGrid();
      }
      else
      {
         if (!m_bNeedsGroupTabs)
         {
            m_pGrid = m_pHaunchDirectSameAsSegmentsGrid;
            m_pGrid->ShowWindow(SW_SHOW);
            m_pHaunchDirectSameAsSegmentsGrid->InvalidateGrid();
         }
         else
         {
            CGXTabBeam& beam = m_HaunchDirectSameAsSegmentsTabWnd.GetBeam();
            auto nTabs = beam.GetCount();
            for (auto i = 0; i < nTabs; i++)
            {
               auto& tabInfo = beam.GetTab(i);
               CHaunchDirectSameAsGrid* pGrid = (CHaunchDirectSameAsGrid*)tabInfo.pExtra;
               pGrid->InvalidateGrid();
            }

            m_HaunchDirectSameAsSegmentsTabWnd.ShowScrollBar(SB_BOTH,FALSE);
            m_HaunchDirectSameAsSegmentsTabWnd.ShowWindow(SW_SHOW);
         }
      }
      break;

   case  pgsTypes::hilPerEach:
      if (bShowSpans)
      {
         m_pGrid = m_pHaunchDirectSpansGrid;
         m_pGrid->ShowWindow(SW_SHOW);
         m_pHaunchDirectSpansGrid->InvalidateGrid();
      }
      else
      {
         if (!m_bNeedsGroupTabs)
         {
            m_pGrid = m_pHaunchDirectSegmentGrid;
            m_pGrid->ShowWindow(SW_SHOW);
            m_pHaunchDirectSegmentGrid->InvalidateGrid();
         }
         else
         {
            CGXTabBeam& beam = m_HaunchDirectSegmentTabWnd.GetBeam();
            auto nTabs = beam.GetCount();
            for (auto i = 0; i < nTabs; i++)
            {
               auto& tabInfo = beam.GetTab(i);
               CHaunchDirectSegmentGrid* pGrid = (CHaunchDirectSegmentGrid*)tabInfo.pExtra;
               pGrid->InvalidateGrid();
            }

            m_HaunchDirectSegmentTabWnd.ShowScrollBar(SB_BOTH,FALSE);
            m_HaunchDirectSegmentTabWnd.ShowWindow(SW_SHOW);
         }
      }
      break;
   default:
      ATLASSERT(0);
      break;
   };

   UpdateGroupBox();
}

void CEditHaunchByHaunchDlg::OnHaunchInputTypeChanged()
{
   UpdateCurrentData(); // get data from grids before changing type

   m_HaunchInputLocationType = GetHaunchInputLocationType();

   UpdateActiveControls();
}

void CEditHaunchByHaunchDlg::OnHaunchInputDistributionTypeChanged()
{
   UpdateCurrentData(); // get data from grids before changing type

   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_HAUNCH_DISTRIBUTION);
   pgsTypes::HaunchInputDistributionType oldDistType = m_HaunchInputDistributionType;
   int curSel = pBox->GetCurSel();
   m_HaunchInputDistributionType = (pgsTypes::HaunchInputDistributionType)pBox->GetItemData(curSel);

   UpdateActiveControls();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();
   UpdateLocationTypeControl(bIsPGSuper);
}

void CEditHaunchByHaunchDlg::OnHaunchLayoutTypeChanged()
{
   // Treat same as if other high-level parameters changed
   OnHaunchInputDistributionTypeChanged();
}

Float64 CEditHaunchByHaunchDlg::GetValueFromGrid(CString cellValue,CDataExchange* pDX,ROWCOL row,ROWCOL col,CGXGridCore* pGrid)
{
   Float64 value;

   bool includeSlab = GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly;

   if (cellValue.IsEmpty() || !WBFL::System::Tokenizer::ParseDouble(cellValue,&value))
   {
      AfxMessageBox(_T("Value is not a number - must be a positive number"),MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row,col,GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
      pDX->Fail();
   }
   else
   {
      value = WBFL::Units::ConvertToSysUnits(value,m_pUnit->UnitOfMeasure);

      Float64 minHaunch = GetBridgeDesc()->GetMinimumAllowableHaunchDepth(GetHaunchInputDepthType());

      CString ErrMsg;
      if (includeSlab)
      {
         ErrMsg = _T("Haunch values must be greater than or equal to Depth of Deck + Fillet.");
      }
      else
      {
         ErrMsg = _T("Haunch values must be greater than the Fillet.");
      }

      if (value + TOLERANCE < minHaunch)
      {
         AfxMessageBox(ErrMsg,MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row,col,GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }

      // We store haunch depth for this case, not slab+haunch
      if (includeSlab)
      {
         value -= m_DeckThickness;
      }
   }

   return value;
}

CString CEditHaunchByHaunchDlg::ConvertValueToGridString(Float64 haunchValue)
{
   // Subtrack deck thickness from input value if required
   if (GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly)
   {
      haunchValue += m_DeckThickness;
   }

   return FormatDimension(haunchValue,*m_pUnit,false);
}

void CEditHaunchByHaunchDlg::OnBnClickedHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_EDIT_HAUNCH);
}

CBridgeDescription2* CEditHaunchByHaunchDlg::GetBridgeDesc()
{
   CEditHaunchDlg* pParent = (CEditHaunchDlg*)GetParent();
   return &(pParent->m_BridgeDesc);
}
