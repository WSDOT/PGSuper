#pragma once

#include <PgsExt\RemoveTemporarySupportsActivity.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\TimelineItemListBox.h>
#include <EAF\EAFDisplayUnits.h>

// CRemoveTempSupportsDlg dialog

class CRemoveTempSupportsDlg : public CDialog
{
	DECLARE_DYNAMIC(CRemoveTempSupportsDlg)

public:
	CRemoveTempSupportsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent = NULL);   // standard constructor
	virtual ~CRemoveTempSupportsDlg();

// Dialog Data
	enum { IDD = IDD_REMOVE_TS };
   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillLists();

	DECLARE_MESSAGE_MAP()

   CComPtr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
};
