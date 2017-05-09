#include <cnet/net/TcpServer.h>

#include <cnet/base/Logging.h>
#include <cnet/net/Acceptor.h>
#include <cnet/net/EventLoop.h>
#include <cnet/net/EventLoopThreadPool.h>
#include <cnet/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace cnet;
using namespace cnet::net;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const string &nameArg,
                     Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      nextConnId_(1)

{
    acceptor_->setNewConnectionCallback(
            boost::bind(&TcpServer::newConnection, this, _1, _2)
    );
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (ConnectionMap::iterator it = connections_.begin();
            it != connections_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();
        conn->getLoop()->runInLoop(
                boost::bind(&TcpConnection::connectDestroyed, conn)
        );
        conn.reset();
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_.getAndAdd(1) == 0)
    {
        threadPool_->start(threadInitCallback_);

        assert(!acceptor_->listening());
        loop_->runInLoop(
                boost::bind(&Acceptor::listen, get_pointer(acceptor_))
        );
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    loop_->assertInLoopThread();
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnectoin [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            boost::bind(&TcpServer::removeConnection, this, _1)
    );

    ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    UNUSED(n);
    assert(n==1);
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}