// 1250_Diff.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <set>

#include <System\tokenizer.h>

typedef std::set< std::_tstring > StringSet;

 
// this class is the brains of the operation
class StateManager
{
public:
   enum State { ReadTD, Dump, Quit, FilesDifferentLength, Error };

   StringSet m_IgnoreList;

   State GetInitialState()
   {
      return ReadTD;
   }

   // process line information and determine next state
   State Process( int lineNum, bool fileStatD, bool fileStatT, std::_tstring& lineD, std::_tstring& lineT);

   // dump out internally stored string (the diff information)
   std::_tstring DumpString();

   // return number of lines that were different
   int NumDiffs()
   {
      return m_NumDiffs;
   }

   // constructor
   StateManager(double percentDiff, double zeroTol, LPCTSTR datum, LPCTSTR test):
   m_PercentDiff(percentDiff),
   m_ZeroTol(zeroTol),
   m_DatumFileName(datum),
   m_TestFileName(test),
   m_MaxDiffLines(4),
   m_LastState(ReadTD),
   m_InternalState(Happy),
   m_NumDiffs(0)
   {
   }


private: 
   enum InternalState {Happy, JustDiffed};

   State          m_LastState;
   InternalState  m_InternalState;

   double         m_PercentDiff;
   double         m_ZeroTol;
   std::_tstring    m_DatumFileName;
   std::_tstring    m_TestFileName;
   int            m_MaxDiffLines;// max number of lines to dump at a time when a diff

   int            m_CurrDiffLine; // number of lines stored in buffer
   int            m_NumDiffs;

   std::_tostringstream m_osd;
   std::_tostringstream m_ost;

   StateManager();
   bool CompareLines( std::_tstring& lineD, std::_tstring& lineT);

};

int _tmain(int argc, TCHAR* argv[])
{
   std::_tcout<< _T("NCRHP 12-50 File Diff Engine...")<<std::endl;
   if (argc<3)
   {
      std::_tcout<< _T("Syntax is..\n")<<std::endl;
      std::_tcout<< _T("1250_Diff datumFile testFile <percentDiff> <ZeroTolerance>")<<std::endl;
      std::_tcout<< _T("Report ID's to Ignore are read from 'ignore.txt'")<<std::endl;
      return 1;
   }

   // Process our input
   double pd_max   = 0.02;    // default max percent difference is 2%
   double zero_tol = 1.0e-05; //  default zero tolerance
   if (argc>3)
   {
      if (sysTokenizer::ParseDouble(argv[3], &pd_max))
      {
         pd_max /= 100.0;
      }
      else
      {
         std::_tcout<< _T("Error Parsing Percent Difference")<<std::endl;
         return 1;
      }

      if (pd_max<0.0 || pd_max>=1.0)
      {
         std::_tcout<< _T("Error - percent difference out of range")<<std::endl;
      }
   }

   if (argc>4)
   {
      if (!sysTokenizer::ParseDouble(argv[4], &zero_tol))
      {
         std::_tcout<< _T("Error Parsing Zero Tolerance")<<std::endl;
         return 1;
      }

      if (zero_tol<0.0)
      {
         std::_tcout<< _T("Error - zero tolerance difference out of range. Must be positive")<<std::endl;
      }
   }

   std::_tifstream datum_file;
   datum_file.open(argv[1]);
   if (!datum_file.is_open())
   {
      std::_tcout<< _T("Error - opening datum file")<<std::endl;
      return 1;
   }

   std::_tifstream test_file;
   test_file.open(argv[2]);
   if (!test_file.is_open())
   {
      std::_tcout<< _T("Error - opening test file")<<std::endl;
      return 1;
   }

   // load up report id's to igmore
   StringSet ignore_list;
   std::_tifstream ignore_file;
   ignore_file.open(_T("ignore.txt"));
   if (ignore_file.is_open())
   {
      std::_tstring line;
      while( std::getline(ignore_file, line))
      {
         if (!line.empty())
            ignore_list.insert(line);
      }
   }

   std::_tcout<< _T("Datum File is: ")<<argv[1]<<std::endl;
   std::_tcout<< _T("Test File is: ")<<argv[2]<<std::endl;
   std::_tcout<< _T("Max percent difference is ")<<pd_max*100<<_T(" Zero tolerance is ")<<zero_tol<<std::endl;
   if (!ignore_list.empty())
   {
      std::_tcout<< _T("The following ReportID's will be ignored:")<<std::endl;
      for (StringSet::iterator it=ignore_list.begin(); it!=ignore_list.end(); it++)
      {
         std::_tcout<<*it<<std::endl;
      }
   }
   std::_tcout<< _T("*******************************************************************")<<std::endl;

   // set up our state manager
   StateManager manager(pd_max, zero_tol, argv[1], argv[2]);
   manager.m_IgnoreList = ignore_list;

   int st = 0;
   std::_tstring lined, linet;
   bool std, stt;
   int line_num=0;
   StateManager::State state = manager.GetInitialState();
   while (true)
   {

      // perform process based on our state
      switch (state)
      {
         case StateManager::ReadTD:
            std = std::getline(datum_file, lined);
            stt = std::getline(test_file, linet);
            line_num++;
            break;

         case StateManager::FilesDifferentLength:
            std::_tcout<< _T("Files Are Different Length - Cannot Continue - Quit in Error") <<std::endl;
            state=StateManager::Error;
            break;

         case StateManager::Dump:
            std::_tcout<< manager.DumpString() <<std::endl;
            break;

         case StateManager::Quit:
            break;

         default:
            std::_tcout<< _T("Invalid State - Quit in Error") <<std::endl;
            state=StateManager::Error;
            break;
      }

      if (state==StateManager::Quit || state==StateManager::Error)
      {
         if (state==StateManager::Error)
            st = 1;

         // terminate loop
         break;
      }

      // transition to the next state
      state = manager.Process( line_num, std, stt,  lined, linet);     
   }

   int nd = manager.NumDiffs();
   if (nd>0)
   {
      std::_tcout<<_T(" Files are different - Number of differences was ")<<nd<<std::endl;
   }
   else
   {
      std::_tcout<<_T(" Files are identical within tolerances")<<std::endl;
   }

	return st;
}


