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

// GirderDropSite.h: interface for the CGirderDropSite class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "GirderModelChildFrame.h"
#include <DManip/DropSite.h>

class CPGSDocBase;

class CGirderDropSite : public WBFL::DManip::iDropSite
{
public:
	CGirderDropSite(CPGSDocBase* pDoc, const CSpanKey& spanKey, CGirderModelChildFrame* pFrame);
	virtual ~CGirderDropSite();

   virtual DROPEFFECT CanDrop(COleDataObject* pDataObject, DWORD dwKeyState, const WBFL::Geometry::Point2d& point) override;
   virtual void OnDropped(COleDataObject* pDataObject, DROPEFFECT dropEffect, const WBFL::Geometry::Point2d& point) override;
   virtual void SetDisplayObject(std::weak_ptr<WBFL::DManip::iDisplayObject> pDO) override;
   virtual std::shared_ptr<WBFL::DManip::iDisplayObject> GetDisplayObject() override;
   virtual void Highlight(CDC* pDC, BOOL bHighlight) override;

private:
   CPGSDocBase* m_pDoc;
   CGirderModelChildFrame* m_pFrame;
   std::weak_ptr<WBFL::DManip::iDisplayObject> m_DispObj;
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

