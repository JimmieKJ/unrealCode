// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;

namespace UnrealBuildTool.Rules
{
    public class SubstanceCore : ModuleRules
    {
	private enum SubstanceEngineType
	{
		CPU,
		GPU,
	};

        public SubstanceCore(TargetInfo Target)
        {
            // internal defines
            Definitions.Add("WITH_SUBSTANCE=1");
            Definitions.Add("SUBSTANCE_PLATFORM_BLEND=1");

            if (Target.Platform == UnrealTargetPlatform.Win32 ||
                Target.Platform == UnrealTargetPlatform.Win64 ||
                Target.Platform == UnrealTargetPlatform.XboxOne)
            {
                Definitions.Add("AIR_USE_WIN32_SYNC=1");
            }
            else if (Target.Platform == UnrealTargetPlatform.Mac ||
                     Target.Platform == UnrealTargetPlatform.PS4 ||
                     Target.Platform == UnrealTargetPlatform.Linux)
            {
                Definitions.Add("AIR_USE_PTHREAD_SYNC=1");
			}

			PublicIncludePaths.Add("SubstanceCore/Private/include");

			PrivateIncludePaths.Add("SubstanceCore/Private");
			PrivateIncludePaths.Add("SubstanceCore/Private/include");

			PrivateDependencyModuleNames.AddRange(new string[] {
                "Projects",
                "Slate",
                "SlateCore",
			});

            PublicDependencyModuleNames.AddRange(new string[] {
                    "AssetRegistry",
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "RenderCore",
                    "RHI",
                    "ImageWrapper",
                    "SessionServices",
                    "RHI"
            });

            if (UEBuildConfiguration.bBuildEditor == true)
            {
                PublicDependencyModuleNames.AddRange(new string[] {
                    "UnrealEd",
                    "AssetTools",
                    "ContentBrowser",
                    "Settings",
                    "TargetPlatform",
                    "MainFrame"
                });

                // used for ps4 cooking
                string SDKDir = System.Environment.GetEnvironmentVariable("SCE_ORBIS_SDK_DIR");
                if ((SDKDir != null) && (SDKDir.Length > 0))
                {
                    PublicIncludePaths.Add(SDKDir + "/target/include_common");
                    PublicLibraryPaths.Add(SDKDir + "/host_tools/lib");

                    PublicAdditionalLibraries.Add("libSceGpuAddress.lib");
                    PublicDelayLoadDLLs.Add("libSceGpuAddress.dll");
                    PublicAdditionalLibraries.Add("libSceGnm.lib");
                    PublicDelayLoadDLLs.Add("libSceGnm.dll");

                    //Toggle on our flag if we are building for PS4
                    if (Target.Platform == UnrealTargetPlatform.PS4 && Target.Type == TargetRules.TargetType.Editor)
                    {
                        Definitions.Add("SUBSTANCE_FOR_PS4=1");
                    }
                }
            }
			
			string BuildConfig = GetBuildConfig(Target);
			string PlatformConfig = GetPlatformConfig(Target);
			string SubstanceLibPath = ModuleDirectory + "/../../Libs/" + BuildConfig + "/" + PlatformConfig + "/";
			
			Dictionary<SubstanceEngineType, string> EngineLibs = new Dictionary<SubstanceEngineType, string>();
			List<string> StaticLibs = new List<string>();

			//add linker libraries
			StaticLibs.Add("pfxlinkercommon");
			StaticLibs.Add("algcompression");
            StaticLibs.Add("tinyxml");

			string LibExtension = "";
			string LibPrefix = "";
            if (Target.Platform == UnrealTargetPlatform.Win32 ||
                Target.Platform == UnrealTargetPlatform.Win64)
            {
                LibExtension = ".lib";

                StaticLibs.Add("substance_linker_static");

                EngineLibs.Add(SubstanceEngineType.CPU, "substance_sse2_blend_static");
                EngineLibs.Add(SubstanceEngineType.GPU, "substance_d3d11pc_blend_static");
            }
            else if (Target.Platform == UnrealTargetPlatform.Mac ||
                     Target.Platform == UnrealTargetPlatform.Linux)
            {
                LibExtension = ".a";
                LibPrefix = "lib";

                StaticLibs.Add("substance_linker");

                EngineLibs.Add(SubstanceEngineType.CPU, "substance_sse2_blend");
                EngineLibs.Add(SubstanceEngineType.GPU, "substance_ogl2_blend");
            }
            else if (Target.Platform == UnrealTargetPlatform.PS4)
            {
                LibExtension = ".a";
                LibPrefix = "lib";

                StaticLibs.Add("substance_linker");

                EngineLibs.Add(SubstanceEngineType.CPU, "substance_avx_blend");
            }
			else if (Target.Platform == UnrealTargetPlatform.XboxOne)
			{
                    LibExtension = ".lib";

				StaticLibs.Add("substance_linker");

				EngineLibs.Add(SubstanceEngineType.CPU, "substance_avx_blend");
			}
            else
            {
                throw new BuildException("Substance Plugin does not support platform " + Target.Platform.ToString());
            }

			if (Target.Type == TargetRules.TargetType.Editor)
			{
				Definitions.Add("AIR_NO_DEFAULT_ENGINE");
				Definitions.Add("SUBSTANCE_ENGINE_DYNAMIC");
				
				StaticLibs.Add("substance_framework_editor");

				if (BuildConfig == "Debug")
					Definitions.Add("SUBSTANCE_ENGINE_DEBUG");
			}
			else
			{
				Definitions.Add("AIR_NO_DYNLOAD");

				SubstanceEngineType type = GetEngineSuffix();
				
				if (EngineLibs.ContainsKey(type))
					StaticLibs.Add(EngineLibs[type]);
				else
					StaticLibs.Add(EngineLibs[SubstanceEngineType.CPU]);
				
				StaticLibs.Add("substance_framework");

				//Linux GPU requires certain libraries added
				if (Target.Platform == UnrealTargetPlatform.Linux && 
				    type == SubstanceEngineType.GPU)
				{
            	   	             PublicAdditionalLibraries.AddRange(new string[] { "X11", "glut", "GLU", "GL" });
				}
			}

			//add our static libs
			foreach (string staticlib in StaticLibs)
			{
				string libname = LibPrefix + staticlib + LibExtension;
				PublicAdditionalLibraries.Add(SubstanceLibPath + libname);
			}
        }

