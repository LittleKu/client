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
client::CClient cli;

void OnSendComplete(const boost::system::error_code &err, client::CMessage::Ptr msg)
{
	std::cout << err.value() << std::endl;
	std::cout << "OnSendComplete" << std::endl;
	std::cout << "OnSendComplete times \"" << times++ << "\"" <<std::endl;
}

void OnReceive(const boost::system::error_code &err, client::StatusCode sc, client::CMessage::Ptr msg)
{
	std::cout << "the error code is " << err.value() << std::endl;
	std::cout << "the times is \"" << times++ << "\"" << std::endl;

	if (!err && sc == client::SC_ReadBody/*&& msg->body()[0]*/)
	{
		boost::shared_ptr<client::CMessage> request(new client::CMessage());
		char str[client::CMessage::max_body_length];

		msg->ReadBegin();
		strncpy(str, msg->ReadString(), sizeof(str));
		std::cout << "string: "<< str << std::endl;
		char b = msg->ReadByte();
		bool result = (b != -1) ? b : false;
		std::cout << "bool:"<< result << std::endl;
		int l = msg->ReadLong();
		std::cout << "long:" << l << std::endl;
		float f = msg->ReadFloat();
		std::cout << "float:" << f << std::endl;
		msg->ReadEnd();
		

		request->WriteBegin();
		request->WriteString(str, strlen(str) + 1);
		request->WriteByte(result);
		request->WriteLong(l);
		request->WriteEnd();
		cli.PostSend(request, boost::bind(&OnSendComplete, _1, _2));
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		printf ( "Hello, world!\n" );

		std::string host = "localhost";
		std::string port = "60000";

		cli.Init( host, port, boost::bind(&OnReceive, _1, _2, _3), 1, 3);

		while (true)
		{
			getchar();
			char line[8] = "demo";

			boost::shared_ptr<client::CMessage> msg(new client::CMessage());
			
			msg->WriteBegin();
			msg->WriteString(line, strlen(line) + 1);
			msg->WriteByte(true);
			msg->WriteLong(10);
			msg->WriteFloat(12.0);
			msg->WriteEnd();

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