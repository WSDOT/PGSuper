///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
//
#include "stdafx.h"
#include "resource.h"
#include "PGSuperColors.h"
#include "MultiBearingSelectDlg.h"

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <Reporting\ReactionInterfaceAdapters.h>
#include <AgentTools.h>


// CMultiBearingSelectDlg dialog



IMPLEMENT_DYNAMIC(CMultiBearingSelectDlg, CDialog)

CMultiBearingSelectDlg::CMultiBearingSelectDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CMultiBearingSelectDlg::IDD, pParent)
{
   m_pGrid = new CMultiBearingSelectGrid();
}

CMultiBearingSelectDlg::~CMultiBearingSelectDlg()
{
   delete m_pGrid;
}

void CMultiBearingSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      m_Bearings = m_pGrid->GetData();

      if (m_Bearings.empty())
      {
         ::AfxMessageBox(_T("At least one bearing must be selected"),MB_ICONEXCLAMATION | MB_OK),
         pDX->Fail();
      }
   }
}

BEGIN_MESSAGE_MAP(CMultiBearingSelectDlg, CDialog)
   ON_BN_CLICKED(IDC_SELECT_ALL, &CMultiBearingSelectDlg::OnBnClickedSelectAll)
   ON_BN_CLICKED(IDC_CLEAR_ALL, &CMultiBearingSelectDlg::OnBnClickedClearAll)
END_MESSAGE_MAP()

BOOL CMultiBearingSelectDlg::OnInitDialog()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDialog::OnInitDialog();

 	m_pGrid->SubclassDlgItem(IDC_SELECT_GRID, this);

   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IBearingDesign, pBearingDesign);
   GET_IFACE2(pBroker, IIntervals, pIntervals);

   GroupGirderCollection grpGdrColl;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {

       GirderReactionLocationOnCollection gdrRLColl;
       grpGdrColl.emplace_back(gdrRLColl);

       GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
       for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
       {

           ReactionLocationOnVector RLOn;
           grpGdrColl[grpIdx].emplace_back(RLOn);

           CGirderKey girderKey(grpIdx, gdrIdx);

           IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

           std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
               pBearingDesign, lastCompositeDeckIntervalIdx, girderKey));

           ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

           IndexType nBearings = 0;
           for (iter.First(); !iter.IsDone(); iter.Next())
           {
               nBearings += 1;
           }
           std::vector<bool> brgOn;
           brgOn.assign(nBearings, false);
           grpGdrColl[grpIdx][gdrIdx] = brgOn; // set all to false

       }

   }

   // set selected reaction locations
   for (std::vector<ReactionLocation>::iterator it = m_Bearings.begin(); it != m_Bearings.end(); it++)
   {
       const ReactionLocation& reactionLocation(*it);

       IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();

       std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(
           pBearingDesign, lastCompositeDeckIntervalIdx, reactionLocation.GirderKey));

       ReactionLocationIter iter = pForces->GetReactionLocations(pBridge);

       IndexType rlIdx = 0;
       IndexType rlOnIdx;
       for (iter.First(); !iter.IsDone(); iter.Next())
       {
           if (iter.CurrentItem() == reactionLocation)
           {
               rlOnIdx = rlIdx;
           }
           rlIdx += 1;
       }

       grpGdrColl[reactionLocation.GirderKey.groupIndex][reactionLocation.GirderKey.girderIndex][rlOnIdx] = true;
   }

   m_pGrid->CustomInit(grpGdrColl);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CMultiBearingSelectDlg::OnBnClickedSelectAll()
{
   m_pGrid->SetAllValues(true);
}

void CMultiBearingSelectDlg::OnBnClickedClearAll()
{
   m_pGrid->SetAllValues(false);
}
