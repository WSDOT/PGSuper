///////////////////////////////////////////////////
// 
// RunRegression.js 
// A script for running PGSuper regression tests.
// Run with /? to get help.
//
//
///////////////////////////////////////////////////

// first some defaults
// first some defaults
var PGSuperVersion = "Debug";
var PGSuperPlatform = "WIN32";
var DoSendEmail=false;
var EmailAddress = new String;
var ExecuteCommands=true; // if false, only show commands
var RunMultipleFiles=false;
var AppName = "PGSuper";
var FileExt = "pgs";


var startDate = new Date();
var startTime = startDate.getTime();

// hack for convenience:
// set up drives so pickings' and brice's machines work without command line options
var PGSuperDrive = "F:";
var objNet = new ActiveXObject( "WScript.Network" );
var machine = objNet.ComputerName;
machine = machine.toUpperCase();
if (machine=="RICHARDSDELL")
   PGSuperDrive = "C:";
else if (machine=="ACERI7WIN7")
   PGSuperDrive = "C:";
else if (machine=="RICHARDSGRAM")
   PGSuperDrive = "C:";
else if (machine=="HQB0630025")
   PGSuperDrive = "F:";
else if (machine=="HQE3609046")
   PGSuperDrive = "F:";
else if (machine=="HQD1764064")
   PGSuperDrive = "F:";

var wsShell = new ActiveXObject("WScript.Shell");
var FSO = new ActiveXObject("Scripting.FileSystemObject");

// Create file name for timer file
var timerFileDate = new String;
timerFileDate = startDate.toString();
timerFileDate = timerFileDate.replace(/ /gi, "_"); // regular expression pinta to make a reasonable file name
timerFileDate = timerFileDate.replace(/:/gi, ";");
var timerFileName = new String;
timerFileName = "\\ARP\\PGSuper\\RegressionTest\\RegTimer_" + timerFileDate + ".log";

// parse the command line and set options
var st = ParseCommandLine();

if ( ExecuteCommands )
{
   var timerFile = FSO.OpenTextFile(timerFileName, 2, true); // 2 = write new file
   timerFile.WriteLine("********** Starting new regr test at: " + startDate.toString() + " elapsed times in minutes *************");
}

if (st!=0)
{
   CleanUpTest();
   WScript.Quit(st);
}

// global vars
var currSpan="All";
var currGirder="EI";

var OldCatalogServer = new String;
var NewCatalogServer = new String("Regression");
var OldCatalogPublisher = new String;
var NewCatalogPublisher = new String("Regression");

var Application = PGSuperDrive+"\\ARP\\BridgeLink\\RegFreeCOM\\" +PGSuperPlatform+"\\"+PGSuperVersion+"\\BridgeLink.exe ";
var StartFolderSpec = new String( PGSuperDrive+"\\ARP\\PGSuper\\RegressionTest" );
var CurrFolderSpec  = new String( PGSuperDrive+"\\ARP\\PGSuper\\RegressionTest\\Current" );
var DatumFolderSpec = new String( PGSuperDrive+"\\ARP\\PGSuper\\RegressionTest\\Datum" );

var ErrorsExist = "FALSE";

var concurrencyCount = 1; // keeps track of number of concurrent processes
var maxConcurrencyCount = 4; // maximum number of concurrent processes

// make sure pgsuper.exe exists
if (!FSO.FileExists(Application))
{
   DisplayMessage("Error - Program File: "+Application+" does not exist - Script Terminated");
   CleanUpTest();
   WScript.Quit(1);
}

// Ensure TxDOT Extension Agent is enabled. It is required for the /Tx commands
var OldTxDOTAgentState = wsShell.RegRead("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Extensions\\{3700B253-8489-457c-8A6D-D174F95C457C}");
if (OldTxDOTAgentState == "Disabled") {
    DisplayMessage("********");
    DisplayMessage("********");
    DisplayMessage("Error - TxDOTAgent extensions are not Enabled - Regression tests depend");
    DisplayMessage("        on this to output test data.");
    DisplayMessage("        To fix: Start PGSuper and enable the TxDOT extensions from");
    DisplayMessage("        the Manage Extensions dialog.");
    DisplayMessage("********");
    DisplayMessage("********");
    WScript.Quit(1);
}


