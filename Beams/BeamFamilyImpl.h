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

#include <Beams\Helper.h>
#include <Plugins\BeamFamilyCLSID.h>
#include <Plugins\BeamFactoryCATID.h>
#include "resource.h"

// forward declaration
class IBeamFactory;

/////////////////////////////////////////////////////////////////////////////
// CIBeamFamily - beam family for I-beams
class CIBeamFamily : public IBeamFamilyImpl
{
public:
	CIBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_WFBeamFamily; }
   const CATID& GetCATID() const override { return CATID_WFBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CUBeamFamily - beam family for U-beams
class CUBeamFamily : public IBeamFamilyImpl
{
public:
	CUBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_UBeamFamily; }
   const CATID& GetCATID() const override { return CATID_UBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFamily - beam family for Box beams
class CBoxBeamFamily : public IBeamFamilyImpl
{
public:
	CBoxBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_BoxBeamFamily; }
   const CATID& GetCATID() const override { return CATID_BoxBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CDeckBulbTeeBeamFamily - beam family for deck bulb tee beams
class ATL_NO_VTABLE CDeckBulbTeeBeamFamily : public IBeamFamilyImpl
{
public:
	CDeckBulbTeeBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_DeckBulbTeeBeamFamily; }
   const CATID& GetCATID() const override { return CATID_DeckBulbTeeBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CDoubleTeeBeamFamily - beam family for Float64 tee beams
class CDoubleTeeBeamFamily : public IBeamFamilyImpl
{
public:
	CDoubleTeeBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_DoubleTeeBeamFamily; }
   const CATID& GetCATID() const override { return CATID_DoubleTeeBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CRibbedBeamFamily - beam family for ribbed beams
class CRibbedBeamFamily : public IBeamFamilyImpl
{
public:
	CRibbedBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_RibbedBeamFamily; }
   const CATID& GetCATID() const override { return CATID_RibbedBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CSlabBeamFamily - beam family for slab beams
class CSlabBeamFamily : public IBeamFamilyImpl
{
public:
	CSlabBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_SlabBeamFamily; }
   const CATID& GetCATID() const override { return CATID_SlabBeamFactory; }
};


/////////////////////////////////////////////////////////////////////////////
// CDeckedSlabBeamFamily - beam family for slab beams
class ATL_NO_VTABLE CDeckedSlabBeamFamily : public IBeamFamilyImpl
{
public:
	CDeckedSlabBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_DeckedSlabBeamFamily; }
   const CATID& GetCATID() const override { return CATID_DeckedSlabBeamFactory; }
};


/////////////////////////////////////////////////////////////////////////////
// CSplicedIBeamFamily - beam family for Spliced I-beams
class CSplicedIBeamFamily : public IBeamFamilyImpl
{
public:
	CSplicedIBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_SplicedIBeamFamily; }
   const CATID& GetCATID() const override { return CATID_SplicedIBeamFactory; }
};

/////////////////////////////////////////////////////////////////////////////
// CSplicedUBeamFamily - beam family for Spliced U-beams
class CSplicedUBeamFamily : public IBeamFamilyImpl
{
public:
	CSplicedUBeamFamily()
	{
	   Init();
	}

protected:
   const CLSID& GetCLSID() const override { return CLSID_SplicedUBeamFamily; }
   const CATID& GetCATID() const override { return CATID_SplicedUBeamFactory; }
};
