///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
class CPGSuperDocBase;
class CGirderModelChildFrame;

class CGirderDropSite : public CCmdTarget  
{
public:
	CGirderDropSite(CPGSuperDocBase* pDoc, const CSpanKey& spanKey, CGirderModelChildFrame* pFrame);
	virtual ~CGirderDropSite();

   virtual void OnFinalRelease();

   DECLARE_INTERFACE_MAP()


   BEGIN_INTERFACE_PART(DropSite,iDropSite)
      STDMETHOD_(DROPEFFECT,CanDrop)(COleDataObject* pDataObject,DWORD dwKeyState,IPoint2d* point);
      STDMETHOD_(void,OnDropped)(COleDataObject* pDataObject,DROPEFFECT dropEffect,IPoint2d* point);
      STDMETHOD_(void,SetDisplayObject)(iDisplayObject* pDO);
      STDMETHOD_(void,GetDisplayObject)(iDisplayObject** dispObj);
      STDMETHOD_(void,Highlite)(CDC* pDC,BOOL bHighlite);
   END_INTERFACE_PART(DropSite)

private:
   CPGSuperDocBase* m_pDoc;
   CGirderModelChildFrame* m_pFrame;
   CComPtr<iDisplayObject> m_DispObj;
   CSpanKey m_SpanKey;
};

#endif // !defined(AFX_GIRDERDROPSITE_H__1F8A97C9_F789_11D4_8B9B_006097C68A9C__INCLUDED_)
