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

#include "Message.hpp"



namespace client
{
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

		void Close()
		{
			m_Socket.close();
		}

		bool IsOpen() const
		{
			return m_Socket.is_open();
		}

		template <typename Handler>
		void PostWrite(CMessage::Ptr msg, boost::function2<void, const boost::system::error_code &, CMessage::Ptr> cb, Handler handler)
		{
			if (msg->data())
			{
				void (CConnection::*f)(const boost::system::error_code &,
					CMessage::Ptr,
					boost::function2<void, const boost::system::error_code &, CMessage::Ptr>,
					boost::tuple<Handler>) = &CConnection::Handle_Write<Handler>;

				boost::asio::async_write(m_Socket, boost::asio::buffer(msg->data(), msg->length()),
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, msg, cb, boost::make_tuple(handler)));
			}
		}
		
		template <typename Handler>
		void Handle_Write(const boost::system::error_code &err, CMessage::Ptr msg, 
			boost::function2<void, const boost::system::error_code &, CMessage::Ptr>cb, boost::tuple<Handler> handler)
		{
			//CClientImpl::CompleteRequest
			cb(err, msg);
			
			//CConnectionPool::QueueConnection(const boost::system::error_code &err, CMessage::Ptr msg, cb_InitConnection cb, CConnection::Ptr connection)
			boost::get<0>(handler)(err, msg);
		}

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
				boost::get<0>(h)(e.code(), m_Response);
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
				boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
				m_Socket.async_connect(endpoint,
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, ++endpoint_iterator, handler));
			}
			else
			{
				//error here
				boost::get<0>(handler)(err, m_Response);
			}
		}

		template <typename Handler>
		void Handle_Connect(const boost::system::error_code &err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator, boost::tuple<Handler> handler)
		{
			if (!err)
			{
				boost::get<0>(handler)(err, m_Response);//CConnectionPool::QueueConnection
				void (CConnection::*f)(const boost::system::error_code &, boost::tuple<Handler>) = &CConnection::Handle_Read_Header<Handler>;

				boost::asio::async_read(m_Socket, boost::asio::buffer(m_Response->data(), m_Response->header_length),
					boost::bind(f, shared_from_this(), 
					boost::asio::placeholders::error, handler));
			}
			else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
			{
				void (CConnection::*f)(const boost::system::error_code &, boost::asio::ip::tcp::resolver::iterator, boost::tuple<Handler>) = &CConnection::Handle_Connect<Handler>;

				m_Socket.close();

				boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
				m_Socket.async_connect(endpoint,
					boost::bind(f, shared_from_this(),
					boost::asio::placeholders::error, ++endpoint_iterator, handler));
			}
			else
			{
				boost::get<0>(handler)(err, m_Response);
			}
		}

		template <typename Handler>
		void Handle_Read_Header(const boost::system::error_code &err, boost::tuple<Handler> handler)
		{
			if (!err && m_Response->decode_header())
			{
				void (CConnection::*f)(const boost::system::error_code &, boost::tuple<Handler>) = &CConnection::Handle_Read_Body<Handler>;

				boost::asio::async_read(m_Socket, boost::asio::buffer(m_Response->body(), m_Response->body_length()),
					boost::bind(f, shared_from_this(), boost::asio::placeholders::error, handler));
			}
			else
			{
				boost::get<0>(handler)(err, m_Response);
			}
		}

		template <typename Handler>
		void Handle_Read_Body(const boost::system::error_code &err, boost::tuple<Handler> handler)
		{
			if (!err)
			{
				boost::get<0>(handler)(err, m_Response);

				void (CConnection::*f)(const boost::system::error_code &, boost::tuple<Handler>) = &CConnection::Handle_Read_Header<Handler>;

				boost::asio::async_read(m_Socket, boost::asio::buffer(m_Response->data(), m_Response->header_length),
					boost::bind(f, shared_from_this(), 
					boost::asio::placeholders::error, handler));
			}
			else
			{
				boost::get<0>(handler)(err, m_Response);
			}
		}
	};
}

#endif