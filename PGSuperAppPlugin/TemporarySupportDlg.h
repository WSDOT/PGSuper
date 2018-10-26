#pragma once

#include "PGSuperAppPlugin\TemporarySupportLayoutPage.h"
#include "PGSuperAppPlugin\TemporarySupportGeometryPage.h"
#include "PGSuperAppPlugin\GirderSegmentSpacingPage.h"

#include <PgsExt\TemporarySupportData.h>

// CTemporarySupportDlg

class CTemporarySupportDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CTemporarySupportDlg)

public:
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CTemporarySupportDlg();

   void Init(const CTemporarySupportData& ts,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,pgsTypes::SupportedBeamSpacing girderSpacingType,pgsTypes::MeasurementLocation spacingMeasureLocation,EventIndexType closureEventIndex);
   const CTemporarySupportData& GetTemporarySupport();
   void SetEvents(EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType closureEventIdx);
   EventIndexType GetErectionEventIndex();
   EventIndexType GetRemovalEventIndex();
   pgsTypes::SupportedBeamSpacing GetGirderSpacingType();
   pgsTypes::MeasurementLocation GetSpacingMeasurementLocation();
   EventIndexType GetClosurePourEventIndex();

protected:
	DECLARE_MESSAGE_MAP()

   void Init(const CBridgeDescription2* pBridgeDesc);

   const CBridgeDescription2* m_pBridgeDesc;

   CTemporarySupportData m_TemporarySupport;
   EventIndexType m_ErectionEventIndex;
   EventIndexType m_RemovalEventIndex;
   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;
   pgsTypes::MeasurementLocation m_GirderSpacingMeasurementLocation;
   EventIndexType m_ClosurePourEventIndex;

   CTemporarySupportLayoutPage   m_General;
   CTemporarySupportGeometryPage m_Geometry;
   CGirderSegmentSpacingPage     m_Spacing;

   friend CTemporarySupportLayoutPage;
   friend CTemporarySupportGeometryPage;
   friend CGirderSegmentSpacingPage;
public:
   virtual BOOL OnInitDialog();
};