// Ensure XBRate Extension Agent is enabled. It is required for the /XBR commands
var OldXBRateAgentState = wsShell.RegRead("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Extensions\\{60BF2930-673C-4d29-B654-8A2E0879DE2B}");
if (OldXBRateAgentState == "Disabled") {
    DisplayMessage("********");
    DisplayMessage("********");
    DisplayMessage("Error - XBRateAgent extensions are not Enabled - Regression tests depend");
    DisplayMessage("        on this to output test data.");
    DisplayMessage("        To fix: Start PGSuper and enable the XBrate extensions from");
    DisplayMessage("        the Manage Extensions dialog.");
    DisplayMessage("********");
    DisplayMessage("********");
    WScript.Quit(1);
}

// First clean up results from any old runs and set up environment
var CurrentFolder = StartFolderSpec;
InitTest(CurrentFolder);

var CurrCommand="TestR";
// DisplayMessage("Before RunTest, Command is " + CurrCommand);
RunTest(CurrentFolder,CurrCommand);

if(ExecuteCommands)
{
    CheckResults(CurrentFolder);
}

CleanUpTest();

var st = 0;
if (!ErrorsExist)
{
   DisplayMessage("Test Successful - no errors");
   st = 1;
}

var endDate = new Date();
var endTime = endDate.getTime();
var elapsed = (endTime-startTime)/60000.0;
DisplayMessage("Elapsed Time was: "+elapsed+" Minutes");

if ( ExecuteCommands )
{
   // only log times if the tess were actually run... (don't log times when /N option is used... it isn't meaninful data)
   var myFile = FSO.OpenTextFile("\\ARP\\PGSuper\\RegressionTest\\RegTest.log",8,true); // 8 = ForAppending (for some reason the ForAppending constant isn't defined)
   myFile.WriteLine(endDate.toString() + " : Elapsed Time was: "+elapsed+" Minutes");
   myFile.close();

   timerFile.WriteLine(endDate.toString() + " : Total Elapsed Time was: " + elapsed + " Minutes");
   timerFile.close();  
}
      
WScript.Quit(st);

