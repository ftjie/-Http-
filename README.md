# 用线程池实现的Http服务器
在Reactor工作模式下，主线程负责监听socket，并且实例化一个的线程池将接收到的
Http请求交给预先开辟好指定数量的线程池中；子线程负责分析处理Http请求，并且组建
一个Http应答包后将其发送到浏览器；其中线程池是基于C++11实现，采用了lambda表
达式，模板类等特性实现线程数组与任务队列，采用互斥锁与条件变量来实现线程同步避
免线程调度资源的竞态问题。其中线程池使用单例模式来避免了多次重复实例化造成的资
源浪费
