///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_PSGLIB_CONCRETEMATERIAL_H_
#define INCLUDED_PSGLIB_CONCRETEMATERIAL_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

// LOCAL INCLUDES
//
#include <System\SubjectT.h>

// FORWARD DECLARATIONS
//
class pgsLibraryEntryDifferenceItem;
class ConcreteLibraryEntry;
class ConcreteLibraryEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL WBFL::System::SubjectT<ConcreteLibraryEntryObserver, ConcreteLibraryEntry>;

// MISCELLANEOUS
//
/*****************************************************************************
CLASS 
   ConcreteLibraryEntryObserver

   A pure virtual entry class for observing concrete material entries.


DESCRIPTION
   This class may be used to describe observe concrete  materials in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS ConcreteLibraryEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(ConcreteLibraryEntry& subject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   ConcreteLibraryEntry

   A library entry class for concrete materials.


DESCRIPTION
   This class may be used to describe concrete materials in a library.

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS ConcreteLibraryEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
   public WBFL::System::SubjectT<ConcreteLibraryEntryObserver, ConcreteLibraryEntry>
{
public:
   static CString GetConcreteType(pgsTypes::ConcreteType type);
   static CString GetConcreteCureMethod(pgsTypes::CureMethod method);
   static CString GetACI209CementType(pgsTypes::ACI209CementType type);
   static CString GetCEBFIPCementType(pgsTypes::CEBFIPCementType type);

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   ConcreteLibraryEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   ConcreteLibraryEntry(const ConcreteLibraryEntry& rOther) = default;

   //------------------------------------------------------------------------
   // Destructor
   virtual ~ConcreteLibraryEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   ConcreteLibraryEntry& operator=(const ConcreteLibraryEntry& rOther) = default;

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);


   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);
/*
   //------------------------------------------------------------------------
   // Description: Attaches an observer. The observer will be notified
   //              of changes.
   // Return:      None
   virtual void Attach(ConcreteLibraryEntryObserver* pObserver);

   //------------------------------------------------------------------------
   // Description: Detaches an observer.
   // Return:      None   
   virtual void Detach(ConcreteLibraryEntryObserver* pObserver);

   //------------------------------------------------------------------------
   // Notify all observers that we changed.
   virtual void Notify();
*/
    // GROUP: ACCESS
   //------------------------------------------------------------------------
   // SetFc - set crushing strength of concrete
   void SetFc(Float64 fc);

   //------------------------------------------------------------------------
   // GetFc - get crushing strength of concrete
   Float64 GetFc() const;

   void SetEc(Float64 ec);
   Float64 GetEc() const;
   void UserEc(bool bUserEc);
   bool UserEc() const;

   //------------------------------------------------------------------------
   // SetEc - get density used for strength calculation
   void SetStrengthDensity(Float64 d);

   //------------------------------------------------------------------------
   // GetStrengthDensity - set density used for strength calculation
   Float64 GetStrengthDensity() const;

   //------------------------------------------------------------------------
   // SetWeightDensity - set weight density of concrete
   void SetWeightDensity(Float64 d);

   //------------------------------------------------------------------------
   // GetWeightDensity - get weight density of concrete
   Float64 GetWeightDensity() const;

   //------------------------------------------------------------------------
   // SetAggregateSize - max aggregate size
   void SetAggregateSize(Float64 s);

   //------------------------------------------------------------------------
   // GetAggregateSize - max aggregate size
   Float64 GetAggregateSize()const;

   //------------------------------------------------------------------------
   // Parameters for the AASHTO material model (as defined in NCHRP Report 496)
   void SetModEK1(Float64 k1);
   Float64 GetModEK1() const;
   void SetModEK2(Float64 k2);
   Float64 GetModEK2() const;

   void SetCreepK1(Float64 k1);
   Float64 GetCreepK1() const;
   void SetCreepK2(Float64 k2);
   Float64 GetCreepK2() const;

   void SetShrinkageK1(Float64 k1);
   Float64 GetShrinkageK1() const;
   void SetShrinkageK2(Float64 k2);
   Float64 GetShrinkageK2() const;

   void SetType(pgsTypes::ConcreteType type);
   pgsTypes::ConcreteType GetType() const;
   void HasAggSplittingStrength(bool bHasFct);
   bool HasAggSplittingStrength() const;
   void SetAggSplittingStrength(Float64 fct);
   Float64 GetAggSplittingStrength() const;

   //------------------------------------------------------------------------
   // Parameters for the PCI UHPC concrete
   void SetPCIUHPC(Float64 ffc, Float64 frr, Float64 fiberLength,Float64 autogenousShrinkage, bool bPCTT);
   void GetPCIUHPC(Float64* ffc, Float64* frr, Float64* pFiberLength,Float64* pAutogenousShrinkage,bool* bPCTT) const;

   //------------------------------------------------------------------------
   // Parameters for the UHPC concrete
   void SetUHPC(Float64 ft_cri, Float64 ft_cr, Float64 ft_loc, Float64 et_loc,Float64 alpha_u,Float64 ecu,bool bExperimentalEcu,Float64 gammaU,Float64 fiberLength);
   void GetUHPC(Float64* ft_cri, Float64* ft_cr, Float64* ft_loc, Float64* et_loc,Float64* alpha_u,Float64* ecu,bool* pbExpermentalEcu,Float64* pGammaU,Float64* pFiberLength) const;

   //------------------------------------------------------------------------
   // Parameters for the ACI 209R-92 model
   bool UserACIParameters() const;
   void UserACIParameters(bool bUser);
   Float64 GetAlpha() const;
   void SetAlpha(Float64 a);
   Float64 GetBeta() const;
   void SetBeta(Float64 b);
   pgsTypes::CureMethod GetCureMethod() const;
   void SetCureMethod(pgsTypes::CureMethod cureMethod);
   pgsTypes::ACI209CementType GetACI209CementType() const;
   void SetACI209CementType(pgsTypes::ACI209CementType cementType);

   //------------------------------------------------------------------------
   // Parameters for the CEB-FIP model
   bool UserCEBFIPParameters() const;
   void UserCEBFIPParameters(bool bUser);
   Float64 GetS() const;
   void SetS(Float64 s);
   Float64 GetBetaSc() const;
   void SetBetaSc(Float64 betaSc);
   pgsTypes::CEBFIPCementType GetCEBFIPCementType() const;
   void SetCEBFIPCementType(pgsTypes::CEBFIPCementType cementType);

   //------------------------------------------------------------------------
   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const ConcreteLibraryEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences,bool& bMustRename,bool bReturnOnFirstDifference=false, bool considerName=false) const;
   
   bool IsEqual(const ConcreteLibraryEntry& rOther,bool bConsiderName=false) const;

