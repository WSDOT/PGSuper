///////////////////////////////////////////////////////////////////////
// PGSuper Beam Factory
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This library is a part of the Washington Bridge Foundation Libraries
// and was developed as part of the Alternate Route Project
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the Alternate Route Library Open Source License as published by 
// the Washington State Department of Transportation, Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but is distributed 
// AS IS, WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE. See the Alternate Route Library Open Source 
// License for more details.
//
// You should have received a copy of the Alternate Route Library Open Source License 
// along with this program; if not, write to the Washington State Department of 
// Transportation, Bridge and Structures Office, P.O. Box  47340, 
// Olympia, WA 98503, USA or e-mail Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// The intent of licensing this interface with the ARLOSL is to provide
// third party developers a method of developing proprietary plug-ins
// to the software.
// 
// Any changes made to the interfaces defined in this file are
// are subject to the terms of the Alternate Route Library Open Source License.
//
// Components that implement the interfaces defined in this file are
// governed by the terms and conditions deemed appropriate by legal 
// copyright holder of said software.
///////////////////////////////////////////////////////////////////////

#pragma once

#include <Units\Units.h>
#include <PGSuperTypes.h>


#include <EAF\Broker.h>
#include <EAF/ComponentManager.h>

class IEffFlangeEngineer;
class pgsPoiMgr;

interface IShape;
interface ISuperstructureMemberSegment;
interface IGirderSection;
interface IStrandMover;

class CBridgeDescription2;
class GirderLibraryEntry;
class CPrecastSegmentData;

namespace PGS
{
   namespace Beams
   {
      class DistFactorEngineer;
      class PsLossEngineerBase;

      // Between PGSuper version 2.x and 3.x, all Class IDs of the beam factories where changed.
      // Files saved with version 2.x have the old CLSIDs and we need the new CLSID to create
      // the beam factory. This is easy to implement for our own beam factories,
      // however, we need a way to translate CLSIDs for extension beams. That is
      // what objects of this type do. External publishers of beam factors
      // must register their BeamFactoryCLSIDTranslator with the BeamFactoryCLSIDTranslator
      // category. We will discover all registered translators and include them in the
      // CLSID translation process.
      class BeamFactoryCLSIDTranslator : public WBFL::EAF::ComponentObject
      {
      public:
         virtual bool TranslateCLSID(LPCTSTR oldCLSID,LPCTSTR* newCLSID) = 0;
      };

      // Note that we play a game here to allow scalars and booleans into the system if the values are: 
      #define BFDIMUNITSCALAR 0  // The dimension is a scalar.
      #define BFDIMUNITBOOLEAN 1 // The dimension is a boolean.

      /// @brief Beam factory objects must be singletons. Beam factory objects must inherit from
      /// this class as well as from BeamFactory
      /// @tparam T The class name of the beam factory.
      template <typename T>
      class BeamFactorySingleton
      {
      public:
         static std::shared_ptr<T> GetInstance() { if (!instance) instance = T::CreateInstance(); return instance; }
      protected:
         BeamFactorySingleton() = default;
      private:
         static std::shared_ptr<T> instance;
      };
#define INIT_BEAM_FACTORY_SINGLETON(class) std::shared_ptr<class> PGS::Beams::BeamFactorySingleton<class>::instance = nullptr;
      
      /// @brief Factory object that creates beam instances and related entities.
      /// BeamFactory objects are singletons and access through the GetInstance method.
      ///
      /// The methods on the BeamFactory interface receive shared_ptr<Broker>. This is
      /// for use with the scope of the called function. Do not store the Broker pointer.
      /// If the Broker pointer must be stored, store it as a weak_ptr so circular references
      /// are not created.
      class BeamFactory : public WBFL::EAF::ComponentObject
      {
      public:
         using Dimension = std::pair<std::_tstring,Float64>;
         using Dimensions = std::vector<Dimension>;

         enum class BeamFace {Top, Bottom};

         // Creates a new girder section using the supplied dimensions.
         // The overall height and top flange height parameters alter the dimensions of the section
         // Use -1 to use the actual dimensions. These parameters are typically used for spliced girders
         virtual void CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const = 0;

