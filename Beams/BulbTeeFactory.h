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

#include <IFace/BeamFactory.h>
#include <Beams\Helper.h>

#include <vector>

namespace PGS
{
   namespace Beams
   {
      class BulbTeeFactory : public BeamFactory,
         public BeamFactoryCompatibility,
         public BeamFactorySingleton<BulbTeeFactory>
      {
      public:
         static std::shared_ptr<BulbTeeFactory> CreateInstance() { return std::shared_ptr<BulbTeeFactory>(new BulbTeeFactory()); }

      protected:
         BulbTeeFactory();

      public:
         // IBeamFactory
         void CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusItemIDType statusID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const override;
         void CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment** ppSegment) const override;
         void CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const override;
         Float64 GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const override;
         void ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const override;
         void LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const override;
         std::unique_ptr<DistFactorEngineer> CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const pgsTypes::SupportedBeamSpacing* pSpacingType, const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const override;
         std::unique_ptr<PsLossEngineerBase> CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID, const CGirderKey& girderKey) const override;
         void CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const override;
         const std::vector<std::_tstring>& GetDimensionNames() const override;
         const std::vector<Float64>& GetDefaultDimensions() const override;
         const std::vector<const WBFL::Units::Length*>& GetDimensionUnits(bool bSIUnits) const override;
         bool ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const override;
         void SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const override;
         BeamFactory::Dimensions LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const override;
         bool IsPrismatic(const BeamFactory::Dimensions& dimensions) const override;
         bool IsPrismatic(const CSegmentKey& segmentKey) const override;
         bool IsSymmetric(const CSegmentKey& segmentKey) const override;
         std::_tstring GetImage() const override;
         std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker, pgsTypes::SupportedDeckType deckType) const override;
         std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker, pgsTypes::SupportedDeckType deckType) const override;
         CLSID GetCLSID() const override;
         CLSID GetFamilyCLSID() const override;
         std::_tstring GetPublisher() const override;
         std::_tstring GetPublisherContactInformation() const override;
         HINSTANCE GetResourceInstance() const override;
         LPCTSTR GetImageResourceName() const override;
         HICON GetIcon() const override;
         pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const override;
         bool IsSupportedDeckType(pgsTypes::SupportedDeckType deckType, pgsTypes::SupportedBeamSpacing sbs) const override { return PGS::Beams::IsSupportedDeckType(deckType, this, sbs); }
         pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings() const override;
         bool IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const override;
         bool ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const override;
         pgsTypes::WorkPointLocations GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const override;
         bool IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation workPointType) const override;
         std::vector<pgsTypes::GirderOrientationType> GetSupportedGirderOrientation() const override;
         bool IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
         pgsTypes::GirderOrientationType ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const override;
         pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms() const override;
         pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const override;
         void GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const override;
         std::vector<pgsTypes::TopWidthType> GetSupportedTopWidthTypes() const override;
         void GetAllowableTopWidthRange(pgsTypes::TopWidthType topWidthType, const BeamFactory::Dimensions& dimensions, Float64* pWleftMin, Float64* pWleftMax, Float64* pWrightMin, Float64* pWrightMax) const override;
         bool CanTopWidthVary() const override { return false; }
         WebIndexType GetWebCount(const BeamFactory::Dimensions& dimensions) const override;
         Float64 GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const override;
         Float64 GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const override;
         void GetBeamTopWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const override;
         bool IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const override;
         void GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const override;
         bool HasLongitudinalJoints() const override;
         bool IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const override;
         bool HasTopFlangeThickening() const override;
         bool CanPrecamber() const override;
         GirderIndexType GetMinimumBeamCount() const override;

      // BeamFactoryCompatibility
      public:
         std::shared_ptr<CompatibilityData> GetCompatibilityData() const override;
         void UpdateBridgeModel(CBridgeDescription2* pBridgeDesc, const GirderLibraryEntry* pGirderEntry) const override;

      private:
         std::vector<std::_tstring> m_DimNames;
         std::vector<Float64> m_DefaultDims;
         std::vector<const WBFL::Units::Length*> m_DimUnits[2];

         mutable bool m_bHaveOldTopFlangeThickening; // set to true if we have an old D8 value that hasn't been retrieved yet (this is just for debugging... we don't want to override a value and lose it)
         mutable Float64 m_OldTopFlangeThickening; /// this is the obsolete D8 value we just loaded

         StatusCallbackIDType m_scidInformationalWarning;
         mutable StatusGroupIDType m_StatusGroupID;

         void GetDimensions(const BeamFactory::Dimensions& dimensions, Float64& c1,
                            Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,
                            Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& wmin,Float64& wmax,
                            Float64& t1,Float64& t2) const;

         Float64 GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const;

         bool IsPrismatic(const CPrecastSegmentData* pSegment) const;

         void ConfigureBeamShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, IBulbTee2* pBeam) const;
         void GetTopFlangeParameters(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64* pC, Float64* pN1, Float64* pN2,Float64* pLeft,Float64* pRight) const;
         void GetTopWidth(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, Float64* pLeft, Float64* pRight) const;
         Float64 GetFlangeThickening(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const;
         void PositionBeamShape(IBulbTee2* pBeam) const;
      };
   };
};
