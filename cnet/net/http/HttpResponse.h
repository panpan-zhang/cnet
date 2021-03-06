#ifndef CNET_HTTPRESPONSE_H
#define CNET_HTTPRESPONSE_H

#include <cnet/base/copyable.h>
#include <cnet/base/Types.h>

#include <map>

namespace cnet
{
namespace net
{
class Buffer;
class HttpResponse : public cnet::copyable
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200OK = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close)
        : statusCode_(kUnknown),
          closeConnection_(close)
    { }

    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    void setStatusMessage(const string& message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }

    void setContentType(const string& contentType)
    { addHeader("Content-Type", contentType); }

    // FIXME: replace string with StringPiece
    void addHeader(const string& key, const string& value)
    { headers_[key] = value; }

    void setBody(const string& body)
    { body_ = body; }
    
    const string& getBody()
    {
        return body_;
    }

    void appendToBuffer(Buffer *output) const;

private:
    std::map<string, string> headers_;
    HttpStatusCode statusCode_;
    // FIXME: add http version
    string statusMessage_;
    bool closeConnection_;
    string body_;
};
}
}

#endif //CNET_HTTPRESPONSE_H
