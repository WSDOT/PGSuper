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

// ExtensionPageManager.h : declaration of CExtensionPageManager
//
// Why this class exists: the underlying Win32 controls DO support ad-hoc
// insertion and removal - CTabCtrl takes an index in TCM_INSERTITEM, and even
// the native property sheet control has PSM_INSERTPAGE (prsht.h) to insert a
// page next to another one after the sheet's HWND exists. MFC's
// CPropertySheet just never wraps that message. Instead, CPropertySheet keeps
// its own private CPtrArray m_pages (afxdlgs.h) as the single source of truth
// behind GetPage(), GetPageCount(), GetPageIndex(), SetActivePage(int), and
// RemovePage() - AddPage() only appends to it, and there is no public
// InsertPage(). Reaching around MFC with a raw
// SendMessage(m_hWnd, PSM_INSERTPAGE, ...) would insert a tab in the live
// control without m_pages ever finding out, desyncing every one of those
// accessors from what's actually on screen. So both problems this class
// solves - where a page belongs, and whether it belongs at all right now -
// have to be resolved entirely on our side, before any page reaches
// CPropertySheet:
//
//  - Position: a dialog registers its built-in pages, then lets each
//    extension's IExtensionPageCallback::GetPropertyPagePosition()
//    (IFace\ExtendUI.h) say where among them its page belongs.
//  - Conditional inclusion: a built-in page can be registered with a
//    predicate - e.g. CGirderDescDlg only wants its strand-extension-and-
//    debond page present for grid-based strand input. Before this class, that
//    kind of thing was done by hand: CGirderDescDlg::OnGirderTypeChanged()
//    called RemovePage() on every page past a hardcoded "first two are always
//    safe" prefix and rebuilt the rest, entirely so it could re-decide
//    whether to include that one conditional page. Commit() generalizes that:
//    it re-evaluates every predicate, diffs the desired page list against
//    what's currently shown, and touches only the differing suffix - the same
//    "leave the untouched prefix alone" trick, just computed instead of
//    hand-maintained, and it composes with extension positioning for free.
//
// Only once the final, current list is known does this class call the
// ordinary, fully-supported AddPage()/RemovePage() to make CPropertySheet's
// state match it. It is used internally by each extendable property sheet
// dialog, for as long as that dialog is alive - not a one-shot construction
// helper, hence "Manager" rather than "Builder": it keeps owning the page
// list and mediating every change to it (via repeated Commit() calls) for the
// dialog's whole lifetime. It is not part of the public extension API (that's
// IEditXxxCallback / ExtensionPagePosition in IFace\ExtendUI.h).

#pragma once

#include <IFace\ExtendUI.h>
#include <functional>

class CExtensionPageManager
{
public:
   // Registers a page under a stable name, in the order it should appear
   // relative to other pages registered so far (built-in pages keep this
   // call order; see InsertExtensionPage for extension pages, which are
   // positioned relative to this order instead).
   //
   // predicate, if given, is re-evaluated on every Commit() call: the page is
   // included only while it returns true. Leave it null (the default) for an
   // always-included page - that's the common case. A page whose predicate
   // currently returns false is left out of the sheet entirely, not created
   // and then hidden.
   void AddPage(LPCTSTR name, CPropertyPage* pPage, std::function<bool()> predicate = nullptr);

   // Inserts an extension-supplied page at the position it requested. No-op
   // if pPage is nullptr (the callback declined to create a page). If the
   // position is Before/After a name that isn't found in the list built so
   // far, falls back to AtEnd() (and asserts in debug builds). Always
   // included once inserted - if a future extension needs conditional
   // inclusion too, add a predicate parameter here the same way AddPage has
   // one.
   void InsertExtensionPage(LPCTSTR name, CPropertyPage* pPage, const ExtensionPagePosition& position);

   // Reconciles pSheet's actual pages with what every registered page's
   // predicate currently says should be shown, preserving relative order.
   // Safe to call repeatedly over the dialog's lifetime, not just once at
   // startup: the first call (before the sheet has an HWND, typically from
   // the dialog's Init()) simply adds everything whose predicate currently
   // passes; a later call (e.g. in response to a UI event, after the sheet is
   // already showing) adds/removes only what changed, and restores the
   // active page if it happened to be one of the pages removed.
   void Commit(CPropertySheet* pSheet);

   // Finds a previously-registered page (built-in or extension) by name.
   // Valid for the lifetime of this manager, i.e. for the lifetime of the
   // owning dialog - Commit() does not clear the registered list, only the
   // set of what's currently shown.
   CPropertyPage* FindPage(LPCTSTR name) const;

private:
   struct Entry
   {
      CString Name;
      CPropertyPage* Page;
      std::function<bool()> Predicate; // null means "always included"
   };
   std::vector<Entry> m_Pages;               // every registered page, in final resolved order
   std::vector<CPropertyPage*> m_Committed;  // subset currently added to the sheet, same relative order
};
