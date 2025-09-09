#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "oscclient.h"

#include <algorithm>   // std::clamp
#include <cmath>       // std::lround
#include <QtMath>
#include <QProgressBar>
#include <QTimer>
#include <climits>
#include <cstring>     // std::memset
#include <QSettings>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

// ===================== COALESCING DOS METERS (buffers + timer) =====================
static constexpr int kNumUiCh = NUMBER_OF_CHANNELS;
static int g_lastPctCh[kNumUiCh] = {0};
static int g_nextPctCh[kNumUiCh] = {0};
static int g_lastPctLR[2] = {0,0};
static int g_nextPctLR[2] = {0,0};
static QTimer* g_uiMeterTimer = nullptr;
// ================================================================================

// ======= CACHE p/ evitar reenvio redundante de mute por canal =======
static int s_lastMuteSent[NUMBER_OF_CHANNELS]; // -1=desconhecido, 0/1 = on enviado (1=unmuted)
static bool s_lastMuteInit = false;
// ===================================================================

#ifdef Q_OS_ANDROID
static void keepScreenOn(bool on) {
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (!activity.isValid()) return;

    QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
    if (!window.isValid()) return;

    const jint FLAG_KEEP_SCREEN_ON = 128; // WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
    if (on)  window.callMethod<void>("addFlags",   "(I)V", FLAG_KEEP_SCREEN_ON);
    else     window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
}
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // inicializa cache de mute enviado
    if (!s_lastMuteInit) {
        for (int i=0;i<NUMBER_OF_CHANNELS;++i) s_lastMuteSent[i] = -1;
        s_lastMuteInit = true;
    }

    inlineEdit = new QLineEdit(ui->centralwidget);
    inlineEdit->hide();
    inlineEdit->setObjectName("InlineTitleEdit");
    inlineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
    inlineEdit->setFocusPolicy(Qt::StrongFocus);
    inlineEdit->setInputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferUppercase);
    connect(inlineEdit, SIGNAL(editingFinished()), this, SLOT(finishInlineEdit()));

#ifdef Q_OS_ANDROID
    keepScreenOn(true);
    connect(qApp, &QGuiApplication::applicationStateChanged, this,
            [](Qt::ApplicationState st){ if (st == Qt::ApplicationActive) keepScreenOn(true); });
