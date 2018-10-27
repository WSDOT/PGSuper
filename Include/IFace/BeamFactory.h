///////////////////////////////////////////////////////////////////////
// PGSuper Beam Factory
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <PgsExt\PoiMgr.h>

struct IDistFactorEngineer;
struct IEffFlangeEngineer;
struct IPsLossEngineer;
struct IBroker;

interface IShape;
interface ISuperstructureMemberSegment;
interface IGirderSection;
interface IStrandMover;

class CBridgeDescription2;
class GirderLibraryEntry;

// In order for PGSuper 2.x to work on the same computer as PGSuper 3.x we
// had to change all the Class IDs of the beam factories. Files saved with
// version 2.x have the old CLSIDs and we need the new CLSID to create
// the beam factory. This is easy to implement for our own beam factories,
// however, we need a way to translate CLSIDs for extension beams. That is
// what objects of this type do. External publishers of beam factors
// must register their BeamFactoryCLSIDTranslator with the BeamFactoryCLSIDTranslator
// category. We will discover all registered translaters and include them in the
// CLSID translation process.

// {2A687193-C7AD-4943-83AC-BF577CF805E2}
DEFINE_GUID(IID_IBeamFactoryCLSIDTranslator, 
0x2a687193, 0xc7ad, 0x4943, 0x83, 0xac, 0xbf, 0x57, 0x7c, 0xf8, 0x5, 0xe2);
interface IBeamFactoryCLSIDTranslator : public IUnknown
{
   virtual bool TranslateCLSID(LPCTSTR oldCLSID,LPCTSTR* newCLSID) = 0;
};


/*****************************************************************************
INTERFACE
   IBeamFactory

   Interface for creating generic precast beams and related objects.

DESCRIPTION
   Interface for creating generic precast beams and related objects.      
*****************************************************************************/
// {D3810B3E-91D6-4aed-A748-8ABEB87FCF44}
DEFINE_GUID(IID_IBeamFactory, 
0xd3810b3e, 0x91d6, 0x4aed, 0xa7, 0x48, 0x8a, 0xbe, 0xb8, 0x7f, 0xcf, 0x44);
struct __declspec(uuid("{D3810B3E-91D6-4aed-A748-8ABEB87FCF44}")) IBeamFactory;
interface IBeamFactory : IUnknown
{
   typedef std::pair<std::_tstring,Float64> Dimension;
   typedef std::vector<Dimension> Dimensions;

   enum BeamFace {BeamTop, BeamBottom};

   //---------------------------------------------------------------------------------
   // Creates a new girder section using the supplied dimensions.
   // The overall height and top flange height parameters alter the dimensions of the section
   // Use -1 to use the actual dimensions. These parameters are typically used for spliced girders
   virtual void CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const = 0;

