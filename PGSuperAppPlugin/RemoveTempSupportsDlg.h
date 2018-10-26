#pragma once

#include <PgsExt\RemoveTemporarySupportsActivity.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\TemporarySupportData.h>
#include <EAF\EAFDisplayUnits.h>

// CRemoveTempSupportsDlg dialog

class CRemoveTempSupportsDlg : public CDialog
{
	DECLARE_DYNAMIC(CRemoveTempSupportsDlg)

public:
	CRemoveTempSupportsDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent = NULL);   // standard constructor
	virtual ~CRemoveTempSupportsDlg();

// Dialog Data
	enum { IDD = IDD_REMOVE_TS };
   CRemoveTemporarySupportsActivity m_RemoveTempSupports;
   const CTimelineManager* m_pTimelineMgr;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillSourceList();
   void FillTargetList();

	DECLARE_MESSAGE_MAP()

   CComPtr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveRight();
   afx_msg void OnMoveLeft();
};
