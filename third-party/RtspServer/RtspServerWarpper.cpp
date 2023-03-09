#include "RtspServerWarpper.h"

#include "xop/RtspServer.h"
#include "net/Timer.h"

#include "memory.h"

class RtspServerWarpper
{
public:
    RtspServerWarpper(int _port)
    {
        loopExit = 0;
        port = _port;
        std::shared_ptr<std::thread> _thread(new std::thread(&RtspServerWarpper::Start, port, std::ref(server), &loopExit));
        usleep(500 * 1000);
        thread = _thread;
        rtsp_url = "rtsp://127.0.0.1:" + std::to_string(port);
    }

    ~RtspServerWarpper()
    {
        // Quit();
    }

    void Quit()
    {
        loopExit = 1;
        if (thread.get())
        {
            thread->join();
            thread = nullptr;
            server = nullptr;
        }
    }

    xop::MediaSessionId AddSession(std::string url_suffix, bool h265)
    {
        xop::MediaSession *session = xop::MediaSession::CreateNew(url_suffix);
        if (h265)
        {
            session->AddSource(xop::channel_0, xop::H265Source::CreateNew());
        }
        else
        {
            session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
        }

        // session->AddSource(xop::channel_1, xop::AACSource::CreateNew(44100, 2));

        session->AddNotifyConnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
                                            { printf("RTSP client connect,sess=%d ip=%s, port=%hu \n", sessionId, peer_ip.c_str(), peer_port); });

        session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
                                               { printf("RTSP client disconnect,sess=%d  ip=%s, port=%hu \n", sessionId, peer_ip.c_str(), peer_port); });

        // RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
        xop::MediaSessionId session_id = server->AddSession(session);

        std::cout << "Play URL: " << rtsp_url << "/" << url_suffix << "   seeeisID:" << session_id << std::endl;
        return session_id;
    }

    void RemoveSession(xop::MediaSessionId sessionID)
    {
        server->RemoveSession(sessionID);
    }

    bool PushFrame(xop::MediaSessionId session_id, xop::MediaChannelId channel_id, xop::AVFrame &videoFrame)
    {
        return server->PushFrame(session_id, channel_id, videoFrame);
    }

private:
    std::shared_ptr<xop::RtspServer> server;
    std::string rtsp_url;
    std::shared_ptr<std::thread> thread;

    int port = 8554;

    volatile int loopExit = 0;
    static void Start(int port, std::shared_ptr<xop::RtspServer> &server, volatile int *loopExit)
    {
        std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
        server = xop::RtspServer::Create(event_loop.get());

        if (!server->Start("0.0.0.0", port))
        {
            printf("RTSP Server listen on %d failed.\n", port);
            return;
        }

        while (!*loopExit)
        {
            xop::Timer::Sleep(100);
        }
        server->Stop();
        event_loop->Quit();
        printf("rtsp server thread quit %d\n", *loopExit);
    }
};

rtsp_server_t rtsp_new_server(int port)
{
    RtspServerWarpper *rtsp_wapper = new RtspServerWarpper(port);
    return rtsp_wapper;
}

void rtsp_rel_server(rtsp_server_t *rtsp_server)
{
    if (rtsp_server && *rtsp_server)
    {
        RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)*rtsp_server;
        rtsp_wapper->Quit();
        delete rtsp_wapper;
        rtsp_wapper = nullptr;
        *rtsp_server = nullptr;
        // printf("server quit\n");
    }
}

rtsp_session_t rtsp_new_session(rtsp_server_t rtsp_server, char *url_suffix, int h265)
{
    RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
    if (!rtsp_wapper)
    {
        return -1;
    }
    xop::MediaSessionId session_id = rtsp_wapper->AddSession(url_suffix, h265);
    return session_id;
}

void rtsp_rel_session(rtsp_server_t rtsp_server, rtsp_session_t rtsp_session)
{
    RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
    if (!rtsp_wapper)
    {
        return;
    }
    rtsp_wapper->RemoveSession(rtsp_session);
}

int rtsp_push(rtsp_server_t rtsp_server, rtsp_session_t rtsp_session, rtsp_buffer_t *buff)
{
    if (buff->vlen > 0)
    {
        xop::AVFrame videoFrame = {0};
        videoFrame.type = 0;                                    // 建议确定帧类型。I帧(xop::VIDEO_FRAME_I) P帧(xop::VIDEO_FRAME_P)
        videoFrame.size = buff->vlen;                           // 视频帧大小
        videoFrame.timestamp = xop::H264Source::GetTimestamp(); // 时间戳, 建议使用编码器提供的时间戳
        videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
        memcpy(videoFrame.buffer.get(), buff->vbuff, videoFrame.size);

        RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
        if (!rtsp_wapper)
        {
            return -1;
        }
        bool ret = rtsp_wapper->PushFrame(rtsp_session, xop::channel_0, videoFrame); // 送到服务器进行转发, 接口线程安全
        return ret ? 0 : -1;
    }
    return -1;
}