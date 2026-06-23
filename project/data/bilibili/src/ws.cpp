#include "kumo/bilibili/ws.h"
// #include <brotli/decode.h>  // Brotli decompression
#include <sstream>
#include <iomanip>
#include <random>

namespace kumo {

BiliWebSocket::BiliWebSocket(std::shared_ptr<EventBus> event_bus)
    : m_inner(std::make_shared<Inner>()) {
    m_inner->event_bus = std::move(event_bus);
}

BiliWebSocket::~BiliWebSocket() {
    disconnect();
}

bool BiliWebSocket::connect(uint64_t room_id, const std::string& server) {
    std::unique_lock lock(m_inner->mutex);
    if (m_inner->connected) return false;

    m_inner->room_id = room_id;
    m_inner->should_stop = false;
    m_inner->connected = true;

    // Start worker thread
    m_inner->worker = std::thread([this, server]() {
        worker_loop(server);
    });

    return true;
}

void BiliWebSocket::disconnect() {
    m_inner->should_stop = true;

    {
        std::unique_lock lock(m_inner->mutex);
        m_inner->connected = false;
    }

    if (m_inner->worker.joinable()) {
        m_inner->worker.join();
    }
}

bool BiliWebSocket::is_connected() const {
    return m_inner->connected;
}

void BiliWebSocket::set_merge_user_info(const std::vector<MergeUserInfo>& info) {
    std::unique_lock lock(m_inner->mutex);
    m_inner->merge_users = info;
}

// Bilibili WebSocket binary packet format:
// Packet = Header (16 bytes) + Body
// Header: [PacketLen:4][HeaderLen:2][Version:2][Op:4][Seq:4]
// All integers are big-endian

static uint32_t read_u32_be(const uint8_t* data, size_t offset) {
    return (static_cast<uint32_t>(data[offset]) << 24) |
           (static_cast<uint32_t>(data[offset + 1]) << 16) |
           (static_cast<uint32_t>(data[offset + 2]) << 8) |
           static_cast<uint32_t>(data[offset + 3]);
}

static uint16_t read_u16_be(const uint8_t* data, size_t offset) {
    return (static_cast<uint16_t>(data[offset]) << 8) |
           static_cast<uint16_t>(data[offset + 1]);
}

void BiliWebSocket::dispatch_message(const json& body) {
    if (!body.contains("cmd")) return;

    const std::string& cmd = body["cmd"].get_ref<const std::string&>();

    // Danmu message
    if (cmd == "DANMU_MSG") {
        if (auto msg = DanmuMessage::from_raw(body)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Gift message
    else if (cmd == "SEND_GIFT") {
        if (auto msg = GiftMessage::from_raw(body, m_inner->room_id)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Guard message
    else if (cmd == "GUARD_BUY") {
        if (auto msg = GuardMessage::from_raw(body, m_inner->room_id)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    else if (cmd == "USER_TOAST_MSG") {
        if (auto msg = GuardMessage::from_raw(body, m_inner->room_id)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // SuperChat message
    else if (cmd == "SUPER_CHAT_MESSAGE") {
        if (auto msg = SuperChatMessage::from_raw(body, m_inner->room_id)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Interact message
    else if (cmd == "INTERACT_WORD") {
        if (auto msg = InteractMessage::from_raw(body)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Entry effect
    else if (cmd == "ENTRY_EFFECT") {
        if (auto msg = EntryEffectMessage::from_raw(body)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Room change
    else if (cmd == "ROOM_CHANGE") {
        if (auto msg = RoomChangeMessage::from_raw(body)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Online count
    else if (cmd == "ONLINE_RANK_COUNT") {
        if (auto msg = OnlineRankCountMessage::from_raw(body)) {
            m_inner->event_bus->emit(UpdateOnlineEvent{msg->count});
        }
    }
    // Warning
    else if (cmd == "WARNING") {
        if (auto msg = WarningMessage::from_raw(body)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Cut off
    else if (cmd == "CUT_OFF") {
        if (auto msg = CutOffMessage::from_raw(body)) {
            m_inner->event_bus->emit(*msg);
        }
    }
    // Live start/end
    else if (cmd == "LIVE") {
        m_inner->event_bus->emit(LiveStartEvent{});
    }
    else if (cmd == "PREPARING") {
        m_inner->event_bus->emit(LiveEndEvent{});
    }
}

void BiliWebSocket::worker_loop(const std::string& server) {
    // Bilibili WebSocket protocol:
    // 1. Connect to wss://{server}/sub
    // 2. Send auth packet (JSON with room_id + uid)
    // 3. Receive binary packets and parse

    // Step 1: Connect
    // httplib::Client cli(server);
    // httplib::Headers headers = {{"Origin", "https://live.bilibili.com"}};
    // auto res = cli.Get("/sub", headers);
    // Parse connection...

    // Step 2: Auth packet (binary format)
    // Build JSON: {"uid": 0, "roomid": room_id, "protover": 3, "platform": "web", ...}
    // Pack as Bilibili binary frame with Op=7 (auth)

    // Step 3: Receive loop
    // Pseudocode:
    // while (!should_stop) {
    //     read 16-byte header
    //     parse packet_len, header_len, op, seq
    //     read body (packet_len - header_len)
    //     if op == 3:  // heartbeat reply (do nothing)
    //     if op == 8:  // auth reply
    //     if op == 5:  // message
    //         if protover == 3:
    //             decompress with brotli
    //             parse each nested packet
    //             dispatch JSON messages
    // }

    // Stub: no actual connection implemented
    m_inner->event_bus->emit(ConnectionStatusEvent{false});
}

} // namespace kumo
