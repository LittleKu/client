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
		//��ʼ��������Ϣ
		m_pPool->Init(host, port, cb, connection_count, connection_limit);

		//���������߳�,��IOCP/epollʹ��
		for (int i = 0; i < thread_count; i++)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&CClientImpl::run, shared_from_this())));
			m_ListThread.push_back(thread);
		}
	}

	//��ʼ����IOCP/epoll
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
		//�û���ֱ������ֹͣ���й���
		//ע��:�˲�����ѵ�ǰ���еĹ����߳��е�����ǿ�����,����!
		m_pPool->Stop();
		m_pWork.reset();

		for (size_t i = 0; i < m_ListThread.size(); i++)
		{
			/** 
			 * ȷ���߳�ִ��������˳�
			 */
			m_ListThread[i]->join();
		}
	}

	void CClientImpl::PostRequest(CMsgBuffer::Ptr msg, cb_Request cb)
	{
		//��ͨ��CConnectionPool::GetConnection��ȡ����Ч�����Ӷ���
		//�����ȡʧ��,�Ȱѷ������󱣴浽�����Ͷ�����,
		//���ȴ�һ����ʱ���,��ͨ��CConnectionPool::CheckAvaliableConnection��ȡ����Ч�ҿ��е����Ӷ���
		//���ѽ��Post��CClientImpl::ProcessRequest��
		m_pPool->GetConnection(boost::bind(&CClientImpl::ProcessRequest, shared_from_this(), msg, cb, _1, _2));
	}

	void CClientImpl::ProcessRequest(CMsgBuffer::Ptr msg, cb_Request cb, const boost::system::error_code &err, CConnection::Ptr connection)
	{
		//��������
		if (connection && !err)
		{
			//����һ���պ�����ַ,����CConnectionPool::QueueConnection�ٴ���һ��cb_InitConnection�ص�
			cb_InitConnection NullCB = NULL;

			//ֱ�ӵ����첽��������,���ѷ��͵Ľ�����ص�CClientImpl::CompleteRequest��
			connection->PostWrite(msg,
				boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, _1, _2),//
				boost::bind(&CConnectionPool::QueueConnection, m_pPool, _1, _2, _3, NullCB, connection));//�˴��õ�������������Ӷ���connection
		}
		//����������
		else
		{
			//���Ȱ���Ч�����Ӷ����������
			if (connection)
			{
				m_pPool->QueueConnection(err, SC_None, msg, NULL, connection);
			}

			//Ȼ���ٻ㱨����
			if (err)
			{
				m_Io_Service.post(boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, err, msg));
			}
			else
			{
				//���û�д���,�Ǿ�ֻ�ܱ�ʶΪû����Ч�����ӿ���ʹ��,������not_connected�����������
				boost::system::error_code error(boost::asio::error::not_connected);
				m_Io_Service.post(boost::bind(&CClientImpl::CompleteRequest, shared_from_this(), cb, error, msg));
			}
		}
	}

	void CClientImpl::CompleteRequest(cb_Request cb, const boost::system::error_code &err, CMsgBuffer::Ptr msg)
	{
		//ֱ�ӷ�Ӧ���û���Ļص�����,��Demo�е�OnSendComplete
		cb(err, msg);
	}
}