///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
   virtual bool ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const override;
   virtual void SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const override;
   virtual IBeamFactory::Dimensions LoadSectionDimensions(sysIStructuredLoad* pLoad) const override;
   virtual void CreateStrandMover(const IBeamFactory::Dimensions& dimensions, Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const override;
   virtual std::_tstring GetImage() const override;
   virtual CLSID GetCLSID() const override;
   virtual LPCTSTR GetImageResourceName() const override;
   virtual HICON GetIcon() const override;
   virtual bool IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const override;
   virtual void GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const override;
   virtual bool HasLongitudinalJoints() const override;
   virtual bool IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const override;
   virtual Float64 GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const override;
   virtual void GetBeamTopWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const override;
   virtual void GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const override;

protected:
   virtual bool ExcludeExteriorBeamShearKeys(const IBeamFactory::Dimensions& dimensions) const override;

   virtual void DimensionBeam(const IBeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const override;


private:
   void GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                    Float64& H1, 
                                    Float64& H2, 
                                    Float64& H3, 
                                    Float64& H4, 
                                    Float64& H5,
                                    Float64& H6, 
                                    Float64& H7, 
                                    Float64& W1, 
                                    Float64& W2, 
                                    Float64& W3, 
                                    Float64& W4, 
                                    Float64& F1, 
                                    Float64& F2, 
                                    Float64& C1,
                                    Float64& J,
                                    Float64& shearKeyDepth,
                                    Float64& endBlockLength,
                                    bool& bFlushExteriorFace) const;
};

#endif //__BOXBEAMFACTORY_H_
