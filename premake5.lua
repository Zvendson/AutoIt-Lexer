workspace "Lexer"
    system ("windows")
    architecture ("x64")
    characterset ("MBCS")

    configurations
    {
        "Debug",
        "Release"
    }

    flags
    {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}"
target_dir = "%{wks.location.name}"

project "AutoIt"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"
        staticruntime "off"

        targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

        files
        {
                "src/**.h",
                "src/**.cpp"
        }

        includedirs
        {
                "src"
        }

        filter "configurations:Debug"
                runtime "Debug"
                symbols "on"

        filter "configurations:Release"
                runtime "Release"
                optimize "on"
                
        filter ""