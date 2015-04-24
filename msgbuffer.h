#ifndef __MSG_BUFFER_H__
#define __MSG_BUFFER_H__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

class CMsgBuffer
	:private boost::noncopyable
{
public:
	typedef boost::shared_ptr<CMsgBuffer> Ptr;
	enum
	{
		NET_MAXMESSAGE = 8192
	};

	CMsgBuffer(const char *buffername = "unnamed", void(*ef)(const char *fmt, ...) = 0);
	virtual			~CMsgBuffer(void);

	void			Clear(void);
	int				GetCurSize(void);
	int				GetMaxSize(void);
	void			*GetData(void);
	void			SetOverflow(bool allowed);
	void			BeginReading(void);
	int				GetReadCount(void);

	void			Push(void);
	void			Pop(void);

	void			WriteByte(int c);
	void			WriteShort(int c);
	void			WriteLong(int c);
	void			WriteFloat(float f);
	void			WriteString(const char *s);
	void			WriteBuf(int iSize, void *buf);
	void			WriteEnd();

	int				ReadByte(void);
	int				ReadShort(void);
	int				ReadLong(void);
	float			ReadFloat(void);
	char			*ReadString(void);
	int				ReadBuf(int iSize, void *pbuf);

	void			SetTime(float time);
	float			GetTime();

private:
	void			*GetSpace(int length);
	void			Write(const void *data, int length);

private:
	const char		*m_pszBufferName;
	void(*m_pfnErrorFunc)(const char *fmt, ...);

	int				m_nReadCount;
	int				m_nPushedCount;
	bool			m_bPushed;
	bool			m_bBadRead;
	int				m_nMaxSize;
	int				m_nCurSize;
	bool			m_bAllowOverflow;
	bool			m_bOverFlowed;
	unsigned char	m_rgData[NET_MAXMESSAGE];
	float			m_fRecvTime;
};

#endif