#endif

    // ====== OscClient como MEMBRO ======
    osc = new OscClient(this);
    // osc->setTarget(QHostAddress("192.168.1.43"), 10024);

    //REF:DIAL ====== Liga arrays de widgets ======
    dials[0] = ui->dial_0; dials[1] = ui->dial_1; dials[2] = ui->dial_2; dials[3] = ui->dial_3;
    dials[4] = ui->dial_4; dials[5] = ui->dial_5; dials[6] = ui->dial_6; dials[7] = ui->dial_7;

    //REF:MUTE
    buttons[0] = ui->pushButton_ch01; buttons[1] = ui->pushButton_ch02;
    buttons[2] = ui->pushButton_ch03; buttons[3] = ui->pushButton_ch04;
    buttons[4] = ui->pushButton_ch05; buttons[5] = ui->pushButton_ch06;
    buttons[6] = ui->pushButton_ch07; buttons[7] = ui->pushButton_ch08;

    //REF:TITLE
    titlesArray[0] = ui->bTitleCH01; titlesArray[1] = ui->bTitleCH02;
    titlesArray[2] = ui->bTitleCH03; titlesArray[3] = ui->bTitleCH04;
    titlesArray[4] = ui->bTitleCH05; titlesArray[5] = ui->bTitleCH06;
    titlesArray[6] = ui->bTitleCH07; titlesArray[7] = ui->bTitleCH08;

    titlesArray[0]->setProperty("labelKey","LABELCH01");
    titlesArray[1]->setProperty("labelKey","LABELCH02");
    titlesArray[2]->setProperty("labelKey","LABELCH03");
    titlesArray[3]->setProperty("labelKey","LABELCH04");
    titlesArray[4]->setProperty("labelKey","LABELCH05");
    titlesArray[5]->setProperty("labelKey","LABELCH06");
    titlesArray[6]->setProperty("labelKey","LABELCH07");
    titlesArray[7]->setProperty("labelKey","LABELCH08");

    labelsPercentArray[0] = ui->labelPercentCH01; labelsPercentArray[1] = ui->labelPercentCH02;
    labelsPercentArray[2] = ui->labelPercentCH03; labelsPercentArray[3] = ui->labelPercentCH04;
    labelsPercentArray[4] = ui->labelPercentCH05; labelsPercentArray[5] = ui->labelPercentCH06;
    labelsPercentArray[6] = ui->labelPercentCH07; labelsPercentArray[7] = ui->labelPercentCH08;

    //REF:VOLUME Barras de VOLUME (fader em %) -> pbarVol_x
    percBarsArray[0] = ui->pbarVol_0; percBarsArray[1] = ui->pbarVol_1;
    percBarsArray[2] = ui->pbarVol_2; percBarsArray[3] = ui->pbarVol_3;
    percBarsArray[4] = ui->pbarVol_4; percBarsArray[5] = ui->pbarVol_5;
    percBarsArray[6] = ui->pbarVol_6; percBarsArray[7] = ui->pbarVol_7;

    //REF:MAIS
    pbPlus[0] = ui->pbPlus_0; pbPlus[1] = ui->pbPlus_1; pbPlus[2] = ui->pbPlus_2; pbPlus[3] = ui->pbPlus_3;
    pbPlus[4] = ui->pbPlus_4; pbPlus[5] = ui->pbPlus_5; pbPlus[6] = ui->pbPlus_6; pbPlus[7] = ui->pbPlus_7;

    //REF:MENOS
    pbMinus[0] = ui->pbMinus_0; pbMinus[1] = ui->pbMinus_1; pbMinus[2] = ui->pbMinus_2; pbMinus[3] = ui->pbMinus_3;
    pbMinus[4] = ui->pbMinus_4; pbMinus[5] = ui->pbMinus_5; pbMinus[6] = ui->pbMinus_6; pbMinus[7] = ui->pbMinus_7;

    //REF:MENU
    pbTauArray[0] = ui->pb_TAU_0; pbTauArray[1] = ui->pb_TAU_1; pbTauArray[2] = ui->pb_TAU_2;
    pbTauArray[3] = ui->pb_TAU_3; pbTauArray[4] = ui->pb_TAU_4; pbTauArray[5] = ui->pb_TAU_5;

    ui->pb_TAU_0->setProperty("sceneKey", "INICIO");
    ui->pb_TAU_1->setProperty("sceneKey", "ORACAO");
    ui->pb_TAU_2->setProperty("sceneKey", "RECITATIVO");
    ui->pb_TAU_3->setProperty("sceneKey", "TESTEMUNHO");
    ui->pb_TAU_4->setProperty("sceneKey", "PALAVRA");
    ui->pb_TAU_5->setProperty("sceneKey", "ENCERRAMENTO");

    sceneGroup = new QButtonGroup(this);
    sceneGroup->setExclusive(true);
    for (uint8_t i=0;i<NUMBER_OF_SCENES;i++){
        pbTauArray[i]->setNormalColor(QColor("#1e1e1e"));
        //pbTauArray[i]->setHoverColor(QColor("gray"));
        //pbTauArray[i]->setPressedColor(QColor("gray"));
        pbTauArray[i]->setTextColor(Qt::white);
        pbTauArray[i]->setCheckedColor(QColor("#00C853"));
        pbTauArray[i]->setRadius(4);
        pbTauArray[i]->setPadding(20);
        pbTauArray[i]->setCheckable(true);


        sceneGroup->addButton(pbTauArray[i]);

    }
    connect(sceneGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(onSceneClicked(QAbstractButton*)));


    // ----- Liga sinais do dial LR -----
    connect(ui->dial_LR, SIGNAL(valueChanged(int)), this, SLOT(onLRDialValueChanged(int)));
    connect(ui->dial_LR, &QDial::sliderPressed,  this, &MainWindow::onLRDialPressed);
    connect(ui->dial_LR, &QDial::sliderReleased, this, &MainWindow::onLRDialReleased);

    // ----- Setup inicial do dial LR (ACUMULADOR: 10 voltas obrigatórias) -----
    ui->dial_LR->setWrapping(true);
    ui->dial_LR->setNotchesVisible(false);
    ui->dial_LR->setRange(0, 999);       // 1 volta = 1000 passos
    ui->dial_LR->setSingleStep(1);
    ui->dial_LR->setPageStep(10);

    ui->dial_LR->setProperty("turns", 10);            // visual: 10 voltas
    ui->dial_LR->setProperty("fullCircle", true);
    ui->dial_LR->setProperty("displayTurnPercent", false); // texto = % total
    ui->dial_LR->setProperty("showValue", true);

    // Aparência
    ui->dial_LR->setProperty("trackColor",    QColor("#e6e6e6"));
    ui->dial_LR->setProperty("progressColor", QColor("#00C853"));
    ui->dial_LR->setProperty("handleColor",   QColor("#ffffff"));
    ui->dial_LR->setProperty("textColor",     QColor("white"));
    ui->dial_LR->setProperty("thickness",     6);

    lastDialLR     = ui->dial_LR->value(); // 0..999
    accumLR        = 0;                    // 0..10000 (10 voltas = 100%)
    currentFaderLR = 0.0f;

    // Timer único (~30 Hz) para envio do LR
    sendTimerLR = new QTimer(this);
    sendTimerLR->setSingleShot(true);
    sendTimerLR->setInterval(33);
    sendTimerLR->setTimerType(Qt::PreciseTimer);
    connect(sendTimerLR, &QTimer::timeout, this, &MainWindow::flushLRFaderSend);

    // Estado visual inicial do LR
    if (ui->labelPercent_LR){
        ui->labelPercent_LR->setText(QString::number(currentFaderLR, 'f', 4));
    }
    if (ui->pbarVol_LR){
        ui->pbarVol_LR->setValue(int(std::lround(currentFaderLR * 100.0f)));
    }

    // ====== Sinais dos dials / plus / minus ======

    // === Plus/Minus do LR ===
    connect(ui->pbPlus_LR,  &QPushButton::clicked, this, &MainWindow::onPlusLRClicked);
    connect(ui->pbMinus_LR, &QPushButton::clicked, this, &MainWindow::onMinusLRClicked);

    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        connect(dials[i], SIGNAL(valueChanged(int)), this, SLOT(onDialValueChanged(int)));
        connect(dials[i], &QDial::sliderPressed,  this, &MainWindow::onDialPressed);
        connect(dials[i], &QDial::sliderReleased, this, &MainWindow::onDialReleased);

        connect(pbPlus[i],  SIGNAL(clicked()), this, SLOT(onPlusClicked()));
        connect(pbMinus[i], SIGNAL(clicked()), this, SLOT(onMinusClicked()));
    }

    //REF:DIAL ====== Config INICIAL dos dials / buffers ======
    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        dials[i]->setMinimum(0);
        dials[i]->setMaximum(999);      // 1 volta = 1000 passos
        dials[i]->setWrapping(true);
        dials[i]->setTracking(true);

        lastDialArr[i]     = dials[i]->value();   // inicial
        accumArr[i]        = 0;                   // começa em 0%
        currentFaderArr[i] = 0.0f;

        if (dials[i]) {
            // progresso total 0..1 para o arco verde
            dials[i]->setProperty("progress01", currentFaderArr[i]);

            // (opcional) mesmo visual do LR
            dials[i]->setProperty("turns", 10);
            dials[i]->setProperty("fullCircle", true);
            dials[i]->setProperty("displayTurnPercent", false); // texto = % total (se showValue=true)
            dials[i]->setProperty("showValue", true);          // normalmente os canais não mostram texto
            dials[i]->setProperty("trackColor",    QColor("#e6e6e6"));
            dials[i]->setProperty("progressColor", QColor("#00C853"));
            dials[i]->setProperty("handleColor",   QColor("#ffffff"));
            dials[i]->setProperty("textColor",     QColor("white"));
            dials[i]->setProperty("thickness",     6);
        }


        // throttle por canal (~30 Hz)
        sendTimers[i] = new QTimer(this);
        sendTimers[i]->setSingleShot(true);
        sendTimers[i]->setInterval(33);
        sendTimers[i]->setTimerType(Qt::PreciseTimer); // timing estável
        connect(sendTimers[i], &QTimer::timeout, this, [this, i](){ flushFaderSend(i); });

        dragging[i] = false;
    }

    //REF:OSC
    // ====== Bind UDP e descoberta ======
    QTimer::singleShot(0, this, [this]() {
        if (!osc->open(12000)) { //ATENCAO: ISSO É PORTA LOCAL, DO APP
            appendLog("Falha ao abrir UDP local. Não operativo.", "red", true, false);
            return;
        }

        QTimer::singleShot(0, this, [this]() {
            const bool found = osc->setTargetFromDiscovery("192.168.1.0/24", 3500);
            if (!found) {
                appendLog("Mixer não encontrado via scan; usando IP de entrada", "yellow", true, false);
                osc->setTarget(QHostAddress(ui->lineEditIP->text()), 10024);
            } else {
                qDebug() << "Mixer em" << osc->targetAddress() << osc->targetPort();
                appendLog("Mixer em " + osc->targetAddress().toString().toUtf8(), "cyan", false, true);
            }
            osc->queryName();

            osc->startFeedbackKeepAlive(5000);
            osc->subscribeMetersAllChannels();
            osc->subscribeMetersLR();
            osc->syncAll(NUMBER_OF_CHANNELS);   // GET inicial (fader+mute) dos canais

        });
    });

    //REF:METER ===================== TIMER ÚNICO DE UI PARA METERS =====================
    if (!g_uiMeterTimer) {
        g_uiMeterTimer = new QTimer(this);
        g_uiMeterTimer->setInterval(30);
        g_uiMeterTimer->setTimerType(Qt::CoarseTimer);
        connect(g_uiMeterTimer, &QTimer::timeout, this, [this](){
            // canais 0..7
            auto meterBarAt = [this](int ch)->QProgressBar*{
                switch (ch) {
                case 0: return ui->progressBar_0;
                case 1: return ui->progressBar_1;
                case 2: return ui->progressBar_2;
                case 3: return ui->progressBar_3;
                case 4: return ui->progressBar_4;
                case 5: return ui->progressBar_5;
                case 6: return ui->progressBar_6;
                case 7: return ui->progressBar_7;
                default: return (QProgressBar*)nullptr;
                }
            };

            for (int ch=0; ch<kNumUiCh; ++ch) {
                if (g_nextPctCh[ch] != g_lastPctCh[ch]) {
                    if (QProgressBar* bar = meterBarAt(ch)) bar->setValue(g_nextPctCh[ch]);
                    g_lastPctCh[ch] = g_nextPctCh[ch];
                }
            }
            // LR
            if (ui->progressBar_L && g_nextPctLR[0] != g_lastPctLR[0]) {
                ui->progressBar_L->setValue(g_nextPctLR[0]);
                g_lastPctLR[0] = g_nextPctLR[0];
            }
            if (ui->progressBar_R && g_nextPctLR[1] != g_lastPctLR[1]) {
                ui->progressBar_R->setValue(g_nextPctLR[1]);
                g_lastPctLR[1] = g_nextPctLR[1];
            }
        });
        g_uiMeterTimer->start();
    }
    // ========================================================================

    // ====== Handler de RX (alimenta UI) ======
    connect(osc, &OscClient::oscMessageReceived, this,
            [this](const QString& addr, const QVariantList& args)
            {
                if (addr == QLatin1String("/xremote")) return;

                // ================================
                // Identificação do mixer (/xinfo)
                // ================================
                if (addr == QLatin1String("/xinfo")) {
                    QStringList sl; sl.reserve(args.size());
                    for (const QVariant &v : args) sl << v.toString();
                    const QString brand  = sl.value(0, "?");
                    const QString model  = sl.value(1, "?");
                    const QString fw     = sl.value(2, "?");
                    const QString proto  = sl.value(3, "?");
                    appendLog(QString("Mixer identificado: %1 %2 — FW %3 — %4").arg(brand, model, fw, proto),
                              "lime", true, false);
                    return;
                }

                // ==========================================================
                //REF:METER Meters (/meters/1) -> progressBar_X (NÃO pbarVol_X)
                // ==========================================================
                if (addr == QLatin1String("/meters/1") && !args.isEmpty()) {
                    const QVariant &v0 = args.first();
                    if (!v0.canConvert<QByteArray>()) return;

                    const QByteArray raw = v0.toByteArray();
                    if (raw.isEmpty()) return;

                    int offset = 0;
                    if (raw.size() >= 6) {
                        quint32 beLen;
                        std::memcpy(&beLen, raw.constData(), 4);
                        const quint32 declared = qFromBigEndian(beLen);
                        const int remaining = raw.size() - 4;
                        if (declared > 0 && declared <= (quint32)remaining && int(declared) == remaining) {
                            offset = 4;
                        }
                    }

                    const int bytes   = raw.size() - offset;
                    const int nShorts = bytes / 2;
                    if (nShorts <= 0) return;

                    auto rdI16 = [&](int idx)->qint16{
                        if (idx < 0 || idx >= nShorts) return INT16_MIN;
                        quint16 be; std::memcpy(&be, raw.constData() + offset + idx*2, 2);
                        return (qint16)qFromBigEndian(be);
                    };

                    const float FLOOR_DB = -70.0f;
                    auto dbTo01 = [&](float dB)->float{
                        if (dB <= FLOOR_DB) return 0.0f;
                        if (dB >= 0.0f)     return 1.0f;
                        return (dB - FLOOR_DB) / (0.0f - FLOOR_DB);
                    };

                    // SEM drop de frames; apenas preenche cache
                    for (int ch = 0; ch < NUMBER_OF_CHANNELS; ++ch) {
                        const qint16 s = rdI16(ch);
                        float lin01 = 0.0f;
                        if (s != INT16_MIN) {
                            const float dB = s / 256.0f;
                            lin01 = dbTo01(dB);
                        }
                        const int pct = int(std::lround(lin01 * 100.0f));
                        g_nextPctCh[ch] = pct; // cache; UI aplica no timer
                    }
                    return;
                }

                // ==========================================================
                //REF:METER Meters LR (/meters/3) -> progressBar_L / progressBar_R
                // ==========================================================
                if (addr == QLatin1String("/meters/3") && !args.isEmpty()) {
                    const QVariant &v0 = args.first();
                    if (!v0.canConvert<QByteArray>()) return;

                    const QByteArray raw = v0.toByteArray();
                    if (raw.isEmpty()) return;

                    int offset = 0;
                    if (raw.size() >= 6) {
                        quint32 beLen;
                        std::memcpy(&beLen, raw.constData(), 4);
                        const quint32 declared = qFromBigEndian(beLen);
                        const int remaining = raw.size() - 4;
                        if (declared > 0 && declared <= (quint32)remaining && int(declared) == remaining) {
                            offset = 4;
                        }
                    }

                    const int nShorts = (raw.size() - offset) / 2;
                    if (nShorts < 2) return;

                    auto rdI16 = [&](int idx)->qint16{
                        if (idx < 0 || idx >= nShorts) return INT16_MIN;
                        quint16 be; std::memcpy(&be, raw.constData() + offset + idx*2, 2);
                        return (qint16)qFromBigEndian(be);
                    };

                    const float FLOOR_DB = -70.0f;
                    auto dbTo01 = [&](float dB)->float{
                        if (dB <= FLOOR_DB) return 0.0f;
                        if (dB >= 0.0f)     return 1.0f;
                        return (dB - FLOOR_DB) / (0.0f - FLOOR_DB);
                    };

                    const qint16 sL = rdI16(0);
                    const qint16 sR = rdI16(1);

                    auto s16ToPct = [&](qint16 s)->int{
                        if (s == INT16_MIN) return 0;
                        const float dB = s / 256.0f;
                        const float lin01 = dbTo01(dB);
                        return int(std::lround(lin01 * 100.0f));
                    };

                    g_nextPctLR[0] = s16ToPct(sL);
                    g_nextPctLR[1] = s16ToPct(sR);
                    return;
                }

                // ==========================================================
                //REF:LR LR on/off (/lr/mix/on) -> pushButton_LR (ícone + check)
                // ==========================================================
                if (addr == QLatin1String("/lr/mix/on") && !args.isEmpty()) {
                    const int on = args.first().toInt();
                    const bool checked = (on != 0);
                    if (ui->pushButton_LR) {
                        if (ui->pushButton_LR->isChecked() != checked) {
                            QSignalBlocker block(ui->pushButton_LR);
                            ui->pushButton_LR->setChecked(checked);
                        }
                        ui->pushButton_LR->setIcon(QIcon(checked
                                                             ? QStringLiteral(":/icons/resources/unmuted.svg")
                                                             : QStringLiteral(":/icons/resources/muted.svg")));
                    }
                    return;
                }

                // ==========================================================
                // Fader LR (0..1) -> acumulador + dial_LR (0..999), label e pbar
                // ==========================================================
                if (addr == QLatin1String("/lr/mix/fader") && !args.isEmpty()) {
                    const float v01 = std::clamp(args.first().toFloat(), 0.0f, 1.0f);

                    currentFaderLR = v01;
                    ui->dial_LR->setProperty("progress01", currentFaderLR);
                    accumLR        = int(v01 * 10000.0f + 0.5f);

                    // Se estiver arrastando, só atualiza label/barra
                    if (draggingLR) {
                        if (ui->labelPercent_LR)
                            ui->labelPercent_LR->setText(QString::number(currentFaderLR, 'f', 4));
                        if (ui->pbarVol_LR)
                            ui->pbarVol_LR->setValue(int(std::lround(v01 * 100.0f)));
                        return;
                    }

                    // move o dial (0..999) sem emitir signals
                    {
                        const int steps = accumLR % 1000;
                        QSignalBlocker block(ui->dial_LR);
                        ui->dial_LR->setValue(steps);
                        lastDialLR = steps;
                    }

                    // >>> REFLETE NA UI DO LR <<<
                    if (ui->labelPercent_LR)
                        ui->labelPercent_LR->setText(QString::number(v01, 'f', 4));
                    if (ui->pbarVol_LR)
                        ui->pbarVol_LR->setValue(int(std::lround(v01 * 100.0f)));

                    return;
                }

                // ==========================================================
                // Demais paths de canal (/ch/NN/...)
                // ==========================================================
                if (!addr.startsWith(QLatin1String("/ch/")) || args.isEmpty()) return;

                bool ok = false;
                const int ch  = addr.mid(4, 2).toInt(&ok);
                if (!ok) return;
                const int idx = ch - 1;
                if (idx < 0 || idx >= NUMBER_OF_CHANNELS) return;

                // Fader (float 0..1) -> pbarVol_X e dials
                if (addr.endsWith(QLatin1String("/mix/fader"))) {
                    if (dragging[idx]) {
                        // Atualiza só o cache enquanto arrasta (não move o dial)
                        currentFaderArr[idx] = std::clamp(args.first().toFloat(), 0.0f, 1.0f);

                        // ↙↙ NOVO: mantém o arco verde proporcional durante o arrasto
                        if (dials[idx]) dials[idx]->setProperty("progress01", currentFaderArr[idx]);

                        // (opcional) se quiser atualizar label/barra durante o arrasto:
                        // labelsPercentArray[idx]->setText(QString::number(currentFaderArr[idx], 'f', 4));
                        // if (percBarsArray[idx]) percBarsArray[idx]->setValue(int(std::lround(currentFaderArr[idx]*100.0f)));
                        return;
                    }

                    float v01 = std::clamp(args.first().toFloat(), 0.0f, 1.0f);
                    currentFaderArr[idx] = v01;
                    accumArr[idx]        = int(v01 * 10000.0f + 0.5f);

                    if (dials[idx]) dials[idx]->setProperty("progress01", currentFaderArr[idx]);

                    if (dials[idx]) {
                        const int steps = accumArr[idx] % 1000;
                        QSignalBlocker block(dials[idx]);
                        dials[idx]->setValue(steps);
                        lastDialArr[idx] = steps;
                    }

                    labelsPercentArray[idx]->setText(QString::number(v01, 'f', 4));
                    if (percBarsArray[idx]) percBarsArray[idx]->setValue(int(std::lround(v01 * 100.0f)));
                    return;
                }

                // Mute (int/bool) — ATUALIZA UI SEM EMITIR SINAL
                if (addr.endsWith(QLatin1String("/mix/on"))) {
                    const int onInt = args.first().toInt(); // 1 => unmuted (ligado), 0 => muted (desligado)
                    const bool shouldChecked = (onInt == 0) ? true : false; // nosso botão checked = muted

                    if (buttons[idx]) {
                        // só toca no botão se realmente mudou
                        if (buttons[idx]->isChecked() != shouldChecked) {
                            QSignalBlocker block(buttons[idx]); // evita idToggled -> onMuteToggled -> loop
                            buttons[idx]->setChecked(shouldChecked);
                        }
                        // ÍCONE **NÃO** é alterado aqui (fica sob controle dos handlers de UI)
                    }
                    return;
                }
            });

    // ====== Timer de decay do meter global  ======
    connect(&meterDecayTimer, &QTimer::timeout, this, &MainWindow::updateMeterDecay);
    meterDecayTimer.start(30);

    //REF:MUTE ====== Grupo de mute por canal ======
    group = new QButtonGroup(this);
    group->setExclusive(false);

    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        auto b = static_cast<ModernButton*>(buttons[i]);
        b->setNormalColor(QColor("#00C853"));
        b->setHoverColor(QColor("#00C853"));
        b->setPressedColor(QColor("#00C853"));
        b->setCheckedColor(QColor("#E53935")); // vermelho quando checked
        b->setTextColor(Qt::white);
        b->setRadius(12);
        b->setPadding(10);
        b->setIconSizePx(18);
    }


    ui->pbPlus_LR->setNormalColor(QColor("#00C853"));
    ui->pbPlus_LR->setHoverColor(QColor("#00C853"));
    ui->pbPlus_LR->setPressedColor(QColor("#00C853"));
    ui->pbPlus_LR->setTextColor(Qt::white);
    ui->pbPlus_LR->setRadius(6);
    ui->pbPlus_LR->setPadding(10);
    ui->pbPlus_LR->setText("+");
    ui->pbPlus_LR->setCheckable(false);

    ui->pbMinus_LR->setNormalColor(QColor("#00C853"));
    ui->pbMinus_LR->setHoverColor(QColor("#00C853"));
    ui->pbMinus_LR->setPressedColor(QColor("#00C853"));
    ui->pbMinus_LR->setTextColor(Qt::white);
    ui->pbMinus_LR->setRadius(6);
    ui->pbMinus_LR->setPadding(10);
    ui->pbMinus_LR->setText("-");
    ui->pbMinus_LR->setCheckable(false);

    for (uint8_t i=0; i<NUMBER_OF_CHANNELS;i++){
        pbPlus[i]->setNormalColor(QColor("#00C853"));
        pbPlus[i]->setHoverColor(QColor("#00C853"));
        pbPlus[i]->setPressedColor(QColor("#00C853"));
        pbPlus[i]->setTextColor(Qt::white);
        pbPlus[i]->setRadius(6);
        pbPlus[i]->setPadding(10);
        pbPlus[i]->setText("+");
        pbPlus[i]->setCheckable(false);

        pbPlus[i]->setMinimumSize(30,30);
        pbPlus[i]->setMaximumSize(30,30);

        pbMinus[i]->setNormalColor(QColor("#00C853"));
        pbMinus[i]->setHoverColor(QColor("#00C853"));
        pbMinus[i]->setPressedColor(QColor("#00C853"));
        pbMinus[i]->setTextColor(Qt::white);
        pbMinus[i]->setRadius(6);
        pbMinus[i]->setPadding(10);
        pbMinus[i]->setText("-");
        pbMinus[i]->setCheckable(false);

        pbMinus[i]->setMinimumSize(30,30);
        pbMinus[i]->setMaximumSize(30,30);
    }

    ui->pushButton_LR->setNormalColor(QColor("#00C853"));
    //ui->pushButton_LR->setHoverColor(QColor("#00C853"));
    ui->pushButton_LR->setPressedColor(QColor("gray"));
    ui->pushButton_LR->setCheckedColor(QColor("#E53935"));
    ui->pushButton_LR->setTextColor(Qt::white);
    ui->pushButton_LR->setRadius(12);
    ui->pushButton_LR->setIcon(QIcon(":/icons/resources/unmuted.svg"));
    //ui->pushButton_LR->setText("LR");
    ui->pushButton_LR->setMinimumSize(35,35);
    ui->pushButton_LR->setMaximumSize(35,35);


    for (uint8_t i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        buttons[i]->setIcon(QIcon(":/icons/resources/unmuted.svg"));
        buttons[i]->setCheckable(true);
        group->addButton(buttons[i], i);
        connect(titlesArray[i], SIGNAL(clicked(bool)), this, SLOT(changeTitle()));
        buttons[i]->setMinimumSize(35,35);
        buttons[i]->setMaximumSize(35,35);
    }
    connect(group, SIGNAL(idToggled(int,bool)), this, SLOT(onMuteToggled(int,bool)));
    connect(ui->pushButton_LR, SIGNAL(clicked(bool)), this, SLOT(onMuteToggledLR(bool)));

    // Perfil / cenas
    connect(ui->pushButtonProfile, SIGNAL(clicked()), this, SLOT(onSaveActiveSceneClicked()));

    // Carrega labels persistidas
    loadChannelLabels();

}

