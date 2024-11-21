#include "Panel.h"
#include "ui_Panel.h"
#include "Utils.h"
#include "QuickSoundSwitcher.h"
#include <QKeyEvent>
#include <QComboBox>
#include <QList>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QFont>

HHOOK Panel::mouseHook = nullptr;
HWND Panel::hwndPanel = nullptr;
Panel* Panel::panelInstance = nullptr;

Panel::Panel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Panel)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(width());
    AudioManager::initialize();

    panelInstance = this;
    hwndPanel = reinterpret_cast<HWND>(this->winId());
    installMouseHook();

    populateComboBoxes();
    //populateApplications();
    setSliders();
    setButtons();
    Utils::setFrameColorBasedOnWindow(this, ui->appFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->inputFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->outputFrame);

    QTimer *audioMeterTimer = new QTimer(this);
    connect(audioMeterTimer, &QTimer::timeout, this, &Panel::outputAudioMeter);
    connect(audioMeterTimer, &QTimer::timeout, this, &Panel::inputAudioMeter);
    audioMeterTimer->start(32);

    connect(ui->outputComboBox, &QComboBox::currentIndexChanged, this, &Panel::onOutputComboBoxIndexChanged);
    connect(ui->inputComboBox, &QComboBox::currentIndexChanged, this, &Panel::onInputComboBoxIndexChanged);
    connect(ui->outputVolumeSlider, &QSlider::valueChanged, this, &Panel::onOutputValueChanged);
    connect(ui->inputVolumeSlider, &QSlider::valueChanged, this, &Panel::onInputValueChanged);
    connect(ui->outputMuteButton, &QPushButton::pressed, this, &Panel::onOutputMuteButtonPressed);
    connect(ui->inputMuteButton, &QPushButton::pressed, this, &Panel::onInputMuteButtonPressed);
    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::muteStateChanged,
            this, &Panel::onMuteStateChanged);
    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::outputMuteStateChanged,
            this, &Panel::onOutputMuteStateChanged);
    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::volumeChangedWithTray,
            this, &Panel::onVolumeChangedWithTray);
}

Panel::~Panel()
{
    AudioManager::cleanup();
    uninstallMouseHook();
    delete ui;
}

void Panel::animateIn(QRect trayIconGeometry)
{
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - this->width() / 2; // Center horizontally
    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int startY = screenGeometry.bottom();  // Start from the bottom of the screen
    int targetY = trayIconGeometry.top() - this->height() - 12; // Final position

    this->move(panelX, startY); // Start at the bottom
    this->show();

    // Animation parameters
    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);

    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();
            this->move(panelX, targetY); // Ensure final position is set
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);

        ++currentStep;
    });
}

void Panel::animateOut(QRect trayIconGeometry)
{
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - this->width() / 2; // Center horizontally
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int startY = this->y();
    int targetY = screenGeometry.bottom(); // Move to the bottom of the screen

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);

    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();

            emit panelClosed();
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);

        ++currentStep;
    });
}

void Panel::installMouseHook()
{
    if (!mouseHook) {
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
    }
}

void Panel::uninstallMouseHook()
{
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = nullptr;
    }
}

// Low-level mouse hook callback
LRESULT CALLBACK Panel::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        // Only process mouse click events (left or right click)
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            // Get the current position of the mouse
            MSLLHOOKSTRUCT *pMouseStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            POINT mousePos = pMouseStruct->pt;

            // Get the window's position and size
            RECT rect;
            GetWindowRect(hwndPanel, &rect);  // Use stored HWND

            if (!PtInRect(&rect, mousePos)) {
                // Use the static panelInstance to emit the lostFocus signal
                if (panelInstance) {
                    emit panelInstance->lostFocus();
                }
            }
        }
    }

    // Pass the event to the next hook in the chain
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void Panel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // Prevent unused parameter warning
    QPainter painter(this);

    // Get the main background color from the widget's palette
    QColor main_bg_color = this->palette().color(QPalette::Window);

    // Set the brush to fill the rectangle with the background color
    painter.setBrush(main_bg_color);
    painter.setPen(Qt::NoPen); // No border for background fill

    // Create a rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(this->rect().adjusted(1, 1, -1, -1), 8, 8); // Adjusted for inner fill

    // Draw the filled rounded rectangle
    painter.drawPath(path);

    // Now set the pen for the border with 30% alpha
    QColor borderColor = QColor(255, 255, 255, 32); // White with 30% alpha (255 * 0.3 = 77)
    QPen borderPen(borderColor);
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush); // No fill for border

    // Draw the border around the adjusted rectangle
    QPainterPath borderPath;
    borderPath.addRoundedRect(this->rect().adjusted(0, 0, -1, -1), 8, 8); // Full size for outer border
    painter.drawPath(borderPath);
}