   //---------------------------------------------------------------------------------
   // Creates a new girder profile shape using the supplied dimensions
   // This shape is used to draw the girder in profile (elevation)
   virtual void CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape) const = 0;

   //---------------------------------------------------------------------------------
   // Lays out the girder along the given superstructure member. This function must
   // create the segments that describe the girder line... ConfigureSegment will then be called to do the actual configuration
   virtual void CreateSegment(IBroker* pBroker, StatusGroupIDType statusGroupID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment** ppSSMbrSegment) const = 0;

   //---------------------------------------------------------------------------------
   // Configures the segment including cross section and material models... called after CreateSegment is called
   virtual void ConfigureSegment(IBroker* pBroker, StatusGroupIDType statusGroupID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const = 0;

   //---------------------------------------------------------------------------------
   // Adds Points of interest at all cross section changes.
   // Note to implementer: use the <section change type> | attribute. Add other attributes as needed.
   // Section change type attributes are POI_SECTCHANGE_LEFTFACE, POI_SECTCHANGE_RIGHTFACE, and POI_SECTCHANGE_TRANSITION.
   // Use POI_SECTCHANGE_RIGHTFACE at start of member and POI_SECTCHANGE_LEFT face at end of member. Also,
   // use when there is an abrupt change in the section. Use POI_SECTCHANGE_TRANSITION wherever there is a smooth transition point.
   // For the casting yard POIs, put at least one poi at the start and end of the girder. 
   // For the bridge site POIs, put at least one poi at the stand and end bearings. DO NOT put bridge site POIs
   // before or after the girder bearings
   virtual void LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const = 0;

   //---------------------------------------------------------------------------------
   // Creates an object that implements the IDistFactorEngineer interface. The returned
   // object is a COM object an must be managed through its reference count.
   //
   // Implementation Note: You must call SetBroker on the newly create object and supply
   // it with the pointer to the broker object provided by the caller.
   // const pointers have valid values to be used if non-nullptr
   virtual void CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType, const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const = 0;

   //---------------------------------------------------------------------------------
   // Creates an object that implements the IPsLossEngineer interface. The returned
   // object is a COM object an must be managed through its reference count.
   //
   // Implementation Note: You must call SetBroker on the newly create object and supply
   // it with the pointer to the broker object provided by the caller.
   virtual void CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const = 0;

   //---------------------------------------------------------------------------------
   // The StrandMover object knows how to move harped strands within the section when
   // the group elevation is changed.
   // Hg is the height of the girder at the location where the strand mover is being created
   // if Hg is < 0, height is computed based on dimensions
   // Hg is typically only used for variable depth (non-prismatic) girders
   virtual void CreateStrandMover(const IBeamFactory::Dimensions& dimensions, Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const = 0;

   //---------------------------------------------------------------------------------
   // Returns a vector of strings representing the names of the dimensions that are used
   // to describe the cross section.
   virtual const std::vector<std::_tstring>& GetDimensionNames() const = 0;

   //---------------------------------------------------------------------------------
   // Returns a vector of length unit objects representing the units of measure of each
   // dimenions. If an item in the vector is nullptr, the dimension is a scalar.
   virtual const std::vector<const unitLength*>& GetDimensionUnits(bool bSIUnits) const = 0;

   //---------------------------------------------------------------------------------
   // Returns a defaults for the dimensions. Values are order to match the vector returned by
   // GetDimensionNames
   virtual const std::vector<Float64>& GetDefaultDimensions() const = 0;

   //---------------------------------------------------------------------------------
   // Validates the dimensions. Return true if the dimensions are OK, otherwise false.
   // Return an error message through the strErrMsg pointer. If the error message
   // contains values, use the unit object to convert the value to display units and
   // append the appropreate unit tag
   virtual bool ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const = 0;

   //---------------------------------------------------------------------------------
   // Saves the section dimensions to the storage unit
   virtual void SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const = 0;

   //---------------------------------------------------------------------------------
   // Load the section dimensions from the storage unit
   virtual IBeamFactory::Dimensions LoadSectionDimensions(sysIStructuredLoad* pLoad) const = 0;

   //---------------------------------------------------------------------------------
   // Returns true if the non-composite beam section is prismatic
   virtual bool IsPrismatic(const IBeamFactory::Dimensions& dimensions) const = 0;

   //---------------------------------------------------------------------------------
   // Returns true if the non-composite beam section is prismatic
   virtual bool IsPrismatic(const CSegmentKey& segmentKey) const = 0;

   //---------------------------------------------------------------------------------
   // Returns true of the non-composite beam is longitudinally symmetric about its mid-point
   virtual bool IsSymmetric(const CSegmentKey& segmentKey) const = 0;

   //---------------------------------------------------------------------------------
   // Returns the interal surface area of voids within the member
   virtual Float64 GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey) const = 0;

   //---------------------------------------------------------------------------------
   // Returns the name of an image file that will be used in reports when the
   // cross section dimensions are reported. The image file must be in the same
   // directory as PGSuper.exe
   virtual std::_tstring GetImage() const = 0;

   virtual std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const = 0;
   virtual std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const = 0;
   virtual std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const = 0;
   virtual std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const = 0;
   virtual std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const = 0;
   virtual std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const = 0;

   //---------------------------------------------------------------------------------
   // Returns the class identifier for the beam factory
   virtual CLSID GetCLSID() const = 0;

   //---------------------------------------------------------------------------------
   // Returns a name string that identifies this beam factory
   // this is not guarenteed to be unique
   virtual std::_tstring GetName() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the class identifier for the beam family
   virtual CLSID GetFamilyCLSID() const = 0;

   //---------------------------------------------------------------------------------
   // Returns a name string that identifies the general type of beam
   // this is not guarenteed to be unique
   virtual std::_tstring GetGirderFamilyName() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the name of the company, organization, and/or person that published the
   // beam factory
   virtual std::_tstring GetPublisher() const = 0;

   // Returns contact information for the beam factory publisher. This
   // information is prented to the user if there is an error creating the factory
   virtual std::_tstring GetPublisherContactInformation() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the instance handle for resources
   virtual HINSTANCE GetResourceInstance() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the string name of the image resource that is used in the girder
   // library entry dialog. This resource must be an enhanced meta file.
   // Height and Width of the image must be equal
   virtual LPCTSTR GetImageResourceName() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the icon associated with the beam type
   // Icon should support both 16x16 and 32x32 formates
   virtual HICON GetIcon() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the deck types that may be used with a giving spacing type
   virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const = 0;
   
   //---------------------------------------------------------------------------------
   // Returns all of methods of beam spacing measurement, supported by this beam
   virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings() const = 0;

   //---------------------------------------------------------------------------------
   // Returns true if spacingType is supported by this beam
   virtual bool IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const = 0;

   //---------------------------------------------------------------------------------
   // Converts an unsupported spacing type and spacing to a supported type. Returns true if successful.
   // This method is generally used to convert spacing type and spacing when a beam type is updated (this happened with Bulb Tee girders)
   virtual bool ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const = 0;

   //---------------------------------------------------------------------------------
   // Returns all of the girder orientations types supported by this girder
   virtual std::vector<pgsTypes::GirderOrientationType> GetSupportedGirderOrientation() const = 0;
   virtual bool IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const = 0;
   virtual pgsTypes::GirderOrientationType ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const = 0;

   //---------------------------------------------------------------------------------
   // Returns all of methods of intermediate diaphragms, supported by this beam
   // if the vector is empty, the beam doesn't support diaphragms
   virtual pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms() const = 0;

   //---------------------------------------------------------------------------------
   // Returns all the types of intermediate diaphragm locations for a specified diaphragm type
   // supported by this beam
   virtual pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const = 0;

   //---------------------------------------------------------------------------------
   // Returns allowable spacing distances for the given deck and spacing type.
   // Max spacing will be MAX_GIRDER_SPACING of no range is specified
   // Spacing is measured normal to the centerline of a typical girder
   virtual void GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const = 0;

   //---------------------------------------------------------------------------------
   // Returns the top width types that are supported.Only applicable
   // when the girder spacing type requires top width as input (pgsTypes::sbsUniformAdjacentWithTopWidth or pgsTypes::sbsGeneralAdjacentWithTopWidth)
   virtual std::vector<pgsTypes::TopWidthType> GetSupportedTopWidthTypes() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the allowable range for the top width of this girder. Only applicable
   // when the girder spacing type requires top width as input (pgsTypes::sbsUniformAdjacentWithTopWidth or pgsTypes::sbsGeneralAdjacentWithTopWidth)
   virtual void GetAllowableTopWidthRange(const IBeamFactory::Dimensions& dimensions, Float64* pWmin, Float64* pWmax) const = 0;

   //---------------------------------------------------------------------------------
   // Returns true if the top width can vary. Only applicable if girder supports a top width type
   virtual bool CanTopWidthVary() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the number of webs that the section has
   virtual WebIndexType GetWebCount(const IBeamFactory::Dimensions& dimensions) const = 0;

   //---------------------------------------------------------------------------------
   // Returns the height of the beam at the specified end
   virtual Float64 GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const = 0;

   //---------------------------------------------------------------------------------
   // Returns the width of the beam at the specified end
   virtual Float64 GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const = 0;

   //---------------------------------------------------------------------------------
   // Shear key.
   // Area comes in two parts: first is contant for any spacing, second is factor to multiply by spacing
   virtual bool IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const = 0;
   virtual void GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const = 0;

   //---------------------------------------------------------------------------------
   // Longitudinal Joint information
   // returns true if the beam supports longitudinal joints
   virtual bool HasLongitudinalJoints() const = 0;
   // returns true if the longitudinal joint is structural
   virtual bool IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const = 0;

   //---------------------------------------------------------------------------------
   // Return true if beam supports top flange thickening
   virtual bool HasTopFlangeThickening() const = 0;

   //---------------------------------------------------------------------------------
   // Return true if beam supports precamber
   virtual bool CanPrecamber() const = 0;

   //---------------------------------------------------------------------------------
   // Returns the minimum number of girders that can be in a cross section.
   // Two girders is the typical minimum for single-stem sections. One girder can
   // be used in a section for multi-step sections like double-tees and U-beams. One
   // girder can also be used for "wide" girders such as box beams and voided slabs
   virtual GirderIndexType GetMinimumBeamCount() const = 0;
};

