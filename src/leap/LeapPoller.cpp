#include "LeapPoller.h"

#include <string.h>

#include <LeapC.h>

LeapPoller::LeapPoller()
{
    m_isRunning = false;
    m_thread = nullptr;
    m_connection = nullptr;
    m_frame = new LEAP_TRACKING_EVENT();
    m_devicesCount = 0;
}

LeapPoller::~LeapPoller()
{
    delete m_frame;
}

void LeapPoller::Start()
{
    if(m_isRunning)
        return;
    if(LeapCreateConnection(nullptr, &m_connection) != eLeapRS::eLeapRS_Success)
        return;
    if(LeapOpenConnection(m_connection) != eLeapRS::eLeapRS_Success)
        return;

    m_isRunning = true;
    m_thread = new std::thread(&LeapPoller::PollThread, this);
}

void LeapPoller::Stop()
{
    if(!m_isRunning)
        return;

    m_isRunning = false;
    LeapCloseConnection(m_connection);

    m_thread->join();
    m_thread = nullptr;

    LeapDestroyConnection(m_connection);
    m_connection = nullptr;

    m_devicesCount = 0;
}

bool LeapPoller::IsConnected() const
{
    return (m_isRunning && (m_devicesCount > 0));
}

bool LeapPoller::GetFrame(LEAP_TRACKING_EVENT *p_target)
{
    if(!m_isRunning)
        return false;

    if(!m_frameLock.try_lock())
        return false;

    memcpy(p_target, m_frame, sizeof(LEAP_TRACKING_EVENT));
    m_frameLock.unlock();
    return true;
}

void LeapPoller::SetTrackingMode(eLeapTrackingMode p_mode)
{
    if(m_isRunning)
        LeapSetTrackingMode(m_connection, p_mode);
}

void LeapPoller::SetPolicy(uint64_t p_set, uint64_t p_clear)
{
    if(m_isRunning)
        LeapSetPolicyFlags(m_connection, p_set, p_clear);
}

void LeapPoller::PollThread()
{
    while(m_isRunning)
    {
        LEAP_CONNECTION_MESSAGE l_message{ sizeof(LEAP_CONNECTION_MESSAGE) };
        if(LeapPollConnection(m_connection, 150U, &l_message) != eLeapRS::eLeapRS_Success)
            continue;

        switch(l_message.type)
        {
            case eLeapEventType_None:
                break;

            case eLeapEventType_Connection:
                OnConnectionEvent();
                break;

            case eLeapEventType_ConnectionLost:
                OnConnectionLostEvent();
                break;

            case eLeapEventType_Device:
                OnDeviceEvent(l_message.device_event);
                break;

            case eLeapEventType_DeviceLost:
                OnDeviceLostEvent(l_message.device_event);
                break;

            case eLeapEventType_Tracking:
                OnTrackingEvent(l_message.tracking_event);
                break;
        }
    }
}

void LeapPoller::OnConnectionEvent()
{
}

void LeapPoller::OnConnectionLostEvent()
{
    m_devicesCount = 0U;
}

void LeapPoller::OnDeviceEvent(const LEAP_DEVICE_EVENT* p_event)
{
    if(!p_event->device.handle)
        return;

    LEAP_DEVICE l_device;
    if(LeapOpenDevice(p_event->device, &l_device) != eLeapRS_Success)
        return;

    m_devicesCount++;
}

void LeapPoller::OnDeviceLostEvent(const LEAP_DEVICE_EVENT *p_event)
{
    // In LeapCSharp there is call for opening device, no idea why
    m_devicesCount--;
}

void LeapPoller::OnTrackingEvent(const LEAP_TRACKING_EVENT *p_event)
{
    m_frameLock.lock();
    memcpy(m_frame, p_event, sizeof(LEAP_TRACKING_EVENT));
    m_frameLock.unlock();
}
