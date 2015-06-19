/****************************************
 * class	: CClient
 * author	: LittleKu (L.K)
 * email	: kklvzl@gmail.com
 * date		: 09-02-2014
 ****************************************/

#ifndef __CLIENT_H__
#define __CLIENT_H__

#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

#include "ClientImpl.h"

namespace client
{
	class CClient
		:private boost::noncopyable
	{
	public:
		CClient();
		~CClient();

		/************************************************************************/
		/*  @brief 初始化连接池并设置接收回调                                   */
		/*  @param _In_ host 连接主机的域名或IP地址                             */
		/*  @param _In_ port 连接主机的端口                                     */
		/*  @param _In_ cb 连接回调函数                                         */
		/*  @param _In_ thread_count 线程个数                                   */
		/*  @param _In_ connection_count 连接个数                               */
		/*  @param _In_ connection_limit 最小连接个数                           */
		/************************************************************************/
		void Init(const std::string &host, const std::string &port, cb_InitConnection cb, int thread_count = 1, int connection_count = 1, int connection_limit = 5);

		/************************************************************************/
		/*  @brief 停止一切连接                                                 */
		/************************************************************************/
		void Stop();

		/************************************************************************/
		/*  @brief 投递一个发送操作                                             */
		/*  @param _In_ msg 指向要发送的一个数据指针                            */
		/*  @param _In_ cb 发送回调函数                                         */
		/************************************************************************/
		void PostSend(CMsgBuffer::Ptr msg, cb_Request cb);

		/************************************************************************/
		/*  @brief 重新初始化                                                   */
		/************************************************************************/
		void Reset();

		/************************************************************************/
		/*  @brief 连接池中是否有有效的连接对象可用                             */
		/*  @return 返回true则代表连接成功，否则连接失败                        */
		/************************************************************************/
		bool IsConnected();
	private:
		boost::shared_ptr<CClientImpl> m_pClientImpl;/**< 连接*/
		cb_InitConnection m_cbInit;/**< 初始化回调函数*/
		std::string m_host;/**< 主机域名或IP地址*/
		std::string m_port;/**< 主机端口*/
	};
}

#endif