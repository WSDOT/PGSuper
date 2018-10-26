///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <IFace\Project.h>

interface IBroker;
interface IProductLoads;
interface IEAFDisplayUnits;
interface IRatingSpecification;
interface ILiveLoads;
interface IIntervals;
interface IPointOfInterest;

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
   // bDesign and bRating are only considered for intervalIdx = live load interval index
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   void BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;

   void BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;

   void BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,IntervalIndexType intervalIdx,
                      pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CCombinedMomentsTable& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const CCombinedMomentsTable& rOther);

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

void GetCombinedResultsPoi(IBroker* pBroker,const CGirderKey& girderKey,IntervalIndexType intervalIdx,std::vector<pgsPointOfInterest>* pPoi,PoiAttributeType* pRefAttribute);


// INLINE METHODS
//
template <class M,class T>
RowIndexType CreateLimitStateTableHeading(rptRcTable** ppTable,LPCTSTR strLabel,bool bPierTable,bool bDesign,bool bPermit,bool bRating,bool bMoment,pgsTypes::AnalysisType analysisType,IProductLoads* pProductLoads,IRatingSpecification* pRatingSpec,IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   // number of columns
   ColumnIndexType nDesignCols = 0;
   if ( bDesign )
   {
      nDesignCols += (bMoment ? 6 : 5); // Service I, Service IA or Fatigue I, Strength I min/max

      if ( analysisType == pgsTypes::Envelope )
      {
         nDesignCols += 3; // min/max for Service I, Service III, ServiceIA/FatigueI
      }

      if ( bPermit )
      {
         nDesignCols += (bMoment ? 3 : 2); // Strength II min/max
      }
   }

   ColumnIndexType nRatingCols = 0;
   if ( bRating )
   {
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
      {
         nRatingCols += (bMoment ? 3 : 2);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
      {
         nRatingCols += (bMoment ? 3 : 2);
      }
   
      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         nRatingCols += (bMoment ? 3 : 2);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         nRatingCols += (bMoment ? 3 : 2);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         nRatingCols += (bMoment ? 5 : 4);
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         nRatingCols += (bMoment ? 5 : 4);
      }
   }

   ColumnIndexType nCols = nDesignCols + nRatingCols + 1;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,strLabel);

   pTable->SetNumberOfHeaderRows(3);

   pTable->SetRowSpan(0,0, pTable->GetNumberOfHeaderRows() );
   pTable->SetRowSpan(1,0,SKIP_CELL);
   pTable->SetRowSpan(2,0,SKIP_CELL);

   RowIndexType ll_title_row = 0;
   RowIndexType ls_title_row = 1;
   RowIndexType min_max_row  = 2;

   ColumnIndexType ll_title_col = 1;
   ColumnIndexType ls_title_col = 1;
   ColumnIndexType min_max_col  = 1;

   if ( bPierTable )
   {
      (*pTable)(0,0) << _T("");
   }
   else
   {
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }


   if ( analysisType == pgsTypes::Envelope )
   {
      if ( bDesign )
      {
         pTable->SetColumnSpan(ll_title_row,ll_title_col,nCols-1);
         (*pTable)(ll_title_row,ll_title_col++) << _T("Design");

         for ( ColumnIndexType i = 0; i < nDesignCols-1; i++ )
         {
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,SKIP_CELL);
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
         (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceI);
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
           (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceIA);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
         (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceIII);
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
           (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::FatigueI);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
         (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI);
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );
         if ( bMoment )
         {
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
         }

         if (bPermit)
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthII);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++)<< COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++)<< COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }
      }

      if ( bRating && 0 < nRatingCols)
      {
         pTable->SetColumnSpan(ll_title_row,ll_title_col,nRatingCols);
         (*pTable)(ll_title_row,ll_title_col++) << _T("Rating");

         for ( ColumnIndexType i = 0; i < nRatingCols-1; i++ )
         {
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,SKIP_CELL);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_Inventory);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_Operating);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_LegalRoutine);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_LegalSpecial);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceI_PermitRoutine);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthII_PermitRoutine);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceI_PermitSpecial);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthII_PermitSpecial);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
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
         {
            nCol += (bMoment ? 3 : 2);
         }

         pTable->SetColumnSpan(ll_title_row,ll_title_col,nCol);
         (*pTable)(ll_title_row,ll_title_col++) << _T("Design");

         for ( ColumnIndexType i = 0; i < nDesignCols-1; i++ )
         {
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,SKIP_CELL);
         }

         pTable->SetRowSpan(ls_title_row,ls_title_col,2);
         pTable->SetRowSpan(min_max_row,min_max_col++,SKIP_CELL);
         (*pTable)(ls_title_row,ls_title_col++) << COLHDR(pProductLoads->GetLimitStateName(pgsTypes::ServiceI), M, unitT );

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pTable->SetRowSpan(ls_title_row,ls_title_col,2);
            pTable->SetRowSpan(min_max_row,min_max_col++,SKIP_CELL);
            (*pTable)(ls_title_row,ls_title_col++) << COLHDR(pProductLoads->GetLimitStateName(pgsTypes::ServiceIA), M, unitT );
         }
         
         pTable->SetRowSpan(ls_title_row,ls_title_col,2);
         pTable->SetRowSpan(min_max_row,min_max_col++,SKIP_CELL);
         (*pTable)(ls_title_row,ls_title_col++) << COLHDR(pProductLoads->GetLimitStateName(pgsTypes::ServiceIII), M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetRowSpan(ls_title_row,ls_title_col,2);
            pTable->SetRowSpan(min_max_row,min_max_col++,SKIP_CELL);
            (*pTable)(ls_title_row,ls_title_col++) << COLHDR(pProductLoads->GetLimitStateName(pgsTypes::FatigueI), M, unitT );
         }

         pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3: 2);
         (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI);
         pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

         if ( bMoment )
         {
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
         }

         if (bPermit)
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthII);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }
      }

      if ( bRating )
      {
         pTable->SetColumnSpan(ll_title_row,ll_title_col,nRatingCols);
         (*pTable)(ll_title_row,ll_title_col++) << _T("Rating");

         for ( ColumnIndexType i = 0; i < nRatingCols-1; i++ )
         {
            pTable->SetColumnSpan(ll_title_row,ll_title_col++,SKIP_CELL);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_Inventory);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_Operating);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_LegalRoutine);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthI_LegalSpecial);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceI_PermitRoutine);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthII_PermitRoutine);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
         {
            pTable->SetColumnSpan(ls_title_row,ls_title_col,2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::ServiceI_PermitSpecial);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            pTable->SetColumnSpan(ls_title_row,ls_title_col,bMoment ? 3 : 2);
            (*pTable)(ls_title_row,ls_title_col++) << pProductLoads->GetLimitStateName(pgsTypes::StrengthII_PermitSpecial);
            pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);

            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Max"), M, unitT );
            (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("Min"), M, unitT );

            if ( bMoment )
            {
               (*pTable)(min_max_row,min_max_col++) << COLHDR(_T("* Deck"), M, unitT );
               pTable->SetColumnSpan(ls_title_row,ls_title_col++,SKIP_CELL);
            }
         }
      }
   }

   *ppTable = pTable;

   return pTable->GetNumberOfHeaderRows();
}