// ====== LR: ACUMULADOR (10 voltas = 100%) ======
void MainWindow::onLRDialValueChanged(int v)
{
    const int STEPS = 1000;   // passos por volta
    const int TOTAL = 10000;  // 10 voltas = 100%

    const int LOW  = STEPS / 4;        // 250
    const int HIGH = (STEPS * 3) / 4;  // 750

    int diff = v - lastDialLR;
    if (lastDialLR > HIGH && v < LOW)       diff = (v + STEPS) - lastDialLR;
    else if (lastDialLR < LOW && v > HIGH)  diff = (v - STEPS) - lastDialLR;

    accumLR += diff;
    if (accumLR < 0)     accumLR = 0;
    if (accumLR > TOTAL) accumLR = TOTAL;
    lastDialLR = v;

    currentFaderLR = accumLR / float(TOTAL); // 0..1 após 10 voltas
    ui->dial_LR->setProperty("progress01", currentFaderLR);

    // >>> REFLETE NA UI DO LR <<<
    if (ui->labelPercent_LR)
        ui->labelPercent_LR->setText(QString::number(currentFaderLR, 'f', 4));
    if (ui->pbarVol_LR)
        ui->pbarVol_LR->setValue(int(std::lround(currentFaderLR * 100.0f)));

    if (sendTimerLR) sendTimerLR->start();   // throttle
}

