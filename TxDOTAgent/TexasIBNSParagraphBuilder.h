///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#ifndef INCLUDED_TEXASIBNSPARAGRAPHBUILDER_H_
#define INCLUDED_TEXASIBNSPARAGRAPHBUILDER_H_

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CTexasIBNSParagraphBuilder

   Paragraph builder for Texas IBNS sheet


DESCRIPTION
   Paragraph builder for Texas IBNS sheet. This is a customized output paragraph
   for TxDOT.

COPYRIGHT
   Copyright © 1997-2002
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 06.13.2006 : Created file
*****************************************************************************/

class CTexasIBNSParagraphBuilder
{
public:
   // GROUP: LIFECYCLE
   CTexasIBNSParagraphBuilder();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS


   //------------------------------------------------------------------------
   rptParagraph* Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                       IEAFDisplayUnits* pDisplayUnits,
                       Uint16 level) const;

   //------------------------------------------------------------------------

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CTexasIBNSParagraphBuilder(const CTexasIBNSParagraphBuilder&);
   CTexasIBNSParagraphBuilder& operator=(const CTexasIBNSParagraphBuilder&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void WriteDebondTable(rptParagraph* pPara, IBroker* pBroker, SpanIndexType span,GirderIndexType girder, IEAFDisplayUnits* pDisplayUnits) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_TEXASIBNSParagraphBUILDER_H_
