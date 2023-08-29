///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "psgLibLib.h"
#include <PGSuperTypes.h>
#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>
#include <System\SubjectT.h>
#include <LRFD\LRFRVersionMgr.h>
#include <array>

class pgsLibraryEntryDifferenceItem;
class CRatingDialog;
class RatingLibraryEntry;
class RatingLibraryEntryObserver;
PSGLIBTPL WBFL::System::SubjectT<RatingLibraryEntryObserver, RatingLibraryEntry>;

class rptChapter;
interface IEAFDisplayUnits;

// Live Load, Load Factor Model before LRFR2013
class PSGLIBCLASS CLiveLoadFactorModel
{
   // the dialog is our friend.
   friend CRatingDialog;

public:
   CLiveLoadFactorModel();

   bool operator!=(const CLiveLoadFactorModel& other) const;
   bool operator==(const CLiveLoadFactorModel& other) const;

   void SetVehicleWeight(Float64 Wlower,Float64 Wupper);
   void GetVehicleWeight(Float64* pWlower,Float64* pWupper) const;

   void SetLiveLoadFactorType(pgsTypes::LiveLoadFactorType gllType);
   pgsTypes::LiveLoadFactorType GetLiveLoadFactorType() const;

   void SetLiveLoadFactorModifier(pgsTypes::LiveLoadFactorModifier gllModifier);
   pgsTypes::LiveLoadFactorModifier GetLiveLoadFactorModifier() const;

   void AllowUserOverride(bool bAllow);
   bool AllowUserOverride() const;

   void SetADTT(Int16 adtt1, Int16 adtt2, Int16 adtt3, Int16 adtt4);
   void GetADTT(Int16* adtt1, Int16* adtt2, Int16* adtt3, Int16* adtt4) const;

   void SetLowerLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetLowerLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   void SetUpperLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetUpperLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   void SetServiceLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetServiceLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   Float64 GetStrengthLiveLoadFactor(Int16 adtt,Float64 W) const;
   Float64 GetServiceLiveLoadFactor(Int16 adtt) const;

   //------------------------------------------------------------------------
   // Save to structured storage
   bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

private:
   Float64 m_Wlower, m_Wupper; // vehicle weight boundaries
   std::array<Int16, 4> m_ADTT; // index, 0=lower,1=middle,2=upper,3=unknown
   std::array<Float64, 4> m_gLL_Lower;
   std::array<Float64, 4> m_gLL_Upper;
   std::array<Float64, 4> m_gLL_Service;
   pgsTypes::LiveLoadFactorType m_LiveLoadFactorType;
   pgsTypes::LiveLoadFactorModifier m_LiveLoadFactorModifier;
   bool m_bAllowUserOverride;
};


// Live Load, Load Factor Model for LRFR2013 and later
class PSGLIBCLASS CLiveLoadFactorModel2
{
   // the dialog is our friend.
   friend CRatingDialog;

public:
   CLiveLoadFactorModel2();

   bool operator!=(const CLiveLoadFactorModel2& other) const;
   bool operator==(const CLiveLoadFactorModel2& other) const;

   void SetPermitWeightRatio(Float64 PWRlower,Float64 PWRupper);
   void GetPermitWeightRatio(Float64* pPWRlower,Float64* pPWRupper) const;

   void SetLiveLoadFactorType(pgsTypes::LiveLoadFactorType gllType);
   pgsTypes::LiveLoadFactorType GetLiveLoadFactorType() const;

   void SetLiveLoadFactorModifier(pgsTypes::LiveLoadFactorModifier gllModifier);
   pgsTypes::LiveLoadFactorModifier GetLiveLoadFactorModifier() const;

   void AllowUserOverride(bool bAllow);
   bool AllowUserOverride() const;

   void SetADTT(Int16 adtt1, Int16 adtt2, Int16 adtt3, Int16 adtt4);
   void GetADTT(Int16* adtt1, Int16* adtt2, Int16* adtt3, Int16* adtt4) const;

   void SetLowerLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetLowerLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   void SetMiddleLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetMiddleLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   void SetUpperLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetUpperLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   void SetServiceLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4);
   void GetServiceLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const;

   Float64 GetStrengthLiveLoadFactor(Int16 adtt,Float64 W) const;
   Float64 GetServiceLiveLoadFactor(Int16 adtt) const;

   //------------------------------------------------------------------------
   // Save to structured storage
   bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

private:
   Float64 m_PWRlower, m_PWRupper; // permit weight ratio boundaries
   std::array<Int16, 4> m_ADTT; // index, 0=lower,1=middle,2=upper,3=unknown
   std::array<Float64, 4> m_gLL_Lower;  // associated with lower value of PWR
   std::array<Float64, 4> m_gLL_Middle; // for PWR between lower and upper PWR, not used unless m_LoadFactorType is gllBilinearWithWeight
   std::array<Float64, 4> m_gLL_Upper;  // associated with upper value of PWR, not used unless m_LoadFactorType is gllBilinearWithWeight
   std::array<Float64, 4> m_gLL_Service;
   pgsTypes::LiveLoadFactorType m_LiveLoadFactorType;
   pgsTypes::LiveLoadFactorModifier m_LiveLoadFactorModifier;
   bool m_bAllowUserOverride;
};

