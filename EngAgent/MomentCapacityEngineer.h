///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   // Copy constructor
   pgsMomentCapacityEngineer(const pgsMomentCapacityEngineer& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsMomentCapacityEngineer();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsMomentCapacityEngineer& operator = (const pgsMomentCapacityEngineer& rOther);

   // GROUP: OPERATIONS

   void SetBroker(IBroker* pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   void ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   void ComputeMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd);
   void ComputeCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);

   void ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   void ComputeMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd);
   void ComputeCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);

   void AnalyzeCrackedSection(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsMomentCapacityEngineer& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsMomentCapacityEngineer& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY
#if defined _DEBUG_SECTION_DUMP
   void DumpSection(const pgsPointOfInterest& poi,IGeneralSection* section, std::map<long,Float64> ssBondFactors,std::map<long,Float64> hsBondFactors,bool bPositiveMoment);
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
      pgsBondTool(IBroker* pBroker,const pgsPointOfInterest& poi,const GDRCONFIG& config);
      pgsBondTool(IBroker* pBroker,const pgsPointOfInterest& poi);

      Float64 GetBondFactor(StrandIndexType strandIdx,pgsTypes::StrandType strandType);
      bool IsDebonded(StrandIndexType strandIdx,pgsTypes::StrandType strandType);
      
   private:
      void Init();

      IBroker* m_pBroker;
      CComPtr<IPrestressForce> m_pPrestressForce;

      pgsPointOfInterest m_Poi;
      pgsPointOfInterest m_PoiMidSpan;
      GDRCONFIG m_Config;
      GDRCONFIG m_CurrentConfig;
      Float64 m_GirderLength;
      Float64 m_DistFromStart;
      bool m_bUseConfig;
      bool m_bNearMidSpan;
   };

   // GROUP: OPERATIONS
   void CreateStrandMaterial(SpanIndexType span,GirderIndexType gdr,IStressStrain** ppSS);

   void ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64 fpe,Float64 e_initial,pgsBondTool& bondTool,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd);
   void ComputeMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const CRACKINGMOMENTDETAILS& cmd,MINMOMENTCAPDETAILS* pmmcd);
   void ComputeCrackingMoment(pgsTypes::Stage stage,const GDRCONFIG& config,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);
   void ComputeCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcpe,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd);

   Float64 GetNonCompositeDeadLoadMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment);
   Float64 GetNonCompositeDeadLoadMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment);
   Float64 GetModulusOfRupture(const pgsPointOfInterest& poi,bool bPositiveMoment);
   Float64 GetModulusOfRupture(const GDRCONFIG& config,bool bPositiveMoment);
   void GetSectionProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,Float64* pSb,Float64* pSbc);
   void GetSectionProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,Float64* pSb,Float64* pSbc);
   void ComputeCrackingMoment(Float64 fr,Float64 fcpe,Float64 Mdnc,Float64 Sb,Float64 Sbc,CRACKINGMOMENTDETAILS* pcmd);


   void BuildCapacityProblem(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64 e_initial,pgsBondTool& bondTool,bool bPositiveMoment,IGeneralSection** ppProblem,IPoint2d** pntCompression,ISize2d** szOffset,Float64* pdt,std::map<StrandIndexType,Float64>* pBondFactors);
   // GROUP: INQUIRY

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
