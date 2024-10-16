#ifndef PANEL_H
#define PANEL_H

#include <QMainWindow>
#include <QShowEvent>
#include <QCloseEvent> // Include for close event
#include <QPoint>
#include <QPropertyAnimation>
#include "audiomanager.h"

using namespace AudioManager;

namespace Ui {
class Panel;
}

class Panel : public QMainWindow
{
    Q_OBJECT

public:
    explicit Panel(QWidget *parent = nullptr);
    ~Panel() override;

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Panel *ui;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;

    void populateComboBoxes();
    void setSliders();
    void setButtons();
    void setAudioDevice(const QString& deviceId);
    void animatePanelIn();
    void animatePanelOut();
    bool userClicked;
    void setFrames();

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onAnimationFinished();
    void onOutputSliderPressed();
    void onOutputValueChanged(int value);
    void onOutputSliderReleased();
    void onInputSliderPressed();
    void onInputValueChanged(int value);
    void onInputSliderReleased();
    void onOutputMuteButtonPressed();
    void onInputMuteButtonPressed();

signals:
    void closed();
    void volumeChanged();
};

#endif // PANEL_H
