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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

#include "common/message_receive_buffer.h"
#include "common/multi_buffer_writer.h"
#include "common/network_address.h"
#include "protocol/packet.h"
#include "common/time_utils.h"

class ModuleMock {
public:
	ModuleMock();
	virtual ~ModuleMock();

	virtual void onNewConnection() {}
	virtual void onIncomingMessage(
			PacketHeader::Type /*type*/,
			const std::vector<uint8_t>& /*message*/) {}
	virtual void onConnectionEnd() {}

	/*
	 * Waits until at least one packet is received since the last call to waitForPacketReceived
	 */
	template<typename Rep, typename Period>
	bool waitForPacketReceived(std::chrono::duration<Rep, Period> timeout);

	/*
	 * Waits until at least n packets are received since the last call to waitForPacketReceived
	 */
	template<typename Rep, typename Period>
	bool waitForPacketsReceived(int n, std::chrono::duration<Rep, Period> timeout);

	/*
	 * Waits until there is signal() called within the mock since the last call to waitForSignal
	 */
	template<typename Rep, typename Period>
	bool waitForSignal(std::chrono::duration<Rep, Period> timeout);

	/*
	 * Starts a thread which manages the network communication with this mock
	 */
	void init();

	/*
	 * Port which has to be used to connect to this mock
	 */
	uint16_t port() const;

	/*
	 * Address which has to be used to connect to this mock
	 */
	NetworkAddress address() const;

	/*
	 * Main loop of the thread which manages the network communication with this mock
	 */
	void operator()();

protected:
	int currentClient() const { return currentClientFd_; }
	void respondToCurrentClient(std::vector<uint8_t> message);
	void disconnectCurrentClient();
	void signal();

private:
	struct ClientRecord {
		std::queue<std::vector<uint8_t>> sendQueue;
		MessageReceiveBuffer receiver;
		MultiBufferWriter writer;

		ClientRecord() : receiver(1024 * 1024 * 10) {}
	};

	// Function used in operator()
	void serveFd(int fd, int flags);

	// listen socket and a thread which manages the network communication with this mock
	int sock_;
	std::thread thread_;
	std::atomic<bool> terminate_;

	// connected clients
	std::map<int, ClientRecord> clients_;
	int currentClientFd_;

	// members used for waiting for received packets
	mutable std::mutex mutex_;
	mutable std::condition_variable cond_;
	int receivedPackets_;
	int signals_;
};

template<typename Rep, typename Period>
bool ModuleMock::waitForPacketReceived(std::chrono::duration<Rep, Period> timeout) {
	Timeout wholeTimeout(timeout);
	std::unique_lock<std::mutex> lock(mutex_);
	while (receivedPackets_ < 1) {
		auto status = cond_.wait_for(lock, wholeTimeout.remainingTime());
		if (status == std::cv_status::timeout || wholeTimeout.expired()) {
			return false;
		}
	}
	--receivedPackets_;
	return true;
}

template<typename Rep, typename Period>
bool ModuleMock::waitForPacketsReceived(int n, std::chrono::duration<Rep, Period> timeout) {
	Timeout wholeTimeout(timeout);
	for (int i = 0; i < n; ++i) {
		if (waitForPacketReceived(wholeTimeout.remainingTime()) == false) {
			return false;
		}
	}
	return true;
}

template<typename Rep, typename Period>
bool ModuleMock::waitForSignal(std::chrono::duration<Rep, Period> timeout) {
	Timeout wholeTimeout(timeout);
	std::unique_lock<std::mutex> lock(mutex_);
	while (signals_ < 1) {
		auto status = cond_.wait_for(lock, wholeTimeout.remainingTime());
		if (status == std::cv_status::timeout || wholeTimeout.expired()) {
			return false;
		}
	}
	--signals_;
	return true;
}
