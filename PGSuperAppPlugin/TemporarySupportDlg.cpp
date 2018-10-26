// TemporarySupportDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"

#include <IFace\Project.h>

// CTemporarySupportDlg

IMPLEMENT_DYNAMIC(CTemporarySupportDlg, CPropertySheet)

CTemporarySupportDlg::CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   Init(pBridgeDesc);
}

CTemporarySupportDlg::CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   Init(pBridgeDesc);
}

CTemporarySupportDlg::~CTemporarySupportDlg()
{
}

void CTemporarySupportDlg::Init(const CTemporarySupportData& ts,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,pgsTypes::SupportedBeamSpacing girderSpacingType,pgsTypes::MeasurementLocation spacingMeasureLocation,IndexType closureEventIdx)
{
   m_TemporarySupport = ts;
   m_ErectionEventIndex = erectionEventIdx;
   m_RemovalEventIndex = removalEventIdx;
   m_GirderSpacingType = girderSpacingType;
   m_GirderSpacingMeasurementLocation = spacingMeasureLocation;
   m_ClosurePourEventIndex = closureEventIdx;

   m_Spacing.m_SpacingGrid.SetSpacingData(*m_TemporarySupport.GetSegmentSpacing());
   m_Spacing.m_RefGirderIdx = m_TemporarySupport.GetSegmentSpacing()->GetRefGirder();
   m_Spacing.m_RefGirderOffset = m_TemporarySupport.GetSegmentSpacing()->GetRefGirderOffset();
   m_Spacing.m_RefGirderOffsetType = m_TemporarySupport.GetSegmentSpacing()->GetRefGirderOffsetType();
}

void CTemporarySupportDlg::SetEvents(IndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType closureEventIdx)
{
   m_ErectionEventIndex = erectionEventIdx;
   m_RemovalEventIndex = removalEventIdx;
   m_ClosurePourEventIndex = closureEventIdx;
}

const CTemporarySupportData& CTemporarySupportDlg::GetTemporarySupport()
{
   if (m_TemporarySupport.GetConnectionType() != pgsTypes::sctContinuousSegment )
   {
      CGirderSpacingData2& spacing = m_Spacing.m_SpacingGrid.GetSpacingData();
      
      spacing.SetRefGirder(m_Spacing.m_RefGirderIdx);
      spacing.SetRefGirderOffset(m_Spacing.m_RefGirderOffset);
      spacing.SetRefGirderOffsetType(m_Spacing.m_RefGirderOffsetType);

      m_TemporarySupport.SetSegmentSpacing(spacing);
   }

   return m_TemporarySupport;
}

EventIndexType CTemporarySupportDlg::GetErectionEventIndex()
{
   return m_ErectionEventIndex;
}

EventIndexType CTemporarySupportDlg::GetRemovalEventIndex()
{
   return m_RemovalEventIndex;
}

pgsTypes::SupportedBeamSpacing CTemporarySupportDlg::GetGirderSpacingType()
{
   return m_GirderSpacingType;
}

pgsTypes::MeasurementLocation CTemporarySupportDlg::GetSpacingMeasurementLocation()
{
   return m_GirderSpacingMeasurementLocation;
}

EventIndexType CTemporarySupportDlg::GetClosurePourEventIndex()
{
   return m_ClosurePourEventIndex;
}

BEGIN_MESSAGE_MAP(CTemporarySupportDlg, CPropertySheet)
END_MESSAGE_MAP()

void CTemporarySupportDlg::Init(const CBridgeDescription2* pBridgeDesc)
{
   m_pBridgeDesc = pBridgeDesc;

   m_ErectionEventIndex = INVALID_INDEX;
   m_RemovalEventIndex  = INVALID_INDEX;

   const CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      if ( m_ErectionEventIndex == INVALID_INDEX && pTimelineEvent->GetErectPiersActivity().IsEnabled() )
      {
         m_ErectionEventIndex = eventIdx;
      }

      if ( m_RemovalEventIndex == INVALID_INDEX && pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
      {
         m_RemovalEventIndex = eventIdx;
      }

      if ( m_ErectionEventIndex != INVALID_INDEX && m_RemovalEventIndex != INVALID_INDEX )
         break;
   }

   if ( m_ErectionEventIndex == INVALID_INDEX )
      m_ErectionEventIndex = 0;
   
   if ( m_RemovalEventIndex == INVALID_INDEX )
      m_RemovalEventIndex = 0;


   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags  |= PSP_HASHELP;
   m_Geometry.m_psp.dwFlags |= PSP_HASHELP;
   m_Spacing.m_psp.dwFlags  |= PSP_HASHELP;

   AddPage(&m_General);
   AddPage(&m_Geometry);
   AddPage(&m_Spacing);
}

// CTemporarySupportDlg message handlers

BOOL CTemporarySupportDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();

   return bResult;
}
