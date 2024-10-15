#ifndef PANEL_H
#define PANEL_H

#include <QMainWindow>
#include <QShowEvent>
#include <QCloseEvent> // Include for close event
#include <QPoint>
#include <QPropertyAnimation>
#include "audiomanager.h"

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
    void showEvent(QShowEvent *event) override; // Override showEvent for animation
    void closeEvent(QCloseEvent *event) override; // Override closeEvent for closing animation

private:
    Ui::Panel *ui;
    QList<AudioManager::AudioDevice> playbackDevices;
    QList<AudioManager::AudioDevice> recordingDevices;

    void populateComboBoxes();
    void setAudioDevice(const QString& deviceId);
    void animatePanelIn(); // Method for opening animation
    void animatePanelOut(); // Method for closing animation

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onAnimationFinished();

signals:
    void closed(); // Signal to indicate panel closure
};

#endif // PANEL_H
