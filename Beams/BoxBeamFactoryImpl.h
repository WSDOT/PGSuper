///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
class ATL_NO_VTABLE CBoxBeamFactoryImpl:
   public IBeamFactory
{
public:
	CBoxBeamFactoryImpl()
	{
	}

public:
   // IBeamFactory
   virtual void CreateGirderProfile(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape);
   virtual void CreateSegment(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr);
   virtual void LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr);
   virtual void CreateDistFactorEngineer(IBroker* pBroker,StatusItemIDType statusID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng);
   virtual void CreatePsLossEngineer(IBroker* pBroker,StatusItemIDType statusID,const CGirderKey& girderKey,IPsLossEngineer** ppEng);
   virtual std::vector<std::_tstring> GetDimensionNames();
   virtual std::vector<const unitLength*> GetDimensionUnits(bool bSIUnits);
   virtual std::vector<Float64> GetDefaultDimensions();
   virtual bool IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey);
   virtual bool IsSymmetric(IBroker* pBroker,const CSegmentKey& segmentKey);
   virtual std::_tstring GetName();
   virtual CLSID GetFamilyCLSID();
   virtual std::_tstring GetGirderFamilyName();
   virtual std::_tstring GetPublisher();
   virtual std::_tstring GetPublisherContactInformation();
   virtual HINSTANCE GetResourceInstance();
   virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs);
   virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings();
   virtual pgsTypes::SupportedDiaphragmTypes GetSupportedDiaphragms();
   virtual pgsTypes::SupportedDiaphragmLocationTypes GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type);
   virtual WebIndexType GetWebCount(const IBeamFactory::Dimensions& dimensions);
   virtual Float64 GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType);
   virtual std::_tstring GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType);
   virtual std::_tstring GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType);
   virtual GirderIndexType GetMinimumBeamCount();

protected:
   std::vector<std::_tstring> m_DimNames;
   std::vector<Float64> m_DefaultDims;
   std::vector<const unitLength*> m_DimUnits[2];

   Float64 GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name);

   virtual bool ExcludeExteriorBeamShearKeys() = 0;
   virtual bool UseOverallWidth() = 0;
};

#endif //__BOXBEAMFACTORYIMPL_H_
