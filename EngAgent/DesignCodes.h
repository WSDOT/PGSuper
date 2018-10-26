///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#ifndef INCLUDED_DESIGNCODES_H_
#define INCLUDED_DESIGNCODES_H_

// Simple class to manage design outcome return codes
class pgsDesignCodes
{
public:
   enum OutcomeType 
   { 
      PermanentStrandsChanged,
      TemporaryStrandsChanged,
      ShearDesignChanged,
      FcChanged,
      FciChanged,
      LiftingConfigChanged,
      HaulingConfigChanged,
      SlabOffsetChanged,
      LiftingRedesignAfterShipping,      // Continue with lifting design after shipping is successful
      RetainStrandProportioning,       // Retain strand proportioning information for next restart
      ChangedForUltimate,
      RaisedStraightStrands,   // Raised design will restart main design alg
      XXBogusOutcome // must be last in enum
   };

   void Reset()
   {
      m_Abort = false;
      m_Outcomes.reset();
   }

   void SetOutcome(OutcomeType outcome)
   {
      ATLASSERT(outcome<XXBogusOutcome);
      m_Outcomes.set(outcome);
   }

   bool GetOutcome(OutcomeType outcome)
   {
      return m_Outcomes[outcome];
   }

   bool DidGirderChange()
   {
      return DidConcreteChange() || DidStrandsChange();
   }

   bool DidConcreteChange()
   {
      return m_Outcomes[FcChanged] || m_Outcomes[FciChanged];
   }

   bool DidFinalConcreteStrengthChange()
   {
      return m_Outcomes[FcChanged];
   }

   bool DidStrandsChange()
   {
      return m_Outcomes[PermanentStrandsChanged] || 
             m_Outcomes[TemporaryStrandsChanged] ||
             m_Outcomes[RaisedStraightStrands];
   }

   bool DidRetainStrandProportioning()
   {
      return m_Outcomes[RetainStrandProportioning];
   }

   bool DidRaiseStraightStrands()
   {
      return m_Outcomes[RaisedStraightStrands];
   }

   // if true, everything is good... keep going
   void AbortDesign()
   {
      m_Abort=true;
   }

   bool WasDesignAborted()
   {
      return m_Abort;
   }

private:
   bool m_Abort;
   std::bitset<XXBogusOutcome> m_Outcomes;
};

#endif  // INCLUDED_DESIGNCODES_H_