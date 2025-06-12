#pragma once

#include <PsgLib\RemoveTemporarySupportsActivity.h>
#include <PsgLib\TimelineManager.h>
#include <PsgLib\TemporarySupportData.h>
#include <PgsExt\TimelineItemListBox.h>
#include <EAF\EAFDisplayUnits.h>

// CRemoveTempSupportsDlg dialog

class CRemoveTempSupportsDlg : public CDialog
{
	DECLARE_DYNAMIC(CRemoveTempSupportsDlg)

public:
	CRemoveTempSupportsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRemoveTempSupportsDlg();

// Dialog Data
	enum { IDD = IDD_REMOVE_TS };
   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
   void FillLists();

	DECLARE_MESSAGE_MAP()

   std::shared_ptr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

   BOOL m_bReadOnly;

public:
   virtual BOOL OnInitDialog() override;
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
