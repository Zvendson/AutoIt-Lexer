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

project "Lexer"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"
        staticruntime "off"

        targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

        files
        {
                "Lexer/**.h",
                "Lexer/**.cpp"
        }

        includedirs
        {
                "Lexer"
        }

        filter "configurations:Debug"
                defines "SB_DEBUG"
                runtime "Debug"
                symbols "on"

        filter "configurations:Release"
                defines "SB_RELEASE"
                runtime "Release"
                optimize "on"
                
        filter ""