/*****************************************************************************
CLASS 
   RatingLibraryEntryObserver

   A pure virtual entry class for observing Rating entries.


DESCRIPTION
   This class may be used to describe observe Rating entries in a library.

LOG
   rab : 12.07.2009 : Created file
*****************************************************************************/
class PSGLIBCLASS RatingLibraryEntryObserver
{
public:

   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(RatingLibraryEntry& subject, Int32 hint)=0;
};


/*****************************************************************************
CLASS 
   RatingLibraryEntry

   Library entry class for a parameterized rating specification


DESCRIPTION
   This class encapsulates all specification information required for
   prestressed girder load rating

LOG
   rab : 12.07.2009 : Created file
*****************************************************************************/

class PSGLIBCLASS RatingLibraryEntry : public WBFL::Library::LibraryEntry, public ISupportIcon,
       public WBFL::System::SubjectT<RatingLibraryEntryObserver, RatingLibraryEntry>
{
   // the dialog is our friend.
   friend CRatingDialog;

public:

   static CString GetLoadRatingType(pgsTypes::LoadRatingType ratingType);
   static CString GetSpecialPermitType(pgsTypes::SpecialPermitType permitType);

   //------------------------------------------------------------------------
   // Default constructor
   RatingLibraryEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   RatingLibraryEntry(const RatingLibraryEntry& rOther) = default;

   //------------------------------------------------------------------------
   // Destructor
   virtual ~RatingLibraryEntry() = default;

   //------------------------------------------------------------------------
   // Assignment operator
   RatingLibraryEntry& operator=(const RatingLibraryEntry& rOther) = default;

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing,int nPage=0);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(WBFL::System::IStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(WBFL::System::IStructuredLoad* pLoad);

   // Compares this library entry with rOther. Returns true if the entries are the same.
   // vDifferences contains a listing of the differences. The caller is responsible for deleting the difference items
   bool Compare(const RatingLibraryEntry& rOther, std::vector<std::unique_ptr<pgsLibraryEntryDifferenceItem>>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference=false,bool considerName=false) const;

   bool IsEqual(const RatingLibraryEntry& rOther,bool bConsiderName=false) const;

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   void SetDescription(LPCTSTR name);
   std::_tstring GetDescription(bool bApplySymbolSubstitution = true) const;

   void UseCurrentSpecification(bool bUseCurrent);
   bool UseCurrentSpecification() const;

   void SetSpecificationVersion(WBFL::LRFD::LRFRVersionMgr::Version version);
   WBFL::LRFD::LRFRVersionMgr::Version GetSpecificationVersion() const;

   // For use with LRFR before LRFR2013
   void SetLiveLoadFactorModel(pgsTypes::LoadRatingType ratingType,const CLiveLoadFactorModel& model);
   const CLiveLoadFactorModel& GetLiveLoadFactorModel(pgsTypes::LoadRatingType ratingType) const;

   // For use with LRFR before LRFR2013
   void SetLiveLoadFactorModel(pgsTypes::SpecialPermitType permitType,const CLiveLoadFactorModel& model);
   const CLiveLoadFactorModel& GetLiveLoadFactorModel(pgsTypes::SpecialPermitType permitType) const;

   // For use with LRFR2013 and later
   void SetLiveLoadFactorModel2(pgsTypes::LoadRatingType ratingType,const CLiveLoadFactorModel2& model);
   const CLiveLoadFactorModel2& GetLiveLoadFactorModel2(pgsTypes::LoadRatingType ratingType) const;

   // For use with LRFR2013 and later
   void SetLiveLoadFactorModel2(pgsTypes::SpecialPermitType permitType,const CLiveLoadFactorModel2& model);
   const CLiveLoadFactorModel2& GetLiveLoadFactorModel2(pgsTypes::SpecialPermitType permitType) const;


   void Report(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits) const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const RatingLibraryEntry& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const RatingLibraryEntry& rOther);

private:

   // general
   bool m_bUseCurrentSpecification;
   WBFL::LRFD::LRFRVersionMgr::Version m_SpecificationVersion;
   std::_tstring m_Description;

   // for use with LRFR before 2013
   std::array<CLiveLoadFactorModel, pgsTypes::lrLoadRatingTypeCount> m_LiveLoadFactorModels; // index is pgsTypes::LoadRatingType excluding lrPermit_Special
   std::array<CLiveLoadFactorModel, 3> m_SpecialPermitLiveLoadFactorModels; // index is pgsTypes::SpecialPermitType


                                                                            // for use with LRFR2013 and later
   std::array<CLiveLoadFactorModel2, pgsTypes::lrLoadRatingTypeCount> m_LiveLoadFactorModels2; // index is pgsTypes::LoadRatingType excluding lrPermit_Special
   std::array<CLiveLoadFactorModel2, 3> m_SpecialPermitLiveLoadFactorModels2; // index is pgsTypes::SpecialPermitType
};
