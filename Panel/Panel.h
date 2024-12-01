#ifndef PANEL_H
#define PANEL_H

#include "AudioManager.h"
#include <QVBoxLayout>
#include <QMainWindow>

namespace Ui {
class Panel;
}

class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget *parent = nullptr);
    ~Panel() override;
    void animateIn(QRect trayIconGeometry);
    void animateOut();
    bool mergeApps;
    void populateApplications();
    void populateComboBoxes();
    void setSliders();
    void setButtons();
    bool isAnimating;
    bool visible;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::Panel *ui;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;

    void setFrames();
    void updateUi();
    void addApplicationControls(QVBoxLayout *vBoxLayout, const QList<Application> &apps);
    QColor borderColor;
    void clearLayout(QLayout *layout);
    void toggleAllWidgets(QWidget *parent, bool state);

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onOutputValueChanged();
    void onInputValueChanged();
    void onOutputMuteButtonPressed();
    void onInputMuteButtonPressed();
    void outputAudioMeter();
    void inputAudioMeter();
    void onOutputMuteStateChanged(bool state);
    void onVolumeChangedWithTray();

signals:
    void volumeChanged();
    void outputMuteChanged();
};

#endif // PANEL_H
