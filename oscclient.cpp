#include "oscclient.h"
#include <QtEndian>
#include <QDebug>
#include <QNetworkInterface>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <cstring>

// ---- utils ----
static inline QByteArray pad4(const QByteArray& in) {
    QByteArray out = in;
    int pad = (4 - (out.size() % 4)) % 4;
    if (pad) out.append(QByteArray(pad, '\0'));
    return out;
}
static inline QString twoDigits(int n) {
    return QString("%1").arg(n, 2, 10, QChar('0'));
}

// ---- ctor / destino ----
OscClient::OscClient(QObject* parent) : QObject(parent) {
    connect(&m_sock, &QUdpSocket::readyRead, this, &OscClient::onReadyRead);
    connect(&m_keepAlive, &QTimer::timeout, this, &OscClient::sendXRemote);
}
void OscClient::setTarget(const QHostAddress& addr, quint16 port) { m_addr = addr; m_port = port; }
QHostAddress OscClient::targetAddress() const { return m_addr; }
quint16      OscClient::targetPort()   const { return m_port; }

// ---- open/bind ----
bool OscClient::open(quint16 localPort) {
    if (m_sock.state() == QAbstractSocket::BoundState) return true;
    bool ok = m_sock.bind(QHostAddress::AnyIPv4, localPort, QUdpSocket::ShareAddress);
    if (!ok) emit error(QStringLiteral("Falha no bind UDP: %1").arg(m_sock.errorString()));
    return ok;
}

// ---- keepalive ----
void OscClient::startFeedbackKeepAlive(int ms) {
    if (!m_keepAlive.isActive()) {
        m_keepAlive.start(ms);
        sendXRemote(); // dispara já
    }
}
void OscClient::stopFeedbackKeepAlive() { m_keepAlive.stop(); }

// --------- pack helpers ----------
QByteArray OscClient::packString(const QString& s) {
    QByteArray b = s.toUtf8();
    b.append('\0');
    return pad4(b);
}
QByteArray OscClient::packInt32(qint32 v) {
    quint32 be = qToBigEndian<quint32>(static_cast<quint32>(v));
    return QByteArray(reinterpret_cast<const char*>(&be), 4);
}
QByteArray OscClient::packFloat(float v) {
    quint32 bits;
    static_assert(sizeof(float)==4, "float 32 bits esperado");
    std::memcpy(&bits, &v, 4);
    bits = qToBigEndian(bits);
    return QByteArray(reinterpret_cast<const char*>(&bits), 4);
}

// --------- send ----------
bool OscClient::send(const QString& address, const QByteArray& typeTags, const QList<QByteArray>& args) {
    if (m_addr.isNull()) { emit error("Endereço do mixer não configurado"); return false; }

    QByteArray pkt;
    pkt += packString(address);
    pkt += packString("," + typeTags);  // typetags sempre iniciam com ','

    for (const auto& a : args)
        pkt += (a.size() % 4 == 0) ? a : pad4(a);

    auto sent = m_sock.writeDatagram(pkt, m_addr, m_port);
    if (sent != pkt.size()) {
        emit error(QStringLiteral("Envio OSC incompleto (%1/%2)").arg(sent).arg(pkt.size()));
        return false;
    }
    return true;
}

void OscClient::sendXRemote() {
    send(QStringLiteral("/xremote"), QByteArray(), {});
}

void OscClient::queryName() { send(QStringLiteral("/xinfo"), QByteArray(), {}); }

void OscClient::setChannelMute(int ch, bool on) {
    ch = qBound(1, ch, 32);
    const QString path = QStringLiteral("/ch/%1/mix/on").arg(twoDigits(ch));
    send(path, "i", { packInt32(on ? 1 : 0) });
}
void OscClient::setChannelFader(int ch, float v01) {
    ch = qBound(1, ch, 32);
    v01 = qBound(0.0f, v01, 1.0f);
    const QString path = QStringLiteral("/ch/%1/mix/fader").arg(twoDigits(ch));
    send(path, "f", { packFloat(v01) });
}
void OscClient::setMainLRFader(float v01) {
    v01 = qBound(0.0f, v01, 1.0f);
    send(QStringLiteral("/lr/mix/fader"), "f", { packFloat(v01) });
}
void OscClient::setMainLRMute(bool on) {
    send(QStringLiteral("/lr/mix/on"), "i", { packInt32(on ? 1 : 0) });
}
void OscClient::getMainLRFader() {
    const QString path = QStringLiteral("/lr/mix/fader");
    send(path, "s", { packString("?") });
    //send(QStringLiteral(""), "s", { packString("?") });
}
void OscClient::getMainLRMute()  { send(QStringLiteral("/lr/mix/on"),    "s", { packString("?") }); }

