///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

// ConfigPackager.cpp : Builds configuration packages and the ConfigurationPackages.json file
// that a configuration server publishes. (This program was called MakePgz.)
//
#include "stdafx.h"
#include "ZipPgz.h"
#include "UnzipPgz.h"
#include "ConfigurationManifest.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace ConfigManifest;

static void PrintCommandLine();

static void PrintError(const std::wstring& message)
{
   fwprintf(stderr, L"Error: %s\n", message.c_str());
}

// Writes <package>.md5 for program versions that don't read checksums from ConfigurationPackages.json
static bool WritePackageMd5File(const std::wstring& packageFile)
{
   std::wstring md5 = ComputeFileMd5(packageFile);
   if (md5.empty())
   {
      PrintError(L"Could not read " + packageFile);
      return false;
   }

   std::wstring md5File = packageFile + L".md5";
   FILE* pFile = nullptr;
   if (_wfopen_s(&pFile, md5File.c_str(), L"wb") != 0 || pFile == nullptr)
   {
      PrintError(L"Could not write " + md5File);
      return false;
   }
   fprintf(pFile, "%s\r\n", ToUtf8(md5).c_str());
   fclose(pFile);

   wprintf(L"MD5 %s written to %s\n", md5.c_str(), md5File.c_str());
   return true;
}

// Package address as written in the manifest: relative to the manifest's folder when the package
// is in that folder or below it, otherwise the full path.
static std::wstring GetRelativeUrl(const fs::path& manifestFile, const fs::path& packageFile)
{
   fs::path base = fs::absolute(manifestFile).parent_path();
   fs::path package = fs::absolute(packageFile);
   std::error_code ec;
   fs::path relative = fs::relative(package, base, ec);
   if (ec || relative.empty() || relative.native().rfind(L"..", 0) == 0)
   {
      return package.wstring();
   }
   return relative.generic_wstring(); // forward slashes
}

static bool LoadManifestIfExists(const fs::path& manifestFile, Manifest& manifest)
{
   if (!fs::exists(manifestFile))
   {
      manifest = Manifest();
      return true;
   }

   std::wstring error;
   if (!ReadManifest(manifestFile.wstring(), manifest, error))
   {
      PrintError(manifestFile.wstring() + L": " + error);
      return false;
   }
   return true;
}

static bool SaveManifest(const fs::path& manifestFile, const Manifest& manifest)
{
   std::wstring error;
   if (!WriteManifest(manifestFile.wstring(), manifest, error))
   {
      PrintError(error);
      return false;
   }
   wprintf(L"%s written\n", manifestFile.wstring().c_str());
   return true;
}

// Returns the value of an option such as /WebLink=..., and removes it from the list
static bool TakeOption(std::vector<std::wstring>& args, const std::wstring& name, std::wstring& value)
{
   std::wstring prefix = L"/" + name + L"=";
   for (auto it = args.begin(); it != args.end(); ++it)
   {
      if (StartsWithNoCase(*it, prefix))
      {
         value = it->substr(prefix.size());
         args.erase(it);
         return true;
      }
   }
   return false;
}

// /AddPackage ConfigurationPackages.json App "Configuration" AppVersion Package.zip [/Url=address] [/WebLink=address]
static int AddPackage(std::vector<std::wstring> args)
{
   std::wstring url, webLink;
   bool bUrl = TakeOption(args, L"Url", url);
   bool bWebLink = TakeOption(args, L"WebLink", webLink);

   if (args.size() != 5)
   {
      PrintCommandLine();
      return -1;
   }

   fs::path manifestFile(args[0]);
   std::wstring appName = args[1];
   std::wstring configurationName = args[2];
   std::wstring appVersion = args[3];
   fs::path packageFile(args[4]);

   unsigned int parts[3];
   if (!ParseVersion(appVersion, parts))
   {
      PrintError(L"The program version must be in the form 1.2.3: " + appVersion);
      return -1;
   }

   std::wstring md5 = ComputeFileMd5(packageFile.wstring());
   if (md5.empty())
   {
      PrintError(L"Could not read the package " + packageFile.wstring());
      return -1;
   }

   if (!bUrl)
   {
      url = GetRelativeUrl(manifestFile, packageFile);
   }

   if (StartsWithNoCase(url, L"http://"))
   {
      PrintError(L"Package addresses must start with https:// or be relative to ConfigurationPackages.json: " + url);
      return -1;
   }

   Manifest manifest;
   if (!LoadManifestIfExists(manifestFile, manifest))
   {
      return -1;
   }

   Application& application = manifest.GetOrAddApplication(appName);
   Configuration* pConfiguration = application.FindConfiguration(configurationName);
   if (pConfiguration == nullptr)
   {
      Configuration configuration;
      configuration.Name = configurationName;
      application.Configurations.push_back(configuration);
      pConfiguration = &application.Configurations.back();
   }

   if (bWebLink)
   {
      pConfiguration->WebLink = webLink;
   }

   Package package;
   package.AppVersion = appVersion;
   package.Url = url;
   package.Md5 = md5;

   auto found = std::find_if(pConfiguration->Packages.begin(), pConfiguration->Packages.end(),
      [&appVersion](const auto& p) {return CompareVersions(p.AppVersion, appVersion) == 0;});
   if (found == pConfiguration->Packages.end())
   {
      pConfiguration->Packages.push_back(package);
   }
   else
   {
      *found = package;
   }

   std::sort(pConfiguration->Packages.begin(), pConfiguration->Packages.end(),
      [](const auto& a, const auto& b) {return CompareVersions(a.AppVersion, b.AppVersion) < 0;});

   wprintf(L"%s, %s, version %s: %s (MD5 %s)\n", appName.c_str(), configurationName.c_str(), appVersion.c_str(), url.c_str(), md5.c_str());
   return SaveManifest(manifestFile, manifest) ? 0 : -1;
}

