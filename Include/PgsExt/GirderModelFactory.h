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

#include <PgsExt\PgsExtExp.h>
#include <WBFLFem2d.h>
#include <WBFLCore.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\PoiPairMap.h>
#include <IFace\AnalysisResults.h>

// This utility class provides methods for creating a simple span FEM2d model for a precast segment, finding members in the segment
// and mapping POI's between PGSuper and the Fem2d model
class PGSEXTCLASS pgsGirderModelFactory
{
public:
   pgsGirderModelFactory(void);
   ~pgsGirderModelFactory(void);

   // Creates a simple span FEM2d model for a precast segment.
   // intervalIdx is used to define the interval at which section properties and section transitions are used
   virtual void CreateGirderModel(IBroker* pBroker,                            // broker to access PGSuper data
                                 IntervalIndexType intervalIdx,               // used for looking up section properties and section transition POIs
                                 const CSegmentKey& segmentKey,               // this is the segment that the modeling is build for
                                 Float64 leftSupportLoc,                      // distance from the left end of the model to the left support location
                                 Float64 rightSupportLoc,                     // distance from the right end of the model to the right support location
                                 Float64 segmentLength,                       // length of the segment
                                 Float64 E,                                   // modulus of elasticity
                                 LoadCaseIDType lcidGirder,                   // load case ID that is to be used to define the girder dead load
                                 bool bModelLeftCantilever,                   // if true, the cantilever defined by leftSupportLoc is modeled
                                 bool bModelRightCantilever,                  // if true, the cantilever defined by rightSupportLoc is modeled
                                 const PoiList& vPoi, // vector of PGSuper POIs that are to be modeld in the Fem2d Model
                                 IFem2dModel** ppModel,                       // the Fem2d Model
                                 pgsPoiPairMap* pPoiMap                           // a mapping of PGSuper POIs to Fem2d POIs
                                 );

   // searches the fem model of a girder to find the member ID and distance from start of member for
   // a location measured from the start of the girder
   static void FindMember(IFem2dModel* pModel,          // the Fem2d model to search
                          Float64 distFromStartOfModel, // distance from the start of the model
                          MemberIDType* pMbrID,         // Fem2d member ID
                          Float64* pDistFromStartOfMbr  // distance from the start of the Fem2d member
                          );

   // adds a PGSuper POI to the fem2d model. Returns the Fem2d POI ID
   static PoiIDPairType AddPointOfInterest(IFem2dModel* pModel,const pgsPointOfInterest& poi);

   // adds a vector of PGSuper POIs to the Fem2d model. Returns a vector of Fem2d POI IDs.
   static std::vector<PoiIDPairType> AddPointsOfInterest(IFem2dModel* pModel,const PoiList& vPoi);


protected:
   // Use template methods to allow children to add functionality
   // BuildModel returns length of model
   virtual void BuildModel(IBroker* pBroker,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey,Float64 segmentLength,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,const PoiList& vPoi,IFem2dModel** ppModel);
   virtual void ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,const PoiList& vPoi,IFem2dModel** ppModel);
   virtual void ApplyPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,const PoiList& vPoi,IFem2dModel** ppModel,pgsPoiPairMap* pPoiMap);

   static PoiIDType ms_FemModelPoiID;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pgsKdotHaulingGirderModelFactory
// 
// Subclass for modelling Kdot hauling analyis using different dynamic factors at cantilevers and girder interior
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PGSEXTCLASS pgsKdotHaulingGirderModelFactory : public pgsGirderModelFactory
{
public:
   pgsKdotHaulingGirderModelFactory(Float64 overhangFactor, Float64 interiorFactor);
   ~pgsKdotHaulingGirderModelFactory(void);

protected:
   // Use template methods to allow children to add functionality
   virtual void ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,const PoiList& vPoi,IFem2dModel** ppModel) override;

private:
   Float64 m_OverhangFactor;
   Float64 m_InteriorFactor;

   pgsKdotHaulingGirderModelFactory();
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pgsDesignHaunchLoadGirderModelFactory
// 
// Subclass for modelling differential haunch load for girder design
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PGSEXTCLASS pgsDesignHaunchLoadGirderModelFactory : public pgsGirderModelFactory
{
public:
   pgsDesignHaunchLoadGirderModelFactory( const std::vector<SlabLoad>& slabLoads, LoadCaseIDType slabLoadCase, LoadCaseIDType slabPadLoadCase);
   ~pgsDesignHaunchLoadGirderModelFactory(void);

protected:
   // Use template methods to allow children to add functionality
   virtual void ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,const PoiList& vPoi,IFem2dModel** ppModel) override;

private:
   pgsDesignHaunchLoadGirderModelFactory();

   std::vector<SlabLoad> m_SlabLoads;
   LoadCaseIDType m_SlabLoadCase;
   LoadCaseIDType m_SlabPadLoadCase;
};