// --------- RX helpers ----------
// CORRIGIDO: consome UM '\0' e alinha para múltiplo de 4
static QString readPaddedString(const QByteArray& buf, int& i) {
    const int start = i;
    while (i < buf.size() && buf[i] != '\0') ++i;
    QString s = QString::fromUtf8(buf.constData() + start, i - start);
    if (i < buf.size()) ++i;                 // consome o '\0'
    while (i % 4 != 0 && i < buf.size()) ++i; // padding até múltiplo de 4
    return s;
}

static bool readBlobPayload(const QByteArray& buf, int& i, QByteArray& out) {
    if (i + 4 > buf.size()) return false;
    quint32 beLen; std::memcpy(&beLen, buf.constData()+i, 4); i += 4;
    const quint32 blen = qFromBigEndian(beLen);
    if (i + int(blen) > buf.size()) return false;
    out = QByteArray(buf.constData()+i, int(blen));
    i += int(blen);
    while (i % 4 != 0 && i < buf.size()) ++i; // padding do blob
    return true;
}

static inline bool isValidMetersBlob(const QByteArray& blob) {
    return blob.size() >= 32 && (blob.size() % 2 == 0);
}
static inline bool isValidLRBlob(const QByteArray& blob) {
    return blob.size() >= 4 && ((blob.size() % 2) == 0);
}

// Aceita tipos: i, f, s, T, F, b
void OscClient::parseDatagram(const QByteArray& d) {
    int i = 0;

    // ---------- bundle ----------
    if (d.size() >= 16 && d.left(8) == QByteArray("#bundle\0", 8)) {
        i = 16; // "#bundle\0"(8) + timetag(8)
        while (i + 4 <= d.size()) {
            quint32 beLen; std::memcpy(&beLen, d.constData()+i, 4); i += 4;
            const quint32 len = qFromBigEndian(beLen);
            if (i + int(len) > d.size()) break;

            QByteArray elem = d.mid(i, int(len)); i += int(len);

            int j = 0;
            QString addr = readPaddedString(elem, j);
            if (addr.isEmpty()) continue;
            QString tags = readPaddedString(elem, j);

            QVariantList args;
            bool hadBlob = false;
            QByteArray blob;

            if (tags.startsWith(',')) {
                for (int k = 1; k < tags.size(); ++k) {
                    const char t = tags[k].toLatin1();
                    if (t == 'i' && j + 4 <= elem.size()) {
                        quint32 be; std::memcpy(&be, elem.constData()+j, 4); j += 4;
                        be = qFromBigEndian(be);
                        qint32 v; std::memcpy(&v, &be, 4);
                        args << int(v);
                    } else if (t == 'f' && j + 4 <= elem.size()) {
                        quint32 be; std::memcpy(&be, elem.constData()+j, 4); j += 4;
                        be = qFromBigEndian(be);
                        float f; std::memcpy(&f, &be, 4);
                        args << f;
                    } else if (t == 's') {
                        args << readPaddedString(elem, j);
                    } else if (t == 'T') {
                        args << true;
                    } else if (t == 'F') {
                        args << false;
                    } else if (t == 'b') {
                        if (readBlobPayload(elem, j, blob)) hadBlob = true;
                    }
                }
            }

            if (addr == "/meters/1") {
                if (hadBlob && isValidMetersBlob(blob)) {
                    args.clear(); args << blob;
                    emit oscMessageReceived(addr, args);
                } else {
                    //qDebug() << "[RX bundle] /meters/1 ignorado (tags=" << tags << ", size=" << blob.size() << ")";
                }
                continue;
            }

            if (addr == "/meters/3") {
                //qDebug() << "[RX bundle] /meters/3 tags=" << tags;
                if (hadBlob && isValidLRBlob(blob)) {
                    args.clear(); args << blob;
                    emit oscMessageReceived(addr, args);
                } else {
                    //qDebug() << "[RX bundle] /meters/3 ignorado (tags=" << tags << ", size=" << blob.size() << ")";
                }
                continue;
            }

            emit oscMessageReceived(addr, args);
        }
        return;
    }

    // ---------- mensagem simples ----------
    QString address = readPaddedString(d, i);
    if (address.isEmpty()) return;
    QString tags = readPaddedString(d, i);

    QVariantList args;
    bool hadBlob = false;
    QByteArray blob;

    if (tags.startsWith(',')) {
        for (int ti = 1; ti < tags.size(); ++ti) {
            const char t = tags[ti].toLatin1();
            if (t == 'i' && i + 4 <= d.size()) {
                quint32 be; std::memcpy(&be, d.constData()+i, 4); i += 4;
                be = qFromBigEndian(be);
                qint32 v; std::memcpy(&v, &be, 4);
                args << int(v);
            } else if (t == 'f' && i + 4 <= d.size()) {
                quint32 be; std::memcpy(&be, d.constData()+i, 4); i += 4;
                be = qFromBigEndian(be);
                float f; std::memcpy(&f, &be, 4);
                args << f;
            } else if (t == 's') {
                args << readPaddedString(d, i);
            } else if (t == 'T') {
                args << true;
            } else if (t == 'F') {
                args << false;
            } else if (t == 'b') {
                if (readBlobPayload(d, i, blob)) hadBlob = true;
            }
        }
    }

    if (address == "/meters/1") {
        if (hadBlob && isValidMetersBlob(blob)) {
            args.clear(); args << blob;
            emit oscMessageReceived(address, args);
        } else {
            //qDebug() << "[RX msg] /meters/1 ignorado (tags=" << tags << ", size=" << blob.size() << ")";
        }
        return;
    }

    if (address == "/meters/3") {
        //qDebug() << "[RX msg] /meters/3 tags=" << tags;
        if (hadBlob && isValidLRBlob(blob)) {
            args.clear(); args << blob;
            emit oscMessageReceived(address, args);
        } else {
            //qDebug() << "[RX msg] /meters/3 ignorado (tags=" << tags << ", size=" << blob.size() << ")";
        }
        return;
    }

    emit oscMessageReceived(address, args);
}

