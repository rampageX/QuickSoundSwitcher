#include "updatemanager.h"

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_reply(nullptr)
{
}

void UpdateManager::startUpdate(const QString& downloadUrl, const QString& targetPath)
{
    m_targetPath = targetPath;

    // Set correct extension based on URL
    QString fileName = QUrl(downloadUrl).fileName();
    if (fileName.isEmpty()) {
        if (downloadUrl.contains(".zip")) {
            fileName = "update.zip";
        } else {
            fileName = "update.exe";
        }
    }

    m_tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                 "/QSSUpdate/" + fileName;

    // Create temp directory
    QDir().mkpath(QFileInfo(m_tempPath).dir().path());

    qDebug() << "Temp path:" << m_tempPath;
    qDebug() << "Download URL:" << downloadUrl;
    qDebug() << "Target path:" << m_targetPath;

    emit statusChanged("Downloading update...");
    emit progressChanged(0.0);

    QNetworkRequest request(downloadUrl);
    request.setRawHeader("User-Agent", "QSSUpdater");

    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &UpdateManager::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateManager::onDownloadFinished);
}

void UpdateManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        double progress = static_cast<double>(bytesReceived) / bytesTotal;
        emit progressChanged(progress);

        double mbReceived = bytesReceived / 1024.0 / 1024.0;
        double mbTotal = bytesTotal / 1024.0 / 1024.0;
        emit statusChanged(QString("Downloading... %.1f MB / %.1f MB").arg(mbReceived, 0, 'f', 1).arg(mbTotal, 0, 'f', 1));
    }
}

void UpdateManager::onDownloadFinished()
{
    if (!m_reply) return;

    if (m_reply->error() != QNetworkReply::NoError) {
        emit error("Download failed: " + m_reply->errorString());
        m_reply->deleteLater();
        return;
    }

    QByteArray downloadedData = m_reply->readAll();
    qDebug() << "Downloaded" << downloadedData.size() << "bytes";

    // Check the actual content type
    QString contentType = m_reply->header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << "Content-Type:" << contentType;

    // Check the first few bytes to identify file type
    if (downloadedData.size() > 4) {
        QByteArray header = downloadedData.left(4);
        qDebug() << "File header (hex):" << header.toHex();

        // ZIP files start with "PK" (0x504B)
        if (header.startsWith("PK")) {
            qDebug() << "File appears to be a ZIP archive";
        } else if (header.startsWith("MZ")) {
            qDebug() << "File appears to be an EXE file";
        }
    }

    // Save downloaded file
    QFile file(m_tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Failed to save downloaded file");
        m_reply->deleteLater();
        return;
    }

    file.write(downloadedData);
    file.close();
    m_reply->deleteLater();
    m_reply = nullptr;

    emit progressChanged(1.0);
    QTimer::singleShot(500, this, &UpdateManager::performBackup);
}

void UpdateManager::performBackup()
{
    emit statusChanged("Creating backup...");
    emit progressChanged(0.0);

    if (isQSSRunning()) {
        emit statusChanged("Waiting for QuickSoundSwitcher to close...");
        waitForQSSToClose();
    }

    QString backupPath = m_targetPath + ".backup";
    if (QFile::exists(m_targetPath)) {
        QFile::remove(backupPath);
        if (!QFile::copy(m_targetPath, backupPath)) {
            emit error("Failed to create backup");
            return;
        }
    }

    emit progressChanged(1.0);
    QTimer::singleShot(500, this, &UpdateManager::installUpdate);
}