void MainWindow::onLRDialPressed()
{
    draggingLR = true;
    // mantemos o timer rodando durante arraste (throttle)
}

void MainWindow::onLRDialReleased()
{
    draggingLR = false;
    if (sendTimerLR) sendTimerLR->stop();
    flushLRFaderSend(); // envio final imediato
}

void MainWindow::flushLRFaderSend()
{
    if (!osc) return;

    // coalescing leve (igual aos canais)
    static bool  s_init = false;
    static float s_lastSent = -1.0f;
    if (!s_init) { s_lastSent = -1.0f; s_init = true; }

    const float v01 = currentFaderLR;
    const float eps = 0.0005f;
    if (s_lastSent < 0.0f || std::fabs(v01 - s_lastSent) > eps) {
        s_lastSent = v01;
        osc->setMainLRFader(v01);
    }
}

void MainWindow::onMuteToggledLR(bool)
{
    const bool muted = ui->pushButton_LR->isChecked();

    // Atualiza ícone (igual já fazia)
    ui->pushButton_LR->setIcon(QIcon(muted
                                         ? QStringLiteral(":/icons/resources/muted.svg")
                                         : QStringLiteral(":/icons/resources/unmuted.svg")));

    // ENVIO OSC para LR: on=1 => unmuted; nosso botão checked=true => muted
    if (osc) {
        osc->setMainLRMute(!muted);
    }
}

