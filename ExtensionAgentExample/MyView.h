///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2018  Washington State Department of Transportation
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

#pragma once
#include <EAF\EAFUIIntegration.h>


// CMyView view

class CMyView : public CView
{
	DECLARE_DYNCREATE(CMyView)

protected:
	CMyView();           // protected constructor used by dynamic creation
	virtual ~CMyView();

   IEAFCommandCallback* m_pAgentCallback; // callback interface for the extension agent
   // use this for adding commands that are to be routed to the agent

public:
	virtual void OnDraw(CDC* pDC) override;      // overridden to draw this view

#ifdef _DEBUG
	virtual void AssertValid() const override;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const override;
#endif
#endif

protected:
	DECLARE_MESSAGE_MAP()

public:
   afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
   afx_msg void OnViewOnlyCommand();
   virtual void OnInitialUpdate();
};


