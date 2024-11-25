#include "MediaSessionWorker.h"
#include "Utils.h"
#include <QDebug>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <windows.h>

#pragma comment(lib, "runtimeobject.lib")

MediaSessionWorker::MediaSessionWorker(QObject* parent)
    : QObject(parent)
    , currentSession(nullptr)
    , monitoringEnabled(false)
{
}

MediaSessionWorker::~MediaSessionWorker()
{
    try
    {
        winrt::uninit_apartment();
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Error during COM deinitialization:" << ex.what();
    }

    //CoUninitialize();
}

void MediaSessionWorker::process()
{
    try
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);

        auto task = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync();

        task.Completed([this](winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager> const& sender, winrt::Windows::Foundation::AsyncStatus status)
                       {
                           if (status == winrt::Windows::Foundation::AsyncStatus::Completed)
                           {
                               try
                               {
                                   auto sessionManager = sender.GetResults();
                                   currentSession = sessionManager.GetCurrentSession();

                                   if (currentSession)
                                   {
                                       MediaSession session;

                                       // Retrieve media properties from the session
                                       auto mediaProperties = currentSession.TryGetMediaPropertiesAsync().get();
                                       session.title = QString::fromStdWString(mediaProperties.Title().c_str());
                                       session.artist = QString::fromStdWString(mediaProperties.Artist().c_str());

                                       // Get album art (icon)
                                       auto thumbnail = mediaProperties.Thumbnail();
                                       if (thumbnail)
                                       {
                                           auto stream = thumbnail.OpenReadAsync().get(); // Open the thumbnail stream
                                           if (stream)
                                           {
                                               // Create DataReader from the input stream
                                               auto inputStream = stream.GetInputStreamAt(0);
                                               winrt::Windows::Storage::Streams::DataReader reader(inputStream);
                                               uint32_t size = stream.Size();
                                               reader.LoadAsync(size).get(); // Load data into the reader

                                               // Convert to QByteArray
                                               QByteArray imageData;
                                               for (uint32_t i = 0; i < size; ++i)
                                               {
                                                   imageData.append(static_cast<char>(reader.ReadByte()));
                                               }

                                               // Load into QPixmap
                                               QPixmap pixmap;
                                               if (pixmap.loadFromData(imageData))
                                               {
                                                   session.icon = pixmap;  // Set the icon if loading is successful
                                               }
                                               else
                                               {
                                                   session.icon = Utils::getPlaceholderIcon();  // Fallback icon
                                               }
                                           }
                                           else
                                           {
                                               session.icon = Utils::getPlaceholderIcon();  // No stream available
                                           }
                                       }
                                       else
                                       {
                                           session.icon = Utils::getPlaceholderIcon();  // No thumbnail available
                                       }

                                       // Get playback control information
                                       auto playbackInfo = currentSession.GetPlaybackInfo();
                                       auto playbackControls = playbackInfo.Controls();
                                       session.canGoNext = playbackControls.IsNextEnabled();
                                       session.canGoPrevious = playbackControls.IsPreviousEnabled();

                                       // Get the playback state (Playing, Paused, Stopped)
                                       auto playbackStatus = playbackInfo.PlaybackStatus();
                                       switch (playbackStatus)
                                       {
                                       case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                                           session.playbackState = "Playing";
                                           break;
                                       case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                                           session.playbackState = "Paused";
                                           break;
                                       case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                                           session.playbackState = "Stopped";
                                           break;
                                       default:
                                           session.playbackState = "Unknown";
                                           break;
                                       }

                                       emit sessionReady(session);
                                   }
                                   else
                                   {
                                       emit sessionError("No active media session found.");
                                   }
                               }
                               catch (const std::exception& ex)
                               {
                                   emit sessionError(QString("Error during session handling: %1").arg(ex.what()));
                               }
                           }
                           else
                           {
                               emit sessionError("Failed to request media session.");
                           }
                       });
    }
    catch (const std::exception& ex)
    {
        emit sessionError(QString("Error initializing COM: %1").arg(ex.what()));
    }
}