StateManager::State StateManager::Process( int lineNum, bool fileStatD, bool fileStatT, std::_tstring& lineD, std::_tstring& lineT)
{
   State state = Error; // assume the worst

   if (m_LastState==ReadTD)
   {
      // read both lines last time - diff if not eol
      if (fileStatD && fileStatT)
      {
         if (CompareLines(lineD, lineT))
         {
            if (m_InternalState==Happy)
            {
               state = ReadTD;
            }
            else // JustDiffed
            {
               // need to dump out our differences
               state = Dump;
            }
         }
         else
         {

            if (m_InternalState==Happy)
            {
               // our first difference - write to our internal strings
               m_osd <<_T("*** ")<<m_DatumFileName<<_T(" ***")<<std::endl;
               m_ost <<_T("*** ")<<m_TestFileName<<_T(" ***")<<std::endl;
               m_CurrDiffLine = 1;

               state = ReadTD;
               m_InternalState = JustDiffed;
            }
            else
            {
               if (++m_CurrDiffLine >= m_MaxDiffLines)
                  state = Dump;
               else
                  state = ReadTD;
            }

            m_osd <<std::setw(4)<<lineNum<<_T(": ")<<lineD<<std::endl;
            m_ost <<std::setw(4)<<lineNum<<_T(": ")<<lineT<<std::endl;
            m_NumDiffs++;
         }
      }
      else if (fileStatD || fileStatT)
      {
         // one file is longer than other
         state = FilesDifferentLength;
      }
      else
      {
         // hit end of both files
         if (m_InternalState==Happy)
         {
            state = Quit;
         }
         else
         {
            state = Dump;
         }
      }
   }
   else if (m_LastState==Dump)
   {
      // last time through we dumped
      m_osd.str(std::_tstring());
      m_ost.str(std::_tstring());
      m_CurrDiffLine = 0;
      m_InternalState = Happy;

      if (fileStatD && fileStatT)
      {
         state = ReadTD;
      }
      else
      {
         state = Quit;
      }
   }
   else
   {
      // outer loop should have quit before getting here
      // this error is probably in the calling code
      std::_tcout<< _T("A State Transition Error Occured in StateManager::Process")<<std::endl;
      state = Error;
   }

   m_LastState = state;
   return state;
}

std::_tstring StateManager::DumpString()
{
   m_osd << m_ost.str();
   return m_osd.str();
}

bool StateManager::CompareLines( std::_tstring& lineD, std::_tstring& lineT)
{

   // break each string into tokens
   LPCTSTR delims[] = {_T(","),_T(" "), 0};
   sysTokenizer tokizerd(delims);
   tokizerd.push_back(lineD);
   sysTokenizer tokizert(delims);
   tokizert.push_back(lineT);

   sysTokenizer::size_type npd = tokizerd.size();
   sysTokenizer::size_type npt = tokizert.size();

   if (npd != npt)
   {
      // different number of tokens - no need to go further
      return false;
   }

   // check if this is even a 12-50 file
   if (npd==7 || npd==6)
   {
      // Not all input files in the test suite have BridgeID's. If there is no bridge id, the parser will pick up
      // six tokens
      int rpt_id_loc = (npd==7) ? 2 : 1;
      int value_loc  = (npd==7) ? 4 : 3;

      // 12 - 50 file. compare tokens
      for (sysTokenizer::size_type i=0; i<npd; i++)
      {
         std::_tstring sd = tokizerd[i];
         std::_tstring st = tokizert[i];

         if (i==rpt_id_loc)
         {
            // the reportid 
            if (sd != st)
               return false;

            // See if this is in our ignore list
            StringSet::iterator it = m_IgnoreList.find(sd);
            if (it != m_IgnoreList.end())
            {
               // we are ignoring this report id
               return true;
            }
         }
         else if (i==value_loc)
         {
            // the result is the only value we use the floating point compare on
            double dd, dt;
            if (!sysTokenizer::ParseDouble(sd.c_str(), &dd) )
            {
               std::_tcout <<_T(" ****** Error Parsing Double ***********")<<std::endl;
               return false;
            }

            if (!sysTokenizer::ParseDouble(st.c_str(), &dt) )
            {
               std::_tcout <<_T(" ****** Error Parsing Double ***********")<<std::endl;
               return false;
            }

            // zero tolerance check first
            if (fabs(dd) > m_ZeroTol && fabs(dt) > m_ZeroTol)
            {
               if (dd != 0.0)
               {
                  // percent difference
                  double pdiff = (fabs(dd-dt))/fabs(dd);

                  if (pdiff > m_PercentDiff)
                     return false;
               }
               else
               {
                  return false;
               }
               
            }
         }
         else
         {
//            if (sd!=st)
//               return false;
         }
      }

   }
   else
   {
      // not a 12-50 file - just bail out
      return lineD==lineT;
   }

   return true;
}
