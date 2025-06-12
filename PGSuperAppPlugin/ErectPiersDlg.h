#pragma once

#include <PsgLib\ErectPiersActivity.h>
#include <PsgLib\BridgeDescription2.h>
#include <PsgLib\TemporarySupportData.h>
#include <PsgLib\TimelineManager.h>
#include <PgsExt\TimelineItemListBox.h>

#include <EAF\EAFDisplayUnits.h>



// CErectPiersDlg dialog

class CErectPiersDlg : public CDialog
{
	DECLARE_DYNAMIC(CErectPiersDlg)

public:
	CErectPiersDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CErectPiersDlg();

// Dialog Data
	enum { IDD = IDD_ERECT_PIERS };
   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void FillLists();

	DECLARE_MESSAGE_MAP()

   std::shared_ptr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

   BOOL m_bReadOnly;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
