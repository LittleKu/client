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

	void CClientImpl::Init(const std::string &host, const std::string &port, cb_InitConnection cb, int thread_count/* = 1*/, int connection_count/* = 5*/, int connection_limit/* = 1*/)
	{
		//初始化连接信息
		m_pPool->Init(host, port, cb, connection_count, connection_limit);

		//创建运行线程,供IOCP/epoll使用
		for (int i = 0; i < thread_count; i++)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&CClientImpl::run, shared_from_this())));
			m_ListThread.push_back(thread);
		}
	}

	//开始运行IOCP/epoll
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
		//用户层直接主动停止所有工作
		//注意:此操作会把当前所有的工作线程中的任务强制清除,慎用!
		m_pPool->Stop();
		m_pWork.reset();

		for (size_t i = 0; i < m_ListThread.size(); i++)
		{
			/** 
			 * 确保线程执行完后再退出
			 */
			m_ListThread[i]->join();
		}
	}

	void CClientImpl::PostRequest(CMsgBuffer::Ptr msg, cb_Request cb)
	{
		//先通过CConnectionPool::GetConnection获取到有效的连接对象
		//如果获取失败,先把发送请求保存到待发送队列中,
		//并等待一定的时间后,再通过CConnectionPool::CheckAvaliableConnection获取到有效且空闲的连接对象
		//最后把结果Post到CClientImpl::ProcessRequest上
		m_pPool->GetConnection(boost::bind(&CClientImpl::ProcessRequest, shared_from_this(), msg, cb, _1, _2));
	}

	void CClientImpl::ProcessRequest(CMsgBuffer::Ptr msg, cb_Request cb, const boost::system::error_code &err, CConnection::Ptr connection)
	{
		//正常流程
		if (connection && !err)
		{
			//传递一个空函数地址,以免CConnectionPool::QueueConnection再触发一次cb_InitConnection回调
			cb_InitConnection NullCB = NULL;

			//直接调用异步发送数据,最后把发送的结果返回到CClientImpl::CompleteRequest来
			connection->PostWrite(msg,
				boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, _1, _2),//
				boost::bind(&CConnectionPool::QueueConnection, m_pPool, _1, _2, _3, NullCB, connection));//此处用到回收用完的连接对象connection
		}
		//非正常流程
		else
		{
			//首先把有效的连接对象回收起来
			if (connection)
			{
				m_pPool->QueueConnection(err, SC_None, msg, NULL, connection);
			}

			//然后再汇报错误
			if (err)
			{
				m_Io_Service.post(boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, err, msg));
			}
			else
			{
				//如果没有错误,那就只能标识为没有有效的连接可以使用,所以用not_connected来处理错误结果
				boost::system::error_code error(boost::asio::error::not_connected);
				m_Io_Service.post(boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, error, msg));
			}
		}
	}

	void CClientImpl::CompleteRequest(cb_Request cb, const boost::system::error_code &err, CMsgBuffer::Ptr msg)
	{
		//直接反应给用户层的回调函数,如Demo中的OnSendComplete
		cb(err, msg);
	}
}