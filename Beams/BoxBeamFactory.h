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

// BoxBeamFactory.h : Declaration of the CBoxBeamFactory

#ifndef __BOXBEAMFACTORY_H_
#define __BOXBEAMFACTORY_H_

#include "resource.h"       // main symbols
#include "IFace\BeamFactory.h"
#include "IBeamFactory.h" // CLSID
#include "BoxBeamFactoryImpl.h"


/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactory
class ATL_NO_VTABLE CBoxBeamFactory : 
   public CBoxBeamFactoryImpl,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CBoxBeamFactory, &CLSID_BoxBeamFactory>
{
public:
	CBoxBeamFactory()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_BOXBEAMFACTORY)
DECLARE_CLASSFACTORY_SINGLETON(CBoxBeamFactory)

BEGIN_COM_MAP(CBoxBeamFactory)
   COM_INTERFACE_ENTRY(IBeamFactory)
END_COM_MAP()

public:
   // IBeamFactory
   virtual void CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection);
   virtual bool ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg);
   virtual void SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions);
   virtual IBeamFactory::Dimensions LoadSectionDimensions(sysIStructuredLoad* pLoad);
   virtual Float64 GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids);
   virtual void CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover);
   virtual std::_tstring GetImage();
   virtual CLSID GetCLSID();
   virtual LPCTSTR GetImageResourceName();
   virtual HICON GetIcon();
   virtual bool IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType);
   virtual void GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint);
   virtual Float64 GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType);
   virtual void GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing);

protected:
   virtual bool ExcludeExteriorBeamShearKeys() { return false; }
   virtual bool UseOverallWidth() { return false; }

private:
   void GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                    double& H1, 
                                    double& H2, 
                                    double& H3, 
                                    double& H4, 
                                    double& H5,
                                    double& H6, 
                                    double& H7, 
                                    double& W1, 
                                    double& W2, 
                                    double& W3, 
                                    double& W4, 
                                    double& F1, 
                                    double& F2, 
                                    double& C1,
                                    double& J,
                                    double& shearKeyDepth,
                                    double& endBlockLength);

};

#endif //__BOXBEAMFACTORY_H_
