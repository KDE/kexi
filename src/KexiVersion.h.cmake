/* This file is part of the KDE project
   Copyright (c) 2003-2015 Kexi Team <kexi@kde.org>

   Version information based on calligraversion.h,
   Copyright (c) 2003 David Faure <faure@kde.org>
   Copyright (c) 2003 Lukas Tinkl <lukas@kde.org>
   Copyright (c) 2004 Nicolas Goutte <goutte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIVERSION_H
#define KEXIVERSION_H

#include "kexicore_export.h"

// -- WARNING: do not edit values below, instead edit KEXI_* constants in /CMakeLists.txt --

class QString;

#define KEXI_APP_NAME "Kexi"

/**
* @def KEXI_VERSION_STRING
* @ingroup KexiMacros
* @brief Version of Kexi as string, at compile time
*
* This macro contains the Kexi version in string form. As it is a macro,
* it contains the version at compile time. See Kexi::versionString() if you need
* a version used at runtime.
*
* @note The version string might contain spaces and special characters,
* especially for development versions of Kexi.
* If you use that macro directly for a file format (e.g. OASIS Open Document)
* or for a protocol (e.g. http) be careful that it is appropriate.
* (Fictional) example: "3.0 Alpha"
*/
#define KEXI_VERSION_STRING "@KEXI_VERSION_STRING@"

/**
 * @def KEXI_STABLE_VERSION_MAJOR
 * @ingroup KexiMacros
 * @brief Major version of stable Kexi, at compile time
 * KEXI_VERSION_MAJOR is computed based on this value.
*/
#define KEXI_STABLE_VERSION_MAJOR @KEXI_STABLE_VERSION_MAJOR@

/**
 * @def KEXI_VERSION_MAJOR
 * @ingroup KexiMacros
 * @brief Major version of Kexi, at compile time
 *
 * Generally it's the same as KEXI_STABLE_VERSION_MAJOR but for unstable x.0
 * x is decreased by one, e.g. 3.0 Beta is 2.99.
*/
#if !defined KEXI_STABLE && @KEXI_STABLE_VERSION_MINOR@ == 0
# define KEXI_VERSION_MAJOR (KEXI_STABLE_VERSION_MAJOR - 1)
#else
# define KEXI_VERSION_MAJOR KEXI_STABLE_VERSION_MAJOR
#endif

/**
 * @def KEXI_STABLE_VERSION_MINOR
 * @ingroup KexiMacros
 * @brief Minor version of stable Kexi, at compile time
 * KEXI_VERSION_MINOR is computed based on this value.
 */
#define KEXI_STABLE_VERSION_MINOR @KEXI_STABLE_VERSION_MINOR@

/**
 * @def KEXI_VERSION_MINOR
 * @ingroup KexiMacros
 * @brief Minor version of Kexi, at compile time
 *
 * Generally it's equal to KEXI_STABLE_VERSION_MINOR for stable releases,
 * equal to 99 for x.0 unstable releases (e.g. it's 3.0 Beta has minor version 99),
 * and equal to KEXI_STABLE_VERSION_MINOR-1 for unstable releases other than x.0.
 */
#ifdef KEXI_STABLE
# define KEXI_VERSION_MINOR KEXI_STABLE_VERSION_MINOR
#elif KEXI_STABLE_VERSION_MINOR == 0
# define KEXI_VERSION_MINOR 99
#else
# define KEXI_VERSION_MINOR (KEXI_STABLE_VERSION_MINOR - 1)
#endif

/**
 * @def KEXI_VERSION_RELEASE
 * @ingroup KexiMacros
 * @brief Release version of Kexi, at compile time.
 * 89 for Alpha.
 */
#define KEXI_VERSION_RELEASE @KEXI_VERSION_RELEASE@

/**
 * @def KEXI_STABLE_VERSION_RELEASE
 * @ingroup KexiMacros
 * @brief Release version of Kexi, at compile time.
 *
 * Equal to KEXI_VERSION_RELEASE for stable releases and 0 for unstable ones.
 */
#ifdef KEXI_STABLE
# define KEXI_STABLE_VERSION_RELEASE 0
#else
# define KEXI_STABLE_VERSION_RELEASE @KEXI_VERSION_RELEASE@
#endif

/**
 * @def KEXI_ALPHA
 * @ingroup KexiMacros
 * @brief If defined (1..9), indicates at compile time that Kexi is in alpha stage
 */
#cmakedefine KEXI_ALPHA @KEXI_ALPHA@

/**
 * @def KEXI_BETA
 * @ingroup KexiMacros
 * @brief If defined (1..9), indicates at compile time that Kexi is in beta stage
 */
#cmakedefine KEXI_BETA @KEXI_BETA@

/**
 * @def KEXI_RC
 * @ingroup KexiMacros
 * @brief If defined (1..9), indicates at compile time that Kexi is in "release candidate" stage
 */
#cmakedefine KEXI_RC @KEXI_RC@

/**
 * @def KEXI_STABLE
 * @ingroup KexiMacros
 * @brief If defined, indicates at compile time that Kexi is in stable stage
 */
#cmakedefine KEXI_STABLE @KEXI_STABLE@

/**
 * @ingroup KexiMacros
 * @brief Make a number from the major, minor and release number of a Kexi version
 *
 * This function can be used for preprocessing when KEXI_IS_VERSION is not
 * appropriate.
 */
#define KEXI_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

