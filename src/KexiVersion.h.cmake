/* This file is part of the KDE project
   Copyright (c) 2003-2016 KEXI Team <kexi@kde.org>

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

#include <core/kexicore_export.h>

// -- WARNING: do not edit values below, instead edit constants in SetKexiVersionInfo.cmake

class QString;

/**
* @def KEXI_VERSION_STRING
* @ingroup KexiMacros
* @brief Version of KEXI as string, at compile time
*
* This macro contains the KEXI version in string form. As it is a macro,
* it contains the version at compile time. See Kexi::versionString() if you need
* a version used at runtime.
*
* @note The version string might contain spaces and special characters,
* especially for development versions of KEXI.
* If you use that macro directly for a file format (e.g. OASIS Open Document)
* or for a protocol (e.g. http) be careful that it is appropriate.
* (Fictional) example: "3.0 Alpha"
*/
#define KEXI_VERSION_STRING "@PROJECT_VERSION_STRING@"

/**
* @def KEXI_VERSION_MAJOR_MINOR_RELEASE
* @ingroup KexiMacros
* @brief Version string containing "major.minor.release"
* @brief Version of KEXI as string, at compile time
*
* This macro contains the KEXI version in string form. As it is a macro,
* it contains the version at compile time.
*
* @note The version string never contains spaces or special characters.
*/
#define KEXI_VERSION_MAJOR_MINOR_RELEASE "@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_RELEASE@"

/**
 * @def KEXI_STABLE_VERSION_MAJOR
 * @ingroup KexiMacros
 * @brief Major version of stable KEXI, at compile time
 * KEXI_VERSION_MAJOR is computed based on this value.
*/
#define KEXI_STABLE_VERSION_MAJOR @PROJECT_STABLE_VERSION_MAJOR@

/**
 * @def KEXI_VERSION_MAJOR
 * @ingroup KexiMacros
 * @brief Major version of KEXI, at compile time
 *
 * Generally it's the same as KEXI_STABLE_VERSION_MAJOR but for unstable x.0
 * x is decreased by one, e.g. 3.0 Beta 1 is 2.91.
*/
#define KEXI_VERSION_MAJOR @PROJECT_VERSION_MAJOR@

/**
 * @def KEXI_STABLE_VERSION_MINOR
 * @ingroup KexiMacros
 * @brief Minor version of stable KEXI, at compile time
 * KEXI_VERSION_MINOR is computed based on this value.
 */
#define KEXI_STABLE_VERSION_MINOR @PROJECT_STABLE_VERSION_MINOR@

/**
 * @def KEXI_VERSION_MINOR
 * @ingroup KexiMacros
 * @brief Minor version of KEXI, at compile time
 *
 * Generally it's equal to KEXI_STABLE_VERSION_MINOR for stable releases,
 * equal to 9x for x.0 unstable releases (e.g. 3.0 Beta 1 has minor version 91).
 */
#define KEXI_VERSION_MINOR @PROJECT_VERSION_MINOR@

/**
 * @def KEXI_VERSION_RELEASE
 * @ingroup KexiMacros
 * @brief Release version of Kexi, at compile time.
 * 90 for Alpha.
 */
#define KEXI_VERSION_RELEASE @PROJECT_VERSION_RELEASE@

/**
 * @def KEXI_STABLE_VERSION_RELEASE
 * @ingroup KexiMacros
 * @brief Release version of KEXI, at compile time.
 *
 * Equal to KEXI_VERSION_RELEASE for stable releases and 0 for unstable ones.
 */
#define KEXI_STABLE_VERSION_RELEASE @PROJECT_STABLE_VERSION_RELEASE@

/**
 * @def KEXI_ALPHA
 * @ingroup KexiMacros
 * @brief If defined (1), indicates at compile time that Kexi is in alpha stage
 */
#cmakedefine KEXI_ALPHA @KEXI_ALPHA@

/**
 * @def KEXI_BETA
 * @ingroup KexiMacros
 * @brief If defined (1, 2..), indicates at compile time that Kexi is in beta stage
 */
#cmakedefine KEXI_BETA @KEXI_BETA@

/**
 * @def KEXI_RC
 * @ingroup KexiMacros
 * @brief If defined (1, 2..), indicates at compile time that Kexi is in "release candidate" stage
 */
#cmakedefine KEXI_RC @KEXI_RC@

/**
 * @def KEXI_STABLE
 * @ingroup KexiMacros
 * @brief If defined, indicates at compile time that KEXI is in stable stage
 */
#cmakedefine KEXI_STABLE @KEXI_STABLE@

/**
 * @def KEXI_STAGE
 * @ingroup KexiMacros
 * @brief Release stage (number) for Kexi, e.g. 1 for Beta 1, 2 for 3.3.2, etc.
 */
#define KEXI_STAGE @KEXI_STAGE@

/**
 * @ingroup KexiMacros
 * @brief Make a number from the major, minor and release number of a KEXI version
 *
 * This function can be used for preprocessing when KEXI_IS_VERSION is not
 * appropriate.
 */
#define KEXI_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

/**
 * @ingroup KexiMacros
 * @brief Version of KEXI as number, at compile time
 *
 * This macro contains the KEXI version in number form. As it is a macro,
 * it contains the version at compile time. See version() if you need
 * the KEXI version used at runtime.
 */
#define KEXI_VERSION \
    KEXI_MAKE_VERSION(KEXI_VERSION_MAJOR,KEXI_VERSION_MINOR,KEXI_VERSION_RELEASE)

