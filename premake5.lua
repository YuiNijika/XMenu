-- Premake Project Generator for XMenu

local XBASE_LIB_DIR = "lib"

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

    defines {
        "IS_PLATFORM_WIN",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NON_CONFORMING_SWPRINTFS"
    }

function configureBuildMode()
    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }

    filter "configurations:Release"
        optimize "On"
        defines { "NDEBUG" }

    filter {}
end

function createPayloadProject(projectID)
    local upperID = string.upper(projectID)
    local gameDefine = upperID == "III" and "GTA3" or "GTA" .. upperID

    project ("XMenuPayload" .. upperID)
        kind "SharedLib"
        targetname ("XMenu" .. upperID)
        targetextension ".dll"
        targetdir "build/bin/XMenu"

        includedirs {
            "include",
            "src"
        }

        files {
            "src/**.h",
            "src/**.hpp",
            "src/**.c",
            "src/**.cpp",
            "src/**.rc",
            "include/XBase/**.h"
        }

        if upperID ~= "SA" then
            removefiles { "src/**_sa.c", "src/**_sa.hpp", "src/**_sa.cpp" }
        end
        if upperID ~= "VC" then
            removefiles { "src/**_vc.c", "src/**_vc.hpp", "src/**_vc.cpp" }
        end
        if upperID ~= "III" then
            removefiles { "src/**_iii.c", "src/**_iii.hpp", "src/**_iii.cpp" }
        end

        defines { gameDefine }
        libdirs { XBASE_LIB_DIR }

        if projectID == "sa" then
            links { "XBaseSA", "PluginSA" }
        elseif projectID == "vc" then
            links { "XBaseVC", "PluginVC" }
        else
            links { "XBaseIII", "PluginIII" }
        end

        links { "XBasePayloadEntry" }
        linkoptions { "/WHOLEARCHIVE:XBasePayloadEntry.lib" }

        configureBuildMode()
end

function createLoaderProject()
    project "XMenu"
        kind "SharedLib"
        targetname "XMenu"
        targetextension ".asi"

        libdirs { XBASE_LIB_DIR }
        links { "XBaseBootstrap" }
        linkoptions { "/WHOLEARCHIVE:XBaseBootstrap.lib" }

        configureBuildMode()
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

        -- 仅由 main.cpp #include，禁止作为独立 TU 编译。
        removefiles { "installer/_ui_rewrite_tail.cpp" }

        links {
            "urlmon",
            "shell32",
            "ole32"
        }

        configureBuildMode()
end

createPayloadProject("sa")
createPayloadProject("vc")
createPayloadProject("iii")
createLoaderProject()
createInstallerProject()