/**
 * @ingroup KexiMacros
 * @brief Version of Kexi as number, at compile time
 *
 * This macro contains the Kexi version in number form. As it is a macro,
 * it contains the version at compile time. See version() if you need
 * the Kexi version used at runtime.
 */
#define KEXI_VERSION \
    KEXI_MAKE_VERSION(KEXI_VERSION_MAJOR,KEXI_VERSION_MINOR,KEXI_VERSION_RELEASE)

/**
 * @ingroup KexiMacros
 * @brief Check if the Kexi version matches a certain version or is higher
 *
 * This macro is typically used to compile conditionally a part of code:
 * @code
 * #if KEXI_IS_VERSION(2,3,0)
 * // Code for Kexi 2.3.0
 * #else
 * // Code for older Kexi
 * #endif
 * @endcode
 *
 * @warning Especially during development phases of Kexi, be careful
 * when choosing the version number that you are checking against.
 * Otherwise you might risk to break the next Kexi release.
 * Therefore be careful that development version have a
 * version number lower than the released version, so do not check
 * e.g. for Kexi 4.3 with KEXI_IS_VERSION(4,3,0)
 * but with the actual version number at a time a needed feature was introduced, e.g. 4.3.2.
 */
#define KEXI_IS_VERSION(a,b,c) ( KEXI_VERSION >= KEXI_MAKE_VERSION(a,b,c) )

/**
 * @def KEXI_YEAR
 * @ingroup KexiMacros
 * @brief Year of the Kexi release, set at compile time
 *
 * This macro is used in "About application" dialog for strings such as "© 2012-..., The Author Team".
*/
#define KEXI_YEAR "@KEXI_YEAR@"

/**
 * @def KEXI_GIT_SHA1_STRING
 * @ingroup CalligraMacros
 * @brief Indicates the git sha1 commit which was used for compilation of Calligra
 */
#cmakedefine KEXI_GIT_SHA1_STRING "@KEXI_SHA1_STRING@"

/**
 * @def KEXI_GIT_BRANCH_STRING
 * @ingroup CalligraMacros
 * @brief Indicates the git branch name which was used for compilation of Calligra
 */
#cmakedefine KEXI_GIT_BRANCH_STRING "@KEXI_GIT_BRANCH_STRING@"

/**
 * Namespace for general Kexi functions.
 */
namespace Kexi
{
/**
 * Returns the encoded number of Kexi's version, see the KEXI_VERSION macro.
 * In contrary to that macro this function returns the number of the actually
 * installed Kexi version, not the number of the Kexi version that was
 * installed when the program was compiled.
 * @return the version number, encoded in a single int
 */
KEXICORE_EXPORT unsigned int version();
/**
 * Returns the major number of Kexi's version, e.g.
 * 1 for Kexi 1.2.3.
 * @return the major version number
 */
KEXICORE_EXPORT unsigned int versionMajor();
/**
 * Returns the minor number of Kexi's version, e.g.
 * 2 for Kexi 1.2.3.
 * @return the minor version number
 */
KEXICORE_EXPORT unsigned int versionMinor();
/**
 * Returns the release of Kexi's version, e.g.
 * 3 for Kexi 1.2.3.
 * @return the release number
 */
KEXICORE_EXPORT unsigned int versionRelease();
/**
 * Returns the Kexi version as string, e.g. "1.2.3"
 * Sometimes it may be even something like "1.2.3 beta 2"
 * @return the Kexi version. You can keep the string forever
 */
KEXICORE_EXPORT const char *versionString();
/**
 * @return the Kexi version string (versionString()) but appends extra information
 * such as "(git 4e06281 master)" if available.
 */
KEXICORE_EXPORT const char *fullVersionString();
/**
 * Returns the encoded number of stable Kexi's version.
 * For 2.3.1 it returns 2.3.1, for 2.5.70 returns 2.6.0, for 2.9.70 returns 3.0.0.
 * In contrary to KEXI_STABLE_VERSION macro this function returns the number
 * of the actually installed Kexi version, not the number of the Kexi version that was
 * installed when the program was compiled.
 * @return the version number, encoded in a single int
 * @see Kexi::version()
 * @see KEXI_STABLE_VERSION
 */
KEXICORE_EXPORT unsigned int stableVersion();
/**
 * Returns the major number of stable Kexi's version, e.g.
 * 1 for Kexi 1.2.3.
 * @return the major stable version number
 */
KEXICORE_EXPORT unsigned int stableVersionMajor();
/**
 * Returns the minor number of stable Kexi's version, e.g.
 * 2 for Kexi 1.2.3.
 * @return the minor stable version number
 */
KEXICORE_EXPORT unsigned int stableVersionMinor();
/**
 * Returns the release of stable Kexi's version, e.g.
 * 3 for Kexi 1.2.3.
 * @return the release stable version number
 */
KEXICORE_EXPORT unsigned int stableVersionRelease();
/**
 * Returns the stable Kexi version as string, e.g. "1.2.3"
 * It never contains alpha, beta or rc part.
 * @return the stable Kexi version.
 */
KEXICORE_EXPORT QString stableVersionString();
}

/*
 * This is the version a part has to be only increase it if the
 * interface isn't binary compatible anymore.
 *
 * Note: update X-Kexi-PartVersion values in kexi*handler.desktop
 * files every time you are increasing this value.
 */

#define KEXI_PART_VERSION 3

#endif
