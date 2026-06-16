-- Premake Project Generator for XMenu

PSDK_DIR = os.getenv("PLUGIN_SDK_DIR")
if (PSDK_DIR == nil or PSDK_DIR == "") then
    print("WARNING: PLUGIN_SDK_DIR environment variable not set in system.")
    print("Falling back to local path relative check...")

    if os.isdir("../plugin-sdk") then
        PSDK_DIR = "../plugin-sdk"
    elseif os.isdir("../../plugin-sdk") then
        PSDK_DIR = "../../plugin-sdk"
    else
        error("PLUGIN_SDK_DIR environment variable not set, and couldn't find plugin-sdk in parent directories. Please set it in Setup.bat or System Environment Variables.")
    end
end

print("Using Plugin-SDK directory: " .. PSDK_DIR)

workspace "XMenu"
    configurations { "Debug", "Release" }
    architecture "x86"
    platforms "Win32"
    language "C++"
    cppdialect "C++20"
    characterset "MBCS"
    staticruntime "On"
    location "build"
    targetdir "build/bin"

    toolset "msc"
    buildoptions { "/utf-8", "/FS" }

    links {
        "d3d9",
        "Pdh",
        "urlmon"
    }

    defines {
        "IS_PLATFORM_WIN",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NON_CONFORMING_SWPRINTFS",
        "_GTA_",
        "RW"
    }

    includedirs {
        "include/",
        "src/",
    }

function createPayloadProject(projectID)
    upperID = string.upper(projectID)
    pathExt = ""
    if (projectID ~= "sa") then
        pathExt = "_" .. projectID
    end

    project ("XMenuPayload" .. upperID)
        kind "SharedLib"
        targetname ("XMenu" .. upperID)
        targetextension ".dll"
        targetdir "build/bin/XMenu"

        includedirs {
            PSDK_DIR .. "/plugin_" .. projectID .. "/",
            PSDK_DIR .. "/plugin_" .. projectID .. "/game_" .. projectID .. "/",
            PSDK_DIR .. "/plugin_" .. projectID .. "/game_" .. projectID .. "/enums/",
            PSDK_DIR .. "/plugin_" .. projectID .. "/game_" .. projectID .. "/rw/",
            PSDK_DIR .. "/shared/",
            PSDK_DIR .. "/shared/game/"
        }

        libdirs {
            PSDK_DIR .. "/output/lib",
            PSDK_DIR .. "/shared/dxsdk",
        }

        files {
            "src/**.h",
            "src/**.hpp",
            "src/**.c",
            "src/**.cpp",
            "include/**.h",
            "include/**.cpp",
            "include/**.c"
        }

        files {
            "src/data/sa/maps.json",
            "src/data/sa/weapons.json",
            "src/data/sa/vehicles.json",
            "src/data/sa/peds.json",
            "src/data/vc/maps.json",
            "src/data/vc/weapons.json",
            "src/data/vc/vehicles.json",
            "src/data/vc/peds.json",
            "src/data/iii/maps.json",
            "src/data/iii/weapons.json",
            "src/data/iii/vehicles.json",
            "src/data/iii/peds.json"
        }

        removefiles {
            "src/loader/**.h",
            "src/loader/**.cpp",
            "src/core/**.h",
            "src/core/**.cpp"
        }

        if (upperID ~= "SA") then
            removefiles {
                "src/**_sa.c",
                "src/**_sa.hpp",
                "src/**_sa.cpp"
            }
        end
        if (upperID ~= "VC") then
            removefiles {
                "src/**_vc.c",
                "src/**_vc.hpp",
                "src/**_vc.cpp"
            }
        end
        if (upperID ~= "III") then
            removefiles {
                "src/**_iii.c",
                "src/**_iii.hpp",
                "src/**_iii.cpp"
            }
        end

        if upperID == "III" then
            upperID = "3"
        end
        defines {
            "GTA" .. upperID,
        }

        filter "configurations:Debug"
            symbols "On"
            defines { "DEBUG" }
            links {
                "plugin" .. pathExt .. "_d.lib"
            }

        filter "configurations:Release"
            optimize "On"
            defines { "NDEBUG" }
            links {
                "plugin" .. pathExt .. ".lib"
            }
end

function createUnifiedProject()
    project "XMenu"
        kind "SharedLib"
        targetname "XMenu"
        targetextension ".asi"

        files {
            "src/loader/**.h",
            "src/loader/**.cpp"
        }

        includedirs { 
            "src/loader/"
        }

        filter "configurations:Debug"
            symbols "On"
            defines { "DEBUG" }

        filter "configurations:Release"
            optimize "On"
            defines { "NDEBUG" }
end

function createInstallerProject()
    project "XMenuInstaller"
        kind "WindowedApp"
        targetname "XMenuInstaller"
        targetextension ".exe"

        files {
            "installer/**.h",
            "installer/**.cpp",
            "installer/**.rc"
        }

        links {
            "urlmon",
            "shell32",
            "ole32"
        }

        filter "configurations:Debug"
            symbols "On"
            defines { "DEBUG" }

        filter "configurations:Release"
            optimize "On"
            defines { "NDEBUG" }
end

createPayloadProject("sa")
createPayloadProject("vc")
createPayloadProject("iii")
createUnifiedProject()
createInstallerProject()
