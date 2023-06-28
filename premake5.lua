workspace "Grounded"
    architecture "x64"
    configurations
    {
        "Debug",
        "Release",
    }

    kind "ConsoleApp"
    language "C"
    cdialect "C11"
    staticruntime "on"
    objdir "obj/%{cfg.buildcfg}" -- Check if we can use cfg.buildcfg in lowercase
    targetdir "bin/%{cfg.buildcfg}"
    characterset "Unicode"

    includedirs
    {
        "include",
    }

    filter "system:linux"
        buildoptions
        {
            "-Wall",
            "-Werror",
            "-Wno-unused-variable",
        	"-Wno-unused-function",
        }
        defines
        {
            "_GNU_SOURCE",
        }
    
    filter "system:window"
        -- TODO: Check if we should enable stuff like Werror/Wall for windows
        defines
        { 
            "_CRT_SECURE_NO_WARNING",
        }
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        symbols "off"
    

project "SimpleWindow"
    files
    {
        "example/simple_window/main.c",
        "src/threading/grounded_threading.c",
        "src/memory/grounded_arena.c",
        "src/memory/grounded_memory.c",
        "src/window/grounded_window.c",
        "src/window/grounded_window_extra.c",
        "src/logger/grounded_logger.c",
        "src/string/grounded_string.c",
    }
