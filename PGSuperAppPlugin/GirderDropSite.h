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

// GirderDropSite.h: interface for the CGirderDropSite class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GIRDERDROPSITE_H__1F8A97C9_F789_11D4_8B9B_006097C68A9C__INCLUDED_)
#define AFX_GIRDERDROPSITE_H__1F8A97C9_F789_11D4_8B9B_006097C68A9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <DManip\DManip.h>
#include "GirderModelChildFrame.h"

class CPGSDocBase;

class CGirderDropSite : public CCmdTarget  
{
public:
	CGirderDropSite(CPGSDocBase* pDoc, const CSpanKey& spanKey, CGirderModelChildFrame* pFrame);
	virtual ~CGirderDropSite();

   virtual void OnFinalRelease();

   DECLARE_INTERFACE_MAP()


   BEGIN_INTERFACE_PART(DropSite,iDropSite)
      STDMETHOD_(DROPEFFECT,CanDrop)(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point) override;
      STDMETHOD_(void,OnDropped)(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point) override;
      STDMETHOD_(void,SetDisplayObject)(iDisplayObject* pDO) override;
      STDMETHOD_(void,GetDisplayObject)(iDisplayObject** dispObj) override;
      STDMETHOD_(void,Highlite)(CDC* pDC,BOOL bHighlite) override;
   END_INTERFACE_PART(DropSite)

private:
   CPGSDocBase* m_pDoc;
   CGirderModelChildFrame* m_pFrame;
   CComPtr<iDisplayObject> m_DispObj;
   CSpanKey m_SpanKey;

   template <class T>
   void InitLoad(T& load) const
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

      load.m_SpanKey = m_SpanKey;
   
      EventIndexType liveLoadEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();

      EventIndexType eventIndex = m_pFrame->GetEvent();
      if (eventIndex == liveLoadEventIdx)
      {
         load.m_LoadCase = UserLoads::LL_IM;
      }
      else
      {
         load.m_LoadCase = UserLoads::DC;
      }
   }
};

#endif // !defined(AFX_GIRDERDROPSITE_H__1F8A97C9_F789_11D4_8B9B_006097C68A9C__INCLUDED_)
