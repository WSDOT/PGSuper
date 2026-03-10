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
//
// 
#ifndef INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNGIRDERDATA_H_
#define INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNGIRDERDATA_H_

// SYSTEM INCLUDES
//


#if !defined INCLUDED_STRDATA_H_
#include <StrData.h>
#endif
// PROJECT INCLUDES
//
#include <Materials/PsStrand.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\StrandData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
   class CTxDOTOptionalDesignData; // our parent

// MISCELLANEOUS
//
// Simple observer interface for watching data
class ITxDataObserver
{
public:
   //------------------------------------------------------------------------
   // An == operator is not enough. We must know the type of change that was
   // made in order to fire the right events. That's what the following enum 
   // and function do.
   enum ChangeType {ctNone         = 0x0000,
                    ctLocal        = 0x0001, // only local application data - not pgsuper model
                    ctGirder       = 0x0002, // girder data only
                    ctPGSuper      = 0x0004 | ctGirder, // pgsuper model data was change
                    ctTemplateFile = 0x0008 | ctPGSuper   // template file changed
   };

   virtual void OnTxDotDataChanged(int change) = 0;
};



/*****************************************************************************
CLASS 
   CTxDOTOptionalDesignGirderData

  Class to manage girder input data for TxDOT Optional Girder Analysis plugin

DESCRIPTION
   Utility class for TxDOT Optional Girder Analysis description data. This class encapsulates all
   girder input data and implements the IStructuredLoad and IStructuredSave persistence interfaces.

LOG
   rdp : 02.19.2010 : Created file
*****************************************************************************/

class CTxDOTOptionalDesignGirderData
{
public:
   // Data
   enum StrandFillType {sfStandard, sfHarpedRows, sfDirectFill};

   // method used to fill strands
   void SetStrandFillType(StrandFillType val);
   StrandFillType GetStrandFillType() const;

   void SetStrandData(WBFL::Materials::PsStrand::Grade grade,
                      WBFL::Materials::PsStrand::Type type,
                     WBFL::Materials::PsStrand::Coating coating,
                      WBFL::Materials::PsStrand::Size size);

   void GetStrandData(WBFL::Materials::PsStrand::Grade* pgrade,
                      WBFL::Materials::PsStrand::Type* ptype,
                     WBFL::Materials::PsStrand::Coating* pcoating,
                      WBFL::Materials::PsStrand::Size* psize);

   void SetFci(Float64 val);
   Float64 GetFci() const;

   void SetFc(Float64 val);
   Float64 GetFc() const;

   // Data for standard fill
   // ========================
   StrandIndexType GetStrandCount();
   void SetNumStrands(StrandIndexType ns);

   // Data for direct manip fill
   // ===========================
   const CDirectStrandFillCollection& GetDirectFilledStraightStrands() const;
   void SetDirectFilledStraightStrands(const CDirectStrandFillCollection& coll) ;

   const std::vector<CDebondData>& GetDirectFilledStraightDebond() const;
   void SetDirectFilledStraightDebond(const std::vector<CDebondData>& info);

   bool ComputeDirectFillEccentricity(const GirderLibraryEntry* pGdrEntry, Float64* pEcc) const;

   // Utilities for standard fill
   // ===========================
   void SetStrandTo(Float64 val);
   Float64 GetStrandTo() const;

   std::vector<StrandIndexType> ComputeAvailableNumStrands(GirderLibrary* pLib); 
   bool ComputeToRange(GirderLibrary* pLib, StrandIndexType ns, Float64* pToLower, Float64* pToUpper);
   bool ComputeEccentricities(GirderLibrary* pLib, StrandIndexType ns, Float64 To, Float64* pEccEnds, Float64* pEccCL);

   // Data for non-standard fill
   // ==========================
   // Struct and container for strand data
   struct StrandRow
   {
      Float64          RowElev;
      StrandIndexType  StrandsInRow;

      StrandRow(): RowElev(-1.0), StrandsInRow(INVALID_INDEX)
      {;}

      StrandRow(Float64 rowElev, StrandIndexType strandsInRow=0):
      RowElev(rowElev), StrandsInRow(strandsInRow)
      {;}

      bool operator==(const StrandRow& rOther) const 
      { 
         if(! ::IsEqual(RowElev,rOther.RowElev))
            return false;

         return StrandsInRow == rOther.StrandsInRow;
      }

      bool operator<(const StrandRow& rOther) const 
      { 
         return RowElev < rOther.RowElev; 
      }
   };

   // Container sorted by elevation
   typedef std::set<StrandRow> StrandRowContainer;
   typedef StrandRowContainer::iterator StrandRowIterator;
   typedef StrandRowContainer::const_iterator StrandRowConstIterator;

   // Strand data at CL (always active)
   const StrandRowContainer GetStrandsAtCL() const;
   void SetStrandsAtCL(const StrandRowContainer& container);

