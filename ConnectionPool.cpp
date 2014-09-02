#include "ConnectionPool.h"

namespace client
{
	CConnectionPool::CConnectionPool(boost::asio::io_service &io_service)
		:m_Host(""),
		m_Port(""),
		m_Io_Service(io_service),
		m_TryTimer(io_service),
		m_TryTimerConnect(io_service)
	{
		m_nConnectionCount = 0;
		m_bIsStop = false;
		m_nConnectionLimit = 10;
		m_TryConnect = 0;
		m_TryCount = 0;
		m_TimeoutRequest = 5;
		m_TimeoutConnect = 2;
	}

	CConnectionPool::~CConnectionPool()
	{
	}

	void CConnectionPool::Init(const std::string &host, const std::string &port, cb_InitConnection cb, int connection_count /*= 5*/, int try_count /*= 1*/, int connection_limit /*= 1*/)
	{
		m_Host = host;
		m_Port = port;
		m_TryCount = try_count;
		m_nConnectionLimit = connection_limit;
		
		for (int i = 0; i < connection_count; i++)
		{
			NewConnection(cb);
		}
	}

	void CConnectionPool::Stop()
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		
		m_bIsStop = true;

		while (!m_DequeValid.empty())
		{
			CConnection::Ptr connection = m_DequeValid.front();
			connection->Close();
			m_DequeValid.pop_front();
		}

		while (!m_ListRun.empty())
		{
			CConnection::Ptr connection = m_ListRun.front();
			connection->Close();
			m_ListRun.pop_front();
		}

		while (!m_ListNew.empty())
		{
			CConnection::Ptr connection = m_ListNew.front();
			connection->Close();
			m_ListNew.pop_front();
		}

		m_TryTimer.cancel();
		m_TryTimerConnect.cancel();

		while (!m_DequeRequest.empty())
		{
			CConnection::Ptr connection;
			cb_addConnection cb = m_DequeRequest.front();
			m_DequeRequest.pop_front();
			boost::system::error_code error(boost::asio::error::not_connected);
			cb(error, connection);
		}
	}

	void CConnectionPool::NewConnection(cb_InitConnection cb)
	{
		if (m_bIsStop)
			return;

		CConnection::Ptr connection(new CConnection(m_Io_Service));
		connection->Connect(m_Host, m_Port, boost::bind(&CConnectionPool::QueueConnection, shared_from_this(), _1, _2, cb, connection));
		m_ListNew.push_back(connection);

		boost::mutex::scoped_lock lock(m_Mutex);
		m_nConnectionCount++;
	}

	void CConnectionPool::QueueConnection(const boost::system::error_code &err, CMessage::Ptr msg, cb_InitConnection cb, CConnection::Ptr connection)
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (connection)
		{
			m_ListRun.remove(connection);
			m_ListNew.remove(connection);

			if (m_bIsStop)
				return;

			if (connection->IsOpen() && !err)
			{
				m_DequeValid.push_back(connection);
			}
			else
			{
				if ( (m_DequeValid.size() + m_ListRun.size() + m_ListNew.size() ) < (size_t)m_nConnectionLimit)
				{
					m_TryCount++;
					if (m_TryCount < 3)
					{
						NewConnection(cb);
					}
					else
					{
						int res = m_TryTimerConnect.expires_from_now(boost::posix_time::seconds(m_TimeoutConnect));
						if (!res)
						{
							m_TryTimerConnect.async_wait(boost::bind(&CConnectionPool::TimeoutNewConnection, shared_from_this(), boost::asio::placeholders::error, cb));
						}
					}
				}
			}
		}

		if (!m_DequeValid.empty() && !m_DequeRequest.empty())
		{
			CConnection::Ptr pConnection = m_DequeValid.front();
			m_DequeValid.pop_front();
			m_ListRun.push_back(pConnection);
			cb_addConnection pCb = m_DequeRequest.front();
			m_DequeRequest.pop_front();
			boost::system::error_code error;
			m_Io_Service.post(boost::bind(pCb, error, pConnection));
		}

		if (cb && msg)
		{
			boost::system::error_code error;
			m_Io_Service.post(boost::bind(cb, error, msg));
		}
	}

	void CConnectionPool::GetConnection(cb_addConnection cb)
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		if (!m_DequeValid.empty())
		{
			CConnection::Ptr connection = m_DequeValid.front();
			m_DequeValid.pop_front();
			m_ListRun.push_back(connection);
			boost::system::error_code error;
			m_Io_Service.post(boost::bind(cb, error, connection));
		}
		else
		{
			m_DequeRequest.push_back(cb);

			int res = m_TryTimer.expires_from_now(boost::posix_time::seconds(m_TimeoutRequest));
			if (!res)
			{
				m_TryTimer.async_wait(boost::bind(&CConnectionPool::CheckAvaliableConnection, shared_from_this(), boost::asio::placeholders::error));
			}
		}
	}

	void CConnectionPool::CheckAvaliableConnection(const boost::system::error_code &err)
	{
		bool aborted = (err == boost::asio::error::operation_aborted) ? true : false;
		if (aborted)
			return;

		boost::mutex::scoped_lock lock(m_Mutex);

		CConnection::Ptr connection;
		boost::system::error_code error;

		if (!m_DequeRequest.empty())
		{
			cb_addConnection cb = m_DequeRequest.front();
			m_DequeRequest.pop_front();

			if (!m_DequeValid.empty())
			{
				connection = m_DequeValid.front();
				m_DequeValid.pop_front();
				m_ListRun.push_back(connection);
			}
			else
			{
				error = boost::asio::error::not_connected;
			}
			m_Io_Service.post(boost::bind(cb, error, connection));
		}
	}

	void CConnectionPool::TimeoutNewConnection(const boost::system::error_code &err, cb_InitConnection cb)
	{
		bool aborted = (err == boost::asio::error::operation_aborted) ? true : false;
		if (aborted)
			return;

		boost::mutex::scoped_lock lock(m_Mutex);

		m_TryConnect = 0;

		NewConnection(cb);
	}
}