void Panel::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit lostFocus();
    }

    QWidget::keyPressEvent(event);
}

void Panel::populateComboBoxes()
{
    AudioManager::enumeratePlaybackDevices(playbackDevices);
    AudioManager::enumerateRecordingDevices(recordingDevices);

    ui->outputComboBox->clear();
    ui->inputComboBox->clear();

    int defaultPlaybackIndex = -1;
    int defaultRecordingIndex = -1;

    for (const AudioDevice &device : playbackDevices) {
        ui->outputComboBox->addItem(device.shortName);

        if (device.isDefault) {
            defaultPlaybackIndex = ui->outputComboBox->count() - 1;
        }
    }

    if (defaultPlaybackIndex != -1) {
        ui->outputComboBox->setCurrentIndex(defaultPlaybackIndex);
    }

    for (const AudioDevice &device : recordingDevices) {
        ui->inputComboBox->addItem(device.shortName);

        if (device.isDefault) {
            defaultRecordingIndex = ui->inputComboBox->count() - 1;
        }
    }

    if (defaultRecordingIndex != -1) {
        ui->inputComboBox->setCurrentIndex(defaultRecordingIndex);
    }
}

void Panel::setSliders()
{
    ui->outputVolumeSlider->setValue(AudioManager::getPlaybackVolume());
    ui->inputVolumeSlider->setValue(AudioManager::getRecordingVolume());
}

void Panel::setButtons()
{
    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, AudioManager::getPlaybackMute()));
    ui->outputMuteButton->setIconSize(QSize(16, 16));
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, AudioManager::getRecordingMute()));
    ui->inputMuteButton->setIconSize(QSize(16, 16));
}

void Panel::setFrames()
{
    Utils::setFrameColorBasedOnWindow(this, ui->outputFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->inputFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->appFrame);
}

void Panel::onOutputComboBoxIndexChanged(int index)
{
    if (index < 0 || index >= playbackDevices.size()) {
        return;
    }

    const AudioDevice &selectedDevice = playbackDevices[index];
    AudioManager::setDefaultEndpoint(selectedDevice.id);

    updateUi();
}

void Panel::onInputComboBoxIndexChanged(int index)
{
    if (index < 0 || index >= recordingDevices.size()) {
        return;
    }

    const AudioDevice &selectedDevice = recordingDevices[index];
    AudioManager::setDefaultEndpoint(selectedDevice.id);

    updateUi();
}

void Panel::onOutputValueChanged()
{
    bool playbackMute = AudioManager::getPlaybackMute();
    if (playbackMute) {
        AudioManager::setPlaybackMute(false);
        ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, !playbackMute));
        emit outputMuteChanged();
    }

    AudioManager::setPlaybackVolume(ui->outputVolumeSlider->value());
    emit volumeChanged();
}

void Panel::onInputValueChanged()
{
    bool recordingMute = AudioManager::getRecordingMute();
    if (recordingMute) {
        ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, !recordingMute));
        AudioManager::setRecordingMute(false);
        emit inputMuteChanged();
    }

    AudioManager::setRecordingVolume(ui->inputVolumeSlider->value());
}

void Panel::onOutputMuteButtonPressed()
{
    bool playbackMute = AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(!playbackMute);
    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, !playbackMute));
    emit outputMuteChanged();
}

void Panel::onInputMuteButtonPressed()
{
    bool recordingMute = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!recordingMute);
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, !recordingMute));
    emit inputMuteChanged();
}

void Panel::onMuteStateChanged()
{
    bool recordingMute = AudioManager::getRecordingMute();
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, recordingMute));
}

void Panel::onOutputMuteStateChanged(bool state)
{
    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, state));
}

