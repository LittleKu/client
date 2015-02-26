/****************************************
 * class	: CMessage
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/
#ifndef __CMESSAGE_H__
#define __CMESSAGE_H__

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>


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
		enum { max_body_length = 8188 };

		CMessage();

		const unsigned char* GetData() const;
		size_t GetLength() const;

		/********************** Read ****************************/
		void			ReadBegin();
		void			ReadEnd();
		unsigned char	ReadByte();
		char*			ReadString();
		int				ReadShort();
		int				ReadLong();
		float			ReadFloat();
		bool			ReadBuf(size_t nSize, void *pBuf);

		/********************** Write ****************************/
		void	WriteBegin();
		void	WriteEnd();
		bool	WriteString(const char *str);
		bool	WriteLong(int c);
		bool	WriteFloat(float f);
		bool	WriteShort(int val);
		bool	WriteByte(unsigned char val);
		bool	WriteBuf(size_t nSize, void *pBuf);

	protected:

		unsigned char*	GetData(bool bReset);
		unsigned char*	GetBody();
		size_t			GetBodyLength() const;
		void			SetBodyLength(size_t new_length);
		bool			DecodeHeader();

	private:
		unsigned char m_rgData[header_length + max_body_length];
		size_t m_nBodyLength;
		size_t m_nCursor;

	private:
		bool	CheckSpace(size_t nSize);
		void	*GetSpace(size_t length);
		bool	Write(const void *pData, size_t nSize);
	};
}
#endif