// =================== LOG ===================
void MainWindow::appendLog(const QString &msg, const QString &color, bool bold, bool italic)
{
    QString style = QString("color:%1;").arg(color);
    if (bold)   style += "font-weight:bold;";
    if (italic) style += "font-style:italic;";
    QString html = QString("<span style='%1'>%2</span>").arg(style, msg.toHtmlEscaped());
    ui->textEdit->append(html);
}

// ============ Inline edit dos títulos ============
void MainWindow::changeTitle()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    editingBtn = btn;

    inlineEdit->setText(btn->text());
    inlineEdit->setFont(btn->font());
    inlineEdit->selectAll();

    const QPoint topLeft = btn->mapTo(ui->centralwidget, QPoint(0,0));
    inlineEdit->setGeometry(QRect(topLeft, btn->size()));

    btn->setVisible(false);
    inlineEdit->show();
    inlineEdit->raise();
    inlineEdit->setFocus(Qt::OtherFocusReason);

    QTimer::singleShot(0, this, SLOT(ensureSoftKeyboard()));
}

void MainWindow::ensureSoftKeyboard()
{
    if (inlineEdit && inlineEdit->isVisible() && inlineEdit->hasFocus()) {
        if (auto *im = qApp->inputMethod()) im->show();
    }
}

void MainWindow::finishInlineEdit()
{
    if (editingBtn) {
        const QString t = inlineEdit->text().trimmed();
        if (!t.isEmpty()) editingBtn->setText(t);
        saveChannelLabel(editingBtn);
        editingBtn->setVisible(true);
        editingBtn = nullptr;
    }
    inlineEdit->hide();
    if (auto *im = qApp->inputMethod()) im->hide();
}

