#pragma once

#include "TemporarySupportLayoutPage.h"
#include "ClosureJointGeometryPage.h"
#include "GirderSegmentSpacingPage.h"

#include <PsgLib\TemporarySupportData.h>

#include <IFace\ExtendUI.h>
#include <EAF\MacroTxn.h>

// CTemporarySupportDlg

class CTemporarySupportDlg : public CPropertySheet, public IEditTemporarySupportData
{
	DECLARE_DYNAMIC(CTemporarySupportDlg)

public:
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx,const std::vector<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

   virtual ~CTemporarySupportDlg();

   virtual BOOL OnInitDialog() override;
	virtual INT_PTR DoModal() override;

   CBridgeDescription2* GetBridgeDescription();

   // IEditTemporarySupportData
   virtual SupportIndexType GetTemporarySupport() override;

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   std::unique_ptr<WBFL::EAF::Transaction> GetExtensionPageTransaction();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   void CommonInitPages();
   void InitPages();
   void InitPages(const std::vector<EditBridgeExtension>& editBridgeExtensions);
   void Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx);

   void CreateExtensionPages();
   void CreateExtensionPages(const std::vector<EditBridgeExtension>& editBridgeExtensions);
   void DestroyExtensionPages();

   CBridgeDescription2 m_BridgeDesc;
   CTemporarySupportData* m_pTS;

   CTemporarySupportLayoutPage   m_General;
   CClosureJointGeometryPage     m_Geometry;
   CGirderSegmentSpacingPage     m_Spacing;

   friend CTemporarySupportLayoutPage;
   friend CClosureJointGeometryPage;
   friend CGirderSegmentSpacingPage;

   WBFL::EAF::MacroTxn m_Macro;
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>> m_ExtensionPages;
   std::vector<EditBridgeExtension> m_BridgeExtensionPages; // sorted based on callback ID
   void NotifyExtensionPages();
   void NotifyBridgeExtensionPages();
};


