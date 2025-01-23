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

#pragma once

#include "LoadsViewController.h"
#include "EditLoadsChildFrm.h"
#include "EditLoadsView.h"

// {ACBCF55B-E484-482D-B033-22FC4F0C9ABE}
DEFINE_GUID(CLSID_LoadsViewController,
   0xacbcf55b, 0xe484, 0x482d, 0xb0, 0x33, 0x22, 0xfc, 0x4f, 0xc, 0x9a, 0xbe);


class ATL_NO_VTABLE CLoadsViewController :
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CLoadsViewController, &CLSID_LoadsViewController>,
   public ILoadsViewController
{
public:
   CLoadsViewController();
   virtual ~CLoadsViewController();

   void Init(CEditLoadsChildFrame* pFrame, IEAFViewController* pStandardController);

   BEGIN_COM_MAP(CLoadsViewController)
      COM_INTERFACE_ENTRY(ILoadsViewController)
      COM_INTERFACE_ENTRY(IEAFViewController)
   END_COM_MAP()

   // IEAFViewController
public:
   virtual bool IsOpen() const override;

   // Closes the view
   virtual void Close() override;

   // implement later....
   //virtual void Move(INT x, INT y) = 0;
   //virtual void Size(INT x, INT y) = 0;
   //virtual void GetSize() = 0;
   //virtual void GetPosition() = 0;
   //virtual void GetState() = 0;
   virtual void Minimize() override;
   virtual void Maximize() override;
   virtual void Restore() override;

   // ILoadsViewController
public:
   virtual void SortBy(Field field,Direction direction) override;
   virtual IndexType GetLoadCount() const override;
   virtual std::_tstring GetFieldValue(IndexType idx, Field field) const override;
   virtual void DeleteLoad(IndexType idx) override;

private:
   CEditLoadsChildFrame* m_pFrame;
   CEditLoadsView* m_pView;
   CComPtr<IEAFViewController> m_pStdController;
};

