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
			: m_nBodyLength(0)
			, m_nCursor(0)
		{
			::memset(m_rgData, 0, sizeof(m_rgData));
		}

		const char* GetData() const
		{
			return m_rgData;
		}

		size_t GetLength() const
		{
			return header_length + m_nBodyLength;
		}

		void ReadBegin()
		{
			DecodeHeader();
			m_nCursor = header_length;
		}

		void ReadEnd()
		{
			m_nCursor = 0;
		}

		unsigned char ReadByte()
		{
			unsigned char c;

			if (m_nCursor + 1 > m_nBodyLength + header_length)
				return -1;

			c = (unsigned char)m_rgData[m_nCursor];
			m_nCursor += 1;
			return c;
		}

		char *ReadString()
		{
			static char str[max_body_length];
			int l, c;
			l = 0;

			do
			{
				c = (char)ReadByte();
				if (c == -1 || c == 0)
					break;

				str[l] = c;
				l++;
			}
			while (l < sizeof(str) - 1);

			str[l] = 0;
			return str;
		}

		int ReadShort()
		{
			int c;
			if (m_nCursor + 2 > m_nBodyLength + header_length)
				return -1;

			::memcpy(&c, &m_rgData[m_nCursor], 2);
			m_nCursor += 2;
			return c;
		}

		int ReadLong()
		{
			int c;
			if (m_nCursor + 4 > m_nBodyLength + header_length)
				return -1;

			::memcpy(&c, &m_rgData[m_nCursor], 4);
			m_nCursor += 4;
			return c;
		}

		float ReadFloat()
		{
			union
			{
				unsigned char b[4];
				float f;
			}dat;

			dat.b[0] = m_rgData[m_nCursor];
			dat.b[1] = m_rgData[m_nCursor + 1];
			dat.b[2] = m_rgData[m_nCursor + 2];
			dat.b[3] = m_rgData[m_nCursor + 3];

			m_nCursor += 4;
			return dat.f;
		}

		/***********************************************************/
		void WriteBegin()
		{
			m_nCursor = header_length;
		}

		void WriteEnd()
		{
			SetBodyLength(m_nCursor - header_length);

			char header[header_length + 1] = "";
			sprintf(header, "%4d", m_nBodyLength);
			memcpy(m_rgData, header, header_length);

			m_nCursor = 0;
		}

		bool WriteString(const char *str, size_t nSize)
		{
			if (str == NULL)
				return false;

			return Write((void*)str, nSize);
		}

		bool WriteLong(int val)
		{
			return Write( &val, 4);
		}

		bool WriteFloat(float val)
		{
			union
			{
				float f;
				int l;
			}dat;

			dat.f = val;
			return Write(&dat.l, 4);
		}

		bool WriteShort(short val)
		{
			return Write(&val, 2);
		}

		bool WriteByte(unsigned char val)
		{
			return Write((void*)&val, 1);
		}
		
		/*void operator = (const CMessage &srcMsg)
		{
			::memcpy(this->m_rgData, srcMsg.GetData(), srcMsg.GetLength());
			this->m_nBodyLength = srcMsg.GetBodyLength();
		}*/

	protected:

		char* GetData(bool bReset)
		{
			if (bReset)
				::memset(m_rgData, 0, sizeof(m_rgData));
			return m_rgData;
		}

		char* GetBody()
		{
			return m_rgData + header_length;
		}

		size_t GetBodyLength() const
		{
			return m_nBodyLength;
		}

		void SetBodyLength(size_t new_length)
		{
			m_nBodyLength += new_length;
			if (m_nBodyLength > max_body_length)
				m_nBodyLength = max_body_length;
		}

		bool DecodeHeader()
		{
			char header[header_length + 1] = "";
			::memcpy(header, m_rgData, header_length);
			m_nBodyLength = atoi(header);
			if (m_nBodyLength > max_body_length)
			{
				m_nBodyLength = 0;
				return false;
			}
			return true;
		}

	private:
		char m_rgData[header_length + max_body_length];
		size_t m_nBodyLength;
		size_t m_nCursor;
	private:
		bool CheckSpace(size_t nSize)//¼ì²éÐ´¿Õ¼äÊÇ·ñ×ã¹»
		{
			if (max_body_length - m_nCursor < nSize)
				return false;

			return true;
		}

		bool Write(const void *pData, size_t nSize)
		{
			if (!CheckSpace(nSize))
				return false;

			::memcpy(&m_rgData[m_nCursor], pData, nSize);
			m_nCursor += nSize;
			return true;
		}
	};
}
#endif // CHAT_MESSAGE_HPP