/****************************************
 * class	: CConnection
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#pragma once

#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "Message.h"



namespace client
{
	enum StatusCode
	{
		SC_None,
		SC_Resolve,
		SC_Connect,
		SC_ReadHeader,
		SC_ReadBody,
		SC_Send,
	};

	class CConnection
		:public boost::enable_shared_from_this<CConnection>,
		private boost::noncopyable
	{
	public:
		typedef boost::shared_ptr<CConnection> Ptr;
	public:
		CConnection(boost::asio::io_service &io_service)
			:m_ioService(io_service),
			m_Resolver(io_service),
			m_Socket(io_service)
		{
			m_Response.reset(new CMessage());
		}
		~CConnection()
		{
		}

		//关闭连接
		void Close()
		{
			m_Socket.close();
		}

		//连接是否已经打开
		bool IsOpen() const
		{
			return m_Socket.is_open();
		}

		//投递一个发送信息的请求
		template <typename Handler>
		void PostWrite(CMessage::Ptr msg, boost::function2<void, const boost::system::error_code &, CMessage::Ptr> cb, Handler handler)
		{
			if (msg->GetData())
			{
				void (CConnection::*f)(const boost::system::error_code &,
					CMessage::Ptr,
					boost::function2<void, const boost::system::error_code &, CMessage::Ptr>,
					boost::tuple<Handler>) = &CConnection::Handle_Write<Handler>;

				boost::asio::async_write(m_Socket, boost::asio::buffer(msg->GetData(), msg->GetLength()),
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, msg, cb, boost::make_tuple(handler)));
			}
		}

		//尝试连接指定IP与端口的服务器
		template <typename Handler>
		void Connect(std::string host, std::string port, Handler handler)
		{
			void (CConnection::*f)(const boost::system::error_code &, boost::asio::ip::tcp::resolver::iterator, boost::tuple<Handler>) = &CConnection::Handle_Resolver<Handler>;

			boost::tuple<Handler> h = boost::make_tuple(handler);

			try
			{
				std::cout << "try to connect " << host << ":" << port << std::endl;

				boost::asio::ip::tcp::resolver::query query(host, port);
				m_Resolver.async_resolve(query, 
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::iterator, h));
			}
			catch (const boost::system::system_error &e)
			{
				boost::get<0>(h)(e.code(), SC_Resolve, m_Response);
			}
		}

	private:
		boost::asio::io_service &m_ioService;
		boost::asio::ip::tcp::socket m_Socket;
		boost::asio::ip::tcp::resolver m_Resolver;

		CMessage::Ptr m_Response;

		template <typename Handler>
		void Handle_Resolver(const boost::system::error_code &err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator, boost::tuple<Handler> handler)
		{
			void (CConnection::*f)(const boost::system::error_code &, boost::asio::ip::tcp::resolver::iterator, boost::tuple<Handler>) = &CConnection::Handle_Connect<Handler>;

			if (!err)
			{
				//未出现错误,直接尝试异步连接
				boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
				m_Socket.async_connect(endpoint,
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, ++endpoint_iterator, handler));
			}
			else
			{
				//设置连接方案出错,如果使用连接池模式时,此时直接调用CConnectionPool::QueueConnection,并传递错误代码
				//如果是直接使用单一的Connection的,此时直接调用用户自定义的回调函数,并传递错误代码
				boost::get<0>(handler)(err, SC_Resolve, m_Response);
			}
		}

		template <typename Handler>
		void Handle_Connect(const boost::system::error_code &err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator, boost::tuple<Handler> handler)
		{
			if (!err)
			{
				//异步连接成功,如果使用连接池模式时,通知CConnectionPool::QueueConnection并传递错误代码(当然此时的错误代码肯定为0)与响应的数据包(当然此时数据包也只是个空的数据包
				//因为我们还未开始接收数据,可以通过判断response的data是否为空就可以得)
				boost::get<0>(handler)(err, SC_Connect, m_Response);
				
				void (CConnection::*f)(const boost::system::error_code &, boost::tuple<Handler>) = &CConnection::Handle_Read_Header<Handler>;
				
				//投递 header_length(4) 字节大小尝试接收数据包头部
				boost::asio::async_read(m_Socket, boost::asio::buffer(m_Response->GetData(true), m_Response->header_length),
					boost::bind(f, shared_from_this(), 
					boost::asio::placeholders::error, handler));
			}
			else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
			{
				//连接出错?那么尝试从创建的连接方案中选择下一个如果还有得选择的话,并尝试再次连接
				void (CConnection::*f)(const boost::system::error_code &, boost::asio::ip::tcp::resolver::iterator, boost::tuple<Handler>) = &CConnection::Handle_Connect<Handler>;
				//再次连接前,先把套接字关闭
				m_Socket.close();

				boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
				m_Socket.async_connect(endpoint,
					boost::bind(f, shared_from_this(),
					boost::asio::placeholders::error, ++endpoint_iterator, handler));
			}
			else
			{
				//无路可走,直接报错吧,连接池通知CConnectionPool::QueueConnection,非连接池通知自定义回调函数
				boost::get<0>(handler)(err, SC_Connect, m_Response);
			}
		}

		template <typename Handler>
		void Handle_Read_Header(const boost::system::error_code &err, boost::tuple<Handler> handler)
		{
			//正常接收到数据,并且能正确转换为数据包头部数据
			if (!err && m_Response->DecodeHeader())
			{
				void (CConnection::*f)(const boost::system::error_code &, boost::tuple<Handler>) = &CConnection::Handle_Read_Body<Handler>;
				//根据解密得到的数据包大小投递接收数据包
				boost::asio::async_read(m_Socket, boost::asio::buffer(m_Response->GetBody(), m_Response->GetBodyLength()),
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, handler));
			}
			else
			{
				boost::system::error_code error;
				error = err;
				if (!err)
				{
					error = boost::asio::error::message_size;
				}
				//报错,连接池通知CConnectionPool::QueueConnection,非连接池通知自定义回调函数
				boost::get<0>(handler)(error, SC_ReadHeader, m_Response);
				//std::cout<< error.message() << std::endl;
			}
		}

		template <typename Handler>
		void Handle_Read_Body(const boost::system::error_code &err, boost::tuple<Handler> handler)
		{
			if (!err)
			{
				//到这步为止,已经接收到一个完整的数据包,所以此时可以通知回调函数,系统已经完整一个数据包的接收
				//连接池通知CConnectionPool::QueueConnection,非连接池通知自定义回调函数
				CMessage::Ptr pMsg;
				pMsg.reset(new CMessage());
				::memcpy(pMsg->GetData(true), m_Response->GetData(), m_Response->GetLength());
				pMsg->SetBodyLength(m_Response->GetBodyLength());

				boost::get<0>(handler)(err, SC_ReadBody, pMsg);

				void (CConnection::*f)(const boost::system::error_code &, boost::tuple<Handler>) = &CConnection::Handle_Read_Header<Handler>;

				//接收完一个数据包后,当然得继续投递,接收一下个数据包如此无限循环
				boost::asio::async_read(m_Socket, boost::asio::buffer(m_Response->GetData(true), m_Response->header_length),
					boost::bind(f, shared_from_this(), 
					boost::asio::placeholders::error, handler));
			}
			else
			{
				//报错,连接池通知CConnectionPool::QueueConnection,非连接池通知自定义回调函数
				boost::get<0>(handler)(err, SC_ReadBody, m_Response);
			}
		}
		
		//处理投递发送信息的请求结果
		template <typename Handler>
		void Handle_Write(const boost::system::error_code &err, CMessage::Ptr msg, 
			boost::function2<void, const boost::system::error_code &, CMessage::Ptr>cb, boost::tuple<Handler> handler)
		{
			//CClientImpl::CompleteRequest
			cb(err, msg);
			
			//CConnectionPool::QueueConnection
			//回收connection
			boost::get<0>(handler)(err, SC_Send, msg);
		}
	};
}

#endif