void MainWindow::saveChannelLabel(QAbstractButton* b)
{
    if (!b) return;
    const QString key = b->property("labelKey").toString();
    if (key.isEmpty()) return;

    QSettings s(profilesIniPath(), QSettings::IniFormat);
    s.setFallbacksEnabled(false);
    s.beginGroup("LABELS");
    s.setValue(key, b->text());
    s.endGroup();
    s.sync();
}

void MainWindow::loadChannelLabels()
{
    QSettings s(profilesIniPath(), QSettings::IniFormat);
    s.setFallbacksEnabled(false);
    s.beginGroup("LABELS");
    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        auto *btn = titlesArray[i];
        if (!btn) continue;
        const QString key = btn->property("labelKey").toString();
        if (key.isEmpty()) continue;
        const QString t = s.value(key, btn->text()).toString();
        btn->setText(t);
    }
    s.endGroup();
}

// ============ Dial (10 voltas, envio OSC com throttle) ============
void MainWindow::onDialValueChanged(int v)
{
    QDial* dial = qobject_cast<QDial*>(sender());
    if (!dial) return;

    const QStringList parts = dial->objectName().split('_');
    if (parts.size() < 2) return;
    bool ok = false;
    int idx = parts.at(1).toInt(&ok);
    if (!ok || idx < 0 || idx >= NUMBER_OF_CHANNELS) return;

    const int STEPS = 1000;
    const int TOTAL = 10000;

    const int LOW  = STEPS / 4;        // 250
    const int HIGH = (STEPS * 3) / 4;  // 750

    int diff = v - lastDialArr[idx];
    if (lastDialArr[idx] > HIGH && v < LOW) {
        diff = (v + STEPS) - lastDialArr[idx];
    } else if (lastDialArr[idx] < LOW && v > HIGH) {
        diff = (v - STEPS) - lastDialArr[idx];
    }

    int& accum = accumArr[idx];
    accum += diff;
    if (accum < 0)     accum = 0;
    if (accum > TOTAL) accum = TOTAL;

    lastDialArr[idx] = v;

    currentFaderArr[idx] = accum / float(TOTAL); // 0..1

    if (dials[idx]) dials[idx]->setProperty("progress01", currentFaderArr[idx]);


    const float perc = currentFaderArr[idx] * 100.0f;
    labelsPercentArray[idx]->setText(QString::number(currentFaderArr[idx], 'f', 4));
    if (percBarsArray[idx]) percBarsArray[idx]->setValue(int(std::lround(perc)));

    // Sempre agenda envio com throttle (~30 Hz), mesmo arrastando
    if (sendTimers[idx]) sendTimers[idx]->start();
}

