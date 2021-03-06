#ifndef CNET_TCPSERVER_H
#define CNET_TCPSERVER_H

#include <cnet/base/Atomic.h>
#include <cnet/base/Types.h>
#include <cnet/net/TcpConnection.h>
#include <cnet/base/noncopyable.h>

#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace cnet
{
namespace net
{
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

/*!
 *  TCP server, supports single-threaded and thread-pool models.
 *
 *  This is an interface class, so don't expose too much details.
 */

class TcpServer : public noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;
    enum Option{
        kNoReusePort,
        kReusePort
    };
    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const string& name,
              Option option = kNoReusePort);

    ~TcpServer();

    const string& ipPort() const { return ipPort_; }
    const string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    /*!
     *  Set the number of threads for handling input
     *
     *  Always accepts new connection in loop's thread.
     *  Must be called before @c start
     *  @param nuThreads
     *  - 0 means all I/O in loop's thread, no thread will created.
     *    this is the default value.
     *  - 1 means all I/O in another thread.
     *  - N means a thread pool with N threads, new connections
     *    are assigned on a round-robin basis.
     */
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }

    /// valid after calling start()
    boost::shared_ptr<EventLoopThreadPool> threadPool()
    { return threadPool_; }

    /*!
     *  Starts the server if it's not listenning.
     *
     *  It's harmless to call it multiple times.
     *  Thread safe.
     */
    void start();

    /*!
     *  Set connection callback
     *  Not thread safe.
     */
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd, const InetAddress& peerAddr);

    /// Thread safe
    void removeConnection(const TcpConnectionPtr& conn);
    /// Not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_; // the acceptor loop
    const string ipPort_;
    const string name_;
    boost::scoped_ptr<Acceptor> acceptor_;
    boost::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    AtomicInt32 started_;
    // alwys in loop thread
    int nextConnId_;
    ConnectionMap connections_;
};
}
}

#endif //CNET_TCPSERVER_H
