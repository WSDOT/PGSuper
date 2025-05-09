///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_TogaGMDisplayMgrEventsImpl_H_
#define INCLUDED_TogaGMDisplayMgrEventsImpl_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TogaGMDisplayMgrEventsImpl.h : header file
//
#include <DManip\DManip.h>
#include "TxDOTOptionalDesignDoc.h"
#include "TxDOTOptionalDesignGirderViewPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTogaGMDisplayMgrEventsImpl command target

class CTogaGMDisplayMgrEventsImpl : public CCmdTarget
{
public:
   CTogaGMDisplayMgrEventsImpl(CTxDOTOptionalDesignDoc* pDoc, CTxDOTOptionalDesignGirderViewPage* pFrame, CWnd* pParent);
   ~CTogaGMDisplayMgrEventsImpl();

public:
   CTxDOTOptionalDesignDoc*            m_pDoc;
   CTxDOTOptionalDesignGirderViewPage* m_pFrame;
   CWnd*                               m_pParent;

   DECLARE_INTERFACE_MAP()

   BEGIN_INTERFACE_PART(Events,iDisplayMgrEvents)
      STDMETHOD_(bool,OnLButtonDblClk)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonDown)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDblClk)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonDown)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnLButtonUp)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnRButtonUp)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseMove)(iDisplayMgr* pDO,UINT nFlags,CPoint point) override;
      STDMETHOD_(bool,OnMouseWheel)(iDisplayMgr* pDO,UINT nFlags,short zDelta,CPoint point) override;
      STDMETHOD_(bool,OnKeyDown)(iDisplayMgr* pDO,UINT nChar, UINT nRepCnt, UINT nFlags) override;
      STDMETHOD_(bool,OnContextMenu)(iDisplayMgr* pDO,CWnd* pWnd,CPoint point) override;
   END_INTERFACE_PART(Events)

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // INCLUDED_TogaGMDisplayMgrEventsImpl_H_
