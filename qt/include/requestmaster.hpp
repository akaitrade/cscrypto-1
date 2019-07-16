#ifndef CSCRYPTO_QT_REQUEST_MASTER_HPP
#define CSCRYPTO_QT_REQUEST_MASTER_HPP

#include <QObject>
#include <QString>

#include <common.hpp>

namespace cscrypto {
namespace gui {

class RequestMaster : public QObject {
    Q_OBJECT

public:
    RequestMaster(const KeyPair& ownKeys, bool serverSide = true);

    enum RequestType : uint8_t {
        KeyExchangeQuery,
        KeyExchangeReply,
        DataTransfer,
        Unknown
    };

    static int requestSize(RequestType);
    bool validateRequest(RequestType, const cscrypto::Bytes&);
    cscrypto::Bytes formReply();

signals:
    void newCommonSecretKeyPair(QString b58SendSk, QString b58ReceiveSk);

private:
    bool verifySenderPublicKey();
    bool checkRequestSignature(const cscrypto::Bytes&);

    void formCommonKeys();

    RequestType containingType_ = Unknown;

    cscrypto::PublicKey senderPubKey_;
    cscrypto::keyexchange::PubExchangeKey exchangePubKey_;
    cscrypto::Signature signature_;

    KeyPair ownKeys_;
    bool serverSide_;
};
} // namespace gui
} // namespace cscrypto
#endif // CSCRYPTO_QT_REQUEST_MASTER_HPP