/**
 * @ingroup KexiMacros
 * @brief Check if the KEXI version matches a certain version or is higher
 *
 * This macro is typically used to compile conditionally a part of code:
 * @code
 * #if KEXI_IS_VERSION(2,3,0)
 * // Code for KEXI 2.3.0
 * #else
 * // Code for older KEXI
 * #endif
 * @endcode
 *
 * @warning Especially during development phases of KEXI, be careful
 * when choosing the version number that you are checking against.
 * Otherwise you might risk to break the next KEXI release.
 * Therefore be careful that development version have a
 * version number lower than the released version, so do not check
 * e.g. for KEXI 4.3 with KEXI_IS_VERSION(4,3,0)
 * but with the actual version number at a time a needed feature was introduced, e.g. 4.3.2.
 */
#define KEXI_IS_VERSION(a,b,c) ( KEXI_VERSION >= KEXI_MAKE_VERSION(a,b,c) )

/**
 * @def KEXI_YEAR
 * @ingroup KexiMacros
 * @brief Year of the KEXI release, set at compile time
 *
 * This macro is used in "About application" dialog for strings such as "© 2012-..., The Author Team".
*/
#cmakedefine KEXI_YEAR "@KEXI_YEAR@"

/**
 * @def KEXI_GIT_SHA1_STRING
 * @ingroup KexiMacros
 * @brief Indicates the git sha1 commit which was used for compilation of KEXI
 */
#cmakedefine KEXI_GIT_SHA1_STRING "@KEXI_GIT_SHA1_STRING@"

/**
 * @def KEXI_GIT_BRANCH_STRING
 * @ingroup KexiMacros
 * @brief Indicates the git branch name which was used for compilation of KEXI
 */
#cmakedefine KEXI_GIT_BRANCH_STRING "@KEXI_GIT_BRANCH_STRING@"

/**
 * @def KEXI_DISTRIBUTION_VERSION
 * @ingroup KexiMacros
 * @brief Name of KEXI version useful to construct co-installabile releases
 * By default is it equal to KEXI_STABLE_VERSION_MAJOR.KEXI_STABLE_VERSION_MINOR.
 * It can be changed at configure stage by setting the KEXI_CUSTOM_DISTRIBUTION_VERSION
 * CMake variable.
 * @see KEXI_BASE_PATH
 */
#cmakedefine KEXI_DISTRIBUTION_VERSION "@KEXI_DISTRIBUTION_VERSION@"

/**
 * @def KEXI_BASE_PATH
 * @ingroup KexiMacros
 * @brief Relative path name useful to construct co-installabile file names and paths
 * It is equal to "kexi/N" where N is KEXI_DISTRIBUTION_VERSION.
 */
#cmakedefine KEXI_BASE_PATH "@KEXI_BASE_PATH@"

/**
 * Namespace for general KEXI functions.
 */
namespace Kexi
{
/**
 * Returns the encoded number of KEXI version, see the KEXI_VERSION macro.
 * In contrary to that macro this function returns the number of the actually
 * installed KEXI version, not the number of the KEXI version that was
 * installed when the program was compiled.
 * @return the version number, encoded in a single int
 */
KEXICORE_EXPORT unsigned int version();
/**
 * Returns the major number of KEXI version, e.g.
 * 1 for KEXI 1.2.3.
 * @return the major version number
 */
KEXICORE_EXPORT unsigned int versionMajor();
/**
 * Returns the minor number of KEXI version, e.g.
 * 2 for KEXI 1.2.3.
 * @return the minor version number
 */
KEXICORE_EXPORT unsigned int versionMinor();
/**
 * Returns the release of KEXI version, e.g.
 * 3 for KEXI 1.2.3.
 * @return the release number
 */
KEXICORE_EXPORT unsigned int versionRelease();
/**
 * Returns the KEXI version as string, e.g. "1.2.3"
 * Sometimes it may be even something like "1.2.3 beta 2"
 * @return the KEXI version. You can keep the string forever
 */
KEXICORE_EXPORT const char *versionString();
/**
 * @return the KEXI version string (versionString()) but appends extra information
 * such as "(git 4e06281 master)" if available.
 */
KEXICORE_EXPORT const char *fullVersionString();
/**
 * Returns the encoded number of stable KEXI version.
 * For 2.3.1 it returns 2.3.1, for 2.5.70 returns 2.6.0, for 2.9.70 returns 3.0.0.
 * In contrary to KEXI_STABLE_VERSION macro this function returns the number
 * of the actually installed KEXI version, not the number of the KEXI version that was
 * installed when the program was compiled.
 * @return the version number, encoded in a single int
 * @see Kexi::version()
 * @see KEXI_STABLE_VERSION
 */
KEXICORE_EXPORT unsigned int stableVersion();
/**
 * Returns the major number of stable KEXI version, e.g.
 * 1 for KEXI 1.2.3.
 * @return the major stable version number
 */
KEXICORE_EXPORT unsigned int stableVersionMajor();
/**
 * Returns the minor number of stable KEXI version, e.g.
 * 2 for KEXI 1.2.3.
 * @return the minor stable version number
 */
KEXICORE_EXPORT unsigned int stableVersionMinor();
/**
 * Returns the release of stable KEXI version, e.g.
 * 3 for KEXI 1.2.3.
 * @return the release stable version number
 */
KEXICORE_EXPORT unsigned int stableVersionRelease();
/**
 * Returns the stable KEXI version as string, e.g. "1.2.3"
 * It never contains alpha, beta or rc part.
 * @return the stable KEXI version.
 */
KEXICORE_EXPORT QString stableVersionString();
}

#endif