        private SubstanceEngineType GetEngineSuffix()
        {
            //grab first project listed in order to resolve ini path
            string iniPath = "";
            List<UProjectInfo> projects = UProjectInfo.FilterGameProjects(false, null);
            foreach (UProjectInfo upi in projects)
            {
                iniPath = upi.Folder.FullName;
                break;
            }

            //no INI, default to CPU
            if (iniPath.Length == 0)
                return SubstanceEngineType.CPU;

            //We can't use the ConfigCacheIni file here because it bypasses the Substance section.
			try
			{
				iniPath = Path.Combine(iniPath,Path.Combine("Config","DefaultEngine.ini"));

				//parse INI file line by line until we find our result
				StreamReader file = new StreamReader(iniPath);
				string line;
				string startsWith = "SubstanceEngine=";
				while ((line = file.ReadLine()) != null)
				{
					if (line.StartsWith(startsWith))
					{
						string value = line.Substring(startsWith.Length);
						
						if (value == "SET_CPU")
							return SubstanceEngineType.CPU;
						else if (value == "SET_GPU")
							return SubstanceEngineType.GPU;

						break;
					}
				}
				
				return SubstanceEngineType.CPU;
			}
			catch(Exception)
			{
				return SubstanceEngineType.CPU;
			}
        }

		public string GetBuildConfig(TargetInfo Target)
		{
			if (Target.Configuration == UnrealTargetConfiguration.Debug)
			{
				if (Target.Platform == UnrealTargetPlatform.Win32 ||
					Target.Platform == UnrealTargetPlatform.Win64)
				{
					if (BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
						return "Debug";
				}
				else
				{
					return "Debug";
				}
			}

			return "Release";
		}

		public string GetPlatformConfig(TargetInfo Target)
		{
			string platform = Target.Platform.ToString();

			if (Target.Platform == UnrealTargetPlatform.Win32 ||
				Target.Platform == UnrealTargetPlatform.Win64)
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
				{
					platform = platform + "_2013";
				}
			}
			
			return platform;
		}
    }
}