// --------------------- functions ------------------
function RunTest (currFolder, currCommand) 
{
   // find all .pgs files and run pgsuper on them
   var Folder = FSO.GetFolder(currFolder);
   var ff = new Enumerator(Folder.SubFolders);
   for (; !ff.atEnd(); ff.moveNext())
   {
      var folderName = new String(ff.item());

      // Don't walk into forbidden 
      if (IsForbiddenFolder(folderName))
      {
         continue;
      }

      var subFolder = FSO.GetFolder(ff.item());

      // Need to parse current command here if in folder name
      var newCommand = ParseCommandFromFolderName(currCommand, subFolder.Name);
      
      if (newCommand!="TxToga")
      {
         // Not TOGA - get pgsuper files
         var fc = new Enumerator(subFolder.Files);

         // Get number of pgs files
         var nPgsFiles = 0;
          for (; !fc.atEnd(); fc.moveNext() )
          {
             s = new String(fc.item());
            
            idx = s.indexOf(FileExt);
            if (-1 != idx)
               nPgsFiles++;
          }
          
          var fileCount = 0;
          for (fc.moveFirst(); !fc.atEnd(); fc.moveNext())
          {
             s = new String(fc.item());
            
            idx = s.indexOf(FileExt);
            if (-1 != idx) {
               fileCount++; // processing a pgs file.. increment count
               
               // Get span and girder from file name
               var newSG = new SpanGirder();
               newSG = ParseGirderFromFileName(s, currSpan, currGirder);

               var outFile= new String;
               if (newCommand == "TestR"  || newCommand == "XBRTest" )
               {
                  // Strip output file name when 1250 is used (1250's generate their own file names)
                  outFile = "";
                  cmd = Application + " /" + newCommand + " " + s; 
               }
               else
               {
                  outFile = s.substring(0,idx) + "@" + newCommand + "_" + newSG.m_Span + "_" + newSG.m_Girder + ".Test";
                  cmd = Application + " /" + newCommand + " " + s + " " + outFile + " " + newSG.m_Span + " " + newSG.m_Girder;
               }


               if (RunMultipleFiles && fileCount != nPgsFiles) 
               {
                  // if we are running multiple files and this is not the last PGS file, add the spawning code
                  // don't ever want to do this for the last non-TOGA file. we must wait until the last non-TOGA
                  // file is done so that we don't change the library too soon

                   if (concurrencyCount < maxConcurrencyCount || 
                       s.search("LRFrame2") != -1 ||
                       s.search("I_5_SCIP") != -1 ||
                       s.search("KEEHI_IV") != -1 ||
                       s.search("ShearKeyLoadBog") != -1
                       )
	               {
        	          cmd = "cmd.exe /C START \"XYZ\" " + cmd
                	  concurrencyCount++;
	               }
	               else 
	               {
        	          concurrencyCount = 1;
	               }
               }

               if(ExecuteCommands)
               {
                   var begDate = new Date();
                   var begrunTime = begDate.getTime();
                   
                   DisplayMessage("Running: " + cmd);
                   DisplayMessage("");
                   st = wsShell.Run(cmd, 1, "TRUE");

                   var endrunDate = new Date();
                   var endrunTime = endrunDate.getTime();
                   var elapsed = (endrunTime - begrunTime) / 60000.0;
                   timerFile.WriteLine(s + ", " + elapsed); // comma-delimited elapsed time for each file
                   
               }
               else
               {
                   DisplayMessage(cmd);
               }
            }
         }   
     }
     else
     {
         if ( AppName != "PGSPLICE" )
         {
         // testing TOGA - need to set library to txdot
         SetTOGALibrary("TxDOTRegressionTest", "TxDOTRegressionTest");
          
         var fc = new Enumerator(subFolder.Files);
         for (; !fc.atEnd(); fc.moveNext())
         {
            s = new String(fc.item());
         
            idx = s.indexOf(".toga");
            if (-1 != idx)
            {
               var outFile= new String;
               outFile = s.substring(0,idx) + "@" + newCommand + ".Test";
               cmd = Application + " /" + newCommand + " " + s + " " + outFile;

               if(ExecuteCommands)
               {
                   DisplayMessage("Running: "+ cmd);
                   DisplayMessage("");
                   st = wsShell.Run(cmd,1,"TRUE");
               }
               else
               {
                   DisplayMessage(cmd);
               }
            }
         }
         
         // Back to current testing library
         SetPGSuperLibrary(NewCatalogServer, NewCatalogPublisher);
         }
     }

     // Recurse
     RunTest(subFolder,newCommand);
   }
}

function InitTest(currFolder) 
{
    if (ExecuteCommands)
    {
       // clean up temporary files - only when actual test
       CleanFolder(currFolder);
    }
   
   // Save initial server and publisher
   OldCatalogServer = wsShell.RegRead("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Options\\CatalogServer2");
   OldCatalogPublisher = wsShell.RegRead("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Options\\Publisher2");
   
   // Run PGSuper to set new server and publisher
   SetPGSuperLibrary(NewCatalogServer, NewCatalogPublisher);
}   

function SetPGSuperLibrary(server, publisher)
{
   var cmd = new String;
   cmd = Application + "/App=" + AppName + " /Configuration=\"" + server + "\":\"" + publisher + "\"";

   if(ExecuteCommands)
   {
       DisplayMessage("Running: "+ cmd);
       DisplayMessage("");
       st = wsShell.Run(cmd,1,"TRUE"); 
   }
   else
   {
       DisplayMessage(cmd);
   }
}