         // Lays out the girder along the given superstructure member. This function must
         // create the segments that describe the girder line... ConfigureSegment will then be called to do the actual configuration
         virtual void CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment** ppSSMbrSegment) const = 0;

         // Creates the shape of a segment at the specified location Xs, using the parameters defined in pSegment
         // This method is useful to getting shapes for the UI associated with proposed changes to the segment data
         virtual void CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const = 0;

         // Returns the height of the segment at the specified location based on parameters defined in pSegment
         virtual Float64 GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const = 0;

         // Configures the segment including cross section and material models... called after CreateSegment is called
         virtual void ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const = 0;

         // Adds Points of interest at all cross section changes.
         // Note to implementer: use the <section change type> | attribute. Add other attributes as needed.
         // Section change type attributes are POI_SECTCHANGE_LEFTFACE, POI_SECTCHANGE_RIGHTFACE, and POI_SECTCHANGE_TRANSITION.
         // Use POI_SECTCHANGE_RIGHTFACE at start of member and POI_SECTCHANGE_LEFT face at end of member. Also,
         // use when there is an abrupt change in the section. Use POI_SECTCHANGE_TRANSITION wherever there is a smooth transition point.
         // For the casting yard POIs, put at least one poi at the start and end of the girder. 
         // For the bridge site POIs, put at least one poi at the stand and end bearings. DO NOT put bridge site POIs
         // before or after the girder bearings
         virtual void LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const = 0;

         // Creates an object that implements the DistFactorEngineerBase interface.
         virtual std::unique_ptr<DistFactorEngineer> CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType, const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const = 0;

         // Creates an object that implements the IPsLossEngineer interface. The returned
         // object is a COM object an must be managed through its reference count.
         //
         // Implementation Note: You must call SetBroker on the newly create object and supply
         // it with the pointer to the broker object provided by the caller.
         virtual std::unique_ptr<PsLossEngineerBase> CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const = 0;

         // The StrandMover object knows how to move harped strands within the section when
         // the group elevation is changed.
         // Hg is the height of the girder at the location where the strand mover is being created
         // if Hg is < 0, height is computed based on dimensions
         // Hg is typically only used for variable depth (non-prismatic) girders
         virtual void CreateStrandMover(const BeamFactory::Dimensions& dimensions, Float64 Hg,
                                        BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                        BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                        Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const = 0;

         // Returns a vector of strings representing the names of the dimensions that are used
         // to describe the cross section.
         virtual const std::vector<std::_tstring>& GetDimensionNames() const = 0;

         // Returns a vector of length unit objects representing the units of measure of each
         // dimension. 
         // Note that we play a game here to allow scalars and booleans into the system if the values are: 
         // (const WBFL::Units::Length*) BFDIMUNITSCALAR : The dimension is a scalar.
         // (const WBFL::Units::Length*) BFDIMUNITBOOLEAN: // The dimension is a boolean.
         virtual const std::vector<const WBFL::Units::Length*>& GetDimensionUnits(bool bSIUnits) const = 0;

         // Returns a defaults for the dimensions. Values are order to match the vector returned by
         // GetDimensionNames
         virtual const std::vector<Float64>& GetDefaultDimensions() const = 0;

         // Validates the dimensions. Return true if the dimensions are OK, otherwise false.
         // Return an error message through the strErrMsg pointer. If the error message
         // contains values, use the unit object to convert the value to display units and
         // append the appropriate unit tag
         virtual bool ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const = 0;

         // Saves the section dimensions to the storage unit
         virtual void SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const = 0;

         // Load the section dimensions from the storage unit
         virtual BeamFactory::Dimensions LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const = 0;

         // Returns true if the non-composite beam section is prismatic
         virtual bool IsPrismatic(const BeamFactory::Dimensions& dimensions) const = 0;

         // Returns true if the non-composite beam section is prismatic
         virtual bool IsPrismatic(const CSegmentKey& segmentKey) const = 0;

         // Returns true of the non-composite beam is longitudinally symmetric about its mid-point
         virtual bool IsSymmetric(const CSegmentKey& segmentKey) const = 0;

         // Returns the name of an image file that will be used in reports when the
         // cross section dimensions are reported. The image file must be in the same
         // directory as PGSuper.exe
         virtual std::_tstring GetImage() const = 0;

         virtual std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const = 0;
         virtual std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const = 0;
         virtual std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const = 0;
         virtual std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const = 0;
         virtual std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const = 0;
         virtual std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker, pgsTypes::SupportedDeckType deckType) const = 0;

