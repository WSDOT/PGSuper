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

#ifndef INCLUDED_PGSEXT_DESIGNARTIFACT_H_
#define INCLUDED_PGSEXT_DESIGNARTIFACT_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_MAP_
#include <map>
#define INCLUDED_MAP_
#endif

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#if !defined INCLUDED_PGSEXT_SHEARZONEDATA_H_
#include <PgsExt\ShearZoneData.h>
#endif

#if !defined INCLUDED_PGSEXT_GIRDERDATA_H_
#include <PgsExt\GirderData.h>
#endif

#include <Material\ConcreteEx.h>
#include <PGSuperTypes.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsDesignArtifact

   Artifact that contains the results of a design attempt.


DESCRIPTION
   Artifact that contains the results of a design attempt.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.09.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsDesignArtifact
{
public:
   // GROUP: ENUMERATORS
   enum Outcome
   {
      Success,
      TooManyStrandsReqd,
      ReleaseStrength,
      OverReinforced,
      UnderReinforced,
      UltimateMomentCapacity,
      StrandSlopeOutOfRange,
      ExceededMaxHoldDownForce,
      ShearExceedsMaxConcreteStrength,
      TooManyStirrupsReqd,
      MaxIterExceeded,
      GirderLiftingConcreteStrength,
      GirderLiftingStability,
      GirderShippingConcreteStrength,
      GirderShippingStability,
      GirderShippingConfiguration,
      StressExceedsConcreteStrength,
      DebondDesignFailed,
      DesignCancelled,
      NoDesignRequested
   };

   // utility class to hold concrete strength design state information
   class PGSEXTCLASS ConcreteStrengthDesignState
   {
   public:
      ConcreteStrengthDesignState():
      m_MinimumControls(true)
      {;}

      void SetState(bool controlledByMin, pgsTypes::Stage stage, pgsTypes::StressType stressType, 
                    pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation);

      bool WasControlledByMinimum() const;
      pgsTypes::Stage Stage() const;
      pgsTypes::StressType StressType() const;
      pgsTypes::LimitState LimitState() const;
      pgsTypes::StressLocation StressLocation() const;

      std::_tstring AsString() const;

      bool operator==(const ConcreteStrengthDesignState& rOther) const;

      void Init() {m_MinimumControls=true;}

   private:
      bool m_MinimumControls;
      pgsTypes::Stage          m_Stage;
      pgsTypes::StressType     m_StressType;
      pgsTypes::LimitState     m_LimitState; // 
      pgsTypes::StressLocation m_StressLocation;
   };


   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsDesignArtifact();
   pgsDesignArtifact(SpanIndexType span,GirderIndexType gdr);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsDesignArtifact(const pgsDesignArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsDesignArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsDesignArtifact& operator = (const pgsDesignArtifact& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS

   void SetOutcome(Outcome outcome);
   Outcome GetOutcome() const;

   //------------------------------------------------------------------------
   SpanIndexType GetSpan() const;

   //------------------------------------------------------------------------
   GirderIndexType GetGirder() const;

   void SetDesignOptions(arDesignOptions options);
   arDesignOptions GetDesignOptions() const;

   // ==== Flexure-Related Properties ======
   //------------------------------------------------------------------------
   // DoDesignFlexure - If this is false, all flexure values are bogus.
   arFlexuralDesignType GetDoDesignFlexure() const;
   
   void SetNumStraightStrands(StrandIndexType Ns);
   StrandIndexType GetNumStraightStrands() const;
   
   void SetNumTempStrands(StrandIndexType Nt);
   StrandIndexType GetNumTempStrands() const;

   void SetNumHarpedStrands(StrandIndexType Nh);
   StrandIndexType GetNumHarpedStrands() const;

   void SetPjackStraightStrands(Float64 Pj);
   Float64 GetPjackStraightStrands() const;

   void SetUsedMaxPjackStraightStrands(bool Pj); // pjack set was max possible
   bool GetUsedMaxPjackStraightStrands() const;

   void SetPjackTempStrands(Float64 Pj);
   Float64 GetPjackTempStrands() const;

   void SetUsedMaxPjackTempStrands(bool Pj); // pjack set was max possible
   bool GetUsedMaxPjackTempStrands() const;

   void SetPjackHarpedStrands(Float64 Pj);
   Float64 GetPjackHarpedStrands() const;

   void SetUsedMaxPjackHarpedStrands(bool Pj); // pjack set was max possible
   bool GetUsedMaxPjackHarpedStrands() const;

   // offsets are measured from original strand grid location from library
   void SetHarpStrandOffsetEnd(Float64 oe);
   Float64 GetHarpStrandOffsetEnd() const;

   void SetHarpStrandOffsetHp(Float64 ohp);
   Float64 GetHarpStrandOffsetHp() const;

   DebondInfoCollection GetStraightStrandDebondInfo() const;
   void SetStraightStrandDebondInfo(const DebondInfoCollection& dbinfo);
   void ClearDebondInfo();

   void SetReleaseStrength(Float64 fci);
   Float64 GetReleaseStrength() const;

   void SetConcrete(matConcreteEx concrete);
   const matConcreteEx& GetConcrete() const;
   void SetConcreteStrength(Float64 fc);
   Float64 GetConcreteStrength() const;

   void SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(pgsTypes::MemberEndType end) const; // "A" dimension

   void SetLiftingLocations(Float64 left,Float64 right);
   Float64 GetLeftLiftingLocation() const;
   Float64 GetRightLiftingLocation() const;

   void SetTruckSupportLocations(Float64 left,Float64 right);
   Float64 GetLeadingOverhang() const;
   Float64 GetTrailingOverhang() const;

   // Set if elastic modulus is defined by user. Used by GetGirderConfiguration
   void SetUserEc(Float64 Ec);
   void SetUserEci(Float64 Eci);

   pgsTypes::TTSUsage GetTemporaryStrandUsage() const;

   GDRCONFIG GetGirderConfiguration() const;

   // design states for concrete strengths
   const ConcreteStrengthDesignState& GetReleaseDesignState() const;
   const ConcreteStrengthDesignState& GetFinalDesignState() const;

   void SetReleaseDesignState(const ConcreteStrengthDesignState& state);
   void SetFinalDesignState(const ConcreteStrengthDesignState& state);

   // ==== Shear (Stirrup)-Related Properties ======
   //------------------------------------------------------------------------
   // DoDesignShear - If this is false, all shear values are bogus.
   bool GetDoDesignShear() const;

   BarSizeType GetConfinementBarSize() const;
   void SetConfinementBarSize(BarSizeType barSize);
   ZoneIndexType GetLastConfinementZone() const;
   void SetLastConfinementZone(ZoneIndexType zone);

   ZoneIndexType GetNumberOfStirrupZonesDesigned() const;
   void SetNumberOfStirrupZonesDesigned(ZoneIndexType num);
   CShearZoneData GetShearZoneData(ZoneIndexType zoneNum) const;
   void SetShearZoneData(ZoneIndexType zoneNum, const CShearZoneData& rdata);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsDesignArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsDesignArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Outcome m_Outcome;

   SpanIndexType m_Span;
   GirderIndexType m_Gdr;

   arDesignOptions m_DesignOptions;

   StrandIndexType   m_Ns;
   StrandIndexType   m_Nh;
   StrandIndexType   m_Nt;
   Float64 m_PjS;
   bool    m_PjSUsedMax;
   Float64 m_PjH;
   bool    m_PjHUsedMax;
   Float64 m_PjT;
   bool    m_PjTUsedMax;
   Float64 m_HarpStrandOffsetEnd;
   Float64 m_HarpStrandOffsetHp;

   DebondInfoCollection m_SsDebondInfo;

   Float64 m_Fci;
   Float64 m_SlabOffset[2]; // "A" dimension at start and end of girder
   Float64 m_LiftLocLeft;
   Float64 m_LiftLocRight;
   Float64 m_ShipLocLeft;
   Float64 m_ShipLocRight;

   bool  m_IsUserEc;
   Float64 m_UserEc;
   bool  m_IsUserEci;
   Float64 m_UserEci;

   matConcreteEx m_Concrete;

   ConcreteStrengthDesignState m_ConcreteReleaseDesignState;
   ConcreteStrengthDesignState m_ConcreteFinalDesignState;

   enum sz {MAXSHEARZONES=4}; // enum hack
   ZoneIndexType  m_NumShearZones;
   CShearZoneData m_ShearZoneData[MAXSHEARZONES];

   ZoneIndexType m_LastConfinementZone;
   BarSizeType m_ConfinementBarSize;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void Init();
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_DESIGNARTIFACT_H_
