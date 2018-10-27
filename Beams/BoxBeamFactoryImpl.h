///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// CBoxBeamFactoryImpl.h : A partial implementation for box beams

#ifndef __BOXBEAMFACTORYIMPL_H_
#define __BOXBEAMFACTORYIMPL_H_

#include "resource.h"       // main symbols
#include "IFace\BeamFactory.h"

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactoryImpl
class ATL_NO_VTABLE CBoxBeamFactoryImpl :
   public IBeamFactory
{
public:
   CBoxBeamFactoryImpl()
   {
   }

public:
   // IBeamFactory
   virtual void CreateGirderProfile(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, const IBeamFactory::Dimensions& dimensions, IShape** ppShape) const override;
   virtual void CreateSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment** ppSegment) const override;
   virtual void ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const override;
   virtual void LayoutSectionChangePointsOfInterest(IBroker* pBroker, const CSegmentKey& segmentKey, pgsPoiMgr* pPoiMgr) const override;
   virtual void CreateDistFactorEngineer(IBroker* pBroker, StatusItemIDType statusID, const pgsTypes::SupportedBeamSpacing* pSpacingType, const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect, IDistFactorEngineer** ppEng) const override;
   virtual void CreatePsLossEngineer(IBroker* pBroker, StatusItemIDType statusID, const CGirderKey& girderKey, IPsLossEngineer** ppEng) const override;
   virtual const std::vector<std::_tstring>& GetDimensionNames() const override;
   virtual const std::vector<const unitLength*>& GetDimensionUnits(bool bSIUnits) const override;
   virtual const std::vector<Float64>& GetDefaultDimensions() const override;
   virtual bool IsPrismatic(const IBeamFactory::Dimensions& dimensions) const override;
   virtual bool IsPrismatic(const CSegmentKey& segmentKey) const override;
   virtual bool IsSymmetric(const CSegmentKey& segmentKey) const override;
   virtual std::_tstring GetName() const override;
   virtual CLSID GetFamilyCLSID() const override;
   virtual std::_tstring GetGirderFamilyName() const override;
   virtual std::_tstring GetPublisher() const override;
   virtual std::_tstring GetPublisherContactInformation() const override;
   virtual HINSTANCE GetResourceInstance() const override;
   virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const override;
   virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings() const override;
   virtual bool IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const override;
   virtual bool ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const override;
   virtual std::vector<pgsTypes::GirderOrientationType> GetSupportedGirderOrientation() const override;
   virtual bool IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
   virtual pgsTypes::GirderOrientationType ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
   virtual pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms() const override;
   virtual pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const override;
   virtual WebIndexType GetWebCount(const IBeamFactory::Dimensions& dimensions) const override;
   virtual Float64 GetBeamHeight(const IBeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType) const override;
   virtual std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker, pgsTypes::SupportedDeckType deckType) const override;
   virtual std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker, pgsTypes::SupportedDeckType deckType) const override;
   virtual bool HasTopFlangeThickening() const override;
   virtual bool CanPrecamber() const override;
   virtual GirderIndexType GetMinimumBeamCount() const override;
   virtual void GetAllowableTopWidthRange(const IBeamFactory::Dimensions& dimensions, Float64* pWmin, Float64* pWmax) const override { *pWmin = 0; *pWmax = 0; }
   virtual std::vector<pgsTypes::TopWidthType> GetSupportedTopWidthTypes() const override { return std::vector<pgsTypes::TopWidthType>(); }
   virtual bool CanTopWidthVary() const override { return false; }

protected:
   std::vector<std::_tstring> m_DimNames;
   std::vector<Float64> m_DefaultDims;
   std::vector<const unitLength*> m_DimUnits[2];

   Float64 GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const;

   virtual bool ExcludeExteriorBeamShearKeys() const = 0;
   virtual bool UseOverallWidth() const = 0;
};

#endif //__BOXBEAMFACTORYIMPL_H_
