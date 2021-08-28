// Copyright 2018-2020 - Roberto De Ioris

#pragma once
#include "Runtime/Launch/Resources/Version.h"

//Old-Style header guard incase multiple plugins have implementations of this header
//and are in shared space.
#ifndef UE_VERSION_INFO_MACROS
#define UE_VERSION_INFO_MACROS

//Will pass if we are 4.xx or 5+
#define UE4_AT_LEAST(x) (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= x) || ENGINE_MAJOR_VERSION > 4
//Will pass if we are < 4.xx
#define UE4_LESS_THAN(x) (ENGINE_MAJOR_VERSION < 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < x))

//Will pass if we are 5.xx or 6+
#define UE5_AT_LEAST(x) (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= x) || ENGINE_MAJOR_VERSION > 5
//Will pass if we are < 5.xx
#define UE5_LESS_THAN(x) (ENGINE_MAJOR_VERSION < 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < x))

#endif //UE_VERSION_INFO_MACROS