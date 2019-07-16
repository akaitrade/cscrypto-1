#include <client.hpp>

#include <QtNetwork>

namespace cscrypto {
namespace gui {

Client::Client(QObject* parent)
        : QObject(parent),
          socket_(new QTcpSocket(this)),
          requestMaster_(false) {
    connect(socket_, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Client::socketErrorHandler);
}

void Client::sendKeyExchangeRequest(QString hostName, quint16 serverPort, const KeyPair& ownKeys) {
    socket_->abort();
    socket_->connectToHost(hostName, serverPort);
    requestMaster_.setOwnKeys(ownKeys);
    cscrypto::Bytes request = requestMaster_.form(RequestMaster::RequestType::KeyExchangeQuery);
    socket_->write(reinterpret_cast<char*>(&request), qint64(request.size()));
}

void Client::onReadyRead() {
    if (serverReqType_ == RequestMaster::Unknown) {
        if (socket_->bytesAvailable() < qint64(sizeof(serverReqType_))) {
            return;
        }
        socket_->read(reinterpret_cast<char*>(&serverReqType_), qint64(sizeof(serverReqType_)));
        if (serverReqType_ == RequestMaster::Unknown) {
            socket_->abort();
            emit error(tr("Invalid data from server."));
            return;
        }
    }

    int numBytesToReceive = RequestMaster::requestSize(serverReqType_);
    if (socket_->bytesAvailable() < numBytesToReceive) {
        return;
    }

    cscrypto::Bytes serverReplyBytes;
    serverReplyBytes.resize(size_t(numBytesToReceive));
    socket_->read(reinterpret_cast<char*>(serverReplyBytes.data()), numBytesToReceive);

    if (!requestMaster_.validate(serverReqType_, serverReplyBytes)) {
        emit error(tr("Invalid data from server."));
        return;
    }
}

void Client::socketErrorHandler(QAbstractSocket::SocketError errorType) {
    switch (errorType) {
        case QAbstractSocket::RemoteHostClosedError :
            break;
        case QAbstractSocket::HostNotFoundError :
            emit error(tr("Host not found. Please check host name."));
            break;
        case QAbstractSocket::ConnectionRefusedError :
            emit error(tr("The connection refused by the peer. "
                          "Make sure that the server is running and check host name."));
            break;
        default:
            emit error(tr("The following error occured: ") + socket_->errorString());
    }
}


} // namespace gui
} // namespace cscrypto