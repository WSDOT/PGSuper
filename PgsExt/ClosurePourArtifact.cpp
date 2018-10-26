///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ClosurePourArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsClosurePourArtifact
****************************************************************************/
pgsClosurePourArtifact::pgsClosurePourArtifact(const CSegmentKey& closurePourKey) :
m_ClosurePourKey(closurePourKey)
{
}

pgsClosurePourArtifact::pgsClosurePourArtifact(const pgsClosurePourArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsClosurePourArtifact::~pgsClosurePourArtifact()
{
}

pgsClosurePourArtifact& pgsClosurePourArtifact::operator= (const pgsClosurePourArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool pgsClosurePourArtifact::operator<(const pgsClosurePourArtifact& rOther) const
{
   return m_ClosurePourKey < rOther.m_ClosurePourKey;
}

void pgsClosurePourArtifact::SetFlexuralStressArtifact(pgsTypes::StressType stressType,const pgsFlexuralStressArtifact& artifact)
{
   m_FlexuralStressArtifact[stressType] = artifact;
}

const pgsFlexuralStressArtifact* pgsClosurePourArtifact::GetFlexuralStressArtifact(pgsTypes::StressType stressType) const
{
   return &m_FlexuralStressArtifact[stressType];
}

pgsFlexuralStressArtifact* pgsClosurePourArtifact::GetFlexuralStressArtifact(pgsTypes::StressType stressType)
{
   return &m_FlexuralStressArtifact[stressType];
}

void pgsClosurePourArtifact::SetFlexuralCapacityArtifact(const pgsFlexuralCapacityArtifact& pmartifact,
                                                         const pgsFlexuralCapacityArtifact& nmartifact)
{
   m_PositiveMomentFlexuralCapacityArtifact = pmartifact;
   m_NegativeMomentFlexuralCapacityArtifact = nmartifact;
}

const pgsFlexuralCapacityArtifact* pgsClosurePourArtifact::GetPositiveMomentFlexuralCapacityArtifact() const
{
   return &m_PositiveMomentFlexuralCapacityArtifact;
}

const pgsFlexuralCapacityArtifact* pgsClosurePourArtifact::GetNegativeMomentFlexuralCapacityArtifact() const
{
   return &m_NegativeMomentFlexuralCapacityArtifact;
}

const pgsStirrupCheckArtifact* pgsClosurePourArtifact::GetStirrupCheckArtifact() const
{
   return &m_StirrupCheckArtifact;
}

pgsStirrupCheckArtifact* pgsClosurePourArtifact::GetStirrupCheckArtifact()
{
   return &m_StirrupCheckArtifact;
}

//const pgsPrecastIGirderDetailingArtifact* pgsClosurePourArtifact::GetPrecastIGirderDetailingArtifact() const
//{
//   return &m_PrecastIGirderDetailingArtifact;
//}
//
//pgsPrecastIGirderDetailingArtifact* pgsClosurePourArtifact::GetPrecastIGirderDetailingArtifact()
//{
//   return &m_PrecastIGirderDetailingArtifact;
//}
//
//void pgsClosurePourArtifact::SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact)
//{
//   m_ConstructabilityArtifact = artifact;
//}
//
//const pgsConstructabilityArtifact* pgsClosurePourArtifact::GetConstructabilityArtifact() const
//{
//   return &m_ConstructabilityArtifact;
//}
//
//pgsConstructabilityArtifact* pgsClosurePourArtifact::GetConstructabilityArtifact()
//{
//   return &m_ConstructabilityArtifact;
//}

//
//pgsDeflectionCheckArtifact* pgsClosurePourArtifact::GetDeflectionCheckArtifact()
//{
//   return &m_DeflectionCheckArtifact;
//}
//
//const pgsDeflectionCheckArtifact* pgsClosurePourArtifact::GetDeflectionCheckArtifact() const
//{
//   return &m_DeflectionCheckArtifact;
//}

bool pgsClosurePourArtifact::Passed() const
{
   bool bPassed = true;

   bPassed &= DidFlexuralStressesPass();

   bPassed &= m_PositiveMomentFlexuralCapacityArtifact.Passed();
   bPassed &= m_NegativeMomentFlexuralCapacityArtifact.Passed();

#pragma Reminder("UPDATE: finish speck checking in closure pour artifict") // if this artifact isn't obsolete
   //bPassed &= m_StirrupCheckArtifact.Passed();

   //bPassed &= m_PrecastIGirderDetailingArtifact.Passed();

   //if (m_pLiftingCheckArtifact.get()!=NULL)
   //   bPassed &= m_pLiftingCheckArtifact->Passed();

   //if (m_pHaulingCheckArtifact.get()!=NULL)
   //   bPassed &= m_pHaulingCheckArtifact->Passed();

   //bPassed &= m_DeflectionCheckArtifact.Passed();

   //for ( Uint16 i = 0; i < 3; i++ )
   //   bPassed &= m_DebondArtifact[i].Passed();

   return bPassed;
}

bool pgsClosurePourArtifact::DidFlexuralStressesPass() const
{
   bool bPassed = true;

   bPassed &= m_FlexuralStressArtifact[pgsTypes::Compression].Passed();
   bPassed &= m_FlexuralStressArtifact[pgsTypes::Tension].Passed();

   return bPassed;
}

Float64 pgsClosurePourArtifact::GetRequiredConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   Float64 fc_reqd = 0;
#pragma Reminder("UPDATE")
   //std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   //for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   //{
   //   const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;
   //   const pgsFlexuralStressArtifactKey& key = artifact.first;

   //   if ( key.GetStage() == stage && key.GetLimitState() == ls )
   //   {
   //      Float64 fc = artifact.second.GetRequiredConcreteStrength();

   //      if ( fc < 0 ) 
   //         return fc;

   //      if ( 0 < fc )
   //         fc_reqd = _cpp_max(fc,fc_reqd);
   //   }
   //}

   return fc_reqd;
}

Float64 pgsClosurePourArtifact::GetRequiredConcreteStrength() const
{
   Float64 fc_reqd = 0;
#pragma Reminder("UPDATE")

   //std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   //for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   //{
   //   const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;

   //   if ( artifact.first.GetStage() == pgsTypes::CastingYard )
   //      continue;

   //   Float64 fc = artifact.second.GetRequiredConcreteStrength();

   //   if ( fc < 0 ) // there is no concrete strength that will work
   //      return fc;

   //   if ( 0 < fc )
   //      fc_reqd = _cpp_max(fc,fc_reqd);
   //}

   //if (m_pHaulingCheckArtifact.get()!=NULL)
   //{
   //   Float64 fc_reqd_hauling_tens,fc_reqd_hauling_comp;
   //   Float64 fcMax = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? ::ConvertToSysUnits(105,unitMeasure::MPa) : ::ConvertToSysUnits(15.0,unitMeasure::KSI);
   //   bool min_rebar_reqd;
   //   m_pHaulingCheckArtifact->GetRequiredConcreteStrength(&fc_reqd_hauling_comp,&fc_reqd_hauling_tens,&min_rebar_reqd,fcMax,false);

   //   Float64 fc_reqd_hauling = max(fc_reqd_hauling_tens,fc_reqd_hauling_comp);

   //   if ( fc_reqd_hauling < 0 ) // there is no concrete strength that will work
   //      return fc_reqd_hauling;

   //   fc_reqd = _cpp_max(fc_reqd,fc_reqd_hauling);
   //}

   return fc_reqd;
}

Float64 pgsClosurePourArtifact::GetRequiredReleaseStrength() const
{
   Float64 fc_reqd = 0;
#pragma Reminder("UPDATE")

   //std::map<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>::const_iterator i;
   //for ( i = m_FlexuralStressArtifacts.begin(); i != m_FlexuralStressArtifacts.end(); i++ )
   //{
   //   const std::pair<pgsFlexuralStressArtifactKey,pgsFlexuralStressArtifact>& artifact = *i;

   //   if ( artifact.first.GetStage() != pgsTypes::CastingYard )
   //      continue;

   //   Float64 fc = artifact.second.GetRequiredConcreteStrength();

   //   if ( fc < 0 ) // there is no concrete strength that will work
   //      return fc;

   //   if ( 0 < fc )
   //      fc_reqd = _cpp_max(fc,fc_reqd);
   //}

   //if (m_pLiftingCheckArtifact.get()!=NULL)
   //{
   //   Float64 fc_reqd_lifting_tens,fc_reqd_lifting_comp;
   //   bool min_rebar_reqd;
   //   m_pLiftingCheckArtifact->GetRequiredConcreteStrength(&fc_reqd_lifting_comp,&fc_reqd_lifting_tens,&min_rebar_reqd);
   //   
   //   Float64 fc_reqd_lifting = max(fc_reqd_lifting_tens,fc_reqd_lifting_comp);

   //   if ( fc_reqd_lifting < 0 ) // there is no concrete strength that will work
   //      return fc_reqd_lifting;

   //   fc_reqd = _cpp_max(fc_reqd,fc_reqd_lifting);
   //}

   return fc_reqd;
}

const CSegmentKey& pgsClosurePourArtifact::GetClosurePourKey() const
{
   return m_ClosurePourKey;
}

void pgsClosurePourArtifact::MakeCopy(const pgsClosurePourArtifact& rOther)
{
   m_ClosurePourKey = rOther.m_ClosurePourKey;

   m_FlexuralStressArtifact[pgsTypes::Tension]     = rOther.m_FlexuralStressArtifact[pgsTypes::Tension];
   m_FlexuralStressArtifact[pgsTypes::Compression] = rOther.m_FlexuralStressArtifact[pgsTypes::Compression];
   m_PositiveMomentFlexuralCapacityArtifact        = rOther.m_PositiveMomentFlexuralCapacityArtifact;
   m_NegativeMomentFlexuralCapacityArtifact        = rOther.m_NegativeMomentFlexuralCapacityArtifact;
   m_StirrupCheckArtifact                          = rOther.m_StirrupCheckArtifact;
   //m_PrecastIGirderDetailingArtifact = rOther.m_PrecastIGirderDetailingArtifact;
   //m_ConstructabilityArtifact        = rOther.m_ConstructabilityArtifact;
   //m_DeflectionCheckArtifact         = rOther.m_DeflectionCheckArtifact;
}

void pgsClosurePourArtifact::MakeAssignment(const pgsClosurePourArtifact& rOther)
{
   MakeCopy( rOther );
}
