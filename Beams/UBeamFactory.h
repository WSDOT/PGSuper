///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// UBeamFactory.h : Declaration of the CUBeamFactory

#ifndef __UBEAMFACTORY_H_
#define __UBEAMFACTORY_H_

#include "resource.h"       // main symbols
#include "IFace\BeamFactory.h"
#include "IBeamFactory.h" // CLSID

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CUBeamFactory
class ATL_NO_VTABLE CUBeamFactory : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CUBeamFactory, &CLSID_UBeamFactory>,
   public IBeamFactory
{
public:
	CUBeamFactory()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_UBEAMFACTORY)
DECLARE_CLASSFACTORY_SINGLETON(CUBeamFactory)

BEGIN_COM_MAP(CUBeamFactory)
   COM_INTERFACE_ENTRY(IBeamFactory)
END_COM_MAP()

public:
   // IBeamFactory
   virtual void CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection);
   virtual void CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape);
   virtual void CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr);
   virtual void LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr);
   virtual void CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng);
   virtual void CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng);
   virtual void CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover);
   virtual std::vector<std::_tstring> GetDimensionNames();
   virtual std::vector<const unitLength*> GetDimensionUnits(bool bSIUnits);
   virtual std::vector<Float64> GetDefaultDimensions();
   virtual bool ValidateDimensions(const Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg);
   virtual void SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions);
   virtual IBeamFactory::Dimensions LoadSectionDimensions(sysIStructuredLoad* pLoad);
   virtual bool IsPrismatic(const IBeamFactory::Dimensions& dimensions);
   virtual bool IsSymmetric(const IBeamFactory::Dimensions& dimensions);
   virtual Float64 GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey);
   virtual std::_tstring GetImage();
   virtual std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType);
   virtual CLSID GetCLSID();
   virtual std::_tstring GetName();
   virtual CLSID GetFamilyCLSID();
   virtual std::_tstring GetGirderFamilyName();
   virtual std::_tstring GetPublisher();
   virtual std::_tstring GetPublisherContactInformation();
   virtual HINSTANCE GetResourceInstance();
   virtual LPCTSTR GetImageResourceName();
   virtual HICON GetIcon();
   virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs);
   virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings();
   virtual pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms();
   virtual pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type);
   virtual void GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing);
   virtual WebIndexType GetWebCount(const IBeamFactory::Dimensions& dimensions);
   virtual Float64 GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType);
   virtual Float64 GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType);
   virtual bool IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType);
   virtual void GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint);
   virtual GirderIndexType GetMinimumBeamCount();

public:
   std::vector<std::_tstring> m_DimNames;
   std::vector<Float64> m_DefaultDims;
   std::vector<const unitLength*> m_DimUnits[2];

   void GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,
                      Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& w5,
                      Float64& t);
   Float64 GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name);

private:
   void ConfigureShape(const IBeamFactory::Dimensions& dimensions, IUBeam* shape);
};

#endif //__UBEAMFACTORY_H_
