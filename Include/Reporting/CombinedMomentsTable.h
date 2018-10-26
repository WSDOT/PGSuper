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
interface IDisplayUnits;

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
   // Builds the strand eccentricity table.
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      SpanIndexType span,GirderIndexType girder,
                      IDisplayUnits* pDisplayUnits,
                      pgsTypes::Stage stage,pgsTypes::AnalysisType analysisType) const;
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
int ConfigureLimitStateTableHeading(rptRcTable* pTable,bool bPierTable,bool bPermit,bool bMoment,pgsTypes::AnalysisType analysisType,IDisplayUnits* pDisplayUnits,const T& unitT)
{
   pTable->SetNumberOfHeaderRows(2);

   ColumnIndexType row0col = 0;
   ColumnIndexType row1col = 0;

   pTable->SetRowSpan(0,row0col,2);
   pTable->SetRowSpan(1,row1col,-1);
   row1col++;
   
   if ( !bPierTable )
      (*pTable)(0,row0col) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   else
      (*pTable)(0,row0col) << "";

   row0col++;

   if ( analysisType == pgsTypes::Envelope )
   {
      pTable->SetColumnSpan(0,row0col,2);
      (*pTable)(0,row0col) << "Service I";
      (*pTable)(1,row1col++) << COLHDR("Max", M, unitT );
      (*pTable)(1,row1col++) << COLHDR("Min", M, unitT );
      row0col++;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         pTable->SetColumnSpan(0,row0col,2);
         (*pTable)(0,row0col) << "Service IA";
         (*pTable)(1,row1col++) << COLHDR("Max", M, unitT );
         (*pTable)(1,row1col++) << COLHDR("Min", M, unitT );
         row0col++;
      }

      pTable->SetColumnSpan(0,row0col,2);
      (*pTable)(0,row0col) << "Service III";
      (*pTable)(1,row1col++) << COLHDR("Max", M, unitT );
      (*pTable)(1,row1col++) << COLHDR("Min", M, unitT );
      row0col++;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pTable->SetColumnSpan(0,row0col,2);
         (*pTable)(0,row0col) << "Fatigue I";
         (*pTable)(1,row1col++) << COLHDR("Max", M, unitT );
         (*pTable)(1,row1col++) << COLHDR("Min", M, unitT );
         row0col++;
      }

      pTable->SetColumnSpan(0,row0col,bMoment ? 3 : 2);
      (*pTable)(0,row0col) << "Strength I";
      (*pTable)(1,row1col++) << COLHDR("Max",  M, unitT );
      (*pTable)(1,row1col++) << COLHDR("Min",  M, unitT );
      if ( bMoment )
         (*pTable)(1,row1col++) << COLHDR("Slab", M, unitT );

      row0col++;

      if (bPermit)
      {
         pTable->SetColumnSpan(0,row0col,bMoment ? 3 : 2);
         (*pTable)(0,row0col) << "Strength II";
         (*pTable)(1,row1col++) << COLHDR("Max",  M, unitT );
         (*pTable)(1,row1col++) << COLHDR("Min",  M, unitT );
         if ( bMoment )
            (*pTable)(1,row1col++) << COLHDR("Slab", M, unitT );
      }
      else
      {
         pTable->SetColumnSpan(0,row0col, -1);
      }
      row0col++;

      pTable->SetColumnSpan(0,row0col++, -1);
      pTable->SetColumnSpan(0,row0col++, -1);
      pTable->SetColumnSpan(0,row0col++, -1);

      if ( bMoment )
         pTable->SetColumnSpan(0,row0col++, -1);

      if ( bPermit )
      {
         pTable->SetColumnSpan(0,row0col++, -1);
         pTable->SetColumnSpan(0,row0col++, -1);

         if ( bMoment )
            pTable->SetColumnSpan(0,row0col++, -1);
      }
   }
   else
   {
      pTable->SetRowSpan(0,row0col,2);
      (*pTable)(0,row0col++) << COLHDR("Service I", M, unitT );
      pTable->SetRowSpan(1,row1col++,-1);

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         pTable->SetRowSpan(0,row0col,2);
         (*pTable)(0,row0col++) << COLHDR("Service IA", M, unitT );
         pTable->SetRowSpan(1,row1col++,-1);
      }
      
      pTable->SetRowSpan(0,row0col,2);
      (*pTable)(0,row0col++) << COLHDR("Service III", M, unitT );
      pTable->SetRowSpan(1,row1col++,-1);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pTable->SetRowSpan(0,row0col,2);
         (*pTable)(0,row0col++) << COLHDR("Fatigue I", M, unitT );
         pTable->SetRowSpan(1,row1col++,-1);
      }

      pTable->SetColumnSpan(0,row0col,bMoment ? 3 : 2);
      (*pTable)(0,row0col++) << "Strength I";
      pTable->SetColumnSpan(0,row0col++,-1);
      pTable->SetColumnSpan(0,row0col++,-1);
      (*pTable)(1,row1col++) << COLHDR("Max", M, unitT );
      (*pTable)(1,row1col++) << COLHDR("Min", M, unitT );
      if ( bMoment )
      {
         (*pTable)(1,row1col++) << COLHDR("Slab", M, unitT );
         pTable->SetColumnSpan(0,row0col++,-1);
      }


      if (bPermit)
      {
         pTable->SetColumnSpan(0,row0col,bMoment ? 3 : 2);
         (*pTable)(0,row0col++) << "Strength II";
         pTable->SetColumnSpan(0,row0col++,-1);
         pTable->SetColumnSpan(0,row0col++,-1);
         (*pTable)(1,row1col++) << COLHDR("Max", M, unitT );
         (*pTable)(1,row1col++) << COLHDR("Min", M, unitT );
         if ( bMoment )
         {
            (*pTable)(1,row1col++) << COLHDR("Slab", M, unitT );
            pTable->SetColumnSpan(0,row0col++,-1);
         }
      }
   }

   return 2;
}

///////////////////////////////////////////////////////////////////

template <class M,class T>
RowIndexType CreateCombinedLoadingTableHeading(rptRcTable** ppTable,const char* strLabel,bool bPierTable,bool bPermit,bool bPedLoading,pgsTypes::Stage stage,pgsTypes::Stage continuityStage,pgsTypes::AnalysisType analysisType,IDisplayUnits* pDisplayUnits,const T& unitT)
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
      nCols = 7;

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         nCols += 2;

      if ( bPermit )
         nCols += 2;

      if ( analysisType == pgsTypes::Envelope )
         nCols += 4;

      if ( bPedLoading )
         nCols += 2;

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