// /Convert ConfigurationPackages.json AppPackages.ini [AppPackages.ini ...] [/Base=address] [/WebLink=address] [/Zip]
static int Convert(std::vector<std::wstring> args)
{
   std::wstring base, webLink;
   TakeOption(args, L"Base", base);
   bool bWebLink = TakeOption(args, L"WebLink", webLink);

   // /Zip: package addresses ending in .pgz end in .zip instead (the files must be renamed when they are published)
   bool bZip = false;
   auto zip = std::find_if(args.begin(), args.end(), [](const auto& a) {return EqualsNoCase(a, L"/Zip");});
   if (zip != args.end())
   {
      bZip = true;
      args.erase(zip);
   }

   if (!base.empty() && base.back() != L'/')
   {
      base += L'/';
   }

   if (args.size() < 2)
   {
      PrintCommandLine();
      return -1;
   }

   fs::path manifestFile(args[0]);
   Manifest manifest;
   if (!LoadManifestIfExists(manifestFile, manifest))
   {
      return -1;
   }

   for (size_t i = 1; i < args.size(); i++)
   {
      fs::path iniFile = fs::absolute(args[i]);
      std::wstring fileName = iniFile.filename().wstring();
      const std::wstring suffix(L"Packages.ini");
      if (!EndsWithNoCase(fileName, suffix) || fileName.size() == suffix.size())
      {
         PrintError(L"The INI file name must be <Program>Packages.ini, for example PGSuperPackages.ini: " + fileName);
         return -1;
      }

      if (!fs::exists(iniFile))
      {
         PrintError(L"Could not find " + iniFile.wstring());
         return -1;
      }

      std::wstring appName = fileName.substr(0, fileName.size() - suffix.size());
      Application application = ReadLegacyIni(iniFile.wstring(), appName);

      for (auto& configuration : application.Configurations)
      {
         if (bWebLink)
         {
            configuration.WebLink = webLink;
         }

         for (auto& package : configuration.Packages)
         {
            if (bZip && EndsWithNoCase(package.Url, L".pgz"))
            {
               package.Url = package.Url.substr(0, package.Url.size() - 4) + L".zip";
            }

            // JSON files require HTTPS
            if (StartsWithNoCase(package.Url, L"http://"))
            {
               package.Url = L"https://" + package.Url.substr(7);
            }

            // addresses on this server become relative to ConfigurationPackages.json
            std::wstring httpsBase = StartsWithNoCase(base, L"http://") ? L"https://" + base.substr(7) : base;
            if (!httpsBase.empty() && StartsWithNoCase(package.Url, httpsBase))
            {
               package.Url = package.Url.substr(httpsBase.size());
            }
         }
      }

      // replace the program's section
      Application* pExisting = manifest.FindApplication(appName);
      if (pExisting)
      {
         *pExisting = application;
      }
      else
      {
         manifest.Applications.push_back(application);
      }

      wprintf(L"%s: %d configurations read from %s\n", appName.c_str(), (int)application.Configurations.size(), iniFile.wstring().c_str());
   }

   wprintf(L"Run ConfigPackager /UpdateMd5 to add the checksums of packages stored with this file.\n");
   return SaveManifest(manifestFile, manifest) ? 0 : -1;
}