void Panel::onVolumeChangedWithTray()
{
    ui->outputVolumeSlider->blockSignals(true);
    ui->outputVolumeSlider->setValue(AudioManager::getPlaybackVolume());
    ui->outputVolumeSlider->blockSignals(false);
}

void Panel::outputAudioMeter() {
    int level = AudioManager::getPlaybackAudioLevel();
    ui->outputAudioMeter->setValue(level);
}

void Panel::inputAudioMeter() {
    int level = AudioManager::getRecordingAudioLevel();
    ui->inputAudioMeter->setValue(level);
}

void Panel::updateUi()
{
    ui->outputVolumeSlider->setValue(AudioManager::getPlaybackVolume());
    ui->inputVolumeSlider->setValue(AudioManager::getRecordingVolume());

    bool playbackMute = AudioManager::getPlaybackMute();
    bool recordingMute = AudioManager::getRecordingMute();

    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, playbackMute));
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, recordingMute));
}

void Panel::populateApplications()
{
    if (!ui->appFrame->layout()) {
        ui->appFrame->setLayout(new QVBoxLayout);
    }

    QVBoxLayout *vBoxLayout = qobject_cast<QVBoxLayout *>(ui->appFrame->layout());
    QList<Application> applications = AudioManager::enumerateAudioApplications();

    int marginLeft, marginTop, marginRight, marginBottom;
    vBoxLayout->getContentsMargins(&marginLeft, &marginTop, &marginRight, &marginBottom);
    int spacing = vBoxLayout->spacing();
    int totalHeight = marginTop + marginBottom;

    bool shouldDisplayLabel = applications.isEmpty() || (applications.size() == 1 && applications[0].name == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202");

    if (shouldDisplayLabel) {
        QLabel *label = new QLabel(tr("All quiet for now."), ui->appFrame);
        label->setAlignment(Qt::AlignCenter);

        QFont labelFont = label->font();
        labelFont.setItalic(true);
        label->setFont(labelFont);

        vBoxLayout->addWidget(label);

        totalHeight += label->sizeHint().height() + spacing;
    } else {
        if (mergeApps) {
            QMap<QString, QList<Application>> groupedApps;

            // Group applications by executableName if mergeApps is true
            for (const Application &app : applications) {
                if (app.name == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202") {
                    continue;
                }
                if (app.executableName == "QuickSoundSwitcher") {
                    continue;
                }
                groupedApps[app.executableName].append(app);
            }

            for (auto it = groupedApps.begin(); it != groupedApps.end(); ++it) {
                const QList<Application> &apps = it.value();

                QGridLayout *gridLayout = new QGridLayout;
                gridLayout->setVerticalSpacing(0);
                gridLayout->setHorizontalSpacing(12);

                QPushButton *muteButton = new QPushButton(ui->appFrame);
                muteButton->setFixedSize(35, 35);

                // Determine if any app in the group is muted
                bool isGroupMuted = std::any_of(apps.begin(), apps.end(), [](const Application &app) {
                    return app.isMuted;
                });

                QIcon originalIcon = apps.first().icon;
                QPixmap originalPixmap = originalIcon.pixmap(16, 16);

                if (isGroupMuted) {
                    QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                    muteButton->setIcon(mutedIcon);
                } else {
                    muteButton->setIcon(originalIcon);
                }

                connect(muteButton, &QPushButton::clicked, [apps, muteButton, originalPixmap]() mutable {
                    // Toggle mute state for all apps in the group
                    bool newMuteState = !AudioManager::getApplicationMute(apps.first().id);
                    for (const Application &app : apps) {
                        AudioManager::setApplicationMute(app.id, newMuteState);
                    }

                    if (newMuteState) {
                        QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                        muteButton->setIcon(mutedIcon);
                    } else {
                        muteButton->setIcon(QIcon(originalPixmap));
                    }
                });

                QSpacerItem *spacer1 = new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding);

                QLabel *nameLabel = new QLabel(apps.first().executableName, ui->appFrame);
                nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

                QFont nameLabelFont = nameLabel->font();
                nameLabelFont.setPointSize(10);
                nameLabel->setFont(nameLabelFont);

                // Determine the lowest volume in the group
                int minVolume = std::min_element(apps.begin(), apps.end(), [](const Application &a, const Application &b) {
                                    return a.volume < b.volume;
                                })->volume;

                QSlider *slider = new QSlider(Qt::Horizontal, ui->appFrame);
                slider->setRange(0, 100);
                slider->setValue(minVolume);

                connect(slider, &QSlider::valueChanged, [apps](int value) {
                    for (const Application &app : apps) {
                        AudioManager::setApplicationVolume(app.id, value);
                    }
                });

                QSpacerItem *spacer2 = new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding);
                QSpacerItem *spacer3 = new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding);
                QSpacerItem *spacer4 = new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding);

                gridLayout->addWidget(muteButton, 0, 0, 6, 1);  // Mute button spans 6 rows (row 0 to row 5)
                gridLayout->addItem(spacer1, 1, 1);              // Spacer in row 1
                gridLayout->addWidget(nameLabel, 2, 1, 1, 1);    // Label in row 2
                gridLayout->addWidget(slider, 3, 1, 1, 1);       // Slider in row 3
                gridLayout->addItem(spacer2, 4, 1);              // Spacer in row 4
                gridLayout->addItem(spacer3, 5, 1);              // Spacer in row 5
                gridLayout->addItem(spacer4, 6, 1);              // Spacer in row 6

                gridLayout->setColumnStretch(1, 1);
                vBoxLayout->addLayout(gridLayout);
                totalHeight += muteButton->height();
            }
        } else {
            // Individual controls for each application
            for (const Application &app : applications) {
                if (app.name == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202") {
                    continue;
                }
                if (app.executableName == "QuickSoundSwitcher") {
                    continue;
                }

                QGridLayout *gridLayout = new QGridLayout;
                gridLayout->setVerticalSpacing(0);

                QPushButton *muteButton = new QPushButton(ui->appFrame);
                muteButton->setFixedSize(35, 35);

                QIcon originalIcon = app.icon;
                QPixmap originalPixmap = originalIcon.pixmap(16, 16);

                if (app.isMuted) {
                    QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                    muteButton->setIcon(mutedIcon);
                } else {
                    muteButton->setIcon(originalIcon);
                }

                connect(muteButton, &QPushButton::clicked, [appId = app.id, muteButton, originalPixmap]() mutable {
                    bool newMuteState = !AudioManager::getApplicationMute(appId);
                    AudioManager::setApplicationMute(appId, newMuteState);

                    if (newMuteState) {
                        QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                        muteButton->setIcon(mutedIcon);
                    } else {
                        muteButton->setIcon(QIcon(originalPixmap));
                    }
                });

                QLabel *nameLabel = new QLabel(app.executableName, ui->appFrame);
                nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

                QFont nameLabelFont = nameLabel->font();
                nameLabelFont.setPointSize(10);
                nameLabel->setFont(nameLabelFont);

                QSlider *slider = new QSlider(Qt::Horizontal, ui->appFrame);
                slider->setRange(0, 100);
                slider->setValue(app.volume);

                connect(slider, &QSlider::valueChanged, [appId = app.id](int value) {
                    AudioManager::setApplicationVolume(appId, value);
                });

                QSpacerItem *spacer1 = new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding);
                QSpacerItem *spacer2 = new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding);

                gridLayout->addWidget(muteButton, 0, 0, 4, 1);  // Mute button spans 4 rows (row 0 to row 3)
                gridLayout->addItem(spacer1, 1, 1);              // Spacer in row 1
                gridLayout->addWidget(nameLabel, 2, 1, 1, 1);    // Label in row 2
                gridLayout->addWidget(slider, 3, 1, 1, 1);       // Slider in row 3
                gridLayout->addItem(spacer2, 4, 1);              // Spacer in row 4

                gridLayout->setColumnStretch(1, 1);
                vBoxLayout->addLayout(gridLayout);
                totalHeight += muteButton->height();
            }
        }
    }

    // Adjust total height by subtracting the last spacing (no spacing after the last row)
    totalHeight -= spacing;

    ui->appFrame->setMinimumHeight(totalHeight);
    setMinimumHeight(height() + 12 + ui->appFrame->height());
}
