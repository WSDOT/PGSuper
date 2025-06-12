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
#include "BoxBeamFactoryImpl.h"

namespace PGS
{
   namespace Beams
   {
      class BoxBeamFactory2 : public BoxBeamFactoryImpl, public BeamFactorySingleton<BoxBeamFactory2>
      {
      public:
         static std::shared_ptr<BoxBeamFactory2> CreateInstance() { return std::shared_ptr<BoxBeamFactory2>(new BoxBeamFactory2()); }

      protected:
         BoxBeamFactory2();

      public:
         // BeamFactory
         bool ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const override;
         void SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const override;
         BeamFactory::Dimensions LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const override;
         void CreateStrandMover(const BeamFactory::Dimensions& dimensions, Float64 Hg,
                                BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const override;
         std::_tstring GetImage() const override;
         CLSID GetCLSID() const override;
         LPCTSTR GetImageResourceName() const override;
         HICON GetIcon() const override;
         bool IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const override;
         void GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const override;
         bool HasLongitudinalJoints() const override;
         bool IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const override;
         Float64 GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const override;
         void GetBeamTopWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const override;
         void GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const override;

      protected:
         bool ExcludeExteriorBeamShearKeys(const BeamFactory::Dimensions& dimensions) const override { return true; }
         void DimensionBeam(const BeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const override;

      private:
         void GetDimensions(const BeamFactory::Dimensions& dimensions,
                                          Float64& H1, 
                                          Float64& H2, 
                                          Float64& H3, 
                                          Float64& H4, 
                                          Float64& H5,
                                          Float64& W1, 
                                          Float64& W2, 
                                          Float64& W3, 
                                          Float64& W4, 
                                          Float64& F1, 
                                          Float64& F2, 
                                          Float64& C1,
                                          Float64& J,
                                          Float64& endBlockLength) const;
      };
   };
};