void UpdateManager::installUpdate()
{
    emit statusChanged("Installing update...");
    emit progressChanged(0.0);

    // Additional wait to ensure processes are fully closed
    QThread::msleep(2000);

    // Check what type of file we downloaded
    QFile file(m_tempPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Cannot read downloaded file");
        return;
    }

    QByteArray header = file.read(4);
    file.close();

    qDebug() << "File to install:" << m_tempPath;
    qDebug() << "File exists:" << QFile::exists(m_tempPath);
    qDebug() << "File size:" << QFileInfo(m_tempPath).size() << "bytes";
    qDebug() << "File header:" << header.toHex();

    if (header.startsWith("PK")) {
        // Handle ZIP file
        qDebug() << "Installing from ZIP file";

        QString extractPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/QSSUpdate/extracted/";
        QDir().mkpath(extractPath);

        qDebug() << "Extract path:" << extractPath;

        // Extract using PowerShell
        QProcess unzipProcess;
        QString command = QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                              .arg(m_tempPath).arg(extractPath);

        qDebug() << "PowerShell command:" << command;

        unzipProcess.start("powershell", QStringList() << "-command" << command);
        unzipProcess.waitForFinished(30000);

        qDebug() << "PowerShell exit code:" << unzipProcess.exitCode();
        qDebug() << "PowerShell stdout:" << unzipProcess.readAllStandardOutput();
        qDebug() << "PowerShell stderr:" << unzipProcess.readAllStandardError();

        if (unzipProcess.exitCode() != 0) {
            emit error("Failed to extract ZIP: " + unzipProcess.readAllStandardError());
            return;
        }

        emit progressChanged(0.3);

        // Check what was extracted
        QDir extractDir(extractPath);
        QStringList extractedItems = extractDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << "Extracted items:" << extractedItems;

        // Look for the QuickSoundSwitcher folder
        QString appFolderPath;
        for (const QString& item : extractedItems) {
            QString itemPath = extractPath + "/" + item;
            if (QDir(itemPath).exists() && item.contains("QuickSoundSwitcher", Qt::CaseInsensitive)) {
                appFolderPath = itemPath;
                break;
            }
        }

        if (appFolderPath.isEmpty()) {
            emit error("QuickSoundSwitcher folder not found in extracted files");
            return;
        }

        qDebug() << "Found app folder:" << appFolderPath;

        // Check the structure inside the app folder
        QDir appDir(appFolderPath);
        QStringList appContents = appDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << "App folder contents:" << appContents;

        // Check for bin/QuickSoundSwitcher.exe
        QString binPath = appFolderPath + "/bin";
        QString newExePath = binPath + "/QuickSoundSwitcher.exe";
        if (!QFile::exists(newExePath)) {
            emit error("QuickSoundSwitcher.exe not found at: " + newExePath);
            return;
        }

        qDebug() << "Found new executable:" << newExePath;
        qDebug() << "New exe size:" << QFileInfo(newExePath).size() << "bytes";

        // Get the target installation directory
        QString currentExeDir = QFileInfo(m_targetPath).dir().path();
        qDebug() << "Current installation directory:" << currentExeDir;

        emit statusChanged("Backing up current installation...");
        emit progressChanged(0.4);

        // Create backup of current installation
        QString backupPath = currentExeDir + "_backup";
        if (QDir(currentExeDir).exists()) {
            if (QDir(backupPath).exists()) {
                QDir(backupPath).removeRecursively();
            }

            if (!copyDirectoryContents(currentExeDir, backupPath)) {
                emit error("Failed to create backup of current installation");
                return;
            }
            qDebug() << "Created backup at:" << backupPath;
        }

        emit statusChanged("Installing new version...");
        emit progressChanged(0.6);

        // First, copy all files from bin/ to the installation directory
        if (!copyDirectoryContents(binPath, currentExeDir)) {
            emit error("Failed to copy executable and DLLs");
            // Restore backup
            if (QDir(backupPath).exists()) {
                QDir(currentExeDir).removeRecursively();
                copyDirectoryContents(backupPath, currentExeDir);
            }
            return;
        }

        qDebug() << "Copied bin contents to installation directory";

        // Then copy other folders (plugins, etc.) to the installation directory
        QDir sourceAppDir(appFolderPath);
        QStringList otherFolders = sourceAppDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString& folder : otherFolders) {
            if (folder == "bin") continue; // Skip bin folder, already copied

            QString sourceFolderPath = appFolderPath + "/" + folder;
            QString destFolderPath = currentExeDir + "/" + folder;

            qDebug() << "Copying folder:" << folder;

            if (!copyDirectoryContents(sourceFolderPath, destFolderPath)) {
                emit error("Failed to copy folder: " + folder);
                // Restore backup
                if (QDir(backupPath).exists()) {
                    QDir(currentExeDir).removeRecursively();
                    copyDirectoryContents(backupPath, currentExeDir);
                }
                return;
            }
        }

        qDebug() << "Successfully copied new installation";

        emit progressChanged(0.9);

        // Verify the new executable exists and is valid
        if (!QFile::exists(m_targetPath)) {
            emit error("Installation failed - executable not found after copy: " + m_targetPath);
            return;
        }

        QFileInfo newFileInfo(m_targetPath);
        qDebug() << "Final executable size:" << newFileInfo.size() << "bytes";

        if (newFileInfo.size() < 1000000) {  // Sanity check
            emit error("Installation failed - new executable appears corrupted (size: " + QString::number(newFileInfo.size()) + " bytes)");
            // Try to restore backup
            if (QDir(backupPath).exists()) {
                QFile::remove(m_targetPath);
                QString backupExe = backupPath + "/" + QFileInfo(m_targetPath).fileName();
                if (QFile::exists(backupExe)) {
                    QFile::copy(backupExe, m_targetPath);
                }
            }
            return;
        }

        qDebug() << "ZIP installation completed successfully";

        // Clean up backup after successful installation
        QTimer::singleShot(5000, [backupPath]() {
            if (QDir(backupPath).exists()) {
                QDir(backupPath).removeRecursively();
            }
        });

    } else if (header.startsWith("MZ")) {
        // Handle EXE file (Inno Setup installer)
        qDebug() << "Installing from EXE file";

        emit statusChanged("Running installer...");

        QProcess installer;
        QStringList args;
        args << "/VERYSILENT" << "/NORESTART" << "/SUPPRESSMSGBOXES";

        qDebug() << "Running installer:" << m_tempPath << "with args:" << args;

        installer.start(m_tempPath, args);

        // Show progress while installer runs
        QTimer progressTimer;
        int progressValue = 50;
        connect(&progressTimer, &QTimer::timeout, [this, &progressValue]() {
            progressValue = (progressValue + 5) % 100;
            if (progressValue < 50) progressValue = 50; // Keep above 50%
            emit progressChanged(progressValue / 100.0);
        });
        progressTimer.start(1000);

        bool finished = installer.waitForFinished(120000); // 2 minute timeout
        progressTimer.stop();

        if (!finished) {
            installer.kill();
            emit error("Installer timed out");
            return;
        }

        int exitCode = installer.exitCode();
        qDebug() << "Installer exit code:" << exitCode;

        if (exitCode != 0) {
            QString errorOutput = installer.readAllStandardError();
            emit error(QString("Installer failed with code %1: %2").arg(exitCode).arg(errorOutput));
            return;
        }

        qDebug() << "EXE installation completed successfully";

        // For EXE installer, don't launch the app - installer handles it
        emit statusChanged("Installation completed successfully!");
        emit progressChanged(1.0);

        cleanup();

        QTimer::singleShot(2000, []() {
            QCoreApplication::quit();
        });
        return;

    } else {
        emit error("Unknown file format downloaded");
        return;
    }

    // Common completion for ZIP files
    emit progressChanged(1.0);
    QTimer::singleShot(1000, this, &UpdateManager::launchApp);
}

