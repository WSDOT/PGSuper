// TemporarySupportDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"

#include "PGSuperDocBase.h"

#include <IFace\Project.h>

#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CTemporarySupportDlg

IMPLEMENT_DYNAMIC(CTemporarySupportDlg, CPropertySheet)

CTemporarySupportDlg::CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx, CWnd* pParentWnd, UINT iSelectPage)
:CPropertySheet(tsIdx == INVALID_INDEX ? _T("Add Temporary Support") : _T("Temporary Support Details"), pParentWnd, iSelectPage)
{
   Init(pBridgeDesc,tsIdx);
   InitPages();
}

CTemporarySupportDlg::CTemporarySupportDlg(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx,const std::vector<EditBridgeExtension>& editBridgeExtensions, CWnd* pParentWnd, UINT iSelectPage)
:CPropertySheet(tsIdx == INVALID_INDEX ? _T("Add Temporary Support") : _T("Temporary Support Details"), pParentWnd, iSelectPage)
{
   Init(pBridgeDesc,tsIdx);
   InitPages(editBridgeExtensions);
}

CTemporarySupportDlg::~CTemporarySupportDlg()
{
   DestroyExtensionPages();
}

CBridgeDescription2* CTemporarySupportDlg::GetBridgeDescription()
{
   return &m_BridgeDesc;
}

SupportIndexType CTemporarySupportDlg::GetTemporarySupport()
{
   return m_pTS->GetIndex();
}


INT_PTR CTemporarySupportDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      if ( 0 < m_BridgeExtensionPages.size() )
      {
         NotifyBridgeExtensionPages();
      }
      else
      {
         NotifyExtensionPages();
      }
   }

   return result;
}

BEGIN_MESSAGE_MAP(CTemporarySupportDlg, CPropertySheet)
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

LRESULT CTemporarySupportDlg::OnKickIdle(WPARAM wp, LPARAM lp)
{
   // The CPropertySheet::OnKickIdle method calls GetActivePage()
   // which doesn't work with extension pages. Since GetActivePage
   // is not virtual, we have to replace the implementation of
   // OnKickIdle.
   // The same problem exists with OnCommandHelp

	ASSERT_VALID(this);

	CPropertyPage* pPage = GetPage(GetActiveIndex());

	/* Forward the message on to the active page of the property sheet */
	if( pPage != nullptr )
	{
		//ASSERT_VALID(pPage);
		return pPage->SendMessage( WM_KICKIDLE, wp, lp );
	}
	else
   {
		return 0;
   }
}

void CTemporarySupportDlg::CommonInitPages()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags  |= PSP_HASHELP;
   m_Geometry.m_psp.dwFlags |= PSP_HASHELP;
   m_Spacing.m_psp.dwFlags  |= PSP_HASHELP;

   AddPage(&m_General);
   AddPage(&m_Geometry);
   AddPage(&m_Spacing);
}

void CTemporarySupportDlg::InitPages()
{
   CommonInitPages();
   CreateExtensionPages();
}

void CTemporarySupportDlg::InitPages(const std::vector<EditBridgeExtension>& editBridgeExtensions)
{
   CommonInitPages();
   CreateExtensionPages(editBridgeExtensions);
}

void CTemporarySupportDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   std::map<IDType,IEditTemporarySupportCallback*> callbacks = pDoc->GetEditTemporarySupportCallbacks();
   std::map<IDType,IEditTemporarySupportCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditTemporarySupportCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditTemporarySupportCallback* pEditTemporarySupportCallback = callbackIter->second;
      CPropertyPage* pPage = pEditTemporarySupportCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pEditTemporarySupportCallback,pPage);
         AddPage(pPage);
      }
   }
}

void CTemporarySupportDlg::CreateExtensionPages(const std::vector<EditBridgeExtension>& editBridgeExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   m_BridgeExtensionPages = editBridgeExtensions;
   std::sort(m_BridgeExtensionPages.begin(), m_BridgeExtensionPages.end()); // must be sorted.. do it here so callers don't have to worry about it


   std::map<IDType,IEditTemporarySupportCallback*> callbacks = pDoc->GetEditTemporarySupportCallbacks();
   std::map<IDType,IEditTemporarySupportCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditTemporarySupportCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditTemporarySupportCallback* pEditTemporarySupportCallback = callbackIter->second;
      IDType editBridgeCallbackID = pEditTemporarySupportCallback->GetEditBridgeCallbackID();
      CPropertyPage* pPage = nullptr;
      if ( editBridgeCallbackID == INVALID_ID )
      {
         pPage = pEditTemporarySupportCallback->CreatePropertyPage(this);
      }
      else
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::vector<EditBridgeExtension>::const_iterator found(std::find(m_BridgeExtensionPages.begin(),m_BridgeExtensionPages.end(), key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            const EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            pPage = pEditTemporarySupportCallback->CreatePropertyPage(this,pBridgePage);
         }
      }

      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pEditTemporarySupportCallback,pPage);
         AddPage(pPage);
      }
   }
}

void CTemporarySupportDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CTemporarySupportDlg::GetExtensionPageTransaction()
{
   if ( 0 < m_Macro.GetTxnCount() )
      return m_Macro.CreateClone();
   else
      return nullptr;
}

void CTemporarySupportDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditTemporarySupportCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

void CTemporarySupportDlg::NotifyBridgeExtensionPages()
{
   // This gets called when this dialog is created from the framing grid and it is closed with IDOK
   // It gives the bridge dialog extension pages to sync their data with whatever got changed in 
   // the extension pages in this dialog
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   std::map<IDType,IEditTemporarySupportCallback*> callbacks = pDoc->GetEditTemporarySupportCallbacks();
   std::map<IDType,IEditTemporarySupportCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditTemporarySupportCallback*>::iterator callbackIterEnd(callbacks.end());
   std::vector<std::pair<IEditTemporarySupportCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   for ( ; callbackIter != callbackIterEnd; callbackIter++, pageIter++ )
   {
      IEditTemporarySupportCallback* pCallback = callbackIter->second;
      IDType editBridgeCallbackID = pCallback->GetEditBridgeCallbackID();
      CPropertyPage* pTemporarySupportPage = pageIter->second;

      if ( editBridgeCallbackID != INVALID_ID )
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::vector<EditBridgeExtension>::iterator found(std::find(m_BridgeExtensionPages.begin(),m_BridgeExtensionPages.end(),key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            extension.pCallback->EditTemporarySupport_OnOK(pBridgePage,pTemporarySupportPage);
         }
      }
   }
}

void CTemporarySupportDlg::Init(const CBridgeDescription2* pBridgeDesc,SupportIndexType tsIdx)
{
   m_BridgeDesc = *pBridgeDesc;

   if ( tsIdx == INVALID_INDEX )
   {
      // create a new temporary support at the mid-point of the first span
      // (is there a better location?)
      CTemporarySupportData* pTS = new CTemporarySupportData();
      Float64 spanLength = m_BridgeDesc.GetSpan(0)->GetSpanLength();
      Float64 station = m_BridgeDesc.GetPier(0)->GetStation() + spanLength/2; // try at center of first span

      bool bLocationOK = false;
      int count = 0;
      while ( bLocationOK == false && count < 10 /*prevents infinite loops*/)
      {
         if ( m_BridgeDesc.IsPierLocation(station) == INVALID_INDEX && m_BridgeDesc.IsTemporarySupportLocation(station) == INVALID_INDEX )
         {
            bLocationOK = true;
         }
         else
         {
            // there is a pier or temporary support here... move by 15% of the length of the first span
            station += 0.15*spanLength;
         }
         count++;
      };
      
      if ( m_BridgeDesc.GetPier(m_BridgeDesc.GetPierCount()-1)->GetStation() < station )
      {
         // somehow we ended up past the end of the bridge... just put it at the center of the bridge and call it good
         station = m_BridgeDesc.GetPier(0)->GetStation() + m_BridgeDesc.GetLength()/2;
      }

      pTS->SetStation(station);

      EventIndexType erectionEventIdx = 0;
      EventIndexType removalEventIdx = 0;
      EventIndexType castClosureJointEventIdx = 0;

      // if the bridge has temporary supports, model the new temporary support after the first
      // temporary support
      if ( 0 < pBridgeDesc->GetTemporarySupportCount() )
      {
         const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

         const CTemporarySupportData* pSeedTS = pBridgeDesc->GetTemporarySupport(0);
         pTS->SetSegmentSpacing(*pSeedTS->GetSegmentSpacing());

         pTimelineMgr->GetTempSupportEvents(pSeedTS->GetID(),&erectionEventIdx,&removalEventIdx);

         const CClosureJointData* pClosure = pSeedTS->GetClosureJoint(0);
         if ( pClosure )
         {
            castClosureJointEventIdx = pBridgeDesc->GetTimelineManager()->GetCastClosureJointEventIndex(pClosure->GetID());
         }
#pragma Reminder("UPDATE: need to figure out if new TS station is at the location of a previously defined TS... if so, need a different initial location")
      }
      else
      {
         // use some default data for the new temporary support
         const CGirderGroupData* pGroup = m_BridgeDesc.GetGirderGroup((GroupIndexType)0L);
         pTS->GetSegmentSpacing()->SetGirderCount(pGroup->GetGirderCount());

         const CGirderSpacing2* pSpacing = pGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead);
         GroupIndexType nSpacingGroups = pSpacing->GetSpacingGroupCount();
         for ( GroupIndexType spacingGrpIdx = 0; spacingGrpIdx < nSpacingGroups; spacingGrpIdx++ )
         {
            pTS->GetSegmentSpacing()->SetGirderSpacing(spacingGrpIdx,pSpacing->GetGirderSpacing(spacingGrpIdx));
         }
      }

      tsIdx = m_BridgeDesc.AddTemporarySupport(pTS,erectionEventIdx,removalEventIdx,castClosureJointEventIdx);
   }

   m_pTS = m_BridgeDesc.GetTemporarySupport(tsIdx);

   m_General.Init(m_pTS);
   m_Geometry.Init(m_pTS);
   m_Spacing.Init(m_pTS);
}

// CTemporarySupportDlg message handlers

BOOL CTemporarySupportDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();

   return bResult;
}
