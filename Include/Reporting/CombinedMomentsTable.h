///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_COMBINEDMOMENTSTABLE_H_
#define INCLUDED_COMBINEDMOMENTSTABLE_H_

#include <Reporting\ReportingExp.h>
#include <Reporting\ReportNotes.h>

interface IBroker;
interface IStageMap;
interface IEAFDisplayUnits;
interface IRatingSpecification;

/*****************************************************************************
CLASS 
   CCombinedMomentsTable

   Encapsulates the construction of the combined moments table.


DESCRIPTION
   Encapsulates the construction of the combined moments table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.08.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CCombinedMomentsTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CCombinedMomentsTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CCombinedMomentsTable(const CCombinedMomentsTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CCombinedMomentsTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CCombinedMomentsTable& operator = (const CCombinedMomentsTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the combined results table
   // bDesign and bRating are only considered from stage = pgsTypes::BridgeSite3
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      SpanIndexType span,GirderIndexType girder,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType,
                      bool bDesign=true,bool bRating=true) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CCombinedMomentsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CCombinedMomentsTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
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
};

// INLINE METHODS
//
template <class M,class T>
int ConfigureLimitStateTableHeading(rptRcTable* pTable,bool bPierTable,bool bDesign,bool bPermit,bool bRating,bool bMoment,pgsTypes::AnalysisType analysisType,IStageMap* pStageMap,IRatingSpecification* pRatingSpec,IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   USES_CONVERSION;

   pTable->SetNumberOfHeaderRows(3);

   pTable->SetRowSpan(0,0, pTable->GetNumberOfHeaderRows() );
   pTable->SetRowSpan(1,0,-1);
   pTable->SetRowSpan(2,0,-1);

   RowIndexType ll_title_row = 0;
   RowIndexType ls_title_row = 1;
   RowIndexType min_max_row  = 2;

   ColumnIndexType ll_title_col = 1;
   ColumnIndexType ls_title_col = 1;
   ColumnIndexType min_max_col  = 1;

   if ( !bPierTable )
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*pTable)(0,0) << "";


   if ( analysisType == pgsTypes::Envelope )
   {
      if ( bDesign )
      {
         ColumnIndexType nCols = (bMoment ? 9 : 8);
         if ( bPermit )
            nCols += (bMoment ? 3 : 2);

         pTable->SetColumnSpan(ll_title_row,ll_title_col,nCols);
         (*pTable)(ll_title_row,ll_title_col++) << "Design";

         for ( int i = 0; i < nCols-1; i++ )
         {
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,-1);
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
         (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI));
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

         (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
           (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceIA));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
         (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceIII));
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

         (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
           (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::FatigueI));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
         (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI));
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

         (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );
         if ( bMoment )
         {
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
         }

         if (bPermit)
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++)<< COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++)<< COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }
      }

      if ( bRating )
      {
         ColumnIndexType nRatingCols = 0;
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            nRatingCols += (bMoment ? 5 : 4);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            nRatingCols += (bMoment ? 5 : 4);

         pTable->SetColumnSpan(ll_title_row,ll_title_col,nRatingCols);
         (*pTable)(ll_title_row,ll_title_col++) << "Rating";

         for ( int i = 0; i < nRatingCols-1; i++ )
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,-1);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_Inventory));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_Operating));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_LegalRoutine));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_LegalSpecial));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI_PermitRoutine));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII_PermitRoutine));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI_PermitSpecial));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII_PermitSpecial));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }
      }
   }
   else
   {
      if ( bDesign )
      {
         ColumnIndexType nCol = (bMoment ? 6 : 5);
         if ( bPermit )
            nCol += (bMoment ? 3 : 2);

         pTable->SetColumnSpan(ll_title_row,ll_title_col,nCol);
         (*pTable)(ll_title_row,ll_title_col++) << "Design";

         for ( ColumnIndexType i = 0; i < nCol-1; i++ )
         {
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,-1);
         }

         pTable->SetRowSpan(ls_title_row,ls_title_col,2);
         pTable->SetRowSpan(min_max_row,min_max_col++,-1);
         (*pTable)(ls_title_row,ls_title_col++) << COLHDR(OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI)), M, unitT );

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pTable->SetRowSpan(ls_title_row,ls_title_col,2);
            pTable->SetRowSpan(min_max_row,min_max_col++,-1);
            (*pTable)(ls_title_row,ls_title_col++) << COLHDR(OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceIA)), M, unitT );
         }
         
         pTable->SetRowSpan(ls_title_row,ls_title_col,2);
         pTable->SetRowSpan(min_max_row,min_max_col++,-1);
         (*pTable)(ls_title_row,ls_title_col++) << COLHDR(OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceIII)), M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetRowSpan(ls_title_row,ls_title_col,2);
            pTable->SetRowSpan(min_max_row,min_max_col++,-1);
            (*pTable)(ls_title_row,ls_title_col++) << COLHDR(OLE2A(pStageMap->GetLimitStateName(pgsTypes::FatigueI)), M, unitT );
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3: 2);
         (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI));
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

         (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

         if ( bMoment )
         {
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
         }

         if (bPermit)
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }
      }

      if ( bRating )
      {
         ColumnIndexType nRatingCols = 0;
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            nRatingCols += (bMoment ? 3 : 2);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            nRatingCols += (bMoment ? 5 : 4);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            nRatingCols += (bMoment ? 5 : 4);

         pTable->SetColumnSpan(ll_title_row,ll_title_col,nRatingCols);
         (*pTable)(ll_title_row,ll_title_col++) << "Rating";

         for ( ColumnIndexType i = 0; i < nRatingCols-1; i++ )
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,-1);

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_Inventory));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_Operating));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_LegalRoutine));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthI_LegalSpecial));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI_PermitRoutine));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII_PermitRoutine));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::ServiceI_PermitSpecial));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << OLE2A(pStageMap->GetLimitStateName(pgsTypes::StrengthII_PermitSpecial));
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);

            (*pTable)(min_max_row,min_max_col++) << COLHDR("Max", M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR("Min", M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR("Slab", M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,-1);
            }
         }
      }
   }

   return pTable->GetNumberOfHeaderRows();
}

///////////////////////////////////////////////////////////////////

template <class M,class T>
RowIndexType CreateCombinedLoadingTableHeading(rptRcTable** ppTable,const char* strLabel,bool bPierTable,bool bDesign,bool bPermit,bool bPedLoading,bool bRating,pgsTypes::Stage stage,pgsTypes::Stage continuityStage,pgsTypes::AnalysisType analysisType,IRatingSpecification* pRatingSpec,IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   int nRows = 0;

   int col1 = 0;
   int col2 = 0;
   int nCols;

   rptRcTable* pTable;
   if ( stage == pgsTypes::CastingYard || stage == pgsTypes::GirderPlacement || stage == pgsTypes::TemporaryStrandRemoval )
   {
      nCols = 3;
      pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,strLabel);

      if ( !bPierTable )
         (*pTable)(0,col1++) << COLHDR(RPT_GDR_END_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      else
         (*pTable)(0,col1++) << "";

      (*pTable)(0,col1++) << COLHDR("DC",          M, unitT );
      (*pTable)(0,col1++) << COLHDR("Service I",   M, unitT );
      nRows = 1;
   }
   else if ( stage == pgsTypes::BridgeSite1)
   {
      nCols = 6;

      if ( analysisType == pgsTypes::Envelope && continuityStage == pgsTypes::BridgeSite1 )
         nCols += 5;

      pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,strLabel);

      if ( analysisType == pgsTypes::Envelope && continuityStage == pgsTypes::BridgeSite1 )
      {
         nRows = 2;

         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col2++,-1);
         if ( !bPierTable )
            (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         else
            (*pTable)(0,col1++) << "";

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "DC";
         (*pTable)(1,col2++) << COLHDR("Max", M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min", M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "DW";
         (*pTable)(1,col2++) << COLHDR("Max", M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min", M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << "DC";
         (*pTable)(1,col2++) << COLHDR("Max", M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min", M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << "DW";
         (*pTable)(1,col2++) << COLHDR("Max", M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min", M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "Service I";
         (*pTable)(1,col2++) << COLHDR("Max", M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min", M, unitT );
      }
      else
      {
         if ( !bPierTable )
            (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         else
            (*pTable)(0,col1++) << "";

         (*pTable)(0,col1++) << COLHDR("DC",          M, unitT );
         (*pTable)(0,col1++) << COLHDR("DW",          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << "DC",          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << "DW",          M, unitT );
         (*pTable)(0,col1++) << COLHDR("Service I", M, unitT );

         nRows = 1;
      }
   }
   else if ( stage == pgsTypes::BridgeSite2)
   {
      nCols = 6;
      if ( analysisType == pgsTypes::Envelope )
         nCols += 5;

      col1 = 0;
      col2 = 0;

      pTable = pgsReportStyleHolder::CreateDefaultTable(nCols, strLabel);

      if ( analysisType == pgsTypes::Envelope )
      {
         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col2++,-1);
   
         if ( !bPierTable )
            (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         else
            (*pTable)(0,col1++) << "";

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "DC";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "DW";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << "DC";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << "DW";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "Service I";
         (*pTable)(1,col2++) << COLHDR("Max", M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min", M, unitT );
         
         nRows = 2;
      }
      else
      {
         if ( !bPierTable )
            (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         else
            (*pTable)(0,col1++) << "";

         (*pTable)(0,col1++) << COLHDR("DC",          M, unitT );
         (*pTable)(0,col1++) << COLHDR("DW",          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << "DC",          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << "DW",          M, unitT );
         (*pTable)(0,col1++) << COLHDR("Service I", M, unitT );
         
         nRows = 1;
      }
   }
   else if ( stage == pgsTypes::BridgeSite3 )
   {
      nCols = 5;

      if ( analysisType == pgsTypes::Envelope )
         nCols += 4; // DC, DW, sum DC, sum DW min/max

      if ( bDesign )
      {
         nCols += 2; // Design LL+IM

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            nCols += 2; // fatigue

         if ( bPedLoading )
            nCols += 2;

         if ( bPermit )
            nCols += 2;
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
            nCols += 2;

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
            nCols += 2;
      }

      col1 = 0;
      col2 = 0;
      
      pTable = pgsReportStyleHolder::CreateDefaultTable(nCols, strLabel);

      pTable->SetRowSpan(0,col1,2);
      pTable->SetRowSpan(1,col2++,-1);
   
      if ( !bPierTable )
         (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      else
         (*pTable)(0,col1++) << "";

      if ( analysisType == pgsTypes::Envelope )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "DC";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "DW";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << "DC";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << "DW";
         (*pTable)(1,col2++) << COLHDR("Max",          M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",          M, unitT );
      }
      else
      {
         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col2++,-1);
         (*pTable)(0,col1++) << COLHDR("DC",          M, unitT );

         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col2++,-1);
         (*pTable)(0,col1++) << COLHDR("DW",          M, unitT );

         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col2++,-1);
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << "DC",          M, unitT );

         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col2++,-1);
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << "DW",          M, unitT );
      }

      if ( bDesign )
      {
         if ( bPedLoading )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* PL";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << "* LL+IM Design";
         (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
         (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Fatigue";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }

         if ( bPermit )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Permit";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }
      }

      if ( bRating )
      {
         if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Design";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Legal Routine";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Legal Special";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Permit Routine";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pTable->SetColumnSpan(0,col1,2);
            (*pTable)(0,col1++) << "* LL+IM Permit Special";
            (*pTable)(1,col2++) << COLHDR("Max",       M, unitT );
            (*pTable)(1,col2++) << COLHDR("Min",       M, unitT );
         }
      }

      nRows = 2;
   }
   else
   {
      ATLASSERT(0); // who added a new stage without telling me?
   }

   for ( int i = col1; i < nCols; i++ )
      pTable->SetColumnSpan(0,i,-1);

   pTable->SetNumberOfHeaderRows(nRows);

   *ppTable = pTable;
   return pTable->GetNumberOfHeaderRows();
}
// EXTERNAL REFERENCES
//

#endif // INCLUDED_COMBINEDMOMENTSTABLE_H_
