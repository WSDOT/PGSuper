///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_GMDisplayMgrEventsImpl_H_
#define INCLUDED_GMDisplayMgrEventsImpl_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GMDisplayMgrEventsImpl.h : header file
//
#include <DManip\DManip.h>
#include "PGSuperDocBase.h"
#include "GirderModelChildFrame.h"

class CPGSuperDoc;

/////////////////////////////////////////////////////////////////////////////
// CGMDisplayMgrEventsImpl command target

class CGMDisplayMgrEventsImpl : public CCmdTarget
{
public:
   CGMDisplayMgrEventsImpl(CPGSDocBase* pDoc, CGirderModelChildFrame* pFrame, CWnd* pParent,bool bGirderElevation);
   ~CGMDisplayMgrEventsImpl();

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayMgrEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayMgr* pDisplayMgr,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayMgr* pDisplayMgr,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayMgr* pDisplayMgr,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayMgr* pDisplayMgr,CWnd* pWnd,CPoint point) override;
   END_INTERFACE_PART(Events)

public:
   CPGSDocBase*        m_pDoc;
   CGirderModelChildFrame* m_pFrame;
   CWnd*                   m_pParent;
   bool                    m_bGirderElevation;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // INCLUDED_GMDisplayMgrEventsImpl_H_
