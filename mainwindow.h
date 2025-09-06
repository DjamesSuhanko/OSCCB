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
// #include <titledialog.h> // (se não estiver usando, pode remover)

#define NUMBER_OF_CHANNELS 8
#define NUMBER_OF_SCENES   6
#define PROFILE_INI "profiles.ini"

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
    void onOscMeter(float val01);
    void onPlusClicked();
    void onMinusClicked();

private slots:
    void updateMeterDecay();
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
    int    meterInstant  = 0;
    int    meterDisplay  = 0;
    QTimer meterDecayTimer;

    // UI arrays
    QPushButton *buttons[NUMBER_OF_CHANNELS]{};
    QButtonGroup *group = nullptr;
    QButtonGroup *sceneGroup = nullptr;
    // QButtonGroup *labelChanGroup = nullptr;

    QPushButton *titlesArray[NUMBER_OF_CHANNELS]{};
    QPushButton *pbTauArray[NUMBER_OF_SCENES]{};     // <- cenas (6)

    QPushButton *pbPlus[NUMBER_OF_CHANNELS]{};
    QPushButton *pbMinus[NUMBER_OF_CHANNELS]{};

    QLabel *labelsPercentArray[NUMBER_OF_CHANNELS]{};
    QProgressBar *percBarsArray[NUMBER_OF_CHANNELS]{};
    QProgressBar*  meterBarsArray[NUMBER_OF_CHANNELS]{};

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
};

#endif // MAINWINDOW_H
