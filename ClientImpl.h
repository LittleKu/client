/****************************************
 * class	: CClientImpl
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#ifndef __CLIENTIMPL_H__
#define __CLIENTIMPL_H__

#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>

#include "ConnectionPool.h"

namespace client
{
	typedef boost::function2<void, const boost::system::error_code &, CMessage::Ptr> cb_Request;

	class CClientImpl
		: public boost::enable_shared_from_this<CClientImpl>,
		private boost::noncopyable
	{
	public:
		typedef boost::shared_ptr<CClientImpl> Ptr;
	public:
		CClientImpl();
		~CClientImpl();

		void Init(const std::string &host, const std::string &port, cb_InitConnection cb, int thread_count, int connection_count, int try_count, int connection_limit);
		void Stop();
		void PostRequest(CMessage::Ptr msg, cb_Request cb);

	protected:
		std::size_t run();

		void ProcessRequest(CMessage::Ptr msg, cb_Request cb, const boost::system::error_code &err, CConnection::Ptr connection);
		void CompleteRequest(cb_Request cb, const boost::system::error_code &err, CMessage::Ptr msg);

	private:
		boost::mutex m_Mutex;
		boost::asio::io_service m_Io_Service;
		boost::shared_ptr<boost::asio::io_service::work> m_pWork;

		boost::shared_ptr<CConnectionPool> m_pPool;

		std::vector<boost::shared_ptr<boost::thread>> m_ListThread;
	};
}

#endif