// {E97F7992-BE87-43cf-8657-A477EFC32B47}
DEFINE_GUID(IID_ISplicedBeamFactory, 
0xe97f7992, 0xbe87, 0x43cf, 0x86, 0x57, 0xa4, 0x77, 0xef, 0xc3, 0x2b, 0x47);
struct __declspec(uuid("{E97F7992-BE87-43cf-8657-A477EFC32B47}")) ISplicedBeamFactory;
interface ISplicedBeamFactory : IBeamFactory
{
   // returns true if the section depth can be variable
   virtual bool SupportsVariableDepthSection() const = 0;
   
   // returns the dimension label for the depth of the section
   virtual LPCTSTR GetVariableDepthDimension() const = 0;

   // returns the supported segment depth variations based on the varible depth section mode
   virtual std::vector<pgsTypes::SegmentVariationType> GetSupportedSegmentVariations(bool bIsVariableDepthSection) const = 0;

   // returns true if variable depth bottom flange is support.
   virtual bool CanBottomFlangeDepthVary() const = 0;

   // returns the dimension label for the bottom flange depth
   virtual LPCTSTR GetBottomFlangeDepthDimension() const = 0;

   // returns true if the section supports end blocks
   virtual bool SupportsEndBlocks() const = 0;
};

// From time to time, we move data out of a girder library entry, and thus out of the beam factory.
// To keep bridge models unchanged, that data has to be moved into bridge model
// Beam factories that implement this interface are given the opportity to
// modify the bridge model with any data that they may have loaded and need to
// set in the bridge model.
// For example, the top flange thickening, "D8", parameter was removed from deck bulb tees
// and was made a parameter in the bridge model. The BulbTeeFactory loads the old D8 value
// and then sets it to the appropate bridge model parameter when the method on this interface
// is called.

