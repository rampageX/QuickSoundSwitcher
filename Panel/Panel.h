#ifndef PANEL_H
#define PANEL_H

#include <QMainWindow>
#include "AudioManager.h"

namespace Ui {
class Panel;
}

class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget *parent = nullptr);
    ~Panel() override;
    static Panel* panelInstance;
    void animateIn(QRect trayIconGeometry);
    void animateOut(QRect trayIconGeometry);
    bool mergeApps;
    void populateApplications();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::Panel *ui;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK mouseHook;
    static HWND hwndPanel;

    void installMouseHook();
    void uninstallMouseHook();
    void populateComboBoxes();
    void setSliders();
    void setButtons();
    void setFrames();
    void updateUi();

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onOutputValueChanged();
    void onInputValueChanged();
    void onOutputMuteButtonPressed();
    void onInputMuteButtonPressed();
    void outputAudioMeter();
    void inputAudioMeter();
    void onMuteStateChanged();
    void onOutputMuteStateChanged(bool state);
    void onVolumeChangedWithTray();

signals:
    void volumeChanged();
    void lostFocus();
    void fadeOutFinished();
    void outputMuteChanged();
    void inputMuteChanged();
    void panelClosed();
};

#endif // PANEL_H