function SetTOGALibrary(server, publisher)
{
   var cmd = new String;
   cmd = Application + "/App=TOGA" + " /Configuration=\"" + server + "\":\"" + publisher + "\"";

   if(ExecuteCommands)
   {
       DisplayMessage("Running: "+ cmd);
       DisplayMessage("");
       st = wsShell.Run(cmd,1,"TRUE"); 
   }
   else
   {
       DisplayMessage(cmd);
   }
}


function CleanFolder(currFolder)
{
   var Folder = FSO.GetFolder(currFolder);
   var ff = new Enumerator(Folder.SubFolders);
   for (; !ff.atEnd(); ff.moveNext())
   {
      var folderName = new String(ff.item());

      // Don't walk into forbidden folders
      if (IsForbiddenFolder(folderName))
      {
         continue;
      }

      var subFolder = FSO.GetFolder(ff.item());

      var fc = new Enumerator(subFolder.Files);
      for (; !fc.atEnd(); fc.moveNext())
      {
        s = new String(fc.item());
        s = s.toLowerCase();
        if ( -1!=s.indexOf(".err") || -1!=s.indexOf(".test") || -1!=s.indexOf(".dbr") || -1!=s.indexOf(".dbp") || -1!=s.indexOf(".log") )
        {
           FSO.DeleteFile(s);
        }

      }

      // recurse to subfolders
      CleanFolder(subFolder);
   }
}

function CleanUpTest() 
{
   // Restore registry to where it was before we started
   // Run PGSuper to set new server and publisher
   SetPGSuperLibrary(OldCatalogServer, OldCatalogPublisher);
}

function CheckResults(currFolder) 
{
   // first check to see if any .err files exist.
   // clean up empty error files 

   var Folder = FSO.GetFolder(currFolder);
   var ff = new Enumerator(Folder.SubFolders);
   for (; !ff.atEnd(); ff.moveNext())
   {
      var folderName = new String(ff.item());

      // Don't walk into forbidden folders
      if (IsForbiddenFolder(folderName))
      {
         continue;
      }

      var subFolder = FSO.GetFolder(ff.item());

      var fc = new Enumerator(subFolder.Files);
      for (; !fc.atEnd(); fc.moveNext())
      {
        s = new String(fc.item());
        s = s.toLowerCase();
        if ( -1!=s.indexOf(".err") )
        {
           fil = fc.item();
           if (fil.size == 0)
           {
               FSO.DeleteFile(fil);
           }
           else
           {
               ErrorsExist="TRUE";
               DisplayMessage("Test Failed - .err files exist in: "+subFolder);
               continue;
           }
        }

      }

      CheckResults(subFolder) 
   }
  
}


function ParseCommandFromFolderName(currCommand, folderName)
{
  var idx = folderName.indexOf("-")
  if ( -1 != idx)
  {
     // We have a -, so parse command to end of name
     var s = folderName.substr(idx+1); 
     s = s.toUpperCase();

     var s3 = s.substr(0,3); // take first three characters to check for Tx commands
     var s4 = s.substr(0,4); // take first four characters to check for Tx commands
     var cmd;

     if (s4=="TXAT")
     {
        cmd = "TxAT";
     }
     else if (s4=="TXDT")
     {
        cmd = "TxDT";
     }
     else if (s4 == "TXDS") {
         cmd = "TxDS";
     }
     else if (s4 == "TXDX")
     {
        cmd = "TxDx";
     }
     else if (s=="TESTDF")
     {
        cmd = "TestDF";
     }
     else if (s3=="125")
     {
        cmd = "TestR"; // only full regression for now
     }
     else if (s=="TXTOGA")
     {
        cmd = "TxToga";
     }
     else if ( s == "XBRTEST" )
     {
        cmd = "XBRTest"
     }
     else if ( s == "TESTGEOMETRY" )
     {
        cmd = "TestGeometry"
     }
     else
     {
         DisplayMessage("Error - Invalid Folder Name, could not parse command: "+s);
         CleanUpTest();
         WScript.Quit(1);
     }

     return cmd;
     
  }
  else
  {
     // No new command, use old one
     return currCommand;
  }
}

