/*
   Copyright 2013-2017 Skytechnology sp. z o.o.
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

#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

inline uint32_t gCleanAcquiredFilesPeriod;
inline uint32_t gCleanAcquiredFilesTimeout;

// Define the AcquiredFileLastTimeUsed struct
struct AcquiredFileLastTimeUsed {
	uint32_t inode;
	uint32_t lastTimeUsed;

	// Constructor
	AcquiredFileLastTimeUsed(uint32_t inode, uint32_t lastTimeUsed)
	    : inode(inode), lastTimeUsed(lastTimeUsed) {}
};

struct CompareByLastTimeUsed {
	bool operator()(const AcquiredFileLastTimeUsed &acquiredFile1,
	                const AcquiredFileLastTimeUsed &acquiredFile2) const {
		return acquiredFile1.lastTimeUsed < acquiredFile2.lastTimeUsed ||
		       (acquiredFile1.lastTimeUsed == acquiredFile2.lastTimeUsed &&
		        acquiredFile1.inode < acquiredFile2.inode);
	}
};

using AcquiredFilesLastTimeUsed =
    std::set<AcquiredFileLastTimeUsed, CompareByLastTimeUsed>;
using InodeToFileMap = std::map<uint32_t, AcquiredFilesLastTimeUsed::iterator>;

inline AcquiredFilesLastTimeUsed acquiredFilesLastTimeUsed;
inline InodeToFileMap inodeToFileMap;
inline std::mutex acquiredFilesLastTimeUsedMutex;

// Function to return the least value according to the custom comparator
inline AcquiredFileLastTimeUsed getLastUsedAcquiredFile() {
	if (!acquiredFilesLastTimeUsed.empty()) {
		return *acquiredFilesLastTimeUsed
		            .begin();  // The first element is the least according to
		                       // the comparator
	}
	throw std::runtime_error("Acquired Files With Last Time Used Set is empty");
}

// Function to add a new AcquiredFileLastTimeUsed
inline void addAcquiredFileLastTimeUsed(uint32_t inode, uint32_t lastTimeUsed) {
	AcquiredFileLastTimeUsed newFile(inode, lastTimeUsed);
	auto insertionResult = acquiredFilesLastTimeUsed.insert(newFile);
	auto it = insertionResult.first;
	inodeToFileMap[inode] = it;
}

// Function to remove an AcquiredFileLastTimeUsed
inline void removeAcquiredFileLastTimeUsed(uint32_t inode) {
	auto it = inodeToFileMap.find(inode);
	if (it != inodeToFileMap.end()) {
		acquiredFilesLastTimeUsed.erase(it->second);
		inodeToFileMap.erase(it);
	}
}

// Function to update the lastTimeUsed of an AcquiredFileLastTimeUsed
inline void updateLastTimeUsedAcquireFile(uint32_t inode, uint32_t newLastTimeUsed) {
	auto it = inodeToFileMap.find(inode);
	if (it != inodeToFileMap.end()) {
		AcquiredFileLastTimeUsed updatedFile = *(it->second);
		updatedFile.lastTimeUsed = newLastTimeUsed;
		acquiredFilesLastTimeUsed.erase(it->second);
		auto newIt = acquiredFilesLastTimeUsed.insert(updatedFile).first;
		inodeToFileMap[inode] = newIt;
	}
}