void OscClient::onReadyRead() {
    while (m_sock.hasPendingDatagrams()) {
        QByteArray d;
        d.resize(int(m_sock.pendingDatagramSize()));
        QHostAddress from; quint16 port;
        m_sock.readDatagram(d.data(), d.size(), &from, &port);
        Q_UNUSED(from)
        Q_UNUSED(port)
        parseDatagram(d);
    }
}

// ==============================
// Descoberta (broadcast/unicast)
// ==============================

QHostAddress OscClient::discoverMixer(int timeoutMs) {
    QUdpSocket sock;
    if (!sock.bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress))
        return QHostAddress::Any;

    auto buildNoArg = [](const QString& addr){
        QByteArray pkt; pkt += OscClient::packString(addr);
        pkt += OscClient::packString(","); // sem args
        return pkt;
    };
    const QByteArray probe = buildNoArg(QStringLiteral("/-prefs/name"));

    sock.writeDatagram(probe, QHostAddress::Broadcast, 10024);
    for (const auto& ni : QNetworkInterface::allInterfaces()) {
        if (!(ni.flags() & QNetworkInterface::IsUp) ||
            !(ni.flags() & QNetworkInterface::IsRunning) ||
            (ni.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto& e : ni.addressEntries()) {
            if (e.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            if (!e.broadcast().isNull())
                sock.writeDatagram(probe, e.broadcast(), 10024);
        }
    }

    QHostAddress found;
    QElapsedTimer t; t.start();
    while (t.elapsed() < timeoutMs) {
        if (!sock.waitForReadyRead(qMin(50, timeoutMs - int(t.elapsed())))) continue;
        while (sock.hasPendingDatagrams()) {
            QByteArray d; d.resize(int(sock.pendingDatagramSize()));
            QHostAddress from; quint16 port;
            sock.readDatagram(d.data(), d.size(), &from, &port);
            Q_UNUSED(port)
            int idx = 0;
            auto readStr = [](const QByteArray& buf, int& i)->QString{
                int s = i; while (i < buf.size() && buf[i] != '\0') ++i;
                QString r = QString::fromUtf8(buf.constData()+s, i-s);
                if (i < buf.size()) ++i;
                while (i % 4 != 0 && i < buf.size()) ++i;
                return r;
            };
            if (readStr(d, idx) == "/-prefs/name" || idx==0 /*sanity*/) {
                found = from; return found;
            }
        }
    }
    return QHostAddress::Any;
}

// --- helpers locais para range ---
static bool parseCidr(const QString& cidr, quint32& base, int& prefix) {
    const auto rx = QRegularExpression(R"(^\s*(\d{1,3}(?:\.\d{1,3}){3})/(\d{1,2})\s*$)");
    auto m = rx.match(cidr);
    if (!m.hasMatch()) return false;
    QHostAddress ip(m.captured(1));
    bool ok = false; int p = m.captured(2).toInt(&ok);
    if (!ok || p < 0 || p > 32) return false;
    base = ip.toIPv4Address();
    prefix = p;
    return true;
}
static inline QHostAddress u32ToIp(quint32 v){ return QHostAddress(v); }

static QByteArray buildOscNoArg(const QString& address) {
    QByteArray pkt; pkt += OscClient::packString(address);
    pkt += OscClient::packString(","); // sem args
    return pkt;
}

QHostAddress OscClient::discoverMixerRange(const QString& cidr, int timeoutMs) {
    quint32 base; int prefix;
    if (!parseCidr(cidr, base, prefix)) {
        qWarning() << "CIDR inválido:" << cidr;
        return QHostAddress();
    }

    quint32 mask = (prefix == 0) ? 0 : 0xFFFFFFFFu << (32 - prefix);
    quint32 network = base & mask;
    quint32 broadcast = network | (~mask);
    quint32 first = (network == 0x00000000u && prefix == 0) ? 1 : network + 1;
    quint32 last  = (broadcast == 0xFFFFFFFFu && prefix == 32) ? 0xFFFFFFFFu : broadcast - 1;

    QUdpSocket sock;
    if (!sock.bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress)) {
        qWarning() << "discoverMixerRange: bind falhou:" << sock.errorString();
        return QHostAddress();
    }

    const QByteArray probeXInfo = buildOscNoArg(QStringLiteral("/xinfo"));
    const QByteArray probeName  = buildOscNoArg(QStringLiteral("/-prefs/name"));

    for (quint32 ip = first; ip <= last; ++ip) {
        const QHostAddress dst = u32ToIp(ip);
        sock.writeDatagram(probeXInfo, dst, 10024);
        sock.writeDatagram(probeName,  dst, 10024);
        if (ip == 0xFFFFFFFFu) break;
    }

    QHostAddress found;
    QElapsedTimer t; t.start();
    while (t.elapsed() < timeoutMs) {
        if (sock.waitForReadyRead(qMin(50, timeoutMs - int(t.elapsed())))) {
            while (sock.hasPendingDatagrams()) {
                QByteArray d; d.resize(int(sock.pendingDatagramSize()));
                QHostAddress from; quint16 port;
                sock.readDatagram(d.data(), d.size(), &from, &port);
                Q_UNUSED(port)

                int idx = 0;
                auto readStr = [](const QByteArray& buf, int& i)->QString{
                    int s = i; while (i < buf.size() && buf[i] != '\0') ++i;
                    QString r = QString::fromUtf8(buf.constData()+s, i-s);
                    if (i < buf.size()) ++i;
                    while (i % 4 != 0 && i < buf.size()) ++i;
                    return r;
                };
                QString addr = readStr(d, idx);

                if (addr == "/xinfo" || addr == "/-prefs/name" || addr.startsWith("/ch/") || addr == "/xremote") {
                    found = from; return found;
                }
            }
        }
    }
    return QHostAddress();
}

