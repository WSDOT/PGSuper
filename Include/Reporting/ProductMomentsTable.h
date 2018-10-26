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

#ifndef INCLUDED_PRODUCTMOMENTSTABLE_H_
#define INCLUDED_PRODUCTMOMENTSTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>
#include "ReportNotes.h"

interface IEAFDisplayUnits;
interface IRatingSpecification;

std::string REPORTINGFUNC LiveLoadPrefix(pgsTypes::LiveLoadType llType);
void REPORTINGFUNC LiveLoadTableFooter(IBroker* pBroker,rptParagraph* pPara,GirderIndexType girder,bool bDesign,bool bRating);

ColumnIndexType REPORTINGFUNC GetProductLoadTableColumnCount(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::AnalysisType analysisType,bool bDesign,bool bRating,
                                                             bool* pbConstruction,bool* pbDeckPanels,bool* pbSidewalk,bool* pbShearKey,bool* pbPedLoading,bool* pbPermit,pgsTypes::Stage* pContinuityStage,SpanIndexType* pStartSpan,SpanIndexType* pNSpans);

/*****************************************************************************
CLASS 
   CProductMomentsTable

   Encapsulates the construction of the product forces table.


DESCRIPTION
   Encapsulates the construction of the product forces table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   virtual rptRcTable* Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::AnalysisType analysisType,
                             bool bDesign,bool bRating,bool bIndicateControllingLoad,IEAFDisplayUnits* pDisplayUnits) const;

protected:

   //------------------------------------------------------------------------
   void MakeCopy(const CProductMomentsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CProductMomentsTable& rOther);

};

template <class M,class T>
int ConfigureProductLoadTableHeading(rptRcTable* p_table,bool bPierTable,bool bConstruction,bool bDeckPanels,bool bSidewalk,bool bShearKey,
                                     bool bDesign,bool bPedLoading,bool bPermit,bool bRating,pgsTypes::AnalysisType analysisType,pgsTypes::Stage continuityStage,
                                     IRatingSpecification* pRatingSpec,IEAFDisplayUnits* pDisplayUnits,const T& unitT)
{
   p_table->SetNumberOfHeaderRows(2);

   //
   // Set up table headings
   //
   int row1col = 0;
   int row2col = 0;

   p_table->SetRowSpan(0,row1col,2);
   p_table->SetRowSpan(1,row2col++,-1);
   if ( bPierTable )
      (*p_table)(0,row1col++) << "";
   else
      (*p_table)(0,row1col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetRowSpan(0,row1col,2);
   p_table->SetRowSpan(1,row2col++,-1);
   (*p_table)(0,row1col++) << COLHDR("Girder",          M, unitT );

   p_table->SetRowSpan(0,row1col,2);
   p_table->SetRowSpan(1,row2col++,-1);
   (*p_table)(0,row1col++) << COLHDR("Diaphragm",       M, unitT );

   if ( bShearKey )
   {
      if ( analysisType == pgsTypes::Envelope && continuityStage == pgsTypes::BridgeSite1 )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "Shear Key";
         (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
      }
      else
      {
         p_table->SetRowSpan(0,row1col,2);
         p_table->SetRowSpan(1,row2col++,-1);
         (*p_table)(0,row1col++) << COLHDR("Shear" << rptNewLine << "Key", M, unitT );
      }
   }

   if ( bConstruction )
   {
      if ( analysisType == pgsTypes::Envelope && continuityStage == pgsTypes::BridgeSite1 )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "Construction";
         (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
      }
      else
      {
         p_table->SetRowSpan(0,row1col,2);
         p_table->SetRowSpan(1,row2col++,-1);
         (*p_table)(0,row1col++) << COLHDR("Construction", M, unitT );
      }
   }

   if ( analysisType == pgsTypes::Envelope && continuityStage == pgsTypes::BridgeSite1 )
   {
      p_table->SetColumnSpan(0,row1col,2);
      (*p_table)(0,row1col++) << "Slab";
      (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
      (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
   }
   else
   {
      p_table->SetRowSpan(0,row1col,2);
      p_table->SetRowSpan(1,row2col++,-1);
      (*p_table)(0,row1col++) << COLHDR("Slab", M, unitT );
   }

   if ( bDeckPanels )
   {
      if ( analysisType == pgsTypes::Envelope && continuityStage == pgsTypes::BridgeSite1 )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "Deck Panel";
         (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
      }
      else
      {
         p_table->SetRowSpan(0,row1col,2);
         p_table->SetRowSpan(1,row2col++,-1);
         (*p_table)(0,row1col++) << COLHDR("Deck" << rptNewLine << "Panel", M, unitT );
      }
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      if ( bSidewalk )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "Sidewalk";
         (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
      }

      p_table->SetColumnSpan(0,row1col,2);
      (*p_table)(0,row1col++) << "Traffic Barrier";
      (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
      (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );

      p_table->SetColumnSpan(0,row1col,2);
      (*p_table)(0,row1col++) << "Overlay";
      (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
      (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
   }
   else
   {
      if ( bSidewalk )
      {
         p_table->SetRowSpan(0,row1col,2);
         p_table->SetRowSpan(1,row2col++,-1);
         (*p_table)(0,row1col++) << COLHDR("Sidewalk", M, unitT );
      }

      p_table->SetRowSpan(0,row1col,2);
      p_table->SetRowSpan(1,row2col++,-1);
      (*p_table)(0,row1col++) << COLHDR("Traffic" << rptNewLine << "Barrier", M, unitT );

      p_table->SetRowSpan(0,row1col,2);
      p_table->SetRowSpan(1,row2col++,-1);
      (*p_table)(0,row1col++) << COLHDR("Overlay", M, unitT );
   }

   if ( bDesign )
   {
      if ( bPedLoading )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "$ Pedestrian";
         (*p_table)(1,row2col++) << COLHDR("Max", M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min", M, unitT );
      }

      p_table->SetColumnSpan(0,row1col,2);
      (*p_table)(0,row1col++) << "* Design Live Load";
      (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
      (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Fatigue Live Load";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }

      if ( bPermit )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Permit Live Load";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }
   }

   if ( bRating )
   {
      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Design Live Load";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Legal Routine";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Legal Special";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Permit Routine";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         p_table->SetColumnSpan(0,row1col,2);
         (*p_table)(0,row1col++) << "* Permit Special";
         (*p_table)(1,row2col++) << COLHDR("Max",   M, unitT );
         (*p_table)(1,row2col++) << COLHDR("Min",   M, unitT );
      }
   }

   for ( ColumnIndexType i = row1col; i < p_table->GetNumberOfColumns(); i++ )
   {
      p_table->SetColumnSpan(0,i,-1);
   }

   return p_table->GetNumberOfHeaderRows(); // index of first row to report data
}

#endif // INCLUDED_PRODUCTMOMENTSTABLE_H_
