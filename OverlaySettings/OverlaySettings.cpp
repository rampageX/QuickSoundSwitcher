#include "overlaysettings.h"
#include "ui_overlaysettings.h"
#include "utils.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <shortcutmanager.h>

using namespace ShortcutManager;

OverlaySettings::OverlaySettings(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OverlaySettings)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    loadSettings();
    setFramesBackground();

    QList<QRadioButton*> radioButtons = {
        ui->topLeftCorner,
        ui->topRightCorner,
        ui->topCenter,
        ui->bottomLeftCorner,
        ui->bottomRightCorner,
        ui->bottomCenter,
        ui->leftCenter,
        ui->rightCenter
    };

    for (QRadioButton* button : radioButtons) {
        connect(button, &QRadioButton::clicked, this, &OverlaySettings::saveSettings);
    }

    connect(ui->overlayCheckBox, &QCheckBox::checkStateChanged, this, &OverlaySettings::onDisableOverlayStateChanged);
    connect(ui->soundCheckBox, &QCheckBox::checkStateChanged, this, &OverlaySettings::saveSettings);
    connect(ui->potatoModeCheckBox, &QCheckBox::checkStateChanged, this, &OverlaySettings::saveSettings);
}

OverlaySettings::~OverlaySettings()
{
    emit closed();
    delete ui;
}

void OverlaySettings::setFramesBackground()
{
    Utils::setFrameColorBasedOnWindow(this, ui->frame);
    Utils::setFrameColorBasedOnWindow(this, ui->frame_2);
    Utils::setFrameColorBasedOnWindow(this, ui->frame_3);
    Utils::setFrameColorBasedOnWindow(this, ui->frame_4);
}

void OverlaySettings::loadSettings()
{
    QString position = settings.value("overlayPosition", "bottomCenter").toString();

    QList<QPair<QRadioButton*, QString>> radioButtons = {
        { ui->bottomCenter, "bottomCenter" },
        { ui->bottomLeftCorner, "bottomLeftCorner" },
        { ui->bottomRightCorner, "bottomRightCorner" },
        { ui->topCenter, "topCenter" },
        { ui->topRightCorner, "topRightCorner" },
        { ui->topLeftCorner, "topLeftCorner" },
        { ui->leftCenter, "leftCenter" },
        { ui->rightCenter, "rightCenter" }
    };

    for (const auto& pair : radioButtons) {
        if (position == pair.second) {
            pair.first->setChecked(true);
            break;
        }
    }

    ui->overlayCheckBox->setChecked(settings.value("disableOverlay", false).toBool());
    ui->soundCheckBox->setChecked(settings.value("disableNotification", false).toBool());
    ui->potatoModeCheckBox->setChecked(settings.value("potatoMode", false).toBool());
}

void OverlaySettings::saveSettings()
{
    QString position;
    QList<QPair<QRadioButton*, QString>> radioButtons = {
        { ui->bottomCenter, "bottomCenter" },
        { ui->bottomLeftCorner, "bottomLeftCorner" },
        { ui->bottomRightCorner, "bottomRightCorner" },
        { ui->topCenter, "topCenter" },
        { ui->topRightCorner, "topRightCorner" },
        { ui->topLeftCorner, "topLeftCorner" },
        { ui->leftCenter, "leftCenter" },
        { ui->rightCenter, "rightCenter" }
    };

    for (const auto& pair : radioButtons) {
        if (pair.first->isChecked()) {
            position = pair.second;
            break;
        }
    }
    settings.setValue("overlayPosition", position);
    settings.setValue("disableOverlay", ui->overlayCheckBox->isChecked());
    settings.setValue("disableNotification", ui->soundCheckBox->isChecked());
    settings.setValue("potatoMode", ui->potatoModeCheckBox->isChecked());

    emit settingsChanged();
}

void OverlaySettings::onDisableOverlayStateChanged(Qt::CheckState state)
{
    bool isChecked = (state == Qt::Checked);

    ui->frame->setDisabled(isChecked);
    ui->label->setDisabled(isChecked);
    saveSettings();
}
