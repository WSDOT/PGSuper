// TemporarySupportDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"

#include <IFace\Project.h>

#include <PgsExt\ClosureJointData.h>

// CTemporarySupportDlg

IMPLEMENT_DYNAMIC(CTemporarySupportDlg, CPropertySheet)

CTemporarySupportDlg::CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx, CWnd* pParentWnd, UINT iSelectPage)
:CPropertySheet(tsIdx == INVALID_INDEX ? _T("Add Temporary Support") : _T("Edit Temporary Support"), pParentWnd, iSelectPage)
{
   InitPages();
   Init(pBridgeDesc,tsIdx);
}

CTemporarySupportDlg::~CTemporarySupportDlg()
{
}

CBridgeDescription2* CTemporarySupportDlg::GetBridgeDescription()
{
   return &m_BridgeDesc;
}

SupportIndexType CTemporarySupportDlg::GetTempSupportIndex()
{
   return m_pTS->GetIndex();
}


BEGIN_MESSAGE_MAP(CTemporarySupportDlg, CPropertySheet)
END_MESSAGE_MAP()

void CTemporarySupportDlg::InitPages()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags  |= PSP_HASHELP;
   m_Geometry.m_psp.dwFlags |= PSP_HASHELP;
   m_Spacing.m_psp.dwFlags  |= PSP_HASHELP;

   AddPage(&m_General);
   AddPage(&m_Geometry);
   AddPage(&m_Spacing);
}

void CTemporarySupportDlg::Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx)
{
   m_BridgeDesc = *pBridgeDesc;

   if ( tsIdx == INVALID_INDEX )
   {
      // creating a new temporary support
      CTemporarySupportData* pTS = new CTemporarySupportData();
      pTS->SetStation(pBridgeDesc->GetPier(0)->GetStation() + pBridgeDesc->GetSpan(0)->GetSpanLength()/2);
      pTS->GetSegmentSpacing()->SetGirderCount(pBridgeDesc->GetGirderCount());
      pTS->GetSegmentSpacing()->SetGirderSpacing(0,pBridgeDesc->GetGirderSpacing());

      EventIndexType erectionEventIdx = 0;
      EventIndexType removalEventIdx = 0;
      EventIndexType castClosureJointEventIdx = 0;
      if ( 0 < pBridgeDesc->GetTemporarySupportCount() )
      {
         const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

         const CTemporarySupportData* pSeedTS = pBridgeDesc->GetTemporarySupport(0);
         pTS->SetSegmentSpacing(*pSeedTS->GetSegmentSpacing());

         pTimelineMgr->GetTempSupportEvents(pSeedTS->GetID(),&erectionEventIdx,&removalEventIdx);

         const CClosureJointData* pClosure = pSeedTS->GetClosureJoint(0);
         if ( pClosure )
         {
            castClosureJointEventIdx = pBridgeDesc->GetTimelineManager()->GetCastClosureJointEventIndex(pClosure->GetID());
         }

#pragma Reminder("UPDATE: need to figure out if new TS station is at the location of a previously defined TS... if so, need a different initial location")
      }

      tsIdx = m_BridgeDesc.AddTemporarySupport(pTS,erectionEventIdx,removalEventIdx,castClosureJointEventIdx);
   }

   m_pTS = m_BridgeDesc.GetTemporarySupport(tsIdx);
   m_General.Init(m_pTS);
   m_Geometry.Init(m_pTS);
   m_Spacing.Init(m_pTS);
}

// CTemporarySupportDlg message handlers

BOOL CTemporarySupportDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();

   return bResult;
}
