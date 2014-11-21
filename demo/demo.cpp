// demo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


/*void callback_connection(const boost::system::error_code& err, client::CMessage::Ptr msg)
{
	if (!err)
		std::cout << "something is work fine" << err.value() << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		std::cout << "Hello World" << std::endl;

		std::string host = "localhost";
        std::string port = "11211";

		boost::asio::io_service io_service;

		boost::shared_ptr<client::CConnection> connection(new client::CConnection(io_service));

		connection->Connect(host, port, boost::bind(&callback_connection, _1, _2));

		boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service)));

		thread->join();
	}
	catch (...)
	{
	}

	return EXIT_SUCCESS;
}*/
int times = 0;

void OnReceive(const boost::system::error_code &err, client::StatusCode sc, client::CMessage::Ptr msg)
{
	std::cout << "the error code is " << err.value() << std::endl;
	std::cout << "the message is " << "\"" << msg->body() << "\"" << std::endl;
	std::cout << "the times is \"" << times++ << "\"" << std::endl;
}

void OnSendComplete(const boost::system::error_code &err, client::CMessage::Ptr msg)
{
	std::cout << err.value() << std::endl;
	std::cout << "OnSendComplete" << msg->body() << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		printf ( "Hello, world!\n" );

		std::string host = "localhost";
		std::string port = "60001";

		client::CClient cli;

		cli.Init( host, port, boost::bind(&OnReceive, _1, _2, _3), 1, 3);

		while (true)
		{
			getchar();
			char line[client::CMessage::max_body_length + 1] = "demo";
			
			boost::shared_ptr<client::CMessage> msg(new client::CMessage());
			msg->body_length(strlen(line));
			memcpy(msg->body(), line, msg->body_length());
			msg->encode_header();

			cli.PostSend(msg, boost::bind(&OnSendComplete, _1, _2));
			Sleep( 10 );
		}

	}
	catch ( std::exception& e )
	{
		std::cout << "Exception :" << e.what() << std::endl;
	}
	return EXIT_SUCCESS;
}