void UpdateManager::launchApp()
{
    emit statusChanged("Launching QuickSoundSwitcher...");

    if (!QProcess::startDetached(m_targetPath)) {
        emit error("Failed to launch updated application");
        return;
    }

    emit statusChanged("Update completed successfully!");

    cleanup();

    QTimer::singleShot(2000, this, &UpdateManager::finished);
}

void UpdateManager::cleanup()
{
    // Clean up temporary files
    QFile::remove(m_tempPath);

    // Clean up temp directory
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/QSSUpdate/";
    QDir(tempDir).removeRecursively();
}

bool UpdateManager::isQSSRunning()
{
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq QuickSoundSwitcher.exe");
    process.waitForFinished(3000);
    return process.readAllStandardOutput().contains("QuickSoundSwitcher.exe");
}

void UpdateManager::waitForQSSToClose()
{
    int attempts = 0;
    while (isQSSRunning() && attempts < 30) { // 30 seconds max
        QThread::msleep(1000);
        QCoreApplication::processEvents();
        attempts++;

        if (attempts % 5 == 0) {
            emit statusChanged(QString("Waiting for application to close... (%1/30)").arg(attempts));
        }
    }
}

bool UpdateManager::copyDirectoryContents(const QString& source, const QString& destination)
{
    QDir sourceDir(source);
    QDir destDir(destination);

    if (!sourceDir.exists()) {
        emit error("Source directory missing: " + source);
        return false;
    }

    // Create destination directory with retries
    if (!destDir.exists()) {
        bool created = false;
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (destDir.mkpath(".")) {
                created = true;
                break;
            }
            QThread::msleep(1000);
        }
        if (!created) {
            emit error("Cannot create destination directory: " + destination);
            return false;
        }
    }

    QFileInfoList entries = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (int i = 0; i < entries.count(); ++i) {
        const QFileInfo& entry = entries[i];
        QString sourcePath = entry.absoluteFilePath();
        QString destPath = destDir.absoluteFilePath(entry.fileName());

        // Update status for user feedback
        emit statusChanged(QString("Installing: %1 (%2/%3)").arg(entry.fileName()).arg(i+1).arg(entries.count()));

        if (entry.isDir()) {
            if (!copyDirectoryContents(sourcePath, destPath)) {
                emit error("Failed to copy folder: " + entry.fileName());
                return false;
            }
        } else {
            // Enhanced file copying with antivirus handling
            if (!copyFileWithRetries(sourcePath, destPath, entry.fileName())) {
                return false;
            }
        }

        // Small delay to let antivirus process
        QThread::msleep(50);
    }

    return true;
}

