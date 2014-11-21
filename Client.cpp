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
		m_pClientImpl->Init(host, port, cb, thread_count, connection_count, connection_limit);
	}

	void CClient::Stop()
	{
		m_pClientImpl->Stop();
	}

	void CClient::PostSend(CMessage::Ptr msg, cb_Request cb)
	{
		m_pClientImpl->PostRequest(msg, cb);
	}
}