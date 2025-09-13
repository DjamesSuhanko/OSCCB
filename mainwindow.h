#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPushButton>
#include <QButtonGroup>
#include <QInputDialog>
#include <QLabel>
#include <QDial>
#include <QProgressBar>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QLineEdit>
#include <QGuiApplication>
#include <QInputMethod>
#include <QAbstractButton>
#include <moderndial.h>
#include <modernbutton.h>
#include <modernprogressbar.h>
#include <meterswidget.h>
#include <channelmeterwidget.h>
#include <QRandomGenerator>
#include <percentbarwidget.h>

#define NUMBER_OF_CHANNELS 8
#define NUMBER_OF_SCENES   6
#define NUMBER_OF_HELPS    9 //botões de help na aba menu
#define PROFILE_INI "profiles.ini"
#define LOCAL_PORT_BIND 12000
#define HAVE_LR_FRAMES 1

// Caminho do INI
static QString profilesIniPath() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    return base + "/profiles.ini";
}

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class OscClient; // forward declaration

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onDialValueChanged(int v);
    void onPlusClicked();
    void onMinusClicked();

    void onPlusLRClicked();
    void onMinusLRClicked();

private slots:
    void onMuteToggled(int id, bool checked);
    void onMuteToggledLR(bool checked);

    // edição inline dos títulos
    void changeTitle();
    void finishInlineEdit();
    void ensureSoftKeyboard();
    void saveChannelLabel(QAbstractButton* b);

    // cenas / perfis
    void onSaveActiveSceneClicked();
    void onSceneClicked(QAbstractButton* b);
    //void onMakeProfile();  // você conecta isso no construtor

    // envio de fader com throttle
    void onDialPressed();
    void onDialReleased();
    void flushFaderSend(int idx);

    void onLRDialValueChanged(int v);
    void onLRDialPressed();
    void onLRDialReleased();
    void flushLRFaderSend();
    void onConnectButton();

    void onHelpButtonsClicked();

    void pbMuteHelpSlot();

private:
    Ui::MainWindow *ui;

    // ===== OSC =====
    OscClient* osc = nullptr;                 // cliente OSC (membro)

    // throttle por canal
    QTimer* sendTimers[NUMBER_OF_CHANNELS]{}; // timers singleShot (~30 Hz)
    bool    dragging[NUMBER_OF_CHANNELS]{};   // está arrastando este dial?

    // editor inline de títulos
    QLineEdit*   inlineEdit = nullptr;
    QPushButton* editingBtn = nullptr;

    // legado (single dial) — pode remover se não usar mais
    int   accum               = 0;
    int   lastDial[8]         = {0};
    float currentFaderValue   = 0.0f;

    // meter
    //QTimer meterDecayTimer;

    // UI arrays
    QPushButton *buttons[NUMBER_OF_CHANNELS]{};
    QButtonGroup *group = nullptr;
    QButtonGroup *sceneGroup = nullptr;
    // QButtonGroup *labelChanGroup = nullptr;

    QButtonGroup *helpGroup = nullptr;
    QPushButton *helpButtons[NUMBER_OF_HELPS];

    QPushButton *titlesArray[NUMBER_OF_CHANNELS]{};
    ModernButton *pbTauArray[NUMBER_OF_SCENES]{};     // <- cenas (6)

    ModernButton *pbPlus[NUMBER_OF_CHANNELS]{};
    ModernButton *pbMinus[NUMBER_OF_CHANNELS]{};

    QLabel *labelsPercentArray[NUMBER_OF_CHANNELS]{};
    ModernProgressBar *percBarsArray[NUMBER_OF_CHANNELS]{};
    QProgressBar*  meterBarsArray[NUMBER_OF_CHANNELS]{};

    MetersWidget *metersWidget;

    QStringList helpTexts;

    QDial* dials[NUMBER_OF_CHANNELS]{};
    int   accumArr[NUMBER_OF_CHANNELS]{};            // 0..10000
    int   lastDialArr[NUMBER_OF_CHANNELS]{};         // 0..999
    float currentFaderArr[NUMBER_OF_CHANNELS]{};     // 0..1

    void loadChannelLabels();
    void appendLog(const QString &msg,
                   const QString &color = "white",
                   bool bold = false,
                   bool italic = false);

    bool startedMsgs = false;

    // ---- LR fader (estado próprio) ----
    int   lastDialLR = 0;
    int   accumLR    = 0;       // 0..10000
    float currentFaderLR = 0.0f;
    bool  draggingLR = false;
    QTimer* sendTimerLR = nullptr;

    QString html_start  = "<div style='text-align: justify;'>";
    QString html_end = "</div>";

    ChannelMeterWidget *meterFrameArray[NUMBER_OF_CHANNELS];
    QFrame *metersFrames[NUMBER_OF_CHANNELS];

    // mainwindow.h
    QFrame*             meterLRFrames[2] = {};      // ui->frameMeter_L / ui->frameMeter_R
    ChannelMeterWidget* meterLR[2]       = {};

    PercentBarWidget* volBars[NUMBER_OF_CHANNELS];
    QFrame*          volFrames[NUMBER_OF_CHANNELS]{};


};

#endif // MAINWINDOW_H
