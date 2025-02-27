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

#ifndef INCLUDED_PRODUCTMOMENTSTABLE_H_
#define INCLUDED_PRODUCTMOMENTSTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\RatingSpecification.h>
#include "ReportNotes.h"

interface IEAFDisplayUnits;

std::_tstring REPORTINGFUNC LiveLoadPrefix(pgsTypes::LiveLoadType llType);
void REPORTINGFUNC LiveLoadTableFooter(IBroker* pBroker,rptParagraph* pPara,const CGirderKey& girderKey,bool bDesign,bool bRating);

ColumnIndexType REPORTINGFUNC GetProductLoadTableColumnCount(IBroker* pBroker,const CGirderKey& girderkKey,pgsTypes::AnalysisType analysisType,bool bDesign,bool bRating,bool bSlabShrinkage,
                                                             bool* pbSegments,bool* pbConstruction,bool* pbDeck,bool* pbDeckPanels,bool* pbSidewalk,bool* pbShearKey,bool* pbLongitudinalJoint,bool* pbPedLoading,bool* pbPermit,bool* pbContinuousBeforeDeckCasting,GroupIndexType* pStartGroup,GroupIndexType* pEndGroup);

/*****************************************************************************
CLASS 
   CProductMomentsTable

   Encapsulates the construction of the product forces table.


DESCRIPTION
   Encapsulates the construction of the product forces table.

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CProductMomentsTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CProductMomentsTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CProductMomentsTable(const CProductMomentsTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CProductMomentsTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CProductMomentsTable& operator = (const CProductMomentsTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,
                             bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const;

protected:

   //------------------------------------------------------------------------
   void MakeCopy(const CProductMomentsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CProductMomentsTable& rOther);

};

template <class M,class T>
RowIndexType ConfigureProductLoadTableHeading(IBroker* pBroker,rptRcTable* p_table,bool bPierTable,bool bSlabShrinkage,bool bSegments,bool bConstruction,bool bDeck, bool bDeckPanels,bool bSidewalk,bool bShearKey,bool bLongitudinalJoints,bool bOverlay,bool bIsFutureOverlay,
                                     bool bDesign,bool bPedLoading,bool bPermit,bool bRating,pgsTypes::AnalysisType analysisType,bool bContinuousBeforeDeckCasting,typename IRatingSpecification* pRatingSpec,IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   p_table->SetNumberOfHeaderRows(2);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);

   //
   // Set up table headings
   //
   ColumnIndexType col = 0;

   p_table->SetRowSpan(0,col,2);
   if ( bPierTable )
   {
      (*p_table)(0,col++) << _T("");
   }
   else
   {
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   if ( bSegments )
   {
      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << COLHDR(_T("Erected") << rptNewLine << _T("Segments"),          M, unitT );
   }

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftGirder),          M, unitT );

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm),       M, unitT );

   if ( bShearKey )
   {
      if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << pProductLoads->GetProductLoadName(pgsTypes::pftShearKey);
         (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );
      }
      else
      {
         p_table->SetRowSpan(0,col,2);
         (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShearKey), M, unitT );
      }
   }

   if (bLongitudinalJoints)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint);
         (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
         (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint), M, unitT);
      }
   }

   if ( bConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << pProductLoads->GetProductLoadName(pgsTypes::pftConstruction);
         (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );
      }
      else
      {
         p_table->SetRowSpan(0,col,2);
         (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftConstruction), M, unitT );
      }
   }

   if (bDeck)
   {
      if (analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting)
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlab);
         (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
         (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);

         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad);
         (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
         (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
      }
      else
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlab), M, unitT);

         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad), M, unitT);
      }

      if (bSlabShrinkage)
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(_T("Deck") << rptNewLine << _T("Shrinkage"), M, unitT);
      }
   }

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && bContinuousBeforeDeckCasting )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel);
         (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );
      }
      else
      {
         p_table->SetRowSpan(0,col,2);
         (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel), M, unitT );
      }
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      if ( bSidewalk )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk);
         (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );
      }

      p_table->SetColumnSpan(0,col,2);
      (*p_table)(0,col) << pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier);
      (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
      (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );

      if ( bOverlay )
      {
         p_table->SetColumnSpan(0,col,2);
         if (bIsFutureOverlay)
         {
            (*p_table)(0,col) << _T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         else
         {
            (*p_table)(0,col) << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay);
         }
         (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );
      }
   }
   else
   {
      if ( bSidewalk )
      {
         p_table->SetRowSpan(0,col,2);
         (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk), M, unitT );
      }

      p_table->SetRowSpan(0,col,2);
      (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier), M, unitT );

      if ( bOverlay )
      {
         p_table->SetRowSpan(0,col,2);
         if (bIsFutureOverlay)
         {
            (*p_table)(0,col++) << COLHDR(_T("Future") << rptNewLine << pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), M, unitT );
         }
         else
         {
            (*p_table)(0,col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), M, unitT );
         }
      }
   }

   if ( bPedLoading )
   {
      p_table->SetColumnSpan(0,col,2);
      (*p_table)(0,col) << _T("$ Pedestrian");
      (*p_table)(1,col++) << COLHDR(_T("Max"), M, unitT );
      (*p_table)(1,col++) << COLHDR(_T("Min"), M, unitT );
   }

   if ( bDesign )
   {

      p_table->SetColumnSpan(0,col,2);
      (*p_table)(0,col) << _T("* Design Live Load");
      (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
      (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );

      if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("* Fatigue Live Load");
         (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );
      }

      if ( bPermit )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("* Permit Live Load");
         (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );
      }
   }

   if ( bRating )
   {
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("* Design Live Load");
         (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("* Legal Routine");
         (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << _T("* Legal Special");
         (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
         (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         p_table->SetColumnSpan(0, col, 2);
         (*p_table)(0, col) << _T("* Legal Emergency");
         (*p_table)(1, col++) << COLHDR(_T("Max"), M, unitT);
         (*p_table)(1, col++) << COLHDR(_T("Min"), M, unitT);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("* Permit Routine");
         (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         p_table->SetColumnSpan(0,col,2);
         (*p_table)(0,col) << _T("* Permit Special");
         (*p_table)(1,col++) << COLHDR(_T("Max"),   M, unitT );
         (*p_table)(1,col++) << COLHDR(_T("Min"),   M, unitT );
      }
   }

   return p_table->GetNumberOfHeaderRows(); // index of first row to report data
}

#endif // INCLUDED_PRODUCTMOMENTSTABLE_H_