void MediaSessionWorker::play()
{
    if (currentSession)
    {
        try
        {
            currentSession.TryPlayAsync();
        }
        catch (const std::exception& ex)
        {
            emit sessionError(QString("Failed to send play command: %1").arg(ex.what()));
        }
    }
}

void MediaSessionWorker::pause()
{
    if (currentSession)
    {
        try
        {
            currentSession.TryPauseAsync();
        }
        catch (const std::exception& ex)
        {
            emit sessionError(QString("Failed to send pause command: %1").arg(ex.what()));
        }
    }
}

void MediaSessionWorker::next()
{
    if (currentSession)
    {
        try
        {
            currentSession.TrySkipNextAsync();
        }
        catch (const std::exception& ex)
        {
            emit sessionError(QString("Failed to send next command: %1").arg(ex.what()));
        }
    }
}

void MediaSessionWorker::previous()
{
    if (currentSession)
    {
        try
        {
            currentSession.TrySkipPreviousAsync();
        }
        catch (const std::exception& ex)
        {
            emit sessionError(QString("Failed to send previous command: %1").arg(ex.what()));
        }
    }
}

void MediaSessionWorker::initializeSessionMonitoring()
{
    if (!currentSession) {
        return;
    }

    currentSession.MediaPropertiesChanged([this](auto const& sender, auto const&)
                                           {
                                               try
                                               {
                                                   auto mediaProperties = sender.TryGetMediaPropertiesAsync().get();
                                                   QString title = QString::fromStdWString(mediaProperties.Title().c_str());
                                                   QString artist = QString::fromStdWString(mediaProperties.Artist().c_str());

                                                   // Get album art (icon)
                                                   QIcon icon = Utils::getPlaceholderIcon(); // Default icon
                                                   auto thumbnail = mediaProperties.Thumbnail();
                                                   if (thumbnail)
                                                   {
                                                       auto stream = thumbnail.OpenReadAsync().get();
                                                       if (stream)
                                                       {
                                                           // Create DataReader from the input stream
                                                           auto inputStream = stream.GetInputStreamAt(0);
                                                           winrt::Windows::Storage::Streams::DataReader reader(inputStream);
                                                           uint32_t size = stream.Size();
                                                           reader.LoadAsync(size).get(); // Load data into the reader

                                                           // Convert to QByteArray
                                                           QByteArray imageData;
                                                           for (uint32_t i = 0; i < size; ++i)
                                                           {
                                                               imageData.append(static_cast<char>(reader.ReadByte()));
                                                           }

                                                           // Use QImage::fromData() and convert to QPixmap
                                                           QImage image = QImage::fromData(imageData);
                                                           QPixmap pixmap = QPixmap::fromImage(image); // Convert QImage to QPixmap
                                                           if (!pixmap.isNull()) {
                                                               icon = QIcon(pixmap);  // Convert QPixmap to QIcon
                                                           } else {
                                                               icon = Utils::getPlaceholderIcon();  // Fallback icon
                                                           }
                                                       }
                                                   }

                                                   emit sessionMediaPropertiesChanged(title, artist);
                                                   emit sessionIconChanged(icon); // Emit the QIcon
                                               }
                                               catch (const std::exception& ex)
                                               {
                                                   qDebug() << "Error retrieving media properties:" << ex.what();
                                               }
                                           });

    currentSession.PlaybackInfoChanged([this](auto const& sender, auto const&)
                                        {
                                            try
                                            {
                                                auto playbackInfo = sender.GetPlaybackInfo();
                                                auto playbackStatus = playbackInfo.PlaybackStatus();

                                                QString playbackState;
                                                switch (playbackStatus)
                                                {
                                                case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                                                    playbackState = "Playing";
                                                    break;
                                                case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                                                    playbackState = "Paused";
                                                    break;
                                                case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                                                    playbackState = "Stopped";
                                                    break;
                                                default:
                                                    playbackState = "Unknown";
                                                    break;
                                                }

                                                emit sessionPlaybackStateChanged(playbackState);
                                            }
                                            catch (const std::exception& ex)
                                            {
                                                qDebug() << "Error retrieving playback info:" << ex.what();
                                            }
                                        });
}
