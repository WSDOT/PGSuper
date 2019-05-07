///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_MOMENTCAPENG_H_
#define INCLUDED_MOMENTCAPENG_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <Details.h>
#include <IFace\PrestressForce.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
interface IGeneralSection;
interface IPoint2d;
interface ISize2d;
interface IStressStrain;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsMomentCapacityEngineer

   Encapsulates the computations of moment capacities


DESCRIPTION
   Encapsulates the computations of moment capacities

LOG
   rab : 12.10.1998 : Created file
*****************************************************************************/

class pgsMomentCapacityEngineer
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsMomentCapacityEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   // Destructor
   ~pgsMomentCapacityEngineer();

   // can't copy or assign because default isn't sufficient and we
   // haven't written explicit versions yet
   pgsMomentCapacityEngineer(const pgsMomentCapacityEngineer& other) = delete;
   pgsMomentCapacityEngineer& operator=(const pgsMomentCapacityEngineer& other) = delete;

   void SetBroker(IBroker* pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   Float64 GetMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const;
   std::vector<Float64> GetMomentCapacity(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const;
   const MOMENTCAPACITYDETAILS* GetMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const;
   std::vector<const MOMENTCAPACITYDETAILS*> GetMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const;

   Float64 GetCrackingMoment(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   const CRACKINGMOMENTDETAILS* GetCrackingMomentDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   void GetCrackingMomentDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config, bool bPositiveMoment, CRACKINGMOMENTDETAILS* pcmd) const;

   Float64 GetMinMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   const MINMOMENTCAPDETAILS* GetMinMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   void GetMinMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config, bool bPositiveMoment, MINMOMENTCAPDETAILS* pmmcd) const;
   std::vector<const MINMOMENTCAPDETAILS*> GetMinMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const;
   std::vector<const CRACKINGMOMENTDETAILS*> GetCrackingMomentDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const;
   std::vector<Float64> GetCrackingMoment(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const;
   std::vector<Float64> GetMinMomentCapacity(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment) const;

   Float64 GetIcr(const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   const CRACKEDSECTIONDETAILS* GetCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   std::vector<const CRACKEDSECTIONDETAILS*> GetCrackedSectionDetails(const PoiList& vPoi, bool bPositiveMoment) const;

private:
   //------------------------------------------------------------------------
   void ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const;
   void ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const;

   void ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const;
   void ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const;

   void AnalyzeCrackedSection(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD) const;

   bool IsDiaphragmConfined(const pgsPointOfInterest& poi) const;

   void ModelShape(IGeneralSection* pSection, IShape* pShape, Float64 dx, Float64 dy, IStressStrain* pMaterial, VARIANT_BOOL bIsVoid) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // GROUP: ACCESS
   // GROUP: INQUIRY
#if defined _DEBUG_SECTION_DUMP
   void DumpSection(const pgsPointOfInterest& poi,IGeneralSection* section, std::map<long,Float64> ssBondFactors,std::map<long,Float64> hsBondFactors,bool bPositiveMoment) const;
#endif // _DEBUG_SECTION_DUMP

private:
   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidUnknown;

   CComPtr<IMomentCapacitySolver> m_MomentCapacitySolver;
   CComPtr<ICrackedSectionSolver> m_CrackedSectionSolver;


   // GROUP: LIFECYCLE
   // GROUP: OPERATORS

   // GROUP: ACCESS
   class pgsBondTool
   {
   public:
      pgsBondTool(IBroker* pBroker,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr);

      Float64 GetBondFactor(StrandIndexType strandIdx,pgsTypes::StrandType strandType) const;
      bool IsDebonded(StrandIndexType strandIdx,pgsTypes::StrandType strandType) const;
      
   private:

      IBroker* m_pBroker;
      CComPtr<IPretensionForce> m_pPrestressForce;

      pgsPointOfInterest m_Poi;
      pgsPointOfInterest m_PoiMidSpan;
      const GDRCONFIG* m_pConfig;
      Float64 m_GirderLength;
      Float64 m_DistFromStart;
      bool m_bNearMidSpan;
   };

   // GROUP: OPERATIONS
   void CreateStrandMaterial(const CSegmentKey& segmentKey,IStressStrain** ppSS) const;
   void CreateTendonMaterial(const CGirderKey& girderKey,IStressStrain** ppSS) const;

   MOMENTCAPACITYDETAILS ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64 fpe_ps,Float64 eps_initial_strand,const std::vector<Float64>& fpe_pt,const std::vector<Float64>& ept_initial,bool bPositiveMoment) const;
   void ComputeMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS* pmcd,const CRACKINGMOMENTDETAILS* pcmd,MINMOMENTCAPDETAILS* pmmcd) const;
   void ComputeCrackingMoment(IntervalIndexType intervalIdx,const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const;
   void ComputeCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const;

   Float64 GetNonCompositeDeadLoadMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const GDRCONFIG* pConfig=nullptr) const;
   Float64 GetModulusOfRupture(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const GDRCONFIG* pConfig=nullptr) const;
   void GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,Float64* pSb,Float64* pSbc) const;
   void GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,Float64* pSb,Float64* pSbc) const;
   void ComputeCrackingMoment(Float64 g1,Float64 g2,Float64 g3,Float64 fr,Float64 fcpe,Float64 Mdnc,Float64 Sb,Float64 Sbc,CRACKINGMOMENTDETAILS* pcmd) const;
   void GetCrackingMomentFactors(bool bPositiveMoment,Float64* pG1,Float64* pG2,Float64* pG3) const;

   void BuildCapacityProblem(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, Float64 eps_initial, const std::vector<Float64>& ept_initial, pgsBondTool& bondTool, bool bPositiveMoment, IGeneralSection** ppProblem, IPoint2d** pntCompression, ISize2d** szOffset, Float64* pdt, Float64* pH, Float64* pHaunch, std::map<StrandIndexType, Float64>* pBondFactors) const;

   // GROUP: INQUIRY

   pgsPointOfInterest GetEquivalentPointOfInterest(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const;

   // Moment capacity can be computed for every interval, however we are only supporting
   // it being computed at the interval when the deck becomes composite
   // index 0 = Moment Type (Positive = 0, Negative = 1)
   using MomentCapacityDetailsContainer = std::map<PoiIDType, MOMENTCAPACITYDETAILS>;
   MOMENTCAPACITYDETAILS ComputeMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig = nullptr) const;
   const MOMENTCAPACITYDETAILS* ValidateMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment,const GDRCONFIG* pConfig=nullptr) const;
   const MOMENTCAPACITYDETAILS* GetCachedMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment,const GDRCONFIG* pConfig=nullptr) const;
   const MOMENTCAPACITYDETAILS* GetStoredMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const MomentCapacityDetailsContainer& container) const;
   const MOMENTCAPACITYDETAILS* StoreMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const MOMENTCAPACITYDETAILS& mcd, MomentCapacityDetailsContainer& container) const;

   mutable MOMENTCAPACITYDETAILS m_InvalidPoiMomentCapacity; // used to store moment capacity results for the last poi with id of INVALID_ID
   mutable MomentCapacityDetailsContainer m_NonCompositeMomentCapacity[2];
   mutable MomentCapacityDetailsContainer m_CompositeMomentCapacity[2];
   void InvalidateMomentCapacity();

   using CrackingMomentContainer = std::map<PoiIDType, CRACKINGMOMENTDETAILS>;
   CrackingMomentContainer m_NonCompositeCrackingMoment[2];
   CrackingMomentContainer m_CompositeCrackingMoment[2];
   const CRACKINGMOMENTDETAILS* ValidateCrackingMoments(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   void InvalidateCrackingMoments();

   // index 0 = Moment Type (Positive = 0, Negative = 1)
   mutable std::map<PoiIDType, MINMOMENTCAPDETAILS> m_NonCompositeMinMomentCapacity[2];
   mutable std::map<PoiIDType, MINMOMENTCAPDETAILS> m_CompositeMinMomentCapacity[2];
   const MINMOMENTCAPDETAILS* ValidateMinMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   void InvalidateMinMomentCapacity();

   using CrackedSectionDetailsContainer = std::map<PoiIDType, CRACKEDSECTIONDETAILS>;
   mutable CrackedSectionDetailsContainer m_CrackedSectionDetails[2]; // 0 = positive moment, 1 = negative moment
   const CRACKEDSECTIONDETAILS* ValidateCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const;
   void InvalidateCrackedSectionDetails();

   // Temporary Moment Capacity (idx 0 = positive moment, idx 1 = negative moment)
   // This is a cache of moment capacities computed based on a supplied GDRCONFIG (m_TempGirderConfig) and not the
   // current state of input. These moment capacities are usually cached for speed during design.
   mutable GDRCONFIG m_TempGirderConfig;
   mutable MomentCapacityDetailsContainer m_TempNonCompositeMomentCapacity[2];
   mutable MomentCapacityDetailsContainer m_TempCompositeMomentCapacity[2];


public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST

   DECLARE_LOGFILE;
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_MOMENTCAPENG_H_
