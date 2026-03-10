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

#include "resource.h"       // main symbols
#include <IFace/BeamFactory.h>
#include <Beams\Helper.h>

#include <vector>

namespace PGS
{
   namespace Beams
   {
      class BoxBeamFactoryImpl : public BeamFactory
      {
      protected:
         BoxBeamFactoryImpl() = default;

      public:
         // BeamFactory
         void CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const BeamFactory::Dimensions& dimensions, Float64 overallHeight, Float64 bottomFlangeHeight, IGirderSection** ppSection) const override;
         void CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment** ppSegment) const override;
         void ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const override;
         void CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const override;
         Float64 GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const override;
         void LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, pgsPoiMgr* pPoiMgr) const override;
         std::unique_ptr<DistFactorEngineer> CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const pgsTypes::SupportedBeamSpacing* pSpacingType, const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const override;
         std::unique_ptr<PsLossEngineerBase> CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID, const CGirderKey& girderKey) const override;
         const std::vector<std::_tstring>& GetDimensionNames() const override;
         const std::vector<const WBFL::Units::Length*>& GetDimensionUnits(bool bSIUnits) const override;
         const std::vector<Float64>& GetDefaultDimensions() const override;
         bool IsPrismatic(const BeamFactory::Dimensions& dimensions) const override;
         bool IsPrismatic(const CSegmentKey& segmentKey) const override;
         bool IsSymmetric(const CSegmentKey& segmentKey) const override;
         CLSID GetFamilyCLSID() const override;
         std::_tstring GetPublisher() const override;
         std::_tstring GetPublisherContactInformation() const override;
         HINSTANCE GetResourceInstance() const override;
         pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const override;
         bool IsSupportedDeckType(pgsTypes::SupportedDeckType deckType, pgsTypes::SupportedBeamSpacing sbs) const override { return PGS::Beams::IsSupportedDeckType(deckType, this, sbs); }
         pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings() const override;
         bool IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const override;
         bool ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const override;
         pgsTypes::WorkPointLocations GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const override;
         bool IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation workPointType) const override;
         std::vector<pgsTypes::GirderOrientationType> GetSupportedGirderOrientation() const override;
         bool IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
         pgsTypes::GirderOrientationType ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
         pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms() const override;
         pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const override;
         WebIndexType GetWebCount(const BeamFactory::Dimensions& dimensions) const override;
         Float64 GetBeamHeight(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType) const override;
         std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker, pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker, pgsTypes::SupportedDeckType deckType) const override;
         bool HasTopFlangeThickening() const override;
         bool CanPrecamber() const override;
         GirderIndexType GetMinimumBeamCount() const override;
         void GetAllowableTopWidthRange(pgsTypes::TopWidthType topWidthType, const BeamFactory::Dimensions& dimensions, Float64* pWleftMin, Float64* pWleftMax, Float64* pWrightMin, Float64* pWrightMax) const override { *pWleftMin = 0; *pWleftMax = 0; *pWrightMin = 0; *pWrightMax = 0; }
         std::vector<pgsTypes::TopWidthType> GetSupportedTopWidthTypes() const override { return std::vector<pgsTypes::TopWidthType>(); }
         bool CanTopWidthVary() const override { return false; }

      protected:
         std::vector<std::_tstring> m_DimNames;
         std::vector<Float64> m_DefaultDims;
         std::vector<const WBFL::Units::Length*> m_DimUnits[2];

         Float64 GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const;

         void ConfigureGirderShape(const CPrecastSegmentData* pSegment, const BeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const;
         virtual void DimensionBeam(const BeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const;

         virtual bool ExcludeExteriorBeamShearKeys(const BeamFactory::Dimensions& dimensions) const = 0;
      };
   };
};