         // Returns the class identifier for the beam factory
         virtual CLSID GetCLSID() const = 0;

         // Returns a name string that identifies this beam factory
         // this is not guaranteed to be unique
         virtual std::_tstring GetName() const
         {
            auto& component_info = WBFL::EAF::ComponentManager::GetInstance().GetComponent(GetCLSID());
            return component_info.name;
         }

         // Returns the class identifier for the beam family
         virtual CLSID GetFamilyCLSID() const = 0;

         // Returns a name string that identifies the general type of beam
         // this is not guaranteed to be unique
         virtual std::_tstring GetGirderFamilyName() const
         {
            auto& component_info = WBFL::EAF::ComponentManager::GetInstance().GetComponent(GetFamilyCLSID());
            return component_info.name;
         }

         // Returns the name of the company, organization, and/or person that published the
         // beam factory
         virtual std::_tstring GetPublisher() const = 0;

         // Returns contact information for the beam factory publisher. This
         // information is presented to the user if there is an error creating the factory
         virtual std::_tstring GetPublisherContactInformation() const = 0;

         // Returns the instance handle for resources
         virtual HINSTANCE GetResourceInstance() const = 0;

         // Returns the string name of the image resource that is used in the girder
         // library entry dialog. This resource must be an enhanced meta file.
         // Height and Width of the image must be equal
         virtual LPCTSTR GetImageResourceName() const = 0;

         // Returns the icon associated with the beam type
         // Icon should support both 16x16 and 32x32 formats
         virtual HICON GetIcon() const = 0;

         // Returns the deck types that may be used with a giving spacing type
         virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const = 0;

         // Returns true if the deck type is supported by this beam
         virtual bool IsSupportedDeckType(pgsTypes::SupportedDeckType deckType, pgsTypes::SupportedBeamSpacing sbs) const = 0;

         // Returns all of methods of beam spacing measurement, supported by this beam
         virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings() const = 0;

         // Returns true if spacingType is supported by this beam
         virtual bool IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const = 0;

         // Converts an unsupported spacing type and spacing to a supported type. Returns true if successful.
         // This method is generally used to convert spacing type and spacing when a beam type is updated (this happened with Bulb Tee girders)
         virtual bool ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const = 0;

         // Returns all of methods of work point location supported by this beam for the given spacing type
         // Generally, variable depth girders only allow the work point at girder top
         virtual pgsTypes::WorkPointLocations GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const = 0;

         // Returns true if work point location type is supported by this beam for the given spacing type
         virtual bool IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation workPointType) const = 0;

         // Returns all of the girder orientations types supported by this girder
         virtual std::vector<pgsTypes::GirderOrientationType> GetSupportedGirderOrientation() const = 0;
         virtual bool IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const = 0;
         virtual pgsTypes::GirderOrientationType ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const = 0;

         // Returns all of methods of intermediate diaphragms, supported by this beam
         // if the vector is empty, the beam doesn't support diaphragms
         virtual pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms() const = 0;

         // Returns all the types of intermediate diaphragm locations for a specified diaphragm type
         // supported by this beam
         virtual pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const = 0;

         // Returns allowable spacing distances for the given deck and spacing type.
         // Max spacing will be MAX_GIRDER_SPACING of no range is specified
         // Spacing is measured normal to the centerline of a typical girder
         virtual void GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const = 0;

         // Returns the top width types that are supported.Only applicable
         // when the girder spacing type requires top width as input (pgsTypes::sbsUniformAdjacentWithTopWidth or pgsTypes::sbsGeneralAdjacentWithTopWidth)
         virtual std::vector<pgsTypes::TopWidthType> GetSupportedTopWidthTypes() const = 0;

