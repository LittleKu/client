/****************************************
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#include "Connection.h"
#include "msgbuffer.h"

namespace client
{
	CConnection::CConnection(boost::asio::io_service &io_service)
		:m_ioService(io_service),
		m_Resolver(io_service),
		m_Socket(io_service)
	{
		m_pMsgBuffer.reset(new CMsgBuffer());
		m_pMsgBuffer->Clear();

		::memset(m_rgData, 0, sizeof(m_rgData));
		m_nBodyLength = 0;
	}
	CConnection::~CConnection()
	{
	}

	//关闭连接
	void CConnection::Close()
	{
		m_Socket.close();
	}

	//连接是否已经打开
	bool CConnection::IsOpen() const
	{
		return m_Socket.is_open();
	}
}