bool UpdateManager::copyFileWithRetries(const QString& source, const QString& dest, const QString& fileName)
{
    // Remove existing file with multiple attempts
    if (QFile::exists(dest)) {
        bool removed = false;
        for (int attempt = 0; attempt < 10; ++attempt) {
            if (QFile::remove(dest)) {
                removed = true;
                break;
            }

            // Longer delays for stubborn files (likely antivirus scanning)
            emit statusChanged(QString("Waiting for file access: %1 (attempt %2/10)").arg(fileName).arg(attempt + 1));
            QThread::msleep(2000); // 2 second wait
        }

        if (!removed) {
            emit error(QString("Cannot replace file: %1 (may be in use or blocked by antivirus)").arg(fileName));
            return false;
        }
    }

    // Wait for any antivirus scanning to complete
    QThread::msleep(500);

    // Copy file with multiple attempts
    bool copied = false;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (QFile::copy(source, dest)) {
            copied = true;
            break;
        }

        emit statusChanged(QString("Retrying copy: %1 (attempt %2/5)").arg(fileName).arg(attempt + 1));
        QThread::msleep(3000); // 3 second wait between attempts
    }

    if (!copied) {
        emit error(QString("Failed to copy: %1 (antivirus may be blocking)").arg(fileName));
        return false;
    }

    // Verify the copied file
    QFileInfo sourceInfo(source);
    QFileInfo destInfo(dest);

    if (!destInfo.exists() || destInfo.size() != sourceInfo.size()) {
        emit error(QString("Copy verification failed: %1").arg(fileName));
        return false;
    }

    // Give antivirus time to scan the new file
    QThread::msleep(100);

    return true;
}
