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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\Helpers.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const CString& GetStrandDefinitionType(pgsTypes::StrandDefinitionType strandDefinitionType,pgsTypes::AdjustableStrandType adjustableStrandType)
{
   static std::array<CString, 5> strStrandDefTypes{
      _T("Number of Permanent Strands"),
      _T("Figure it out"),
      _T("Strand Locations"),
      _T("Strand Rows"),
      _T("Individual Strands")
   };

   static CString strAdjHarped(_T("Number of Straight and Harped Strands"));
   static CString strAdjStraight(_T("Number of Straight and Adjustable Straight Strands"));
   static CString strAdjStraightHarped(_T("Number of Straight and Number of Adjustable Strands"));

   if (strandDefinitionType == pgsTypes::sdtStraightHarped)
   {
      return adjustableStrandType == pgsTypes::asHarped ? strAdjHarped : (adjustableStrandType == pgsTypes::asStraight ? strAdjStraight : strAdjStraightHarped);
   }
   else
   {
      return strStrandDefTypes[strandDefinitionType];
   }
}

CString GetGirderSpacingType(pgsTypes::SupportedBeamSpacing spacingType, bool bSplicedGirder)
{
   switch (spacingType)
   {
   case pgsTypes::sbsUniform:
      return _T("Same spacing for all girders");

   case pgsTypes::sbsGeneral:
      if (bSplicedGirder)
      {
         return _T("Unique spacing for each precast segment");
      }
      else
      {
         return _T("Unique spacing for each girder");
      }

   case pgsTypes::sbsUniformAdjacent:
      if (bSplicedGirder)
      {
         return _T("Adjacent girders with same joint spacing in all groups");
      }
      else
      {
         return _T("Adjacent girders with same joint spacing in all spans");
      }

   case pgsTypes::sbsGeneralAdjacent:
      if (bSplicedGirder)
      {
         return _T("Adjacent girders with unique joint spacing for each group");
      }
      else
      {
         return _T("Adjacent girders with unique joint spacing for each span");
      }

   case pgsTypes::sbsConstantAdjacent:
      return _T("Adjacent girders with the same width for all girders");

   case pgsTypes::sbsUniformAdjacentWithTopWidth:
      if (bSplicedGirder)
      {
         return _T("Adjacent girders with same joint spacing and top flange width in all groups");
      }
      else
      {
         return _T("Adjacent girders with same joint spacing and top flange width in all spans");
      }

   case pgsTypes::sbsGeneralAdjacentWithTopWidth:
      if (bSplicedGirder)
      {
         return _T("Adjacent girders with unique joint spacing and top flange width for each group");
      }
      else
      {
         return _T("Adjacent girders with unique joint spacing and top flange width for each span");
      }

   default:
      ATLASSERT(false); // is there a new spacing type???
      return _T("Unknown");
   }
}

CString GetTopWidthType(pgsTypes::TopWidthType type)
{
   switch (type)
   {
   case pgsTypes::twtSymmetric:
      return CString(_T("Symmetric"));

   case pgsTypes::twtCenteredCG:
      return CString(_T("Centered CG"));

   case pgsTypes::twtAsymmetric:
      return CString(_T("Asymmetric"));

   default:
      ATLASSERT(false);
      return CString(_T("Unknown"));
   }
}


std::_tstring GetDesignTypeName(arFlexuralDesignType type)
{
   switch (type)
   {
   case dtDesignForHarping:
      return std::_tstring(_T("Harped Strand"));
      break;

   case dtDesignForDebonding:
      return std::_tstring(_T("Debonded Straight Strand"));
      break;

   case dtDesignFullyBonded:
      return std::_tstring(_T("Straight Strand"));
      break;

   case dtDesignFullyBondedRaised:
      return std::_tstring(_T("Raised Straight Strand"));
      break;

   case dtDesignForDebondingRaised:
      return std::_tstring(_T("Debonded and Raised Straight Strand"));
      break;

   case dtNoDesign:
      return std::_tstring(_T("Flexure Design Not Attempted"));
      break;

   default:
      ATLASSERT(0);
   }

   return std::_tstring(_T("Unknown Design Type"));
}

