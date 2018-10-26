#pragma once

#include "PGSuperAppPlugin\TemporarySupportLayoutPage.h"
#include "PGSuperAppPlugin\ClosurePourGeometryPage.h"
#include "PGSuperAppPlugin\GirderSegmentSpacingPage.h"

#include <PgsExt\TemporarySupportData.h>

// CTemporarySupportDlg

class CTemporarySupportDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CTemporarySupportDlg)

public:
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CTemporarySupportDlg();

   virtual BOOL OnInitDialog();

   const CBridgeDescription2* GetBridgeDescription();

   void SetEvents(EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType closureEventIdx);

   SupportIndexType GetTemporarySupportIndex();

   const CTemporarySupportData& GetTemporarySupport();

   // Data from General Page
   Float64 GetStation();
   LPCTSTR GetOrientation();
   pgsTypes::TemporarySupportType GetTemporarySupportType();
   EventIndexType GetErectionEventIndex();
   EventIndexType GetRemovalEventIndex();

   // Data from Geometry Page
   pgsTypes::SegmentConnectionType GetConnectionType();
   void SetConnectionType(pgsTypes::SegmentConnectionType type);
   EventIndexType GetClosurePourEventIndex();
   Float64 GetBearingOffset();
   ConnectionLibraryEntry::BearingOffsetMeasurementType GetBearingOffsetMeasurementType();
   Float64 GetEndDistance();
   ConnectionLibraryEntry::EndDistanceMeasurementType GetEndDistanceMeasurementType();
   Float64 GetSupportWidth();

   // Data from Spacing Page
   pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   pgsTypes::MeasurementLocation GetSpacingMeasurementLocation();

protected:
	DECLARE_MESSAGE_MAP()

   void Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx);

   const CBridgeDescription2* m_pBridgeDesc;

   CTemporarySupportData m_TemporarySupport;

   CTemporarySupportLayoutPage   m_General;
   CClosurePourGeometryPage      m_Geometry;
   CGirderSegmentSpacingPage     m_Spacing;
};


