#include "Message.h"

namespace client
{
	CMessage::CMessage()
		: m_nBodyLength(0)
		, m_nCursor(0)
	{
		::memset(m_rgData, 0, sizeof(m_rgData));
	}

	const unsigned char* CMessage::GetData() const
	{
		return m_rgData;
	}

	size_t CMessage::GetLength() const
	{
		return header_length + m_nBodyLength;
	}

	void CMessage::ReadBegin()
	{
		DecodeHeader();
		m_nCursor = header_length;
	}

	void CMessage::ReadEnd()
	{
		m_nCursor = 0;
	}

	unsigned char CMessage::ReadByte()
	{
		unsigned char c;

		if (m_nCursor + 1 > m_nBodyLength + header_length)
			return -1;

		c = (unsigned char)m_rgData[m_nCursor];
		m_nCursor += 1;
		return c;
	}

	char *CMessage::ReadString()
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

	int CMessage::ReadShort()
	{
		int c;
		if (m_nCursor + 2 > m_nBodyLength + header_length)
			return -1;

		c = (short)(m_rgData[m_nCursor] + (m_rgData[m_nCursor + 1] << 8));
		m_nCursor += 2;
		return c;
	}

	int CMessage::ReadLong()
	{
		int c;
		if (m_nCursor + 4 > m_nBodyLength + header_length)
			return -1;

		c = m_rgData[m_nCursor] + (m_rgData[m_nCursor + 1] << 8) + (m_rgData[m_nCursor + 2] << 16) + (m_rgData[m_nCursor + 3] << 24);
		m_nCursor += 4;
		return c;
	}

	float CMessage::ReadFloat()
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

	bool CMessage::ReadBuf(size_t nSize, void *pBuf)
	{
		if (m_nCursor + nSize > m_nBodyLength + header_length)
			return false;

		::memcpy(pBuf, &m_rgData[m_nCursor], nSize);
		return true;
	}

	void CMessage::WriteBegin()
	{
		m_nCursor = header_length;
	}

	void CMessage::WriteEnd()
	{
		SetBodyLength(m_nCursor - header_length);

		memcpy(m_rgData, &m_nBodyLength, header_length);

		m_nCursor = 0;
	}

	bool CMessage::WriteString(const char *str)
	{
		if (!str)
			return Write("", 1);
		return Write(str, strlen(str) + 1);
	}

	bool CMessage::WriteLong(int c)
	{
		unsigned char *b = NULL;

		b = (unsigned char *)GetSpace(4);
		if (b != NULL)
		{
			b[0] = c & 0xff;
			b[1] = (c >> 8) & 0xff;
			b[2] = (c >> 16) & 0xff;
			b[3] = c >> 24;
			return true;
		}
		return false;
	}

	bool CMessage::WriteFloat(float f)
	{
		union
		{
			float f;
			int l;
		}dat;

		dat.f = f;
		return Write(&dat.l, 4);
	}

	bool CMessage::WriteShort(int val)
	{
		unsigned char *b = NULL;
		b = (unsigned char *)GetSpace(2);
		if (b != NULL)
		{
			b[0] = (val & 0xFF);
			b[1] = (val >> 8);
			return true;
		}
		return false;
	}

	bool CMessage::WriteByte(unsigned char val)
	{
		unsigned char *b = NULL;
		b = (unsigned char *)GetSpace(1);
		if (b != NULL)
		{
			b[0] = val;
			return true;
		}
		return false;
	}

	bool CMessage::WriteBuf(size_t nSize, void *pBuf)
	{
		if (!pBuf)
			return false;

		return Write(pBuf, nSize);
	}

	unsigned char* CMessage::GetData(bool bReset)
	{
		if (bReset)
			::memset(m_rgData, 0, sizeof(m_rgData));
		return m_rgData;
	}

	unsigned char* CMessage::GetBody()
	{
		return m_rgData + header_length;
	}

	size_t CMessage::GetBodyLength() const
	{
		return m_nBodyLength;
	}

	void CMessage::SetBodyLength(size_t new_length)
	{
		m_nBodyLength += new_length;
		if (m_nBodyLength > max_body_length)
			m_nBodyLength = max_body_length;
	}

	bool CMessage::DecodeHeader()
	{
		::memcpy(&m_nBodyLength, m_rgData, header_length);

		if (m_nBodyLength > max_body_length)
		{
			m_nBodyLength = 0;
			return false;
		}
		return true;
	}

	bool CMessage::CheckSpace(size_t nSize)//¼ì²éÐ´¿Õ¼äÊÇ·ñ×ã¹»
	{
		if ((max_body_length + header_length) - m_nCursor < nSize)
			return false;

		return true;
	}

	void *CMessage::GetSpace(size_t length)
	{
		void *d;

		if (!CheckSpace(length))
			return NULL;

		d = m_rgData + m_nCursor;
		m_nCursor += length;
		return d;
	}

	bool CMessage::Write(const void *pData, size_t nSize)
	{
		unsigned char *b = NULL;
		b = (unsigned char*)GetSpace(nSize);
		if (b != NULL)
		{
			::memcpy(b, pData, nSize);
			return true;
		}
		return false;
	}
}