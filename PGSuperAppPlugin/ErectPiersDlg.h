#pragma once

#include <PgsExt\ErectPiersActivity.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TimelineItemListBox.h>

#include <EAF\EAFDisplayUnits.h>



// CErectPiersDlg dialog

class CErectPiersDlg : public CDialog
{
	DECLARE_DYNAMIC(CErectPiersDlg)

public:
	CErectPiersDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = NULL);   // standard constructor
	virtual ~CErectPiersDlg();

// Dialog Data
	enum { IDD = IDD_ERECT_PIERS };
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

   BOOL m_bReadOnly;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