// /UpdateMd5 ConfigurationPackages.json
static int UpdateMd5(const std::vector<std::wstring>& args)
{
   if (args.size() != 1)
   {
      PrintCommandLine();
      return -1;
   }

   fs::path manifestFile(args[0]);
   Manifest manifest;
   std::wstring error;
   if (!ReadManifest(manifestFile.wstring(), manifest, error))
   {
      PrintError(manifestFile.wstring() + L": " + error);
      return -1;
   }

   fs::path base = fs::absolute(manifestFile).parent_path();
   int nUpdated = 0, nSkipped = 0;
   for (auto& application : manifest.Applications)
   {
      for (auto& configuration : application.Configurations)
      {
         for (auto& package : configuration.Packages)
         {
            if (IsAbsoluteLocation(package.Url) && package.Url.find(L"://") != std::wstring::npos)
            {
               wprintf(L"Skipped (not stored with this file): %s\n", package.Url.c_str());
               nSkipped++;
               continue;
            }

            fs::path packageFile = IsAbsoluteLocation(package.Url) ? fs::path(package.Url) : base / fs::path(package.Url);
            std::wstring md5 = ComputeFileMd5(packageFile.wstring());
            if (md5.empty())
            {
               PrintError(L"Could not read " + packageFile.wstring());
               return -1;
            }
            package.Md5 = md5;
            nUpdated++;
         }
      }
   }

   wprintf(L"%d checksums updated, %d skipped\n", nUpdated, nSkipped);
   return SaveManifest(manifestFile, manifest) ? 0 : -1;
}

// /Check ConfigurationPackages.json
static int Check(const std::vector<std::wstring>& args)
{
   if (args.size() != 1)
   {
      PrintCommandLine();
      return -1;
   }

   fs::path manifestFile(args[0]);
   Manifest manifest;
   std::wstring error;
   if (!ReadManifest(manifestFile.wstring(), manifest, error))
   {
      PrintError(manifestFile.wstring() + L": " + error);
      return -1;
   }

   if (CurrentSchemaVersion < manifest.SchemaVersion)
   {
      wprintf(L"Warning: schemaVersion %d is newer than this program understands (%d).\n", manifest.SchemaVersion, CurrentSchemaVersion);
   }

   fs::path base = fs::absolute(manifestFile).parent_path();
   int nProblems = 0;
   for (const auto& application : manifest.Applications)
   {
      wprintf(L"%s\n", application.Name.c_str());
      for (const auto& configuration : application.Configurations)
      {
         wprintf(L"   %s\n", configuration.Name.c_str());
         for (const auto& package : configuration.Packages)
         {
            std::wstring status;
            if (IsAbsoluteLocation(package.Url) && package.Url.find(L"://") != std::wstring::npos)
            {
               status = package.Md5.empty() ? L"not checked (on another server); no md5" : L"not checked (on another server)";
               if (package.Md5.empty())
               {
                  nProblems++;
               }
            }
            else
            {
               fs::path packageFile = IsAbsoluteLocation(package.Url) ? fs::path(package.Url) : base / fs::path(package.Url);
               std::wstring md5 = ComputeFileMd5(packageFile.wstring());
               if (md5.empty())
               {
                  status = L"MISSING";
                  nProblems++;
               }
               else if (package.Md5.empty())
               {
                  // Allowed: the programs read <package>.md5 from the server instead, as older versions do.
                  // That file must be published next to the package.
                  fs::path md5File(packageFile.wstring() + L".md5");
                  std::vector<std::wstring> hashes;
                  if (ReadMd5List(md5File.wstring(), hashes) && IsInMd5List(packageFile.wstring(), hashes))
                  {
                     status = L"OK (md5 read from " + md5File.filename().wstring() + L")";
                  }
                  else
                  {
                     status = L"no md5, and no matching " + md5File.filename().wstring() + L" (run /UpdateMd5)";
                     nProblems++;
                  }
               }
               else if (!EqualsNoCase(md5, package.Md5))
               {
                  status = L"MD5 DOES NOT MATCH (run /UpdateMd5)";
                  nProblems++;
               }
               else
               {
                  status = L"OK";
               }
            }
            wprintf(L"      %-8s %s  %s\n", package.AppVersion.c_str(), package.Url.c_str(), status.c_str());
         }
      }
   }

   if (nProblems == 0)
   {
      wprintf(L"No problems found.\n");
      return 0;
   }

   wprintf(L"%d problem(s) found.\n", nProblems);
   return 1;
}

