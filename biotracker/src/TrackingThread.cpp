#include "TrackingThread.h"

#include <iostream>

#include <chrono>
#include <thread>

#include <QFileInfo>

#include "settings/Messages.h"
#include "settings/Settings.h"
#include "settings/ParamNames.h"

#include <QCoreApplication>
#include <QtOpenGL/qgl.h>

#include <QPainter>

namespace BioTracker {
namespace Core {

using GUIPARAM::MediaType;

TrackingThread::TrackingThread(Settings &settings) :
    m_imageStream(make_ImageStreamNoMedia()),
    m_playing(false),
    m_playOnce(false),
    m_somethingIsLoaded(false),
    m_status(TrackerStatus::NothingLoaded),
    m_fps(30),
    m_runningFps(0),
    m_maxSpeed(false),
    m_mediaType(MediaType::NoMedia),
    m_settings(settings),
    m_texture(nullptr),
    m_openGLLogger(this) {
    Interpreter::Interpreter p;
    std::cout << "inter:" << p.interpret() << "\n";
}

TrackingThread::~TrackingThread(void) {
}

void TrackingThread::initializeOpenGL(QOpenGLContext *context,
                                      TextureObject &texture) {
    m_texture = &texture;

    m_openGLLogger.initialize(); // initializes in the current context, i.e. ctx
    connect(&m_openGLLogger, &QOpenGLDebugLogger::messageLogged, this,
            &TrackingThread::handleLoggedMessage);
    m_openGLLogger.startLogging();

    QThread::start();
}

void TrackingThread::loadFromSettings() {
    std::string filenameStr = m_settings.getValueOfParam<std::string>
                              (CAPTUREPARAM::CAP_VIDEO_FILE);
    boost::filesystem::path filename {filenameStr};
    m_imageStream = make_ImageStreamVideo(filename);
    if (m_imageStream->type() == GUIPARAM::MediaType::NoMedia) {
        // could not open video
        std::string errorMsg = "unable to open file " + filename.string();
        Q_EMIT notifyGUI(errorMsg, MSGS::MTYPE::FAIL);
        m_status = TrackerStatus::Invalid;
        return;
    } else {
        playOnce();
    }

    m_fps = m_imageStream->fps();

    Q_EMIT fileOpened(filenameStr, m_imageStream->numFrames());

    std::string note = "opened file: " + filenameStr + " (#frames: "
                       + QString::number(m_imageStream->numFrames()).toStdString() + ")";
    Q_EMIT notifyGUI(note, MSGS::MTYPE::FILE_OPEN);
}

void TrackingThread::loadVideo(const boost::filesystem::path &filename) {
    m_imageStream = make_ImageStreamVideo(filename);
    if (m_imageStream->type() == GUIPARAM::MediaType::NoMedia) {
        // could not open video
        std::string errorMsg = "unable to open file " + filename.string();
        Q_EMIT notifyGUI(errorMsg, MSGS::MTYPE::FAIL);
        m_status = TrackerStatus::Invalid;
        return;
    } else {
        playOnce();
    }

    m_fps = m_imageStream->fps();

    m_settings.setParam(CAPTUREPARAM::CAP_VIDEO_FILE, filename.string());

    std::string note = filename.string() + " (#frames: "
                       + QString::number(m_imageStream->numFrames()).toStdString() + ")";
    Q_EMIT fileOpened(filename.string(), m_imageStream->numFrames());
    Q_EMIT notifyGUI(note, MSGS::MTYPE::FILE_OPEN);
}

void TrackingThread::loadPictures(std::vector<boost::filesystem::path>
                                  &&filenames) {
    m_fps = 1;
    m_imageStream = make_ImageStreamPictures(std::move(filenames));
    if (m_imageStream->type() == GUIPARAM::MediaType::NoMedia) {
        // could not open video
        std::string errorMsg = "unable to open files [";
        for (boost::filesystem::path filename: filenames) {
            errorMsg += ", " + filename.string();
        }
        errorMsg += "]";
        Q_EMIT notifyGUI(errorMsg, MSGS::MTYPE::FAIL);
        m_status = TrackerStatus::Invalid;
        return;
    } else {
        playOnce();
    }
}

void TrackingThread::openCamera(int device) {
    m_imageStream = make_ImageStreamCamera(device);
    if (m_imageStream->type() == GUIPARAM::MediaType::NoMedia) {
        // could not open video
        std::string errorMsg = "unable to open camera " + QString::number(
                                   device).toStdString();
        Q_EMIT notifyGUI(errorMsg, MSGS::MTYPE::FAIL);
        m_status = TrackerStatus::Invalid;
        return;
    }
    m_status = TrackerStatus::Running;
    m_fps = m_imageStream->fps();
    std::string note = "open camera " + QString::number(device).toStdString();
    Q_EMIT notifyGUI(note, MSGS::MTYPE::NOTIFICATION);
    m_somethingIsLoaded = true;
}

void TrackingThread::run() {
    std::chrono::system_clock::time_point t;
    bool firstLoop = true;

    while (true) {
        std::unique_lock<std::mutex> lk(m_tickMutex);
        m_conditionVariable.wait(lk, [&] {return (m_playing || m_playOnce) && !m_isRendering;});
        m_isRendering = true;

        //if thread just started (or is unpaused) start clock here
        //after this timestamp will be taken right before picture is drawn
        //to take the amount of time into account it takes to draw the picture
        if (firstLoop)
            // measure the capture start time
        {
            t = std::chrono::system_clock::now();
        }
        firstLoop = false;

        if ((m_imageStream->type() == GUIPARAM::MediaType::Video)
                && m_imageStream->lastFrame()) {
            // TODO is this still correct?
            break;
        }

        std::chrono::microseconds target_dur(static_cast<int>(1000000. / m_fps));
        std::chrono::microseconds dur =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - t);
        if (!m_maxSpeed) {
            if (dur <= target_dur) {
                target_dur -= dur;
            } else {
                target_dur = std::chrono::microseconds(0);
            }
        } else {
            target_dur = std::chrono::microseconds(0);
        }

        // calculate the running fps.
        // TODO: why is this a member var??
        m_runningFps = 1000000. / std::chrono::duration_cast<std::chrono::microseconds>
                       (dur + target_dur).count();

        if (m_playOnce && !m_playing) {
            m_runningFps = -1;
        }

        tick(m_runningFps);

        std::this_thread::sleep_for(target_dur);
        t = std::chrono::system_clock::now();

        m_playOnce = false;
        // unlock mutex
        lk.unlock();
    }
}

void TrackingThread::tick(const double fps) {
    m_renderMutex.lock();
    std::string fileName = m_imageStream->currentFilename();
    doTracking();
    const size_t currentFrame = m_imageStream->currentFrameNumber();
    if (m_playing) {
        nextFrame();
    }
    m_renderMutex.unlock();
    Q_EMIT frameCalculated(currentFrame, fileName, fps);
}

void TrackingThread::setFrameNumber(size_t frameNumber) {
    m_renderMutex.lock();
    if (m_imageStream->setFrameNumber(frameNumber)) {
        if (m_tracker) {
            m_tracker->setCurrentFrameNumber(frameNumber);
        }
        playOnce();
    }
    m_renderMutex.unlock();
}
void TrackingThread::nextFrame() {
    if (m_imageStream->nextFrame()) { // increments the frame number if possible
        if (m_tracker) {
            m_tracker->setCurrentFrameNumber(m_imageStream->currentFrameNumber());
        }
    } else {
        m_playing = false;
        m_playOnce = false;
    }
}

void TrackingThread::doTracking() {
    MutexLocker trackerLock(m_trackerMutex);
    if (!m_tracker) {
        return;
    }

    // do nothing if we aint got a frame
    if (m_imageStream->currentFrameIsEmpty()) {
        return;
    }
    try {
        m_tracker->track(m_imageStream->currentFrameNumber(),
                         m_imageStream->currentFrame());
    } catch (const std::exception &err) {
        Q_EMIT notifyGUI("critical error in selected tracking algorithm: " +
                         std::string(
                             err.what()), MSGS::FAIL);
    }
}

size_t TrackingThread::getVideoLength() const {
    return m_imageStream->numFrames();
}

void TrackingThread::mouseEvent(QMouseEvent *event) {
    MutexLocker lock(m_trackerMutex);
    if (m_tracker) {
        QCoreApplication::sendEvent(m_tracker.get(), event);
    }
}

void TrackingThread::keyboardEvent(QKeyEvent *event) {
    MutexLocker lock(m_trackerMutex);
    if (m_tracker) {
        QCoreApplication::sendEvent(m_tracker.get(), event);
    }
}

size_t TrackingThread::getFrameNumber() const {
    return m_imageStream->currentFrameNumber();
}

void TrackingThread::setPause() {
    m_playing = false;
    m_status = TrackerStatus::Paused;
}

void TrackingThread::setPlay() {
    m_playing = true;
    m_status = TrackerStatus::Running;
    m_conditionVariable.notify_all();
}

void TrackingThread::paintOverlay(QPainter &painter) {
    if (m_somethingIsLoaded) {
        QRect myQRect(10,10,200,200);
        QLinearGradient gradient(myQRect.topLeft(),
                                 myQRect.bottomRight()); // diagonal gradient from top-left to bottom-right
        gradient.setColorAt(0, Qt::white);
        gradient.setColorAt(1, Qt::red);
        painter.fillRect(myQRect, gradient);
    }
}

void TrackingThread::paintRaw() {
    if (m_somethingIsLoaded) {
        ProxyPaintObject proxy(m_imageStream->currentFrame().clone());
        m_texture->setImage(proxy.getmat());
    }
}

void TrackingThread::paintDone() {
    if (m_somethingIsLoaded) {
        m_isRendering = false;
        m_conditionVariable.notify_all();
    }
}

void TrackingThread::togglePlaying() {
    if (m_playing) {
        setPause();
    } else {
        setPlay();
    }
}

void TrackingThread::playOnce() {
    m_status = TrackerStatus::Paused;
    m_playOnce = true;
    m_somethingIsLoaded = true;
    m_conditionVariable.notify_all();
}

bool TrackingThread::isPaused() const {
    return !m_playing;
}

bool TrackingThread::isRendering() const {
    return m_isRendering;
}

double TrackingThread::getFps() const {
    return m_fps;
}

void TrackingThread::setFps(double fps) {
    m_fps = fps;
}

void TrackingThread::setTrackingAlgorithm(std::shared_ptr<TrackingAlgorithm>
        trackingAlgorithm) {
    {
        MutexLocker lock(m_trackerMutex);
        m_tracker = trackingAlgorithm;
    }
}

void TrackingThread::setMaxSpeed(bool enabled) {
    m_maxSpeed = enabled;
}

void TrackingThread::handleLoggedMessage(const QOpenGLDebugMessage
        &debugMessage) {
    std::cout << debugMessage.message().toStdString() << std::endl;
}

}
}