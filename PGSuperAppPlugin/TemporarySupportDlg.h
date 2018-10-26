#pragma once

#include "PGSuperAppPlugin\TemporarySupportLayoutPage.h"
#include "PGSuperAppPlugin\ClosureJointGeometryPage.h"
#include "PGSuperAppPlugin\GirderSegmentSpacingPage.h"

#include <PgsExt\TemporarySupportData.h>
#include <IFace\ExtendUI.h>

// CTemporarySupportDlg

class CTemporarySupportDlg : public CPropertySheet, public IEditTemporarySupportData
{
	DECLARE_DYNAMIC(CTemporarySupportDlg)

public:
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx,const std::set<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   virtual ~CTemporarySupportDlg();

   virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();

   CBridgeDescription2* GetBridgeDescription();

   // IEditTemporarySupportData
   virtual SupportIndexType GetTemporarySupport();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   void CommonInitPages();
   void InitPages();
   void InitPages(const std::set<EditBridgeExtension>& editBridgeExtensions);
   void Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx);

   void CreateExtensionPages();
   void CreateExtensionPages(const std::set<EditBridgeExtension>& editBridgeExtensions);
   void DestroyExtensionPages();

   CBridgeDescription2 m_BridgeDesc;
   CTemporarySupportData* m_pTS;

   CTemporarySupportLayoutPage   m_General;
   CClosureJointGeometryPage     m_Geometry;
   CGirderSegmentSpacingPage     m_Spacing;

   friend CTemporarySupportLayoutPage;
   friend CClosureJointGeometryPage;
   friend CGirderSegmentSpacingPage;

   txnMacroTxn m_Macro;
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>> m_ExtensionPages;
   std::set<EditBridgeExtension> m_BridgeExtensionPages;
   void NotifyExtensionPages();
   void NotifyBridgeExtensionPages();
};