class pgsCompatibilityData
{
public:
   pgsCompatibilityData() {};
   pgsCompatibilityData(const pgsCompatibilityData* pOther) { m_Values = pOther->m_Values; }
   virtual ~pgsCompatibilityData() {};

   void AddValue(LPCTSTR strKey, Float64 value) { m_Values.insert(std::make_pair(strKey, value)); }
   Float64 GetValue(LPCTSTR strKey) const { return m_Values.find(strKey)->second; }

protected:
   std::map<std::_tstring, Float64> m_Values;
};

// {A49B0E40-8228-423F-9C36-A4D264AD374B}
DEFINE_GUID(IID_IBeamFactoryCompatibility ,
   0xa49b0e40, 0x8228, 0x423f, 0x9c, 0x36, 0xa4, 0xd2, 0x64, 0xad, 0x37, 0x4b);
struct __declspec(uuid("{A49B0E40-8228-423F-9C36-A4D264AD374B}")) IBeamFactoryCompatibility;
interface IBeamFactoryCompatibility : IUnknown
{
   virtual pgsCompatibilityData* GetCompatibilityData() const = 0;
   virtual void UpdateBridgeModel(CBridgeDescription2* pBridgeDesc, const GirderLibraryEntry* pGirderEntry) const = 0;
};
