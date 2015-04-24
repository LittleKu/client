/****************************************
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#include "Client.h"

namespace client
{
	CClient::CClient()
	{
		m_pClientImpl.reset(new CClientImpl());
	}

	CClient::~CClient()
	{
	}

	void CClient::Init(const std::string &host, const std::string &port, cb_InitConnection cb, int thread_count/* = 1*/, int connection_count/* = 5*/, int connection_limit/* = 1*/)
	{
		//初始化客户端连接所需信息
		m_pClientImpl->Init(host, port, cb, thread_count, connection_count, connection_limit);
		m_host = host;
		m_port = port;
		m_cbInit = cb;
	}

	void CClient::Stop()
	{
		//客户端主动强制关闭所有工作连接,慎用(确保在所有工作都已经完成后,使用最佳)
		m_pClientImpl->Stop();
	}

	//向服务端发送信息
	void CClient::PostSend(CMsgBuffer::Ptr msg, cb_Request cb)
	{
		m_pClientImpl->PostRequest(msg, cb);
	}

	void CClient::Reset()
	{
		if (m_host.c_str() && m_port.c_str() && m_cbInit)
		{
			Init(m_host, m_port, m_cbInit);
		}
	}
}