   // Strand data at Ends (only active if GetUseDepressedStrands==true)
   const StrandRowContainer GetStrandsAtEnds() const;
   void SetStrandsAtEnds(const StrandRowContainer& container);

   // Utility functions for non-standard fill
   // Available rows and strands in row for UI
   struct StrandIncrement
   {
      // For each possible number of strands in a row there is a total number, 
      // an associated total number of depressed strands, and the global fill order for this increment
      StrandIndexType TotalStrands;
      StrandIndexType GlobalFill;

      bool WasFilled; // For use by later filling routines

      StrandIncrement():
      TotalStrands(0), GlobalFill(INVALID_INDEX), WasFilled(false)
      {;}
   };

   struct AvailableStrandsInRow
   {
      Float64          RowElev;
      StrandIndexType  MaxHarped;
      std::vector<StrandIncrement> AvailableStrandIncrements;

      AvailableStrandsInRow(): RowElev(-1.0), MaxHarped(0)
      {;}

      AvailableStrandsInRow(Float64 rowElev):
      RowElev(rowElev)
      {;}

      bool operator==(const AvailableStrandsInRow& rOther) const 
      { 
         return ::IsEqual(RowElev,rOther.RowElev);
      }

      bool operator<(const AvailableStrandsInRow& rOther) const 
      { 
         return RowElev < rOther.RowElev; 
      }
   };

   typedef std::set<AvailableStrandsInRow> AvailableStrandsInRowContainer;
   typedef AvailableStrandsInRowContainer::iterator AvailableStrandsInRowIterator;
   typedef AvailableStrandsInRowContainer::const_iterator AvailableStrandsInRowConstIterator;

   // Compute all available rows and available numbers of strands in each row
   // for the current library entry
   AvailableStrandsInRowContainer ComputeAvailableStrandRows(const GirderLibraryEntry* pGdrEntry); 

   // This nasty function checks that input strandrows can fit into the master library entry,
   // and if pCloneGdrEntry!=nullptr copies the strand locations into the clone. 
   // false is returned if an error occurs and rErrMsg will give a descriptive error.
   bool CheckAndBuildStrandRows(const GirderLibraryEntry* pMasterGdrEntry, 
                                const StrandRowContainer& rClRows, const StrandRowContainer& rEndRows, 
                                CString& rErrMsg, GirderLibraryEntry* pCloneGdrEntry=nullptr); 


   static void  GetGlobalStrandCoordinate(const GirderLibraryEntry* pGdrEntry, StrandIndexType globalIdx, Float64* pX, Float64* pY);

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   CTxDOTOptionalDesignGirderData(CTxDOTOptionalDesignData* pParent);

   //------------------------------------------------------------------------
   // Copy constructor
   CTxDOTOptionalDesignGirderData(const CTxDOTOptionalDesignGirderData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CTxDOTOptionalDesignGirderData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CTxDOTOptionalDesignGirderData& operator = (const CTxDOTOptionalDesignGirderData& rOther);

   //------------------------------------------------------------------------
   // Resets all the prestressing input to default values.
   void ResetData();

   //------------------------------------------------------------------------
   // Resets all the strand counts to zero
   void ResetStrandNoData();

   ////------------------------------------------------------------------------
   //bool operator==(const CTxDOTOptionalDesignGirderData& rOther) const;

   ////------------------------------------------------------------------------
   //bool operator!=(const CTxDOTOptionalDesignGirderData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

   // GROUP: ACCESS
   // Get library entry name from our parent
   CString GetGirderEntryName();

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CTxDOTOptionalDesignGirderData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CTxDOTOptionalDesignGirderData& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // no default construction
   CTxDOTOptionalDesignGirderData();
   // GROUP: DATA MEMBERS

   StrandFillType m_StrandFillType;

   WBFL::Materials::PsStrand::Grade m_Grade;
   WBFL::Materials::PsStrand::Type  m_Type;
   WBFL::Materials::PsStrand::Size  m_Size;
   WBFL::Materials::PsStrand::Coating m_Coating;

   Float64 m_Fci;
   Float64 m_Fc;

   // Data for standard fill
   // ========================
   StrandIndexType m_NumStrands;
   Float64 m_StrandTo;

   // Data for harped-row fill
   // ==========================
   StrandRowContainer m_StrandRowsAtCL;
   StrandRowContainer m_StrandRowsAtEnds;

   // Data for direct manip fill
   // ===========================
   CDirectStrandFillCollection m_DirectFilledStraightStrands;
   std::vector<CDebondData>   m_DirectFilledStraightDebond;

   // our parent
   CTxDOTOptionalDesignData* m_pParent;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   void FireChanged(ITxDataObserver::ChangeType change);

   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


#endif // INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNGIRDERDATA_H_

