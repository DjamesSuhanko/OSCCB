#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QVariantList>
#include <QRegularExpression>

class OscClient : public QObject {
    Q_OBJECT
public:
    explicit OscClient(QObject* parent=nullptr);

    // Alvo (IP/porta do mixer)
    void setTarget(const QHostAddress& addr, quint16 port = 10024);
    QHostAddress targetAddress() const;
    quint16      targetPort()   const;


    // Abre/binda o socket local (porta 0 = efêmera)
    bool open(quint16 localPort = 0);

    // Keep-alive / feedback (/xremote)
    void startFeedbackKeepAlive(int ms = 5000);
    void stopFeedbackKeepAlive();

    // Chamadas comuns
    void queryName();                         // "/xinfo"
    void setChannelMute(int ch, bool on);     // "/ch/NN/mix/on"   (int 0/1)
    void setChannelFader(int ch, float v01);  // "/ch/NN/mix/fader" (float 0..1)

    // GET explícitos por canal
    void getChannelFader(int ch);
    void getChannelMute(int ch);

    // ===== Main LR (bus master) =====
    void setMainLRFader(float v01);   // "/lr/mix/fader"   (float 0..1)
    void setMainLRMute(bool on);      // "/lr/mix/on"      (int 0/1)
    void getMainLRFader();            // "/lr/mix/fader"   + "?"
    void getMainLRMute();             // "/lr/mix/on"      + "?"


    // Consulta “tudo” (ativa keepalive e pede fader/mute de 1..channels)
    void syncAll(int channels = 8);

    // Descoberta automática
    static QHostAddress discoverMixer(int timeoutMs = 2000);
    QHostAddress discoverMixerRange(const QString& cidr, int timeoutMs = 1500);
    QHostAddress discoverOnAllIfaces(int timeoutMs = 1500);
    bool setTargetFromDiscovery(const QString& cidrOrEmpty = QString(), int timeoutMs = 1500);

    // Helpers de empacotamento OSC
    static QByteArray packString(const QString& s);

    void subscribeMetersAllChannels();    // /meters/1 (ALL CHANNELS)
    void subscribeMetersLR();

    bool isOpen() const;
    void close();
    void requestStatDump();

signals:
    void oscMessageReceived(QString address, QVariantList args);
    void error(QString message);

private slots:
    void onReadyRead();
    void sendXRemote();

private:
    // Packing
    static QByteArray packInt32(qint32 v);
    static QByteArray packFloat(float v);
    bool send(const QString& address, const QByteArray& typeTags, const QList<QByteArray>& args);

    // Parsing
    void parseDatagram(const QByteArray& d);

    bool m_subMetersCh = false;
    bool m_subMetersLR = false;

private:
    QHostAddress m_addr{QHostAddress::Any};
    quint16      m_port{10024};
    QUdpSocket   m_sock;
    QTimer       m_keepAlive;
};