// Use bizarre javascript syntax to create a spanGirder combination so our function below has something to return
function SpanGirder()
{
   this.m_Span = "Error";
   this.m_Girder = "Error";
}


function ParseGirderFromFileName(fileName, currSpan, currGirder)
{
  // Our defaults if nothing is parsed
  var newSpanGirder = new SpanGirder();
  newSpanGirder.m_Span = currSpan;
  newSpanGirder.m_Girder = currGirder;

  // First strip path from fileName
  var slashIdx = fileName.lastIndexOf("\\");
  if ( -1 != slashIdx)
  {
     fileName = fileName.substring(slashIdx, fileName.length);
  }

  var firstDashIdx = fileName.indexOf("-")
  if ( -1 != firstDashIdx)
  {
     var secondDashIdx = fileName.indexOf("-",firstDashIdx+1);
     var firstDotIdx = fileName.indexOf(".");
     
     // span is first
     var spanName = new String;
     if (-1 != secondDashIdx)
     {
         spanName = fileName.substring(firstDashIdx+1,secondDashIdx);
     }
     else
     {
         spanName = fileName.substring(firstDashIdx+1,firstDotIdx);
     }

     spanName = spanName.toUpperCase();
     if (spanName=="ALL")
     {
        newSpanGirder.m_Span = "ALL";
     }
     else
     {
         // span must be a number
         var spanNum = new Number(spanName);
         if (!isNaN(spanNum))
         {
            newSpanGirder.m_Span = spanNum;
         }
         else
         {
            DisplayMessage("Error - Could not parse span number from File Name: "+fileName);
            CleanUpTest();
            WScript.Quit(1);
         }
     }

     // Go after girder name next
     if (-1 != secondDashIdx)
     {
         var girderName = new String;
         girderName = fileName.substring(secondDashIdx+1,firstDotIdx);
         girderName = girderName.toUpperCase();

         if (girderName=="ALL")
         {
            newSpanGirder.m_Girder = "ALL"
         }
         else if (girderName=="EI")
         {
            newSpanGirder.m_Girder = "EI";
         }
         else
         {
            // valid girder names at this time are A-Z
            if (girderName.length==1 && girderName>="A" && girderName<="Z")
            {
               newSpanGirder.m_Girder = girderName;
            }
            else
            {
               DisplayMessage("Error - Could not parse girder number from File Name: "+fileName);
               CleanUpTest();
               WScript.Quit(1);
            }
         }
     }
  }

  return newSpanGirder;
}

