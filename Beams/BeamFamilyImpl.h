///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <IFace\BeamFamily.h>
#include <Plugins\BeamFamilyCLSID.h>
#include <Plugins\BeamFactoryCATID.h>
#include "BeamFamilyImpl.h"
#include "resource.h"

// forward declaration
interface IBeamFactory;

/////////////////////////////////////////////////////////////////////////////
// CBeamFamilyImplBase
class IBeamFamilyImpl :
   public IBeamFamily
{
public:
   // IBeamFactory
   virtual CString GetName();
   virtual std::vector<CString> GetFactoryNames();
   virtual CLSID GetFactoryCLSID(LPCTSTR strName);
   virtual HRESULT CreateFactory(LPCTSTR strName,IBeamFactory** ppFactory);

protected:
   HRESULT Init();

   virtual const CLSID& GetCLSID() = 0;
   virtual const CATID& GetCATID() = 0;

   typedef std::map<CString,CLSID> FactoryContainer;
   FactoryContainer m_Factories;
};

/////////////////////////////////////////////////////////////////////////////
// CIBeamFamily - beam family for I-beams
class ATL_NO_VTABLE CIBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CIBeamFamily, &CLSID_WFBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CIBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_IBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CIBeamFamily)

BEGIN_COM_MAP(CIBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_WFBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CUBeamFamily - beam family for U-beams
class ATL_NO_VTABLE CUBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CUBeamFamily, &CLSID_UBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CUBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_UBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CUBeamFamily)

BEGIN_COM_MAP(CUBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_UBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFamily - beam family for Box beams
class ATL_NO_VTABLE CBoxBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CBoxBeamFamily, &CLSID_BoxBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CBoxBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_BOXBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CBoxBeamFamily)

BEGIN_COM_MAP(CBoxBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_BoxBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CDeckBulbTeeBeamFamily - beam family for deck bulb tee beams
class ATL_NO_VTABLE CDeckBulbTeeBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CDeckBulbTeeBeamFamily, &CLSID_DeckBulbTeeBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CDeckBulbTeeBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_DECKBULBTEEBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CDeckBulbTeeBeamFamily)

BEGIN_COM_MAP(CDeckBulbTeeBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_DeckBulbTeeBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CDoubleTeeBeamFamily - beam family for Float64 tee beams
class ATL_NO_VTABLE CDoubleTeeBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CDoubleTeeBeamFamily, &CLSID_DoubleTeeBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CDoubleTeeBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_DOUBLETEEBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CDoubleTeeBeamFamily)

BEGIN_COM_MAP(CDoubleTeeBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_DoubleTeeBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CRibbedBeamFamily - beam family for ribbed beams
class ATL_NO_VTABLE CRibbedBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CRibbedBeamFamily, &CLSID_RibbedBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CRibbedBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_RIBBEDBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CRibbedBeamFamily)

BEGIN_COM_MAP(CRibbedBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_RibbedBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CSlabBeamFamily - beam family for slab beams
class ATL_NO_VTABLE CSlabBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CSlabBeamFamily, &CLSID_SlabBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CSlabBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_SLABBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CSlabBeamFamily)

BEGIN_COM_MAP(CSlabBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_SlabBeamFactory; }
};


/////////////////////////////////////////////////////////////////////////////
// CDeckedSlabBeamFamily - beam family for slab beams
class ATL_NO_VTABLE CDeckedSlabBeamFamily : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CDeckedSlabBeamFamily, &CLSID_DeckedSlabBeamFamily>,
   public IBeamFamilyImpl
{
public:
	CDeckedSlabBeamFamily()
	{
	}

   HRESULT FinalConstruct() { return Init(); }

DECLARE_REGISTRY_RESOURCEID(IDR_DECKEDSLABBEAMFAMILY)
DECLARE_CLASSFACTORY_SINGLETON(CDeckedSlabBeamFamily)

BEGIN_COM_MAP(CDeckedSlabBeamFamily)
   COM_INTERFACE_ENTRY(IBeamFamily)
END_COM_MAP()

protected:
   virtual const CLSID& GetCLSID() { return GetObjectCLSID(); }
   virtual const CATID& GetCATID() { return CATID_DeckedSlabBeamFactory; }
};