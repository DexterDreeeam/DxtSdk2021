/* ********* * ********* * ********* * ********* * *********
 *
 * Environment/Interface.h
 *
 * proj: Environment
 * desc: Project Interface
 * autr: dexterdreeeam
 * date: 20211001
 * tips:
 *
 ********* * ********* * ********* * ********* * ********* */

#pragma once

#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wformat="
#pragma GCC diagnostic ignored "-Wformat-security"
#pragma GCC diagnostic ignored "-Wnonnull"
#pragma GCC diagnostic ignored "-Woverflow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-variable"


#include "Definition.hpp"
#include "PlatformHeader.hpp"

#include "BuildinDefinition/ComplierDirective.hpp"
#include "BuildinDefinition/BuildinType.hpp"

#include "PlatformOperation/Assert.hpp"
#include "PlatformOperation/Atomic.hpp"
#include "PlatformOperation/CString.hpp"
#include "PlatformOperation/CStringConversion.hpp"
#include "PlatformOperation/Date.hpp"
#include "PlatformOperation/Event.hpp"
#include "PlatformOperation/File.hpp"
#include "PlatformOperation/Lock.hpp"
#include "PlatformOperation/Math.hpp"
#include "PlatformOperation/Memory.hpp"
#include "PlatformOperation/Network.hpp"
#include "PlatformOperation/New.hpp"
#include "PlatformOperation/Print.hpp"
// #include "PlatformOperation/SIMD.hpp"
#include "PlatformOperation/Spin.hpp"
#include "PlatformOperation/Thread.hpp"
#include "PlatformOperation/Timer.hpp"
