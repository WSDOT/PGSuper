///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#ifndef INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNBROKERRETRIEVER_H_
#define INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNBROKERRETRIEVER_H_

// SYSTEM INCLUDES
//

#include <WBFLCore.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   ITxDOTBrokerRetriever

  Interface to get an updated broker for TOGA 

DESCRIPTION
   Pure virtual utility Interface to get an updated broker for TOGA. Hides the document implementation from 
   windows that aren't views

LOG
   rdp : 02.19.2010 : Created file
*****************************************************************************/

// simple exception class that will be thrown if the retrieval fails
class TxDOTBrokerRetrieverException
{
public:
   TxDOTBrokerRetrieverException()
   {
      int a = 2; // somewhere to break if an exception is getting made
   }

   CString Message; // message thrown if things go wrong
};

class ITxDOTBrokerRetriever
{
public:
   // Update the broker and return it to caller.
   // NOTE: Do not hold on to this pointer while document data can be changed.
   virtual IBroker* GetUpdatedBroker() = 0;

   // Get broker without updating input data
   virtual IBroker* GetClassicBroker() = 0;

   // Get girder library without updating broker
   virtual GirderLibrary* GetGirderLibrary() = 0;

   // Get girder connection library without updating broker
   virtual ConnectionLibrary* GetConnectionLibrary() = 0;

   // Master library
   virtual SpecLibrary* GetSpecLibrary() = 0;
};


#endif // INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNBROKERRETRIEVER_H_

