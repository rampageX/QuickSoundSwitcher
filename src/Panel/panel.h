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
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Panel *ui;
    QList<AudioManager::AudioDevice> playbackDevices;
    QList<AudioManager::AudioDevice> recordingDevices;

    void populateComboBoxes();
    void setAudioDevice(const QString& deviceId);
    void animatePanelIn();
    void animatePanelOut();

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onAnimationFinished();

signals:
    void closed();
};

#endif // PANEL_H
