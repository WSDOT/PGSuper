#pragma once

#include "PGSuperAppPlugin\TemporarySupportLayoutPage.h"
#include "PGSuperAppPlugin\ClosureJointGeometryPage.h"
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

   CBridgeDescription2* GetBridgeDescription();
   SupportIndexType GetTempSupportIndex();

protected:
	DECLARE_MESSAGE_MAP()

   void InitPages();
   void Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx);

   CBridgeDescription2 m_BridgeDesc;
   CTemporarySupportData* m_pTS;

   CTemporarySupportLayoutPage   m_General;
   CClosureJointGeometryPage     m_Geometry;
   CGirderSegmentSpacingPage     m_Spacing;

   friend CTemporarySupportLayoutPage;
   friend CClosureJointGeometryPage;
   friend CGirderSegmentSpacingPage;
};


