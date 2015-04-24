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

void OnSendComplete(const boost::system::error_code &err, CMsgBuffer::Ptr msg)
{
	std::cout << "ErrorCode" << err.value() << std::endl;
	if (err.value() == boost::asio::error::not_connected)
	{
		cli.Reset();
		return;
	}
	std::cout << "OnSendComplete" << std::endl;
	std::cout << "OnSendComplete times \"" << times++ << "\"" <<std::endl;
}

void OnReceive(const boost::system::error_code &err, client::StatusCode sc, CMsgBuffer::Ptr msg)
{
	std::cout << "the error code is " << err.value() << std::endl;
	std::cout << "the times is \"" << times++ << "\"" << std::endl;

	if (!err && sc == client::SC_ReadBody/*&& msg->body()[0]*/)
	{
		//CMsgBuffer::Ptr request(new CMsgBuffer());
		char str[128];

		int size = msg->ReadLong();
		strncpy(str, msg->ReadString(), sizeof(str));
		std::cout << "string: "<< str << std::endl;
		char b = msg->ReadByte();
		bool result = (b != -1) ? b : false;
		std::cout << "bool:"<< result << std::endl;
		int l = msg->ReadLong();
		std::cout << "long:" << l << std::endl;
		float f = msg->ReadFloat();
		std::cout << "float:" << f << std::endl;
		

		/*request->WriteString(str);
		request->WriteByte(result);
		request->WriteLong(l);
		request->WriteFloat(f);
		request->WriteString(str);

		cli.PostSend(request, boost::bind(&OnSendComplete, _1, _2));*/
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		printf ( "Hello, Boost !!!\n" );

		std::string host = "localhost";
		std::string port = "81";

		cli.Init( host, port, boost::bind(&OnReceive, _1, _2, _3), 1, 1);

		while (true)
		{
			getchar();
			if (cli.IsConnected())
			{
				char line[8] = "demo";

				CMsgBuffer::Ptr msg(new CMsgBuffer());

				msg->WriteString(line);
				msg->WriteByte(true);
				msg->WriteLong(10);
				msg->WriteFloat(12.0);
				msg->WriteString(line);
				msg->WriteEnd();

				cli.PostSend(msg, boost::bind(&OnSendComplete, _1, _2));
			}
			Sleep( 10 );
		}

	}
	catch ( std::exception& e )
	{
		std::cout << "Exception :" << e.what() << std::endl;
	}
	return EXIT_SUCCESS;
}