function ParseCommandLine()
{
   var didDrive=0; // drive on command line?
   var didVersion=0; // Release or Debug on command line?
   var didPlatform=0;
   var objArgs = WScript.Arguments;
   var I;
   for (I=0; I<objArgs.Count(); I++)
   {
      var s = new String(objArgs(I));
        
      if (s.charAt(0)=="/" || s.charAt(0)=="-")
      {
       if (s.charAt(1)=="?")
       {
         DisplayMessage("--- Help for Test.js - Regression Testing Script ---");
         DisplayMessage("Command Line options:");
         DisplayMessage("    /?             - Help (you are here)");
         DisplayMessage("    /D<drive-name> - Drive where PGSuper is installed (e.g., /DC:)");
         DisplayMessage("    /V<version>    - Version of PGSuper to test (either \"Debug\", \"Profile\", or \"Release\"");
         DisplayMessage("    /P<platform>   - Platform (Win32 or X64)");
         DisplayMessage("    /N             - No execute. Display but do not execute pgsuper commands.");
         DisplayMessage("    /M             - Run multiple files.");
         DisplayMessage("");
         return 1;
       }
       else if (s.charAt(1)=="A" || s.charAt(1)=="a")
       {
          var app = s.substring(2,s.length);
          app = app.toUpperCase();
          if ( app=="PGSUPER" || app=="PGSPLICE" )
          {
              AppName = app;
              if ( app=="PGSUPER" )
                 FileExt = ".pgs";
              else
                 FileExt = ".spl";
          }
          else
          {
             DisplayMessage("Invalid application - must be either PGSuper or PGSplice");
             DisplayMessage("String was: \""+app+"\"");
             return 1;
          }
       }
       else if (s.charAt(1)=="D" || s.charAt(1)=="d")
       {
          var dr = s.substring(2,s.length);
          dr = dr.toUpperCase();
          if (dr.length==1 && dr.charAt(1)!=":")
          {
             DisplayMessage("Invalid format for Drive Letter - must be in form C:");
             DisplayMessage("String was: \""+dr+"\"");
             return 1;
          }
          PGSuperDrive = dr;
          didDrive=1;          
       }
       else if (s.charAt(1)=="V" || s.charAt(1)=="v")
       {
          var ver = s.substring(2,s.length);
          ver = ver.toUpperCase();
          if (ver=="DEBUG" || ver=="RELEASE"|| ver=="PROFILE")
          {
             PGSuperVersion = ver;
             didVersion=1;
          }
          else
          {
             DisplayMessage("Invalid PGSuper version on command line - must be either Release, Profile, or Debug");
             DisplayMessage("String was: \""+ver+"\"");
             return 1;
          }
       }
       else if (s.charAt(1)=="P" || s.charAt(1)=="p")
       {
          var platform = s.substring(2,s.length);
          platform = platform.toUpperCase();
          if (platform =="WIN32" || platform =="X64")
          {
             PGSuperPlatform = platform;
             didPlatform=1;
          }
          else
          {
             DisplayMessage("Invalid platform on command line - must be either Win32 or x64");
             DisplayMessage("String was: \""+platform+"\"");
             return 1;
          }
       }
       else if (s.charAt(1)=="S" || s.charAt(1)=="s")
       {
          // set up for sending email
          EmailAddress = s.substring(2,s.length);
          DoSendEmail = true;
       }
       else if (s.charAt(1)=="N" || s.charAt(1)=="n")
       {
          // No execute. Display but do not execute commands.
          ExecuteCommands=false;
       }
       else if (s.charAt(1)=="M" || s.charAt(1)=="m")
       {
          // Run multiple files... though this messes up timing.
          RunMultipleFiles=true;
       }
       else
       {
         DisplayMessage("Error - Bad Command line parameter"+s);
         return 1;
       }
     }
     else
     {
        DisplayMessage("Error - Bad Command line parameter: \""+s+"\"");
        return 1;
     }
   }
   
   return 0;
}

function SendEmail(status)
{
// latch onto Outlook - error will be catastrophic
var ol = new ActiveXObject("Outlook.Application");

// Return a reference to the MAPI layer
var ns = ol.GetNamespace("MAPI");

// Create a new mail message item
var newMail = ol.CreateItem(0); // olMailItem from typelib - dangerous, but only way we can play in jscript

// Add the subject of the mail message
if (st==0)
   newMail.Subject = "PGSuper Regression Test - Passed";
else
   newMail.Subject = "PGSuper Regression Test - Failed";

// Create some body text
newMail.Body = "See attached diff file for more information";

// Add  recipient and test to make sure that the address is valid using the Resolve method
var rec = newMail.Recipients.Add(EmailAddress);
rec.Type = 1; // olTo
if (! rec.Resolve )
{
    DisplayMessage( "Unable to resolve address.");
}

// Attach diff file to message
var attch = newMail.Attachments.Add(DiffFile, 1); //olByValue 
attch.DisplayName = "Diff File";

//Send the mail message
newMail.Send();
}

function IsForbiddenFolder(folderName)
{
   // Don't walk into cvs or special folders
   folderName = folderName.toUpperCase();
   var matchcvs = folderName.match("CVS");
   var matchdtm = folderName.match("DATUM");
   var match125 = folderName.match("1250_Diff");
   if (null!=matchcvs || null!=matchdtm || null!=match125)
   {
      return true;
   }

   return false;
}

function DisplayMessage(msg)
{
   // could make this modal and add dialog-type messages for interactive mode
   WScript.Echo(msg);
}