         // Returns the allowable range for the top width of this girder. Only applicable
         // when the girder spacing type requires top width as input (pgsTypes::sbsUniformAdjacentWithTopWidth or pgsTypes::sbsGeneralAdjacentWithTopWidth)
         // The min/max range values are provided for the left/right "halves" of the top width. If topWidthType is twtSymmetric or twtCenteredCG, there is only
         // one min/max range as the top width is defined as a single value. In this case only the left min/max parameters are valid
         virtual void GetAllowableTopWidthRange(pgsTypes::TopWidthType topWidthType, const BeamFactory::Dimensions& dimensions, Float64* pWleftMin, Float64* pWleftMax, Float64* pWrightMin, Float64* pWrightMax) const = 0;

         // Returns true if the top width can vary. Only applicable if girder supports a top width type
         virtual bool CanTopWidthVary() const = 0;

         // Returns the number of webs that the section has
         virtual WebIndexType GetWebCount(const BeamFactory::Dimensions& dimensions) const = 0;

         // Returns the height of the beam at the specified end
         virtual Float64 GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const = 0;

         // Returns the width of the beam at the specified end
         virtual Float64 GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const = 0;

         // Get the left and right widths from centerline of the top surface of the beam (e.g., out to out of all mating surface(s))
         // If beam has variable width top flange, this function returns the min specified width.
         virtual void GetBeamTopWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const = 0;

         // Shear key.
         // Area comes in two parts: first is constant for any spacing, second is factor to multiply by spacing
         virtual bool IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const = 0;
         virtual void GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const = 0;

         // Longitudinal Joint information
         // returns true if the beam supports longitudinal joints
         virtual bool HasLongitudinalJoints() const = 0;
         // returns true if the longitudinal joint is structural
         virtual bool IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const = 0;

         // Return true if beam supports top flange thickening
         virtual bool HasTopFlangeThickening() const = 0;

         // Return true if beam supports precamber
         virtual bool CanPrecamber() const = 0;

         // Returns the minimum number of girders that can be in a cross section.
         // Two girders is the typical minimum for single-stem sections. One girder can
         // be used in a section for multi-step sections like double-tees and U-beams. One
         // girder can also be used for "wide" girders such as box beams and voided slabs
         virtual GirderIndexType GetMinimumBeamCount() const = 0;