QHostAddress OscClient::discoverOnAllIfaces(int timeoutMs) {
    const auto ifs = QNetworkInterface::allInterfaces();
    for (const auto& ni : ifs) {
        if (!(ni.flags() & QNetworkInterface::IsUp) ||
            !(ni.flags() & QNetworkInterface::IsRunning) ||
            (ni.flags() & QNetworkInterface::IsLoopBack)) continue;

        for (const auto& e : ni.addressEntries()) {
            if (e.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            quint32 mask = e.netmask().toIPv4Address();
            if (!mask) continue;

            int prefix = 0; quint32 m = mask;
            while (m & 0x80000000u) { ++prefix; m <<= 1; }

            QString cidr = QString("%1/%2").arg(e.ip().toString()).arg(prefix);
            QHostAddress found = discoverMixerRange(cidr, timeoutMs);
            if (!found.isNull()) return found;
        }
    }
    return QHostAddress();
}

bool OscClient::setTargetFromDiscovery(const QString& cidrOrEmpty, int timeoutMs) {
    QHostAddress found = cidrOrEmpty.isEmpty()
    ? discoverOnAllIfaces(timeoutMs)
    : discoverMixerRange(cidrOrEmpty, timeoutMs);

    if (found.isNull()) return false;
    setTarget(found, 10024);
    return true;
}

// --------- GET helpers / sync ---------
void OscClient::getChannelFader(int ch) {
    ch = qBound(1, ch, 32);
    const QString path = QStringLiteral("/ch/%1/mix/fader").arg(twoDigits(ch));
    send(path, "s", { packString("?") });
}
void OscClient::getChannelMute(int ch) {
    ch = qBound(1, ch, 32);
    const QString path = QStringLiteral("/ch/%1/mix/on").arg(twoDigits(ch));
    send(path, "s", { packString("?") });
}
void OscClient::syncAll(int channels) {
    startFeedbackKeepAlive(5000);
    channels = qBound(1, channels, 32);
    for (int ch = 1; ch <= channels; ++ch) {
        getChannelFader(ch);
        getChannelMute(ch);
    }
    getMainLRFader();
    getMainLRMute();
}

// --------- Meters subscribe helper ----------
void OscClient::subscribeMetersAllChannels() {
    send(QStringLiteral("/meters"), "si",
         { packString(QStringLiteral("/meters/1")), packInt32(0) });
}
void OscClient::subscribeMetersLR() {
    send(QStringLiteral("/meters"), "si",
         { packString(QStringLiteral("/meters/3")), packInt32(0) });
}
