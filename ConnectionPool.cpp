/****************************************
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#include "ConnectionPool.h"

namespace client
{
	//����
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
		m_TimeoutRequest = 5;
		m_TimeoutConnect = 2;
	}

	//�鹹
	CConnectionPool::~CConnectionPool()
	{
	}

	void CConnectionPool::Init(const std::string &host, const std::string &port, cb_InitConnection cb, int connection_count /*= 5*/, int connection_limit /*= 1*/)
	{
		//�����Ҫ����Ϣ
		m_Host = host;
		m_Port = port;
		m_nConnectionLimit = connection_limit;
		
		//��������(��)
		for (int i = 0; i < connection_count; i++)
		{
			NewConnection(cb);
		}
	}

	void CConnectionPool::Stop()
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		
		//��ʶ�ͻ����Ѿ��Զ���������������
		m_bIsStop = true;

		//��������Ч�Ŀ��е�����ȫ���ر�
		while (!m_ListValid.empty())
		{
			CConnection::Ptr connection = m_ListValid.front();
			connection->Close();
			m_ListValid.pop_front();
		}

		//�����������е�����ȫ���ر�
		while (!m_ListRun.empty())
		{
			CConnection::Ptr connection = m_ListRun.front();
			connection->Close();
			m_ListRun.pop_front();
		}

		//���½�������ȫ���ر�
		while (!m_ListNew.empty())
		{
			CConnection::Ptr connection = m_ListNew.front();
			connection->Close();
			m_ListNew.pop_front();
		}

		//�ر����ж�ʱ��
		m_TryTimer.cancel();
		m_TryTimerConnect.cancel();

		//������д����Ͷ���
		while (!m_DequeRequest.empty())
		{
			CConnection::Ptr connection;
			cb_addConnection cb = m_DequeRequest.front();
			m_DequeRequest.pop_front();
			boost::system::error_code error(boost::asio::error::not_connected);
			//����δ������ɵ���Ϣ,���Ը���ʵ�������ʵ���д���
			cb(error, connection);//������Demo�е�OnSendComplete
		}
	}

	void CConnectionPool::NewConnection(cb_InitConnection cb)
	{
		//ϵͳ����׼�������������,�����Ѿ���������Ͽ�������
		if (m_bIsStop)
			return;

		boost::mutex::scoped_lock lock(m_Mutex);
		
		//�����µ����Ӷ���,�����������������,�������ӳɹ�ʱ�ȴ��������ݰ��ĵ���
		CConnection::Ptr connection(new CConnection(m_Io_Service));
		connection->Connect(m_Host, m_Port, boost::bind(&CConnectionPool::QueueConnection, shared_from_this(), _1, _2, _3, cb, connection));
		//�´��������Ӷ�������½������б���
		m_ListNew.push_back(connection);

		//�������ӵĻ�����1
		m_nConnectionCount++;
	}

	//async_resolve,async_connect,async_read,async_write�ɹ����,������ô˺�����Ϊ�ص�����,�����ݴ�����뼰���ݰ�
	void CConnectionPool::QueueConnection(const boost::system::error_code &err, StatusCode sc, CMessage::Ptr msg, cb_InitConnection cb, CConnection::Ptr connection)
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		//StatusCode���Ե�֪��ǰ�����˻ص�����һ������
		//1.CConnection::Handle_Write���ͳɹ���ʧ��ʱҪ����connection
		//2.CConnection::Connect�в�׽������,��ʱ��connection��NewConnection���ݹ���,Ҳ�ڴ˴���
		//3.CConnection::Handle_Resolver������,��ʱ��connection��NewConnection���ݹ���,Ҳ�ڴ˴���
		//4.CConnection::Handle_Connect���ӳɹ���ʧ��ʱ,֪ͨ�˻ص�,��ʱ��connection��NewConnection���ݹ���,Ҳ�ڴ˴���
		//5.CConnection::Handle_Read_Header������,��ʱ��connection��NewConnection���ݹ���,Ҳ�ڴ˴���
		//6.CConnection::Handle_Read_Body�ɹ�����һ���������ݰ������ʧ��ʱ,֪ͨ�˻ص�,��ʱ��connection��NewConnection���ݹ���,Ҳ�ڴ˴���

		/**
		 * ��NewConnectionʱ�����ڴ�ʧ����,��ʱ��connection������Ϊ��
		 */
		if (connection)
		{
			//����CConnection::Handle_Write����ʱ��connection��m_ListRun������,�����Ķ�����m_ListRun,���������������,��ʱ�����԰�connection��m_ListRun�Ƴ�
			m_ListRun.remove(connection);
			//�������Ӵ��������ݹ���,����Ӧ���������½��б�����,������������б���Ļ�,���Ƴ�
			m_ListNew.remove(connection);

			//����û��Ѿ�ֹͣ���������һ����Ϣ����,��ô��ֱ�ӷ���
			if (m_bIsStop)
				return;

			//���Ӷ����״̬�Ƿ�����,����û���κδ���
			if (connection->IsOpen() && !err)
			{
				//��û����������Ӷ������ȴ�ʹ�õ��б���,�Ա���ʱȡ��ʹ��
				m_ListValid.push_back(connection);
			}
			//״̬���쳣�����д���ʱ
			else
			{
				//����Ч�������б����Ƴ�
				m_ListValid.remove(connection);
				//�鿴��Ч��������+�������е�������+�½����������Ƿ�С��ϵͳ�趨����С������
				if ( (m_ListValid.size() + m_ListRun.size() + m_ListNew.size() ) < (size_t)m_nConnectionLimit)
				{
					//���Լ������ӵĻ�����1
					m_TryConnect++;
					//���������ӵĻ���С��3ʱ
					if (m_TryConnect < 3)
					{
						//ϵͳ���������ٴ����µ�����
						m_Io_Service.post(boost::bind(&CConnectionPool::NewConnection, shared_from_this(), cb));
					}
					//����������ӵĻ������ڻ����3ʱ
					else
					{
						//��ʱ���ó�ʱ�ȴ�
						int res = m_TryTimerConnect.expires_from_now(boost::posix_time::seconds(m_TimeoutConnect));
						if (!res)
						{
							m_TryTimerConnect.async_wait(boost::bind(&CConnectionPool::TimeoutNewConnection, shared_from_this(), boost::asio::placeholders::error, cb));
						}
					}
				}
			}
		}

		//��Ч�����Ӷ�����в�Ϊ��,��������������еȴ����͵�����
		if (!m_ListValid.empty() && !m_DequeRequest.empty())
		{
			//����Ч�����Ӷ������ȡ��һ�����õ����Ӷ���
			CConnection::Ptr pConnection = m_ListValid.front();
			m_ListValid.pop_front();

			//�������Ч�����Ӷ����������ʹ�õ����Ӷ������
			m_ListRun.push_back(pConnection);

			//�ӷ������������ȡ��һ������,������
			cb_addConnection pCb = m_DequeRequest.front();
			m_DequeRequest.pop_front();

			boost::system::error_code error;
			//��ʹ��������ʱ,�˴��Ļص�CClientImpl::ProcessRequest,�����ʹ��CConnection,�˴��ص�Ϊ�û��Լ���ʵ��
			m_Io_Service.post(boost::bind(pCb, error, pConnection));
		}

		if (cb && msg)
		{
			if (m_ListValid.empty())
			{
				boost::system::error_code error;
				error = boost::asio::error::not_connected;
				m_Io_Service.post(boost::bind(cb, error, sc, msg));
			}
			else
			{
				m_Io_Service.post(boost::bind(cb, err, sc, msg));
			}
		}
	}

	//���Դ���Ч�����б���ȡ�����е�connection
	void CConnectionPool::GetConnection(cb_addConnection cb)
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		//��ֱ�ӿ��õĿ���connection
		if (!m_ListValid.empty())
		{
			CConnection::Ptr connection = m_ListValid.front();
			m_ListValid.pop_front();
			
			m_ListRun.push_back(connection);
			boost::system::error_code error;
			
			/*֪ͨ�ص�,���ڻص��л���connection
			void CClientImpl::ProcessRequest(CMessage::Ptr msg, cb_Request cb,const boost::system::error_code &err, CConnection::Ptr connection)*/
			m_Io_Service.post(boost::bind(cb, error, connection));
		}
		//����
		else
		{
			//���б�������ص�
			m_DequeRequest.push_back(cb);

			//���ö�ʱ��,һ��ʱ��������¼��
			int res = m_TryTimer.expires_from_now(boost::posix_time::seconds(m_TimeoutRequest));
			if (!res)
			{
				m_TryTimer.async_wait(boost::bind(&CConnectionPool::CheckAvaliableConnection, shared_from_this(), boost::asio::placeholders::error));
			}
		}
	}

	//�ٴμ�Ⲣ��ȡ��Ч��connection
	void CConnectionPool::CheckAvaliableConnection(const boost::system::error_code &err)
	{
		bool aborted = (err == boost::asio::error::operation_aborted) ? true : false;
		if (aborted)
			return;

		boost::mutex::scoped_lock lock(m_Mutex);

		CConnection::Ptr connection;
		boost::system::error_code error;

		//�������в�Ϊ��
		if (!m_DequeRequest.empty())
		{
			cb_addConnection cb = m_DequeRequest.front();
			m_DequeRequest.pop_front();

			//�ٴγ��Ի�ȡ���е�connection
			if (!m_ListValid.empty())
			{
				connection = m_ListValid.front();
				m_ListValid.pop_front();
				m_ListRun.push_back(connection);
			}
			//û���ٴλ�ȡ�����е����Ӷ���
			else
			{
				error = boost::asio::error::not_connected;
			}
			/*֪ͨ�ص�,���ڻص��л���connection
			void CClientImpl::ProcessRequest(CMessage::Ptr msg, cb_Request cb,const boost::system::error_code &err, CConnection::Ptr connection)*/
			m_Io_Service.post(boost::bind(cb, error, connection));
		}
	}

	//����ʧ��,���´����µ�connection
	void CConnectionPool::TimeoutNewConnection(const boost::system::error_code &err, cb_InitConnection cb)
	{
		//���ӳ�ʱ,�������Ϊ�û���ֹ����,��ô��ֱ�ӷ���
		bool aborted = (err == boost::asio::error::operation_aborted) ? true : false;
		if (aborted)
			return;

		//���������������ӻ���,�����ٳ�������
		boost::mutex::scoped_lock lock(m_Mutex);

		//Ͷ��һ�����´����µ�connection
		m_TryConnect = 0;
		m_Io_Service.post(boost::bind(&CConnectionPool::NewConnection, shared_from_this(), cb));
	}
}