void MainWindow::onConnectButton()
{
    // 1) Normaliza IP/porta
    const QString ipText  = ui->lineEditIP->text().trimmed();
    const int     port    = qMax(1, ui->lineEditPort->text().toInt()); // porta >0
    const int     oscPort = (port == 0 ? 10024 : port);                // padrão X AIR

    if (ipText.isEmpty()) {
        appendLog("IP vazio. Informe o endereço do mixer.", "red", true, false);
        return;
    }

    // 2) Abre/binda a porta local (do app) — idempotente
    if (!osc->isOpen()) {
        if (!osc->open(12000)) { // porta LOCAL do app
            appendLog("Falha ao abrir UDP local. Não operativo.", "red", true, false);
            return;
        }
    }

    // 3) Define o alvo
    osc->setTarget(QHostAddress(ipText), oscPort);
    appendLog("Conexão iniciada pelo usuário", "yellow", false, true);
    appendLog("Mixer em " + osc->targetAddress().toString().toUtf8()
                  + " porta " + QByteArray::number(oscPort),
              "cyan", false, true);

    // 4) Identificação (opcional, só para logar modelo/fw)
    osc->queryName(); // /xinfo

    // 5) Registro de feedback ("/xremote") com renovação periódica
    //    (se seu OscClient já se protege de múltiplas chamadas, ótimo;
    //     senão, pare o anterior antes de iniciar outro)
    osc->startFeedbackKeepAlive(5000);

    // 6) Assina meters (canais e LR)
    //    Garanta que estes métodos sejam idempotentes (não duplicar assinaturas).
    osc->subscribeMetersAllChannels(); // geralmente /meters/1
    osc->subscribeMetersLR();          // geralmente /meters/3

    // 7) GET inicial dos canais (fader+mute)
    osc->syncAll(NUMBER_OF_CHANNELS);

    // 8) ***FALTAVA***: GET inicial do LR (fader e mute)
    //    Use helpers se existirem; caso contrário mande GET “cru”.
    //    Ex.: osc->getMainLRFader(); osc->getMainLRMute();
    //osc->sendGet(QStringLiteral("/lr/mix/fader"));
    //osc->sendGet(QStringLiteral("/lr/mix/on"));

    // 9) (Opcional) Dump total para “forçar” estado completo
    // osc->requestStatDump(); // se implementado (ex. "/-stat/dump")
}


void MainWindow::onDialPressed()
{
    QDial* dial = qobject_cast<QDial*>(sender());
    if (!dial) return;
    const auto parts = dial->objectName().split('_');
    if (parts.size() < 2) return;
    bool ok=false; int idx = parts.at(1).toInt(&ok);
    if (!ok || idx < 0 || idx >= NUMBER_OF_CHANNELS) return;

    dragging[idx] = true;
    // não paramos o timer — enviamos durante o arrasto com throttle
}

void MainWindow::onDialReleased()
{
    QDial* dial = qobject_cast<QDial*>(sender());
    if (!dial) return;
    const auto parts = dial->objectName().split('_');
    if (parts.size() < 2) return;
    bool ok=false; int idx = parts.at(1).toInt(&ok);
    if (!ok || idx < 0 || idx >= NUMBER_OF_CHANNELS) return;

    dragging[idx] = false;
    if (sendTimers[idx]) sendTimers[idx]->stop();
    flushFaderSend(idx); // envio final imediato quando solta
}

void MainWindow::flushFaderSend(int idx)
{
    if (!osc) return;
    if (idx < 0 || idx >= NUMBER_OF_CHANNELS) return;

    // Coalescing: só envia se mudou (epsilon)
    static bool s_init = false;
    static float s_lastSentFader[NUMBER_OF_CHANNELS];
    if (!s_init) {
        for (int i=0;i<NUMBER_OF_CHANNELS;i++) s_lastSentFader[i] = -1.0f;
        s_init = true;
    }

    const float v01 = currentFaderArr[idx];
    const float last = s_lastSentFader[idx];
    const float eps  = 0.0005f; // ~0.05%

    if (last < 0.0f || std::fabs(v01 - last) > eps) {
        s_lastSentFader[idx] = v01;
        osc->setChannelFader(idx + 1, v01); // canal OSC é 1-based
    }
}

// ============ Perfis / cenas ============
void MainWindow::onSaveActiveSceneClicked()
{
    QAbstractButton* b = sceneGroup->checkedButton();
    if (!b) return;

    const QString key = b->property("sceneKey").toString(); // INICIO, ORACAO, ...
    QSettings s(profilesIniPath(), QSettings::IniFormat);
    s.setFallbacksEnabled(false);

    s.beginGroup(key);
    for (uint8_t i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        s.setValue(QString("m%1").arg(i), buttons[i]->isChecked());
    }
    s.endGroup();
    s.sync();
}

