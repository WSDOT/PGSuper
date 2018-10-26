#pragma once

#include <PgsExt\ErectPiersActivity.h>
#include <PgsExt\TimelineManager.h>
#include <EAF\EAFDisplayUnits.h>

// Pier and Temporary Support indicies are stored in the ItemData member of the
// list box controls. To differentiate between a Pier index and a Temporary Support index
// the Temporary Support indices are encoded/decoded with the following methods
#include <WBFLTypes.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\TimelineItemListBox.h>

bool IsTSIndex(SupportIndexType tsIdx);
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx);
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx);
CString GetLabel(const CPierData2* pPier,IEAFDisplayUnits* pDisplayUnits);
CString GetLabel(const CTemporarySupportData* pTS,IEAFDisplayUnits* pDisplayUnits);

// CCastClosureJointDlg dialog

class CCastClosureJointDlg : public CDialog
{
	DECLARE_DYNAMIC(CCastClosureJointDlg)

public:
	CCastClosureJointDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCastClosureJointDlg();

// Dialog Data
	enum { IDD = IDD_CAST_CLOSURE_JOINT };

   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillLists();


   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

   BOOL m_bReadOnly;

   DECLARE_MESSAGE_MAP()

   const CBridgeDescription2* m_pBridgeDesc;
   CComPtr<IEAFDisplayUnits> m_pDisplayUnits;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
