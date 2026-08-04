///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as
// published by the Washington State Department of Transportation,
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source
// License along with this program; if not, write to the Washington
// State Department of Transportation, Bridge and Structures Office,
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// ExtensionPageManager.cpp : implementation of CExtensionPageManager

#include "stdafx.h"
#include "ExtensionPageManager.h"

void CExtensionPageManager::AddPage(LPCTSTR name, CPropertyPage* pPage, std::function<bool()> predicate)
{
   m_Pages.push_back(Entry{ CString(name), pPage, std::move(predicate) });
}

void CExtensionPageManager::InsertExtensionPage(LPCTSTR name, CPropertyPage* pPage, const ExtensionPagePosition& position)
{
   if (pPage == nullptr)
   {
      return;
   }

   Entry entry{ CString(name), pPage, nullptr };

   switch (position.Placement)
   {
   case ExtensionPagePlacement::Start:
      m_Pages.insert(m_Pages.begin(), std::move(entry));
      return;

   case ExtensionPagePlacement::AtIndex:
   {
      int idx = position.Index;
      if (idx < 0) idx = 0;
      if ((size_t)idx > m_Pages.size()) idx = (int)m_Pages.size();
      m_Pages.insert(m_Pages.begin() + idx, std::move(entry));
      return;
   }

   case ExtensionPagePlacement::Before:
   case ExtensionPagePlacement::After:
   {
      CString refName(position.ReferencePageName.c_str());
      auto found = std::find_if(m_Pages.begin(), m_Pages.end(), [&refName](const Entry& e)
         {
            return e.Name == refName;
         });

      if (found == m_Pages.end())
      {
         ATLASSERT(false); // ReferencePageName not found - typo? page registered under a different name?
         m_Pages.push_back(std::move(entry));
         return;
      }

      auto insertAt = (position.Placement == ExtensionPagePlacement::Before) ? found : found + 1;
      m_Pages.insert(insertAt, std::move(entry));
      return;
   }

   case ExtensionPagePlacement::End:
   default:
      m_Pages.push_back(std::move(entry));
      return;
   }
}

void CExtensionPageManager::Commit(CPropertySheet* pSheet)
{
   std::vector<CPropertyPage*> desired;
   for (const auto& entry : m_Pages)
   {
      if (!entry.Predicate || entry.Predicate())
      {
         desired.push_back(entry.Page);
      }
   }

   // Longest already-correct prefix: leave it alone so a rebuild triggered while the
   // dialog is open doesn't disturb the active page/tab when it's part of that prefix
   // (this is what CGirderDescDlg::OnGirderTypeChanged used to do by hand, hardcoded to
   // "the first two pages are always safe" - here it's whatever prefix is actually
   // unchanged, computed instead of assumed).
   size_t stable = 0;
   while (stable < desired.size() && stable < m_Committed.size() && desired[stable] == m_Committed[stable])
   {
      ++stable;
   }

   bool bWasLive = pSheet->GetSafeHwnd() != nullptr;
   CPropertyPage* pPreviouslyActivePage = bWasLive ? pSheet->GetActivePage() : nullptr;

   // Remove the differing suffix, from the end backward so earlier indices stay valid.
   for (int idx = (int)m_Committed.size() - 1; idx >= (int)stable; --idx)
   {
      int pageIndex = pSheet->GetPageIndex(m_Committed[idx]);
      if (pageIndex != -1)
      {
         pSheet->RemovePage(pageIndex);
      }
   }

   // Add the differing suffix back, in final order.
   for (size_t i = stable; i < desired.size(); ++i)
   {
      pSheet->AddPage(desired[i]);
   }

   m_Committed = std::move(desired);

   // If the page that was active got removed and not re-added, fall back to the first
   // page so the sheet doesn't end up with no active tab.
   if (bWasLive && pSheet->GetPageIndex(pPreviouslyActivePage) == -1 && 0 < pSheet->GetPageCount())
   {
      pSheet->SetActivePage(0);
   }
}

CPropertyPage* CExtensionPageManager::FindPage(LPCTSTR name) const
{
   CString findName(name);
   auto found = std::find_if(m_Pages.begin(), m_Pages.end(), [&findName](const Entry& e)
      {
         return e.Name == findName;
      });

   return found == m_Pages.end() ? nullptr : found->Page;
}