void MainWindow::onMuteToggled(int id, bool checked)
{
    if (id < 0 || id >= NUMBER_OF_CHANNELS) return;

    // UI: ícone continua como estava (sua convenção)
    buttons[id]->setIcon(QIcon(checked
                                   ? QStringLiteral(":/icons/resources/muted.svg")
                                   : QStringLiteral(":/icons/resources/unmuted.svg")));

    // Convenção do mixer: /ch/NN/mix/on = 1 => canal LIGADO (unmuted)
    // Nosso botão: checked=true => MUTED. Logo enviamos on = !checked.
    const int onToSend = (!checked) ? 1 : 0;

    // Envia somente se mudou em relação ao último envio
    if (osc) {
        if (s_lastMuteSent[id] != onToSend) {
            s_lastMuteSent[id] = onToSend;
            osc->setChannelMute(id + 1, onToSend);
        }
    }
}

void MainWindow::onOscMeter(float val01)
{
    meterInstant = qBound(0, int(val01 * 1000.0f + 0.5f), 1000);
    if (meterInstant > meterDisplay) {
        meterDisplay = meterInstant;
        ui->progressBar_0->setValue(meterDisplay);
    }
}

void MainWindow::onMinusLRClicked()
{
    const int TOTAL = 10000;     // 0..100.00% em unidades
    const int STEP  = 100;       // 1%

    int units = accumLR - STEP;
    if (units < 0) units = 0;
    accumLR = units;

    currentFaderLR = accumLR / 10000.0f;
    ui->dial_LR->setProperty("progress01", currentFaderLR);


    // move o dial sem emitir signals (0..999)
    if (ui->dial_LR) {
        int steps = accumLR % 1000;
        QSignalBlocker block(ui->dial_LR);
        ui->dial_LR->setValue(steps);
        lastDialLR = steps;
    }

    // atualiza UI (label e barra)
    if (ui->labelPercent_LR)
        ui->labelPercent_LR->setText(QString::number(currentFaderLR, 'f', 4));
    if (ui->pbarVol_LR)
        ui->pbarVol_LR->setValue(int(std::lround(currentFaderLR * 100.0f)));

    // throttle de envio
    if (sendTimerLR) sendTimerLR->start();
}

void MainWindow::onMinusClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int idx = -1;
    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        if (pbMinus[i] == btn) { idx = i; break; }
    }
    if (idx < 0) return;

    int units = accumArr[idx] - 100;    // -1%
    if (units < 0) units = 0;
    accumArr[idx] = units;

    currentFaderArr[idx] = accumArr[idx] / 10000.0f;

    if (dials[idx]) {
        int steps = accumArr[idx] % 1000;
        QSignalBlocker block(dials[idx]);
        dials[idx]->setValue(steps);
        lastDialArr[idx] = steps;

        if (dials[idx]) {
            dials[idx]->setProperty("progress01", currentFaderArr[idx]);
        }
    }

    const float perc = accumArr[idx] / 100.0f;
    labelsPercentArray[idx]->setText(QString::number(perc/100.0f, 'f', 4));
    if (percBarsArray[idx]) percBarsArray[idx]->setValue(int(std::lround(perc)));
}

void MainWindow::onPlusClicked()
{
    ModernButton *btn = qobject_cast<ModernButton*>(sender());
    if (!btn) return;

    int idx = -1;
    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i) {
        if (pbPlus[i] == btn) { idx = i; break; }
    }
    if (idx < 0) return;

    int units = accumArr[idx] + 100;    // +1%
    if (units > 10000) units = 10000;
    accumArr[idx] = units;

    currentFaderArr[idx] = accumArr[idx] / 10000.0f;

    if (dials[idx]) {
        int steps = accumArr[idx] % 1000;
        QSignalBlocker block(dials[idx]);
        dials[idx]->setValue(steps);
        lastDialArr[idx] = steps;

        if (dials[idx]) {
            dials[idx]->setProperty("progress01", currentFaderArr[idx]);
        }
    }

    const float perc = accumArr[idx] / 100.0f;
    labelsPercentArray[idx]->setText(QString::number(perc/100.0f, 'f', 4));
    if (percBarsArray[idx]){
        percBarsArray[idx]->setValue(int(std::lround(perc)));


    }
}

void MainWindow::onSceneClicked(QAbstractButton* b)
{
    const QString key = b->property("sceneKey").toString();
    QSettings s(profilesIniPath(), QSettings::IniFormat);
    s.setFallbacksEnabled(false);

    if (!s.childGroups().contains(key))
        return;

    s.beginGroup(key);
    for (int ch = 0; ch < NUMBER_OF_CHANNELS; ++ch) {
        const bool mute = s.value(QString("m%1").arg(ch), false).toBool();
        buttons[ch]->setChecked(mute);
        // osc->setChannelMute(ch + 1, mute);
    }
    s.endGroup();
}

// ============ Meter decay ============
void MainWindow::updateMeterDecay()
{
    if (meterDisplay > meterInstant) {
        meterDisplay -= 5;
        if (meterDisplay < meterInstant)
            meterDisplay = meterInstant;
        ui->progressBar_0->setValue(meterDisplay);
    }
}

void MainWindow::onPlusLRClicked()
{
    const int TOTAL = 10000;     // 0..100.00% em unidades
    const int STEP  = 100;       // 1%

    int units = accumLR + STEP;
    if (units > TOTAL) units = TOTAL;
    accumLR = units;

    currentFaderLR = accumLR / 10000.0f;
    ui->dial_LR->setProperty("progress01", currentFaderLR);


    // move o dial sem emitir signals (0..999)
    if (ui->dial_LR) {
        int steps = accumLR % 1000;
        QSignalBlocker block(ui->dial_LR);
        ui->dial_LR->setValue(steps);
        lastDialLR = steps;
        //ui->dial_LR->setValue(steps);
    }

    // atualiza UI (label e barra)
    if (ui->labelPercent_LR){
        ui->labelPercent_LR->setText(QString::number(currentFaderLR, 'f', 4));
    }
    if (ui->pbarVol_LR){
        ui->pbarVol_LR->setValue(int(std::lround(currentFaderLR * 100.0f)));
    }

    // throttle de envio
    if (sendTimerLR){
        sendTimerLR->start();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
