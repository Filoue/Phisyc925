if(NOT DEFINED ENV{DEVKITPRO} OR "$ENV{DEVKITPRO}" STREQUAL "")
    message(FATAL_ERROR
        "DEVKITPRO is not set. Install devkitPro and set DEVKITPRO to its "
        "installation directory (for example, /opt/devkitpro).")
endif()

set(GPR925_NDS_TOOLCHAIN "$ENV{DEVKITPRO}/cmake/NDS.cmake")
if(NOT EXISTS "${GPR925_NDS_TOOLCHAIN}")
    message(FATAL_ERROR
        "Could not find ${GPR925_NDS_TOOLCHAIN}. Check that DEVKITPRO points "
        "to a devkitPro installation containing cmake/NDS.cmake.")
endif()

include("${GPR925_NDS_TOOLCHAIN}")
