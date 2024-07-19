/*
   Copyright 2013-2014 EditShare
   Copyright 2013-2015 Skytechnology sp. z o.o.
   Copyright 2023      Leil Storage OÜ

   This file is part of SaunaFS.

   SaunaFS is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3.

   SaunaFS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with SaunaFS. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/platform.h"

#include <cstdint>
#include <string>
#include <memory>

#include "common/exception.h"
#include "common/lockfile.h"

extern const char kMetadataFilename[];
extern const char kMetadataTmpFilename[];
extern const char kMetadataLegacyFilename[];
extern const char kMetadataMlFilename[];
extern const char kMetadataMlTmpFilename[];
extern const char kMetadataEmergencyFilename[];
extern const char kChangelogFilename[];
extern const char kChangelogTmpFilename[];
extern const char kChangelogMlFilename[];
extern const char kChangelogMlTmpFilename[];
extern const char kSessionsFilename[];
extern const char kSessionsTmpFilename[];
extern const char kSessionsMlFilename[];
extern const char kSessionsMlTmpFilename[];

SAUNAFS_CREATE_EXCEPTION_CLASS(MetadataCheckException, Exception);

/**
 * Returns version of a metadata file.
 * Throws MetadataCheckException if the file is corrupted, ie. contains wrong header or end marker.
 * \param file -- path to the metadata binary file
 */
uint64_t metadataGetVersion(const std::string& file);

/**
 * Returns version of the first entry in a changelog.
 * Returns 0 in case of any error.
 * \param file -- path to the changelog file
 */
uint64_t changelogGetFirstLogVersion(const std::string& fname);

/**
 * Returns version of the last entry in a changelog.
 * Returns 0 in case of any error.
 * \param file -- path to the changelog file
 */
uint64_t changelogGetLastLogVersion(const std::string& fname);

/**
 * Rename changelog files from old to new version
 * from <name>.X.sfs to <name>.sfs.X
 * Used only once - after upgrade from version before 1.6.29
 * \param name -- changelog name before first dot
 */
void changelogsMigrateFrom_1_6_29(const std::string& fname);

extern std::unique_ptr<Lockfile> gMetadataLockfile;

