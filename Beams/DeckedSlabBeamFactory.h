///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// DeckedSlabBeamFactory.h : Declaration of the CDeckedSlabBeamFactory

#pragma once

#include "resource.h"       // main symbols
#include "IFace\BeamFactory.h"
#include "IBeamFactory.h" // CLSID
#include <Beams\Helper.h>

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CDeckedSlabBeamFactory
class ATL_NO_VTABLE CDeckedSlabBeamFactory : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CDeckedSlabBeamFactory, &CLSID_DeckedSlabBeamFactory>,
   public IBeamFactory
{
public:
	CDeckedSlabBeamFactory()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_DECKEDSLABBEAMFACTORY)
DECLARE_CLASSFACTORY_SINGLETON(CDeckedSlabBeamFactory)

BEGIN_COM_MAP(CDeckedSlabBeamFactory)
   COM_INTERFACE_ENTRY(IBeamFactory)
END_COM_MAP()

public:
   // IBeamFactory
   virtual void CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const override;
   virtual void CreateSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment** ppSegment) const override;
   virtual void CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const override;
   virtual Float64 GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const override;
   virtual void ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const override;
   virtual void LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const override;
   virtual void CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const override;
   virtual void CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const override;
   virtual void CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const override;
   virtual const std::vector<std::_tstring>& GetDimensionNames() const override;
   virtual const std::vector<const WBFL::Units::Length*>& GetDimensionUnits(bool bSIUnits) const override;
   virtual const std::vector<Float64>& GetDefaultDimensions() const override;
   virtual bool ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const override;
   virtual void SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const override;
   virtual IBeamFactory::Dimensions LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const override;
   virtual bool IsPrismatic(const IBeamFactory::Dimensions& dimensions) const override;
   virtual bool IsPrismatic(const CSegmentKey& segmentKey) const override;
   virtual bool IsSymmetric(const CSegmentKey& segmentKey) const override;
   virtual std::_tstring GetImage() const override;
   virtual std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const override;
   virtual CLSID GetCLSID() const override;
   virtual std::_tstring GetName() const override;
   virtual CLSID GetFamilyCLSID() const override;
   virtual std::_tstring GetGirderFamilyName() const override;
   virtual std::_tstring GetPublisher() const override;
   virtual std::_tstring GetPublisherContactInformation() const override;
   virtual HINSTANCE GetResourceInstance() const override;
   virtual LPCTSTR GetImageResourceName() const override;
   virtual HICON GetIcon() const override;
   virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const override;
   virtual bool IsSupportedDeckType(pgsTypes::SupportedDeckType deckType, pgsTypes::SupportedBeamSpacing sbs) const override { return ::IsSupportedDeckType(deckType, this, sbs); }
   virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings() const override;
   virtual bool IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const override;
   virtual bool ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const override;
   virtual pgsTypes::WorkPointLocations GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const override;
   virtual bool IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation workPointType) const override;
   virtual std::vector<pgsTypes::GirderOrientationType> GetSupportedGirderOrientation() const override;
   virtual bool IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
   virtual pgsTypes::GirderOrientationType ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
   virtual pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms() const override;
   virtual pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const override;
   virtual void GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const override;
   virtual std::vector<pgsTypes::TopWidthType> GetSupportedTopWidthTypes() const override { return std::vector<pgsTypes::TopWidthType>(); }
   virtual void GetAllowableTopWidthRange(pgsTypes::TopWidthType topWidthType, const IBeamFactory::Dimensions& dimensions, Float64* pWleftMin, Float64* pWleftMax, Float64* pWrightMin, Float64* pWrightMax) const override { *pWleftMin = 0; *pWleftMax = 0; *pWrightMin = 0; *pWrightMax = 0; }
   virtual bool CanTopWidthVary() const override { return false; }
   virtual WebIndexType GetWebCount(const IBeamFactory::Dimensions& dimensions) const override;
   virtual Float64 GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const override;
   virtual void GetBeamTopWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const override;
   virtual Float64 GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const override;
   virtual bool IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const override;
   virtual void GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const override;
   virtual bool HasLongitudinalJoints() const override;
   virtual bool IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const override;
   virtual bool HasTopFlangeThickening() const override;
   virtual bool CanPrecamber() const override;
   virtual GirderIndexType GetMinimumBeamCount() const override;

private:
   std::vector<std::_tstring> m_DimNames;
   std::vector<Float64> m_DefaultDims;
   std::vector<const WBFL::Units::Length*> m_DimUnits[2];

   void GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      Float64& A,Float64& B,Float64& C,Float64& F,Float64& W,Float64& Tt,Float64& Tb,Float64& Jmax,Float64& EndBlockLength) const;

   Float64 GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const;
   void PositionBeam(IDeckedSlabBeam* pBeam) const;
};
