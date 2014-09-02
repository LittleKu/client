/****************************************
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#include "ClientImpl.h"

namespace client
{
	CClientImpl::CClientImpl()
		:m_Io_Service(),
		m_pWork(new boost::asio::io_service::work(m_Io_Service))
	{
		m_pPool.reset(new CConnectionPool(m_Io_Service));
	}

	CClientImpl::~CClientImpl()
	{
	}

	void CClientImpl::Init(const std::string &host, const std::string &port, cb_InitConnection cb, int thread_count/* = 1*/, int connection_count/* = 5*/, int try_count/* = 1*/, int connection_limit/* = 1*/)
	{
		m_pPool->Init(host, port, cb, connection_count, try_count, connection_limit);

		for (int i = 0; i < thread_count; i++)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&CClientImpl::run, shared_from_this())));
			m_ListThread.push_back(thread);
		}
	}

	std::size_t CClientImpl::run()
	{
		std::size_t ret = 0;
		for (;;)
		{
			try
			{
				ret = m_Io_Service.run();
			}
			catch (...)
			{
			}
		}
		return ret;
	}

	void CClientImpl::Stop()
	{
		m_pPool->Stop();
		m_pWork.reset();

		for (size_t i = 0; i < m_ListThread.size(); i++)
		{
			/** 
			 * 确保线程执行完后再退出(make sure all have been finished,then exit the thread)
			 */
			m_ListThread[i]->join();
		}
	}

	void CClientImpl::PostRequest(CMessage::Ptr msg, cb_Request cb)
	{
		m_pPool->GetConnection(boost::bind(&CClientImpl::ProcessRequest, shared_from_this(), msg, cb, _1, _2));
	}

	void CClientImpl::ProcessRequest(CMessage::Ptr msg, cb_Request cb,const boost::system::error_code &err, CConnection::Ptr connection)
	{
		if (connection && !err)
		{
			cb_InitConnection cb_Init = NULL;

			connection->PostWrite(msg,
				boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, _1, _2),//
				boost::bind(&CConnectionPool::QueueConnection, m_pPool, _1, _2, cb_Init, connection));//
		}
		else
		{
			if (connection)
			{
				m_pPool->QueueConnection(err, msg, NULL, connection);
			}
			if (err)
			{
				m_Io_Service.post(boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, err, msg));
			}
			else
			{
				boost::system::error_code error(boost::asio::error::not_connected);
				m_Io_Service.post(boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, error, msg));
			}
		}
	}

	void CClientImpl::CompleteRequest(cb_Request cb, const boost::system::error_code &err, CMessage::Ptr msg)
	{
		cb(err, msg);
	}
}