template <class M,class T>
RowIndexType CreateCombinedDeadLoadingTableHeading(rptRcTable** ppTable,IBroker* pBroker,const CGirderKey& girderKey,LPCTSTR strLabel,bool bPierTable, bool bRating,IntervalIndexType intervalIdx,
                                               pgsTypes::AnalysisType analysisType,
                                               IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bTimeStepMethod = pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP;

   RowIndexType nRows = 0;

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;
   ColumnIndexType nCols = 0;

   rptRcTable* pTable;
   nCols = (bTimeStepMethod ? 14 : 6);

   if ( bRating )
   {
      nCols += (analysisType == pgsTypes::Envelope ? 4 : 2);
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      ATLASSERT(!bTimeStepMethod); // can't use envelope with time-step
      //nCols += (bTimeStepMethod ? 13 : 5);
      nCols += 5;
   }

   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      if ( analysisType == pgsTypes::Envelope )
      {
         nCols -= 2;
      }
      else
      {
         nCols--;
      }
   }

   pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,strLabel);

   if ( analysisType == pgsTypes::Envelope )
   {
      nRows = 2;

      pTable->SetRowSpan(0,col1,2);
      pTable->SetRowSpan(1,col2++,SKIP_CELL);
      if ( bPierTable )
      {
         (*pTable)(0,col1++) << _T("");
      }
      else
      {
         (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      pTable->SetColumnSpan(0,col1,2);
      (*pTable)(0,col1++) << _T("DC");
      (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
      (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

      pTable->SetColumnSpan(0,col1,2);
      (*pTable)(0,col1++) << _T("DW");
      (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
      (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

      if (bRating)
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << _T("DW") << rptNewLine << _T("Rating");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );
      }

      pTable->SetColumnSpan(0,col1,2);
      (*pTable)(0,col1++) << symbol(SUM) << _T("DC");
      (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
      (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

      pTable->SetColumnSpan(0,col1,2);
      (*pTable)(0,col1++) << symbol(SUM) << _T("DW");
      (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
      (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

      ATLASSERT(!bTimeStepMethod); // can't use envelope mode with time-step
/*
      if ( bTimeStepMethod )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << _T("CR");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << _T("SH");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << _T("RE");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );

         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << _T("PS");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );
      }
*/
      if (bRating)
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << symbol(SUM) << _T("DW") << rptNewLine << _T("Rating");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );
      }

      if ( intervalIdx < liveLoadIntervalIdx )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << _T("Service I");
         (*pTable)(1,col2++) << COLHDR(_T("Max"), M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"), M, unitT );
      }
   }
   else
   {
      if ( bPierTable )
      {
         (*pTable)(0,col1++) << _T("");
      }
      else
      {
         (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      (*pTable)(0,col1++) << COLHDR(_T("DC"),          M, unitT );
      (*pTable)(0,col1++) << COLHDR(_T("DW"),          M, unitT );

      if(bRating)
      {
         (*pTable)(0,col1++) << COLHDR(_T("DW") << rptNewLine << _T("Rating"),          M, unitT );
      }


      if ( bTimeStepMethod )
      {
         (*pTable)(0,col1++) << COLHDR(_T("CR"),          M, unitT );
         (*pTable)(0,col1++) << COLHDR(_T("SH"),          M, unitT );
         (*pTable)(0,col1++) << COLHDR(_T("RE"),          M, unitT );
         (*pTable)(0,col1++) << COLHDR(_T("PS"),          M, unitT );
      }

      (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("DC"),          M, unitT );
      (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("DW"),          M, unitT );

      if (bRating)
      {
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("DW") << rptNewLine << _T("Rating"),          M, unitT );
      }

      if ( bTimeStepMethod )
      {
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("CR"),          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("SH"),          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("RE"),          M, unitT );
         (*pTable)(0,col1++) << COLHDR(symbol(SUM) << _T("PS"),          M, unitT );
      }

      if ( intervalIdx < liveLoadIntervalIdx )
      {
         (*pTable)(0,col1++) << COLHDR(_T("Service I"), M, unitT );
      }

      nRows = 1;
   }

   for ( ColumnIndexType i = col1; i < nCols; i++ )
   {
      pTable->SetColumnSpan(0,i,SKIP_CELL);
   }

   pTable->SetNumberOfHeaderRows(nRows);

   *ppTable = pTable;
   return pTable->GetNumberOfHeaderRows();
}

template <class M,class T>
RowIndexType CreateCombinedLiveLoadingTableHeading(rptRcTable** ppTable,LPCTSTR strLabel,bool bPierTable,bool bDesign,bool bPermit,
                                                   bool bPedLoading,bool bRating,bool is4Stress, bool includeImpact,
                                                   pgsTypes::AnalysisType analysisType,IRatingSpecification* pRatingSpec,
                                                   IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   //ATLASSERT ( stage == pgsTypes::BridgeSite3 );
   ATLASSERT( !(bDesign && bRating) ); // These are different tables - must create separately

   rptRcTable* pTable;

   int nCols = 1; // location
   int nVhls = 0; // number of vehicles for design

   if ( bDesign )
   {
      nCols += 2; // Design LL+IM
      nVhls++;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         nCols += 2; // fatigue
         nVhls++;
      }

      if ( bPermit && !is4Stress )
      {
         nCols += 2;
         nVhls++;
      }

      if ( bPedLoading )
      {
         nCols += 2;

         // we have a Float64-width table (except for location and ped)
         nCols += nCols-3;
      }
   }

   if ( bRating )
   {
      if (bPedLoading && pRatingSpec->IncludePedestrianLiveLoad())
      {
         nCols += 2;
      }

      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine)  && !is4Stress )
      {
         nCols += 2;
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && !is4Stress )
      {
         nCols += 2;
      }
   }

   int col1 = 0;
   int col2 = 0;
   
   pTable = pgsReportStyleHolder::CreateDefaultTable(nCols, strLabel);

   pTable->SetRowSpan(0,0,3);
   pTable->SetRowSpan(1,0,SKIP_CELL);

   if ( bPierTable )
   {
      (*pTable)(0,col1++) << _T("");
   }
   else
   {
      (*pTable)(0,col1++) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   col2++;

   int nRows;
   if ( bDesign )
   {
      nRows = 3;
      int col3 = col2;

      pTable->SetRowSpan(2,0,SKIP_CELL);

      pTable->SetNumberOfHeaderRows(nRows);

      if ( bPedLoading )
      {
         pTable->SetColumnSpan(0,col2++,2);
         pTable->SetRowSpan(0,col1,2);
         pTable->SetRowSpan(1,col1,SKIP_CELL);
         (*pTable)(0,col1++) << _T("* PL");
         (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );

         pTable->SetColumnSpan(0,col1,nVhls*2);
         (*pTable)(0,col1++) << (includeImpact ? _T("Vehicles (LL+IM)") : _T("Vehicles (LL)"));
         pTable->SetColumnSpan(1,col2,2);
         (*pTable)(1,col2++) << _T("* Design");
         (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetColumnSpan(1,col2,2);
            (*pTable)(1,col2++) << _T("* Fatigue");
            (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
            (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );
         }

         if ( bPermit  && !is4Stress)
         {
            pTable->SetColumnSpan(1,col2,2);
            (*pTable)(1,col2++) << _T("* Permit");
            (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
            (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );
         }

         // Total Live Load
         int lcnt=1;
         pTable->SetColumnSpan(0,col1,nVhls*2);
         (*pTable)(0,col1++) << (includeImpact ? _T("Total Live Load (PL and LL+IM)") : _T("Total Live Load (PL and LL)"));
         pTable->SetColumnSpan(1,col2,2);
         (*pTable)(1,col2++) << _T("* Design")<<Super(lcnt++);
         (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetColumnSpan(1,col2,2);
            (*pTable)(1,col2++) << _T("* Fatigue")<<Super(lcnt++);
            (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
            (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );
         }

         if ( bPermit  && !is4Stress)
         {
            pTable->SetColumnSpan(1,col2,2);
            (*pTable)(1,col2++) << _T("* Permit")<<Super(lcnt++);
            (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
            (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );
         }
      }
      else
      {
         pTable->SetColumnSpan(0,col1,nVhls*2);
         (*pTable)(0,col1++) << _T("Live Load");
         pTable->SetColumnSpan(1,col2,2);
         (*pTable)(1,col2++) << (includeImpact ? _T("* LL+IM Design") : _T("* LL Design"));
         (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            pTable->SetColumnSpan(1,col2,2);
            (*pTable)(1,col2++) << (includeImpact ? _T("* LL+IM Fatigue") : _T("* LL Fatigue"));
            (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
            (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );
         }

         if ( bPermit  && !is4Stress)
         {
            pTable->SetColumnSpan(1,col2,2);
            (*pTable)(1,col2++) << (includeImpact ? _T("* LL+IM Permit") : _T("* LL Permit"));
            (*pTable)(2,col3++) << COLHDR(_T("Max"),       M, unitT );
            (*pTable)(2,col3++) << COLHDR(_T("Min"),       M, unitT );
         }
      }

      // skip unused columns
      for (int ic=col1; ic<nCols; ic++)
      {
         pTable->SetColumnSpan(0,ic,SKIP_CELL);
      }

      for (int ic=col2; ic<nCols; ic++)
      {
         pTable->SetColumnSpan(1,ic,SKIP_CELL);
      }
   }

   if ( bRating )
   {
      nRows = 2;
      pTable->SetNumberOfHeaderRows(nRows);

      if (bPedLoading && pRatingSpec->IncludePedestrianLiveLoad())
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << _T("*$ PED");
         (*pTable)(1,col2++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"),       M, unitT );
      }

      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << (includeImpact ? _T("* LL+IM Design") : _T("* LL Design"));
         (*pTable)(1,col2++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"),       M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << (includeImpact ? _T("* LL+IM Legal Routine") : _T("* LL Legal Routine"));
         (*pTable)(1,col2++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"),       M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << (includeImpact ? _T("* LL+IM Legal Special") :  _T("* LL Legal Special"));
         (*pTable)(1,col2++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"),       M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) && !is4Stress )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << (includeImpact ? _T("* LL+IM Permit Routine") : _T("* LL Permit Routine"));
         (*pTable)(1,col2++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"),       M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && !is4Stress )
      {
         pTable->SetColumnSpan(0,col1,2);
         (*pTable)(0,col1++) << (includeImpact ? _T("* LL+IM Permit Special") : _T("* LL Permit Special"));
         (*pTable)(1,col2++) << COLHDR(_T("Max"),       M, unitT );
         (*pTable)(1,col2++) << COLHDR(_T("Min"),       M, unitT );
      }

      // skip unused columns
      for (int ic=col1; ic<nCols; ic++)
      {
         pTable->SetColumnSpan(0,ic,SKIP_CELL);
      }
   }

   *ppTable = pTable;
   return nRows;
}

inline std::_tstring PedestrianFootnote(ILiveLoads::PedestrianLoadApplicationType appType)
{
   if (appType==ILiveLoads::PedDontApply)
   {
      return _T(" - Pedestrian live load NOT applied with vehicular loads");
   }
   else if (appType==ILiveLoads::PedConcurrentWithVehicular)
   {
      return _T(" - Pedestrian live load applied concurrently with vehicular loads");
   }
   else if (appType==ILiveLoads::PedEnvelopeWithVehicular)
   {
      return _T(" - Pedestrian live load enveloped with vehicular loads");
   }
   else
   {
      ATLASSERT(false);
      return _T(" - Unknown pedestrian live load application");
   }
 }

inline void SumPedAndLiveLoad(ILiveLoads::PedestrianLoadApplicationType appType, std::vector<Float64>& minLL, std::vector<Float64>& maxLL,
                              const std::vector<Float64>& minPed, const std::vector<Float64>& maxPed)
{
   ATLASSERT(minLL.size()==minPed.size());
   ATLASSERT(maxLL.size()==maxPed.size());

   if (appType==ILiveLoads::PedDontApply)
   {
      return; // nothing to do here
   }
   else if (appType==ILiveLoads::PedConcurrentWithVehicular)
   {
      // summ values
      std::vector<Float64>::iterator minIt = minLL.begin();
      std::vector<Float64>::iterator minEnd = minLL.end();
      std::vector<Float64>::const_iterator minPedIt = minPed.begin();
      while(minIt != minEnd)
      {
         *minIt += *minPedIt;
         minIt++;
         minPedIt++;
      }

      std::vector<Float64>::iterator maxIt = maxLL.begin();
      std::vector<Float64>::iterator maxEnd = maxLL.end();
      std::vector<Float64>::const_iterator maxPedIt = maxPed.begin();
      while(maxIt != maxEnd)
      {
         *maxIt += *maxPedIt;
         maxIt++;
         maxPedIt++;
      }
   }
   else if (appType==ILiveLoads::PedEnvelopeWithVehicular)
   {
      // envelope values
      std::vector<Float64>::iterator minIt = minLL.begin();
      std::vector<Float64>::iterator minEnd = minLL.end();
      std::vector<Float64>::const_iterator minPedIt = minPed.begin();
      while(minIt != minEnd)
      {
         *minIt = Min(*minIt, *minPedIt);
         minIt++;
         minPedIt++;
      }

      std::vector<Float64>::iterator maxIt = maxLL.begin();
      std::vector<Float64>::iterator maxEnd = maxLL.end();
      std::vector<Float64>::const_iterator maxPedIt = maxPed.begin();
      while(maxIt != maxEnd)
      {
         *maxIt = Max(*maxIt, *maxPedIt);
         maxIt++;
         maxPedIt++;
      }
   }
}

inline void SumPedAndLiveLoad(ILiveLoads::PedestrianLoadApplicationType appType, std::vector<sysSectionValue>& minLL, std::vector<sysSectionValue>& maxLL,
                              const std::vector<sysSectionValue>& minPed, const std::vector<sysSectionValue>& maxPed)
{
   ATLASSERT(minLL.size()==minPed.size());
   ATLASSERT(maxLL.size()==maxPed.size());

   if (appType==ILiveLoads::PedDontApply)
   {
      return; // nothing to do here
   }
   else if (appType==ILiveLoads::PedConcurrentWithVehicular)
   {
      // summ values
      std::vector<sysSectionValue>::iterator minIt = minLL.begin();
      std::vector<sysSectionValue>::iterator minEnd = minLL.end();
      std::vector<sysSectionValue>::const_iterator minPedIt = minPed.begin();
      while(minIt != minEnd)
      {
         *minIt += *minPedIt;
         minIt++;
         minPedIt++;
      }

      std::vector<sysSectionValue>::iterator maxIt = maxLL.begin();
      std::vector<sysSectionValue>::iterator maxEnd = maxLL.end();
      std::vector<sysSectionValue>::const_iterator maxPedIt = maxPed.begin();
      while(maxIt != maxEnd)
      {
         *maxIt += *maxPedIt;
         maxIt++;
         maxPedIt++;
      }
   }
   else if (appType==ILiveLoads::PedEnvelopeWithVehicular)
   {
      // envelope values
      std::vector<sysSectionValue>::iterator minIt = minLL.begin();
      std::vector<sysSectionValue>::iterator minEnd = minLL.end();
      std::vector<sysSectionValue>::const_iterator minPedIt = minPed.begin();
      while(minIt != minEnd)
      {
         minIt->Left()  = Min(minIt->Left(), minPedIt->Left());
         minIt->Right() = Min(minIt->Right(), minPedIt->Right());
         minIt++;
         minPedIt++;
      }

      std::vector<sysSectionValue>::iterator maxIt = maxLL.begin();
      std::vector<sysSectionValue>::iterator maxEnd = maxLL.end();
      std::vector<sysSectionValue>::const_iterator maxPedIt = maxPed.begin();
      while(maxIt != maxEnd)
      {
         maxIt->Left()  = Max(maxIt->Left(), maxPedIt->Left());
         maxIt->Right() = Max(maxIt->Right(), maxPedIt->Right());
         maxIt++;
         maxPedIt++;
      }
   }
}


// EXTERNAL REFERENCES
//

#endif // INCLUDED_COMBINEDMOMENTSTABLE_H_