CString GetLiveLoadTypeName(pgsTypes::LiveLoadType llType)
{
   CString strName;
   switch (llType)
   {
   case pgsTypes::lltDesign:
      strName = "Design";
      break;

   case pgsTypes::lltFatigue:
      strName = "Fatigue";
      break;

   case pgsTypes::lltLegalRating_Routine:
      strName = "Legal Load - Routine Commercial Traffic";
      break;

   case pgsTypes::lltLegalRating_Special:
      strName = "Legal Load - Specialized Hauling Vehicles";
      break;

   case pgsTypes::lltLegalRating_Emergency:
      strName = "Legal Load - Emergency Vehicles";
      break;

   case pgsTypes::lltPedestrian:
      strName = "Pedestrian";
      break;

   case pgsTypes::lltPermit:
      strName = "Design Permit";
      break;

   case pgsTypes::lltPermitRating_Routine:
      strName = "Rating Permit - Routine/Annual Permit";
      break;

   case pgsTypes::lltPermitRating_Special:
      strName = "Rating Permit - Special/Limited Crossing Permit";
      break;

   default:
      ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return strName;
}

CString GetLiveLoadTypeName(pgsTypes::LoadRatingType ratingType)
{
   pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);
   CString strName = GetLiveLoadTypeName(llType);

   if (ratingType == pgsTypes::lrDesign_Inventory)
   {
      strName += CString(" - Inventory");
   }

   if (ratingType == pgsTypes::lrDesign_Operating)
   {
      strName += CString(" - Operating");
   }

   return strName;
}

LPCTSTR GetDeckTypeName(pgsTypes::SupportedDeckType deckType)
{
   switch (deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      return _T("Composite cast-in-place deck");

   case pgsTypes::sdtCompositeSIP:
      return _T("Composite stay-in-place deck panels");

   case pgsTypes::sdtCompositeOverlay:
      return _T("Composite structural overlay");

   case pgsTypes::sdtNonstructuralOverlay:
      return _T("Nonstructural overlay");

   case pgsTypes::sdtNone:
      return _T("None");

   default:
      ATLASSERT(false); // is there a new deck type?
   }

   return _T("Unknown deck type");
}

LPCTSTR GetCastDeckEventName(pgsTypes::SupportedDeckType deckType)
{
   switch (deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      return _T("Cast Deck");

   case pgsTypes::sdtCompositeOverlay:
      return _T("Install composite overlay");

   case pgsTypes::sdtNonstructuralOverlay:
      return _T("Install non-structural overlay");

   default:
      ATLASSERT(false); // shouldn't get here... deck type sdtNone is never cast
   }
   return _T("Bad deck type");
}


// Changes in girder fill can make debonding invalid. This algorithm gets rid of any
// debonding of strands that don't exist
bool ReconcileDebonding(const ConfigStrandFillVector& fillvec, std::vector<CDebondData>& rDebond)
{
   bool didErase = false;
   StrandIndexType strsize = fillvec.size();

   std::vector<CDebondData>::iterator it = rDebond.begin();
   while (it != rDebond.end())
   {
      if (it->strandTypeGridIdx > strsize || fillvec[it->strandTypeGridIdx] == 0)
      {
         it = rDebond.erase(it);
         didErase = true;
      }
      else
      {
         it++;
      }
   }

   return didErase;
}

bool ReconcileExtendedStrands(const ConfigStrandFillVector& fillvec, std::vector<GridIndexType>& extendedStrands)
{
   bool didErase = false;
   StrandIndexType strsize = fillvec.size();

   std::vector<GridIndexType>::iterator it(extendedStrands.begin());
   while (it != extendedStrands.end())
   {
      GridIndexType gridIdx = *it;
      if (strsize < gridIdx || fillvec[gridIdx] == 0)
      {
         it = extendedStrands.erase(it);
         didErase = true;
      }
      else
      {
         it++;
      }
   }

   return didErase;
}