      protected:
         BeamFactory() = default;
      };

      class SplicedBeamFactory : public BeamFactory
      {
      public:
         // returns true if the section depth can be variable
         virtual bool SupportsVariableDepthSection() const = 0;
   
         // returns the dimension label for the depth of the section
         virtual LPCTSTR GetVariableDepthDimension() const = 0;

         // returns the supported segment depth variations based on the variable depth section mode
         virtual std::vector<pgsTypes::SegmentVariationType> GetSupportedSegmentVariations(bool bIsVariableDepthSection) const = 0;

         // returns true if variable depth bottom flange is support.
         virtual bool CanBottomFlangeDepthVary() const = 0;

         // returns the dimension label for the bottom flange depth
         virtual LPCTSTR GetBottomFlangeDepthDimension() const = 0;

         // returns true if the section supports end blocks
         virtual bool SupportsEndBlocks() const = 0;

         virtual Float64 GetBottomFlangeDepth(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const = 0;

      protected:
         SplicedBeamFactory() = default;
      };

      // From time to time, we move data out of a girder library entry, and thus out of the beam factory.
      // To keep bridge models unchanged, that data has to be moved into bridge model
      // Beam factories that implement this interface are given the opportunity to
      // modify the bridge model with any data that they may have loaded and need to
      // set in the bridge model.
      // For example, the top flange thickening, "D8", parameter was removed from deck bulb tees
      // and was made a parameter in the bridge model. The BulbTeeFactory loads the old D8 value
      // and then sets it to the appropriate bridge model parameter when the method on this interface
      // is called.

      class CompatibilityData
      {
      public:
         CompatibilityData() = default;
         CompatibilityData(const CompatibilityData& other) = default;
         ~CompatibilityData() = default;

         void AddValue(LPCTSTR strKey, Float64 value) { m_Values.insert(std::make_pair(strKey, value)); }
         bool GetValue(LPCTSTR strKey, Float64* pValue) const
         {
            auto found = m_Values.find(strKey);
            if (found == m_Values.end())
            {
               return false;
            }
            else
            {
               *pValue = found->second;
               return true;
            }
         }

      protected:
         std::map<std::_tstring, Float64> m_Values;
      };

      class BeamFactoryCompatibility
      {
      public:
         virtual std::shared_ptr<CompatibilityData> GetCompatibilityData() const = 0;
         virtual void UpdateBridgeModel(CBridgeDescription2* pBridgeDesc, const GirderLibraryEntry* pGirderEntry) const = 0;
      };



      template <class T>
      T ConvertIBeamDimensions(const T& dimensions)
      {
         // Convert old dimensions IBeam dimensions to new dimensions and put them in the correct order
         // Dimensions changed we the BeamFactory was updated to use the extended top flange width dimension

         // New W2 = Old W1
         // New W3 = Old W2
         // New W1 = 0
         // New W5 = Old W3
         // New D6 = Old D4
         // New D4 = Old D6
         // New H = Old D1 + D2 + D3 + D4 + D5 + D6 + D7

         // order of m_DimNames for new dimensions
         //m_DimNames.emplace_back(_T("D1"));
         //m_DimNames.emplace_back(_T("D2"));
         //m_DimNames.emplace_back(_T("D3"));
         //m_DimNames.emplace_back(_T("D4"));
         //m_DimNames.emplace_back(_T("D5"));
         //m_DimNames.emplace_back(_T("D6"));
         //m_DimNames.emplace_back(_T("H"));
         //m_DimNames.emplace_back(_T("T1"));
         //m_DimNames.emplace_back(_T("T2"));
         //m_DimNames.emplace_back(_T("W1"));
         //m_DimNames.emplace_back(_T("W2"));
         //m_DimNames.emplace_back(_T("W3"));
         //m_DimNames.emplace_back(_T("W4"));
         //m_DimNames.emplace_back(_T("W5"));
         //m_DimNames.emplace_back(_T("C1"));
         //m_DimNames.emplace_back(_T("EndBlockWidth"));
         //m_DimNames.emplace_back(_T("EndBlockLength"));
         //m_DimNames.emplace_back(_T("EndBlockTransition"));

         // H = old D1 + ... + old D7
         Float64 h = dimensions[1].second + dimensions[2].second + dimensions[3].second + dimensions[4].second + dimensions[5].second + dimensions[6].second + dimensions[7].second;

         BeamFactory::Dimensions new_dimensions;
         new_dimensions.emplace_back(dimensions[1]); // D1
         new_dimensions.emplace_back(dimensions[2]); // D2
         new_dimensions.emplace_back(dimensions[3]); // D3
         new_dimensions.emplace_back(_T("D4"), dimensions[6].second); // D4 = old D6
         new_dimensions.emplace_back(dimensions[5]); // D5
         new_dimensions.emplace_back(_T("D6"), dimensions[4].second); // D6 = old D4
         new_dimensions.emplace_back(_T("H"), h); // H
         new_dimensions.emplace_back(dimensions[8]); // T1
         new_dimensions.emplace_back(dimensions[9]); // T2
         new_dimensions.emplace_back(_T("W1"), 0.0); // W1
         new_dimensions.emplace_back(_T("W2"), dimensions[10].second); // W2 = old W1
         new_dimensions.emplace_back(_T("W3"), dimensions[11].second); // W3 = old W2
         new_dimensions.emplace_back(dimensions[13]); // W4
         new_dimensions.emplace_back(_T("W5"), dimensions[12].second); // W5 = old W3
         new_dimensions.emplace_back(dimensions[0]); // C1
         new_dimensions.emplace_back(dimensions[14]); // EndBlockWidth
         new_dimensions.emplace_back(dimensions[15]); // EndBlockLength
         new_dimensions.emplace_back(dimensions[16]); // EndBlockTransition

         return new_dimensions;
      }
   };
};
