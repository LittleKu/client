client
=========================================
- author: LittleKu (L.K)
- email	: kklvzl@gmail.com
=========================================


Credit
-----------------------------------------
Nagist
 
Description
-----------------------------------------
A client side connection with boost, include the connection pool
 
- [github website](https://github.com/LittleKu/client)
- [git.oschina website](https://git.oschina.net/LittleKu/client)
 
Update Log:
-----------------------------------------
- 2014/11/20	修复断开重连的BUG
- 2014/11/21	修复错误代码的传递,增加系统运行过程的流程状态识别代码(StatusCode)
- 2014/11/22	移除Readme.txt
		目前代码已测试过,基本上按照当初的设计思路走,未来计划是先完善好各行代码的注释,
		以方便初学者理解.另计划重写封包与解包的类(Message),目前的Message类只是当初简单的封装,
		单纯为了框架的测试写的
- 2014/12/18	重写CMessage类,完善整个框架
- 2015/01/12	修复CMessage类
 

注:代码注释还有待完善^_^

Off topic:
==========================================================================
- 使用协议：WTFPL
- 鉴于在天朝神马协议都被无视，甚至很多大公司都不遵守，
- 故索性采用了【DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE】协议~O(∩_∩)O~

- 代码安全性：
- 此项目为示例项目，为了方便大家编译，没用三方库，因此写了一些简单的函数，很多逻辑判断也只是保证正常操作，实际使用请自行保证代码安全~O(∩_∩)O~
==========================================================================
