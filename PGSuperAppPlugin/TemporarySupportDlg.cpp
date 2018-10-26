// TemporarySupportDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"

#include <IFace\Project.h>

// CTemporarySupportDlg

IMPLEMENT_DYNAMIC(CTemporarySupportDlg, CPropertySheet)

CTemporarySupportDlg::CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx, CWnd* pParentWnd, UINT iSelectPage)
:CPropertySheet(tsIdx == INVALID_INDEX ? _T("Add Temporary Support") : _T("Edit Temporary Support"), pParentWnd, iSelectPage)
{
   Init(pBridgeDesc,tsIdx);
}

CTemporarySupportDlg::~CTemporarySupportDlg()
{
}

//void CTemporarySupportDlg::Init(const CTemporarySupportData& ts,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,pgsTypes::SupportedBeamSpacing girderSpacingType,pgsTypes::MeasurementLocation spacingMeasureLocation,IndexType closureEventIdx)
//{
//#pragma Reminder("UPDATE: it seems that the only parameter needes is ts")
//   m_TemporarySupport = ts;
//
//   m_General.Init(ts);
//   ATLASSERT(m_General.m_ErectionEventIndex == erectionEventIdx);
//   ATLASSERT(m_General.m_RemovalEventIndex == removalEventIdx);
//
//#pragma Reminder("UPDATE: don't need to pass events into this method... they can be retreived by other means")
//   m_Geometry.Init(ts);
//   ATLASSERT(m_Geometry.m_ClosurePourEventIndex == closureEventIdx);
//
//   m_Spacing.Init(ts);
//   ATLASSERT(m_Spacing.m_GirderSpacingType == girderSpacingType);
//   ATLASSERT(m_Spacing.m_GirderSpacingMeasurementLocation == spacingMeasureLocation);
//}

void CTemporarySupportDlg::SetEvents(IndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType closureEventIdx)
{
   m_General.m_ErectionEventIndex = erectionEventIdx;
   m_General.m_RemovalEventIndex = removalEventIdx;
   m_Geometry.m_ClosurePourEventIndex = closureEventIdx;
}

const CBridgeDescription2* CTemporarySupportDlg::GetBridgeDescription()
{
   return m_pBridgeDesc;
}

SupportIndexType CTemporarySupportDlg::GetTemporarySupportIndex()
{
   return m_TemporarySupport.GetIndex();
}

const CTemporarySupportData& CTemporarySupportDlg::GetTemporarySupport()
{
   // Data from General Page
   m_TemporarySupport.SetStation(GetStation());
   m_TemporarySupport.SetOrientation(GetOrientation());
   m_TemporarySupport.SetSupportType(GetTemporarySupportType());

   // Data from Geometry Page
   m_TemporarySupport.SetConnectionType(GetConnectionType());
   m_TemporarySupport.SetBearingOffset(GetBearingOffset(),GetBearingOffsetMeasurementType());
   m_TemporarySupport.SetGirderEndDistance(GetEndDistance(),GetEndDistanceMeasurementType());
   m_TemporarySupport.SetSupportWidth(GetSupportWidth());

   // Data from Spacing Page
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

Float64 CTemporarySupportDlg::GetStation()
{
   return m_General.m_Station;
}

LPCTSTR CTemporarySupportDlg::GetOrientation()
{
   return m_General.m_strOrientation.c_str();
}

pgsTypes::TemporarySupportType CTemporarySupportDlg::GetTemporarySupportType()
{
   return m_General.m_Type;
}

EventIndexType CTemporarySupportDlg::GetErectionEventIndex()
{
   return m_General.m_ErectionEventIndex;
}

EventIndexType CTemporarySupportDlg::GetRemovalEventIndex()
{
   return m_General.m_RemovalEventIndex;
}

EventIndexType CTemporarySupportDlg::GetClosurePourEventIndex()
{
   return m_Geometry.m_ClosurePourEventIndex;
}

pgsTypes::SegmentConnectionType CTemporarySupportDlg::GetConnectionType()
{
   return m_Geometry.m_TSConnectionType;
}

void CTemporarySupportDlg::SetConnectionType(pgsTypes::SegmentConnectionType type)
{
   m_Geometry.m_TSConnectionType = type;
}

Float64 CTemporarySupportDlg::GetBearingOffset()
{
   return m_Geometry.m_BearingOffset;
}

ConnectionLibraryEntry::BearingOffsetMeasurementType CTemporarySupportDlg::GetBearingOffsetMeasurementType()
{
   return m_Geometry.m_BearingOffsetMeasurementType;
}

Float64 CTemporarySupportDlg::GetEndDistance()
{
   return m_Geometry.m_EndDistance;
}

ConnectionLibraryEntry::EndDistanceMeasurementType CTemporarySupportDlg::GetEndDistanceMeasurementType()
{
   return m_Geometry.m_EndDistanceMeasurementType;
}

Float64 CTemporarySupportDlg::GetSupportWidth()
{
   return m_Geometry.m_SupportWidth;
}

pgsTypes::SupportedBeamSpacing CTemporarySupportDlg::GetGirderSpacingType()
{
   return m_Spacing.m_GirderSpacingType;
}

pgsTypes::MeasurementLocation CTemporarySupportDlg::GetSpacingMeasurementLocation()
{
   return m_Spacing.m_GirderSpacingMeasurementLocation;
}

BEGIN_MESSAGE_MAP(CTemporarySupportDlg, CPropertySheet)
END_MESSAGE_MAP()

void CTemporarySupportDlg::Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx)
{
   m_pBridgeDesc = pBridgeDesc;

   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   if ( 0 < nTS )
   {
      // If a temporary support exists, use the first one to initialize the data in the dialog
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx == INVALID_INDEX ? 0 : tsIdx);
      m_General.Init(*pTS);
      m_Geometry.Init(*pTS);
      m_Spacing.Init(*pTS);

      if ( tsIdx == INVALID_INDEX )
      {
         // we don't know where the new temporary support is going, sot put it at station 0+00
         // if this is not a valid location, DoDataExchange will catch it
         m_General.m_Station = 0.0;
      }
      else
      {
         m_TemporarySupport = *pTS;
      }
   }


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
