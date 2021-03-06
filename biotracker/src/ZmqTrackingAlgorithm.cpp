#include "../ZmqTrackingAlgorithm.h"

#include "biotracker/settings/Settings.h"
#include "biotracker/zmq/ZmqInfoFile.h"
#include "biotracker/zmq/ZmqHelper.h"
#include "biotracker/zmq/ZmqMessageParser.h"

namespace BioTracker {
namespace Core {
namespace Zmq {

/**
 * @brief ZmqTrackingAlgorithm::ZmqTrackingAlgorithm
 */
ZmqTrackingAlgorithm::ZmqTrackingAlgorithm(std::shared_ptr<ZmqClientProcess> process, Settings &settings) :
    TrackingAlgorithm(settings),
    m_tools(std::make_shared<QFrame>()),
    m_process(process),
    m_events(this) {

    // ATTACH EVENTS
    QObject::connect(&m_events, &EventHandler::btnClicked, this, &ZmqTrackingAlgorithm::btnClicked);
    QObject::connect(&m_events, &EventHandler::sldValueChanged, this, &ZmqTrackingAlgorithm::sldValueChanged);

    QObject::connect(&m_events, &EventHandler::notifyGUI, this, &ZmqTrackingAlgorithm::notifyGUI);
    QObject::connect(&m_events, &EventHandler::update, this, &ZmqTrackingAlgorithm::update);
    QObject::connect(&m_events, &EventHandler::forceTracking, this, &ZmqTrackingAlgorithm::forceTracking);
    QObject::connect(&m_events, &EventHandler::pausePlayback, this, &ZmqTrackingAlgorithm::pausePlayback);
    QObject::connect(&m_events, &EventHandler::jumpToFrame, this, &ZmqTrackingAlgorithm::jumpToFrame);

}

ZmqTrackingAlgorithm::~ZmqTrackingAlgorithm() {
    shutdown();
}

void ZmqTrackingAlgorithm::track(size_t frameNumber, const cv::Mat &frame) {
    SendTrackMessage message(frame, static_cast<ulong>(frameNumber));
    m_process->send(message, m_events);
}

void ZmqTrackingAlgorithm::paint(size_t, ProxyMat &m, const View &v) {
    SendPaintMessage message(99, v.name, m.getMat());
    m_process->send(message, m_events);
}

void ZmqTrackingAlgorithm::paintOverlay(size_t, QPainter *p, const View &v) {
    SendPaintOverlayMessage message(v.name, p);
    m_process->send(message, m_events);
}

QPointer<QWidget> ZmqTrackingAlgorithm::getToolsWidget() {
    SendRequestWidgetsMessage message(TrackingAlgorithm::getToolsWidget());
    m_process->send(message, m_events);
    return TrackingAlgorithm::getToolsWidget();
}

void ZmqTrackingAlgorithm::prepareSave() {

}

void ZmqTrackingAlgorithm::postLoad() {

}

void ZmqTrackingAlgorithm::postConnect() {

}

void ZmqTrackingAlgorithm::mouseMoveEvent(QMouseEvent *) {

}

void ZmqTrackingAlgorithm::mousePressEvent(QMouseEvent *) {

}

void ZmqTrackingAlgorithm::mouseWheelEvent(QWheelEvent *) {

}

void ZmqTrackingAlgorithm::keyPressEvent(QKeyEvent *) {

}

void ZmqTrackingAlgorithm::shutdown() {
}

// == EVENTS ==

void ZmqTrackingAlgorithm::btnClicked(const QString widgetId) {
    SendButtonClickMessage message(widgetId);
    m_process->send(message, m_events);
}

void ZmqTrackingAlgorithm::sldValueChanged(const QString widgetId, int value) {
    SendValueChangedMessage message(widgetId, value);
    m_process->send(message, m_events);
}


}
}
}
