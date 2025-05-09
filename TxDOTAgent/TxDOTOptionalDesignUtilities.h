///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#ifndef INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_
#define INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_

// SYSTEM INCLUDES
//


// PROJECT INCLUDES
//
#include <Materials/PsStrand.h>
#include <System\Tokenizer.h>
#include <System\FileStream.h>

#include <PGSuperTypes.h>
#include <pgsExt\StrandData.h>

#include <IFace\Bridge.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>
#include "TxDOTAppPlugin.h"

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
// Span and girder for our template model
#define TOGA_SPAN 0
#define TOGA_NUM_GDRS 8
#define TOGA_ORIG_GDR 3
#define TOGA_FABR_GDR 4

/*****************************************************************************
CLASS 

DESCRIPTION
   Utilitity functions and classes

LOG
   rdp : 02.19.2010 : Created file
*****************************************************************************/

   // Location of template folders
inline CString GetTOGAFolder()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)EAFGetDocument()->GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CTxDOTAppPlugin* pAppP = dynamic_cast<CTxDOTAppPlugin*>(pAppPlugin.p);

   CString tfolder;
   pAppP->GetTemplateFolders(tfolder);

   return tfolder;
}


inline CString get_strand_size(WBFL::Materials::PsStrand::Size size )
{
   return CString(WBFL::Materials::PsStrand::GetSize(size, true/*US units*/).c_str());
}

BOOL DoParseTemplateFile(const LPCTSTR lpszPathName, CString& girderEntry, 
                              CString& leftConnEntry, CString& rightConnEntry,
                              CString& projectCriteriaEntry,
                              CString& folderName,
                              Float64& girderUnitWeight);

class OptionalDesignHarpedFillUtil
{
public:

   // utility struct to temporarily store and sort rows
   struct StrandRow
   {
      Float64 Elevation;
      std::_tstring fillListString; // "ABC..."

      StrandRow():
         Elevation(0.0)
      {;}

      StrandRow(Float64 elev):
         Elevation(elev)
         {;}

      bool operator==(const StrandRow& rOther) const 
      { 
         return ::IsEqual(Elevation,rOther.Elevation); 
      }

      bool operator<(const StrandRow& rOther) const 
      { 
         return Elevation < rOther.Elevation; 
      }
   };
   typedef std::set<StrandRow> StrandRowSet;
   typedef StrandRowSet::iterator StrandRowIter;

   // Function to retrieve fill letter for X location
   static TCHAR GetFillString(Float64 X)
   {
      ATLASSERT(X>0.0); // should be weeding out negative values

      // This is VERY TxDOT I beam specific. Expect X locations at 2" spacing: 1.0, 3.0, 5.0,...
      Float64 xin = WBFL::Units::ConvertFromSysUnits( X, WBFL::Units::Measure::Inch );
      int index = (int)Round((xin-1.0)/2.0);
      TCHAR label = (index % 26) + _T('A');

      return label;
   }

   static StrandRowSet GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi);
};

OptionalDesignHarpedFillUtil::StrandRowSet GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi);

//
// Take a girder entry with harped and straight strands and its clone. Make clone have strands converted to all
// straight with debonding
class GirderLibraryEntry;

void MakeHarpedCloneStraight(const GirderLibraryEntry* pGdrEntry, GirderLibraryEntry* pGdrClone);


// Function to compute columns in table that attempts to group all girders in a span per table
// Returns a list of number of columns per table. Size of list is number of tables to be created
static const int MIN_TBL_COLS=3; // Minimum columns in multi-girder table
static const int MAX_TBL_COLS=8; // Maximum columns in multi-girder table

std::list<ColumnIndexType> ComputeTableCols(const std::vector<CGirderKey>& girderKeys);

// Function to return number of raised straight strands as defined by top kern point
StrandIndexType GetNumRaisedStraightStrands(IBroker* pBroker, const CSegmentKey& segmentKey);

// Clasify the type of strand layout we have.
enum txcwStrandLayoutType { tslStraight, tslHarped, tslDebondedStraight, tslRaisedStraight, tslMixedHarpedRaised, tslMixedHarpedDebonded };

txcwStrandLayoutType GetStrandLayoutType(IBroker* pBroker, const CGirderKey& girderKey);

// Treatment of I Beams is unique for TxDOT
bool IsIBeam(IBroker* pBroker, const CGirderKey& girderKey);

// Criteria for TxDOT Non/Standard design
bool IsTxDOTStandardStrands(txcwStrandLayoutType strandLayoutType, pgsTypes::StrandDefinitionType sdtType, const CSegmentKey& segmentKey, IBroker* pBroker);



#endif // INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_

