/****************************************
 * class	: CMessage
 * author	: http://www.boost.org/doc/libs/1_56_0/doc/html/boost_asio/example/cpp03/chat/chat_message.hpp
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/
#ifndef __CMESSAGE_HPP__
#define __CMESSAGE_HPP__

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace client
{
	class CConnection;

	class CMessage
		:private boost::noncopyable
	{
	public:
		friend class CConnection;
		typedef boost::shared_ptr<CMessage> Ptr;

		enum { header_length = 4 };
		enum { max_body_length = 512 };

		CMessage()
			: body_length_(0)
		{
			::memset(data_, 0, sizeof(data_));
		}

		const char* data() const
		{
			return data_;
		}

		char* data()
		{
			return data_;
		}

		size_t length() const
		{
			return header_length + body_length_;
		}

		const char* body() const
		{
			return data_ + header_length;
		}

		char* body()
		{
			return data_ + header_length;
		}

		size_t body_length() const
		{
			return body_length_;
		}

		void body_length(size_t new_length)
		{
			body_length_ = new_length;
			if (body_length_ > max_body_length)
				body_length_ = max_body_length;
		}

		bool decode_header()
		{
			using namespace std; // For strncat and atoi.
			char header[header_length + 1] = "";
			strncat(header, data_, header_length);
			body_length_ = atoi(header);
			if (body_length_ > max_body_length)
			{
				body_length_ = 0;
				return false;
			}
			return true;
		}

		void encode_header()
		{
			using namespace std; // For sprintf and memcpy.
			char header[header_length + 1] = "";
			sprintf(header, "%4d", body_length_);
			memcpy(data_, header, header_length);
		}

	private:
		char data_[header_length + max_body_length];
		size_t body_length_;
	};
}
#endif // CHAT_MESSAGE_HPP