// /Md5List FileOrFolder ListFile
static int Md5List(const std::vector<std::wstring>& args)
{
   if (args.size() != 2)
   {
      PrintCommandLine();
      return -1;
   }

   std::wstring error;
   if (!WriteMd5List(fs::absolute(args[0]).wstring(), fs::absolute(args[1]).wstring(), error))
   {
      PrintError(error);
      return -1;
   }

   wprintf(L"%s written\n", args[1].c_str());
   return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
   std::vector<std::wstring> args(argv + 1, argv + argc);

   if (!args.empty() && args[0].size() > 1 && args[0][0] == L'/')
   {
      std::wstring command = args[0];
      args.erase(args.begin());

      if (EqualsNoCase(command, L"/AddPackage"))
      {
         return AddPackage(args);
      }
      else if (EqualsNoCase(command, L"/Convert"))
      {
         return Convert(args);
      }
      else if (EqualsNoCase(command, L"/UpdateMd5"))
      {
         return UpdateMd5(args);
      }
      else if (EqualsNoCase(command, L"/Check"))
      {
         return Check(args);
      }
      else if (EqualsNoCase(command, L"/Md5List"))
      {
         return Md5List(args);
      }

      PrintCommandLine();
      return -1;
   }

   if (argc==5 || argc==6)
   {
      // Zipping master library file and templates
      int st = ZipPGZ(argv[2], // master library file
                      argv[3], // root of workgroup templates
                      argv[1], // package file
                      argv[4], // file extension
                      argc == 6 ? argv[5] : nullptr); // file extension alt
      if (st != 0)
      {
         return st;
      }

      return WritePackageMd5File(argv[1]) ? 0 : -1;
   }
   else if ( argc == 3 )
   {
      // unzipping or listing
      if (EqualsNoCase(argv[2],L"/L")) // listing
      {
         return ListPGZ(argv[1]);
      }
      else if (EqualsNoCase(argv[2],L"/U")) // unzipping
      {
         return UnZipPGZ(argv[1], (TCHAR*)_T(".\\"),true);
      }
   }

   // command line error
   PrintCommandLine();
   return -1;
}

static void PrintCommandLine()
{
   _tprintf(_T("ConfigPackager - builds configuration packages and ConfigurationPackages.json\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("Create a package (also writes Package.zip.md5 for older program versions):\n"));
   _tprintf(_T("    ConfigPackager Package.zip Library.lbr RootTemplateFolder TemplateExtension [TemplateExtension2]\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("List or uncompress a package (.zip or .pgz):\n"));
   _tprintf(_T("    ConfigPackager Package.zip /L\n"));
   _tprintf(_T("    ConfigPackager Package.zip /U\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("Add a package to ConfigurationPackages.json (creates the file if needed):\n"));
   _tprintf(_T("    ConfigPackager /AddPackage ConfigurationPackages.json Program \"Configuration\" ProgramVersion Package.zip [/Url=address] [/WebLink=address]\n"));
   _tprintf(_T("       Program        = PGSuper, PGSplice or TOGA\n"));
   _tprintf(_T("       ProgramVersion = lowest program version the package is for, e.g. 9.1.0\n"));
   _tprintf(_T("       /Url           = package address to publish; default is the package path relative to the JSON file\n"));
   _tprintf(_T("       /WebLink       = address of the configuration's documentation\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("Convert <Program>Packages.ini files to ConfigurationPackages.json:\n"));
   _tprintf(_T("    ConfigPackager /Convert ConfigurationPackages.json PGSuperPackages.ini [PGSplicePackages.ini ...] [/Base=address] [/WebLink=address] [/Zip]\n"));
   _tprintf(_T("       /Base    = address of the folder the packages were published in; addresses below it become relative\n"));
   _tprintf(_T("       /WebLink = documentation address to use for every configuration\n"));
   _tprintf(_T("       /Zip     = change package addresses ending in .pgz to .zip (rename the files when you publish them)\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("Fill in the md5 of every package stored with the JSON file:\n"));
   _tprintf(_T("    ConfigPackager /UpdateMd5 ConfigurationPackages.json\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("Check a JSON file before publishing it:\n"));
   _tprintf(_T("    ConfigPackager /Check ConfigurationPackages.json\n"));
   _tprintf(_T("\n"));
   _tprintf(_T("Write a checksum list for a \"Local or Network File System\" server:\n"));
   _tprintf(_T("    ConfigPackager /Md5List Library.lbr Library.lbr.md5\n"));
   _tprintf(_T("    ConfigPackager /Md5List TemplateFolder TemplateFolder\\WorkgroupTemplates.md5\n"));
}
