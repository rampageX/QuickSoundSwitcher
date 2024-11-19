#ifndef PANEL_H
#define PANEL_H

#include <QMainWindow>
#include <QShowEvent>
#include <QCloseEvent>
#include <QPoint>
#include <QPropertyAnimation>
#include "AudioManager.h"
#include <Windows.h>
namespace Ui {
class Panel;
}

class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget *parent = nullptr);
    ~Panel() override;
    void fadeIn();
    void fadeOut();
    static Panel* panelInstance;

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::Panel *ui;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);  // Mouse hook callback
    static HHOOK mouseHook;
    static HWND hwndPanel;

    void installMouseHook();
    void uninstallMouseHook();
    void populateComboBoxes();
    void setSliders();
    void setButtons();
    void setFrames();
    void updateUi();
    void populateApplications();

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onOutputValueChanged();
    void onInputValueChanged();
    void onOutputMuteButtonPressed();
    void onInputMuteButtonPressed();
    void outputAudioMeter();
    void inputAudioMeter();

signals:
    void volumeChanged();
    void lostFocus();
    void fadeOutFinished();
    void outputMuteChanged();
    void inputMuteChanged();
};

#endif // PANEL_H
