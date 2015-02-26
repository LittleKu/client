/****************************************
 * class	: CConnectionPool
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#ifndef __CONNECTION_POOL_H__
#define __CONNECTION_POOL_H__

#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdlib>
#include <deque>
#include <list>
#include <iostream>

#include "Connection.h"
#include "Message.h"

namespace client
{
	class CClientImpl;

	typedef boost::function3<void, const boost::system::error_code &, StatusCode, CMessage::Ptr> cb_InitConnection;
	typedef boost::function2<void, const boost::system::error_code &, CConnection::Ptr> cb_addConnection;

	class CConnectionPool
		: public boost::enable_shared_from_this<CConnectionPool>,
		private boost::noncopyable
	{
		friend class CClientImpl;
	public:
		//typedef boost::function2<void, const boost::system::error_code &, CMessage::Ptr> cb_InitConnection;
		
	public:
		CConnectionPool(boost::asio::io_service &io_service);
		~CConnectionPool();

		void Init(const std::string &host, const std::string &port, cb_InitConnection cb, int connection_count, int connection_limit);
		void Stop();
		void GetConnection(cb_addConnection cb);
	private:
		void QueueConnection(const boost::system::error_code &err, StatusCode sc, CMessage::Ptr msg, cb_InitConnection cb, CConnection::Ptr connection);
		void NewConnection(cb_InitConnection cb);
		void CheckAvaliableConnection(const boost::system::error_code &err);
		void TimeoutNewConnection(const boost::system::error_code &err, cb_InitConnection cb);
	private:
		boost::mutex m_Mutex;
		std::string m_Host;
		std::string m_Port;

		bool m_bIsStop;
		boost::asio::io_service &m_Io_Service;

		std::list<CConnection::Ptr> m_ListNew;
		std::list<CConnection::Ptr> m_ListValid;
		std::list<CConnection::Ptr> m_ListRun;
		std::deque<cb_addConnection> m_DequeRequest;

		int m_nConnectionCount;
		int m_nConnectionLimit;

		boost::asio::deadline_timer m_TryTimer;
		boost::asio::deadline_timer m_TryTimerConnect;
		int m_TryConnect;
		int m_TimeoutRequest;
		int m_TimeoutConnect;
	};
}

#endif