private:
   // GROUP: DATA MEMBERS
   Float64 m_Fc;
   Float64 m_Ds;
   Float64 m_Dw;
   Float64 m_AggSize;
   Float64 m_FiberLength;
   bool m_bUserEc;
   Float64 m_Ec;
   pgsTypes::ConcreteType m_Type;

   // AASHTO Model Parameters
   Float64 m_EccK1;
   Float64 m_EccK2;
   Float64 m_CreepK1;
   Float64 m_CreepK2;
   Float64 m_ShrinkageK1;
   Float64 m_ShrinkageK2;
   bool m_bHasFct;
   Float64 m_Fct;

   // PCI UHPC Parameters
   Float64 m_Ffc;
   Float64 m_Frr;
   bool m_bPCTT;
   Float64 m_AutogenousShrinkage;

   // AASHTO UHPC Parameters
   Float64 m_ftcri;
   Float64 m_ftcr;
   Float64 m_ftloc;
   Float64 m_etloc;
   Float64 m_alpha_u;
   Float64 m_ecu;
   bool m_bExperimental_ecu;
   Float64 m_gamma_u;

   // ACI Model Parameters
   bool m_bUserACIParameters;
   Float64 m_A, m_B;
   pgsTypes::CureMethod m_CureMethod;
   pgsTypes::ACI209CementType m_ACI209CementType;

   // CEB-FIP Model Parameters
   bool m_bUserCEBFIPParameters;
   Float64 m_S,m_BetaSc;
   pgsTypes::CEBFIPCementType m_CEBFIPCementType;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PSGLIB_CONCRETEMATERIAL_H_
